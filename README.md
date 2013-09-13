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