#include <cstdlib>
#include <iostream>
#include <cerrno>
#include "RF24.h"

#include "config.h"

#define DEBUG

using namespace std;

// CE and CSN pins On header using GPIO numbering (not pin numbers)
RF24 radio("/dev/spidev0.0",8000000,24);  // Setup for GPIO 24 CE


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
            time_t t; time(&t);
            tm t_parts; gmtime_r(&t, &t_parts);            
            char timebuff[24]; strftime(timebuff, sizeof(timebuff), "%Y-%m-%dT%H:%M:%SZ", &t_parts);
            printf("%s ", timebuff);
            printf("Received broadcast: now=%u switchAugerCount=%u remoteAugerCount=%u waterTemp=%u humidity=%u airTemp=%u nc=%u\n",
                receivedBroadcast[1], receivedBroadcast[2], receivedBroadcast[3], receivedBroadcast[4], receivedBroadcast[5], receivedBroadcast[6], receivedBroadcast[7]);
        }
    }
    
    char key = '\0';
    ssize_t res = read(STDIN_FILENO, &key, 1);
#ifdef DEBUG
    //printf("key=%c, res=%i, errno=%i\n", key, res, errno);
#endif
    if (res) {
        uint32_t command[8] = {0};
        strcpy((char*)command, "n8vw");
        switch (key) {
            case 'F':
                command[1] = 0xFEED;
                break;
            case 'B':
                command[1] = 0x05EE;
                break;
            case 'O':
                command[1] = 0xAAA0;
                break;
            case 'I':
                command[1] = 0xAAA1;
                break;
            default:
                goto wait;
        }
        radio.stopListening();
        bool ok = radio.write(command, 32);
        radio.startListening();
#ifdef DEBUG
    printf("Command send success: %i\n", ok);
#endif
    }

wait:
    usleep(0.05e6);
}

int main(int argc, char** argv) {
	setup();
	while(1)
		loop();
	return 0;
}
