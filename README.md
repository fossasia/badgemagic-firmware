# badgemagic-firmware
Hardware details and information to build an open firmware for LED badge

## Hardware Information

BadgeMagic target hardware features a custom 11x44 LED matrix display, 
Bluetooth LE and low power consumption.

### Chipset

The design relies on a single MCU. 
MCU type has changed depending on manufacturer batches.

* ARM M0 variant based on MM32W062
* RISC-V variant based on CH582

### Display

The custom 11x44 LED dot matrix is using persistence of vision.
(assets/dotmatrix.png)

## Firmware

### Functionalities

The badge stores bitmaps in 

