#include <SPI.h>
#include <RF24.h>
#include <Servo.h>
#include <OneWire.h>
#include "printf.h"

#include "config.h"

//#define DEBUG

#define rfCE 9
#define rfCS 10

const unsigned long broadcastInterval = 5e3;
const unsigned long switchTimeout = 60e3;

#define augerPin 3
#define switchPin 4


#define tankTempPin 7
//const uint8_t addr[] = {0x28, 0x94, 0xDB, 0xE6 0x03 0x00 0x00 0xF9};

#define humidityPin 2
/*
#define airTempPin 6

*/

// listen for: switch pulls, auger request
// broadcast: pull count, temp/humid readings

RF24 radio(rfCE, rfCS);
OneWire tankTemp(tankTempPin);
Servo auger;


void setup() {
#ifdef DEBUG
  Serial.begin(57600);
  printf_begin();
  printf("\n\nHello.\n\n");
#endif

  radio.begin();
  radio.enableDynamicPayloads();
  radio.setDataRate(CONFIG_RF_DATARATE);
  radio.setPALevel(RF24_PA_MAX);
  radio.setChannel(CONFIG_RF_CHANNEL);
  radio.setRetries(15,15);

  radio.openWritingPipe(CONFIG_HUB_RX_PIPE); 
  radio.openReadingPipe(1,CONFIG_HUB_TX_PIPE); 

#ifdef DEBUG
  radio.printDetails();
#endif
  
  auger.attach(augerPin);
  pinMode(switchPin, INPUT);
}

uint32_t switchAugerCount = 0;
uint32_t remoteAugerCount = 0;

unsigned long nextSwitchAllowed = 0;
unsigned long nextBroadcastTime = 0;

void runAuger() {
#ifdef DEBUG
  printf("Starting auger.\n");
#endif
  auger.writeMicroseconds(4500); delay(500);  // cw to prevent jam
  auger.writeMicroseconds(0); delay(2000);    // ccw to feed
  auger.writeMicroseconds(1500);              // stop
#ifdef DEBUG
  printf("Auger stopped.\n");
#endif
}

void waterTemp() {
  /*
  ds.reset();
  ds.select(addr);
  ds.write(0x44,1);         // start conversion, with parasite power on at the end
  delay(1000);     // maybe 750ms is enough, maybe not
  ds.reset();
  */
}


void loop() {
  unsigned long now = millis();
  
  if (radio.available()) {
    uint32_t command[8] = {0};    // declare in 4-byte blocks for mnemonic ease
    uint8_t len = radio.getDynamicPayloadSize();
    bool ok = radio.read(command, len);
    if (ok && strncmp((char*)command, "n8vw", 4) == 0) {
      switch (command[1]) {
        case 0xFEED:
          remoteAugerCount += 1;
          runAuger();
          // fall through to trigger broadcast
        case 0x05EE:
          nextBroadcastTime = now;
          break;
      }
    }
#ifdef DEBUG
    printf("Radio read success: %i, command: %i\n", ok, command[1]);
#endif
  }
  
  if (now >= nextBroadcastTime) {
    uint32_t broadcastMessage[8] = {0};
    
    strcpy((char*)broadcastMessage, "aqua");
    broadcastMessage[1] = now;
    broadcastMessage[2] = switchAugerCount;
    broadcastMessage[3] = remoteAugerCount;
    // TODO: gather temp/humid data
    
    radio.stopListening();
    bool ok = radio.write(broadcastMessage, 32);
    radio.startListening();
    nextBroadcastTime = now + broadcastInterval;
#ifdef DEBUG
    printf("Radio broadcast success: %i\n", ok);
#endif
  }
  
  if (now >= nextSwitchAllowed && digitalRead(switchPin)) {
    nextSwitchAllowed = now + switchTimeout;
    switchAugerCount += 1;
    runAuger();
  }
}
