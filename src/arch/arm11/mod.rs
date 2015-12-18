//! Currently, these module for ARM architecture has been developed for Raspberry Pi Zero.
//! This Pi's CPU is BCM2835.
//! References:
//! (RASPBERRY PI HARDWARE)[https://www.raspberrypi.org/documentation/hardware/raspberrypi/README.md]
//! (RPi Hub)[http://elinux.org/RPi_Hub]
//! (RPi BCM2835 GPIOs)[http://elinux.org/RPi_BCM2835_GPIOs]
//! (BCM2835 datasheet errata)[http://elinux.org/BCM2835_datasheet_errata]

mod peripheral;

pub fn init()
{
    peripheral::init();
}
