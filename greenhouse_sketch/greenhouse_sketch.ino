#include <SPI.h>
#include <RF24.h>
#include <Servo.h>
#include <OneWire.h>
#include "printf.h"

#include "config.h"

#define DEBUG

#define rfCE 9
#define rfCS 10
const unsigned long broadcastInterval = 5e3;
RF24 radio(rfCE, rfCS);

#define augerPin 5
#define AUGER_FEED 0
#define AUGER_BACK 4500
#define AUGER_STOP 1500
Servo auger;

#define augerSwitch 6
const unsigned long augerTimeout = 2e3;

#define tankTempPin 7
const uint8_t tankTempAddr[] = {0x28, 0x94, 0xDB, 0xE6, 0x03, 0x00, 0x00, 0xF9};
OneWire tankTemp(tankTempPin);

#define humidityPin 2

// NOTE: these need http://web.archive.org/web/20130524110602/http://placeboaudio.com/how-tu-use-atmega-pb6-pb7-with-arduino-when-n
// (summary: add two PB at end of digital_pin_to_port_PGM and BV(6), BV(7) at end of digital_pin_to_bit_mask_PGM in pins_arduino.h)
#define aux1 20
#define aux2 21

#define airTempPin A0
#define batteryBackup A1

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
  radio.setAutoAck(true);
  radio.setRetries(15,15);

  radio.openWritingPipe(CONFIG_HUB_RX_PIPE); 
  radio.openReadingPipe(1,CONFIG_HUB_TX_PIPE); 

#ifdef DEBUG
  radio.printDetails();
#endif
  
  auger.attach(augerPin);
  pinMode(augerSwitch, INPUT);
}

uint32_t switchAugerCount = 0;
uint32_t remoteAugerCount = 0;

unsigned long nextSwitchAllowed = 0;
unsigned long nextBroadcastTime = 0;

void runAuger(unsigned ms) {
  unsigned count = ms / 1000;
  unsigned leftover = ms % 1000;
#ifdef DEBUG
  printf("Running auger for %ums (%u count + %u leftover).\n", ms, count, leftover);
#endif
  auger.writeMicroseconds(AUGER_BACK); delay(125);       // cw to prevent jam
  auger.writeMicroseconds(AUGER_FEED); delay(leftover);  // ccw to feed
  for (unsigned i = 0; i < count; ++i) {
    auger.writeMicroseconds(AUGER_BACK); delay(125);
    auger.writeMicroseconds(AUGER_FEED); delay(875);
  }
  auger.writeMicroseconds(AUGER_STOP);
#ifdef DEBUG
  printf("Auger stopped.\n");
#endif
}

uint16_t waterTemp() {
  // see http://datasheets.maximintegrated.com/en/ds/DS18B20.pdf
  
  // request scratchpad read
  tankTemp.reset();
  tankTemp.select(tankTempAddr);
  tankTemp.write(0xBE);
  
  byte data[12];
  for (uint8_t i = 0; i < 9; i++) {           // only "need" first two bytes, but CRC is sent
    data[i] = tankTemp.read();
    //Serial.print(data[i], HEX);
    //Serial.print(" ");
  }
  byte crc = OneWire::crc8(data, 8);
  
  // request convert T for next broadcast
  tankTemp.reset();
  tankTemp.select(tankTempAddr);
  tankTemp.write(0x44,1);
  //delay(750);     // Tconv time for 12-bit resolution
  
  return (data[8] == crc) ? *(uint16_t*)data : 0;
}

uint32_t humidityLevel() {          // for RC circuit
  // charge the capacitive sensor
  pinMode(humidityPin, OUTPUT);
  digitalWrite(humidityPin, HIGH);
  delay(1);
  
  // time how long until charge drains
  pinMode(humidityPin, INPUT);
  digitalWrite(humidityPin, LOW);
  unsigned long start = micros();
  while (digitalRead(humidityPin)) ;
  return micros() - start;
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
          runAuger(750);
          // fall through to trigger broadcast
          break;
        case 0x05EE:
          nextBroadcastTime = now;
          break;
        case 0xAAA0:
          digitalWrite(aux1, LOW);
          break;
        case 0xAAA1:
          digitalWrite(aux1, HIGH);
          break;
        case 0xAAB0:
          digitalWrite(aux2, LOW);
          break;
        case 0xAAB1:
          digitalWrite(aux2, HIGH);
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
    broadcastMessage[4] = waterTemp();
    broadcastMessage[5] = 0; //humidityLevel();
    broadcastMessage[6] = analogRead(airTempPin);
    broadcastMessage[7] = analogRead(batteryBackup);

#ifdef DEBUG
    printf("broadcastMessage");
    for (unsigned i = 1; i < 8; ++i) printf(" %u", broadcastMessage[i]);
    printf("\n");
#endif
    
    radio.stopListening();
    bool ok = radio.write(broadcastMessage, 32);
    radio.startListening();
    nextBroadcastTime = now + broadcastInterval;
#ifdef DEBUG
    printf("Radio broadcast success: %i\n", ok);
#endif
  }
  
  if (now >= nextSwitchAllowed && digitalRead(augerSwitch)) {
    nextSwitchAllowed = now + augerTimeout;
    switchAugerCount += 1;
    runAuger(250);
  }
}
