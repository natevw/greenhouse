# Greenhouse monitoring/control stuff

This is what is hooked up around my aquaponics system. So far: demand/remote auger feeder, water temperature, air temperature and humidity. Upcoming(?): battery backup status, door sensor, pH probe

TBD: make all sensor reads non-blocking, continued debugging/enhancing
TBD: make web-accessible via [microstates](https://github.com/natevw/microstates), etc.


## Installation notes

    # Unblacklist SPI kernel module, via http://scruss.com/blog/2013/01/19/the-quite-rubbish-clock/#spi
    sudo vi /etc/modprobe.d/raspi-blacklist.conf        # comment out SPI line
    sudo modprobe spi-bcm2708

    mkdir radio && cd radio
    git clone https://github.com/stanleyseow/RF24.git
    cd RF24/librf24-rpi/librf24
    make
    ln -s librf24.so.1.0 librf24.so.1
    ln -s librf24.so.1.0 librf24.so

    cd radio
    ln -s RF24/librf24-rpi/librf24
    git clone https://github.com/natevw/greenhouse.git
    cd greenhouse && make
    sudo LD_LIBRARY_PATH=../librf24:$LD_LIBRARY_PATH ./greenhub

## Hack-ity poke-ity

For some undiagnosed reason, the listener on the Raspberry Pi side "stops working" until any command is sent. So here's how I'm automatically sending a B keypress to keep it logging:

    # via http://serverfault.com/a/407923/123535
    mkfifo rx_hack.fifo
    # I run this in `screen`…
    cat >rx_hack.fifo &
    cat rx_hack.fifo | sudo LD_LIBRARY_PATH=../librf24:$LD_LIBRARY_PATH stdbuf -i0 -o0 ./greenhub | tee -a green.log

    crontab -e
    
        */5 * * * * echo "B" > radio/greenhouse/rx_hack.fifo

I then use [linepost](https://github.com/natevw/linepost) to pipe the logs to CouchDB, e.g. `tail -f green.log | linepost http://localhost:5984/greenhub/_design/greenhouse/_update/greenhub_log`.