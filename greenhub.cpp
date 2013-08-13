/* 
 *
 *  Filename : rpi-hub.cpp
 *
 *  This program makes the RPi as a hub listening to all six pipes from the remote 
 *  sensor nodes ( usually Arduino  or RPi ) and will return the packet back to the 
 *  sensor on pipe0 so that the sender can calculate the round trip delays
 *  when the payload matches.
 *  
 *  Refer to RF24/examples/rpi_hub_arduino/ for the corresponding Arduino sketches 
 * to work with this code.
 *  
 *  CE is connected to GPIO25
 *  CSN is connected to GPIO8 
 *
 *  Refer to RPi docs for GPIO numbers
 *
 *  Author : Stanley Seow
 *  e-mail : stanleyseow@gmail.com
 *  date   : 4th Apr 2013
 *
 */

#include <cstdlib>
#include <iostream>
#include "RF24.h"

#include "config.h"

using namespace std;

// CE and CSN pins On header using GPIO numbering (not pin numbers)
RF24 radio("/dev/spidev0.0",8000000,25);  // Setup for GPIO 25 CSN


void setup(void) {
	radio.begin();
    radio.enableDynamicPayloads();
    radio.setDataRate(CONFIG_RF_DATARATE);
    radio.setPALevel(RF24_PA_MAX);
    radio.setChannel(CONFIG_RF_CHANNEL);
    radio.setRetries(15,15);
    
	radio.openWritingPipe(CONFIG_HUB_TX_PIPE);
	radio.openReadingPipe(1,CONFIG_HUB_RX_PIPE);
    
	radio.printDetails();
    
	radio.startListening();
}

void loop(void) {
    if (radio.available()) {
        uint32_t receivedBroadcast[8] = {0};    // declare in 4-byte blocks for mnemonic ease
        uint8_t len = radio.getDynamicPayloadSize();
        bool ok = radio.read(command, len);
        if (ok && strncmp((char*)command, "aqua", 4) == 0) {
            printf("Received broadcast: now=%i switchAugerCount=%i remoteAugerCount=%i\n", receivedBroadcast[1], receivedBroadcast[2], receivedBroadcast[3]);
        }
    }
    
    // TODO: check for "F" keypress and send feed?
    char key = '\0';
    ssize_t res = read(STDIN_FILENO, &key, 1);
    printf("key=%c, res=%i, errno=%i", key, res, errno);
    sleep(1);
	//usleep(20);
}

int main(int argc, char** argv) {
	setup();
	while(1)
		loop();
	return 0;
}
