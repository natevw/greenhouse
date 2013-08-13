#include <cstdlib>
#include <iostream>
#include <cerrno>
#include "RF24.h"

#include "config.h"

#define DEBUG

using namespace std;

// CE and CSN pins On header using GPIO numbering (not pin numbers)
RF24 radio("/dev/spidev0.0",8000000,25);  // Setup for GPIO 25 CSN


void setup(void) {
    // configure non-blocking stdin
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(STDIN_FILENO, F_SETFL, flags);

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
        bool ok = radio.read(receivedBroadcast, len);
        if (ok && strncmp((char*)receivedBroadcast, "aqua", 4) == 0) {
            printf("Received broadcast: now=%i switchAugerCount=%i remoteAugerCount=%i\n", receivedBroadcast[1], receivedBroadcast[2], receivedBroadcast[3]);
        }
    }
    
    // TODO: check for "F" keypress and send feed?
    char key = '\0';
    ssize_t res = read(STDIN_FILENO, &key, 1);
#ifdef DEBUG
    printf("key=%c, res=%i, errno=%i\n", key, res, errno);
#endif
    if (res && key == 'F' || key == 'B') {
        uint32_t command[8] = {0};
        strcpy((char*)command, "n8vw");
        command[1] = (key == 'F') ? 0xFEED : 0x05EE;
        radio.stopListening();
        bool ok = radio.write(command, 32);
        radio.startListening();
#ifdef DEBUG
    printf("Command send success: %i\n", ok);
#endif
    }
    
    sleep(1);
	//usleep(20);
}

int main(int argc, char** argv) {
	setup();
	while(1)
		loop();
	return 0;
}
