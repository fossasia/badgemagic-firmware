# badgemagic-firmware

Hardware details and information to build an open firmware for Bluetooth LED badges, compatible with [Badge Magic app](https://github.com/fossasia/badgemagic-android)

## Hardware Information

BadgeMagic target hardware features a custom 11x44 LED matrix display, 
Bluetooth LE and low power consumption.

### Chipset

The design relies on a single MCU. 
MCU type has changed depending on manufacturer batches.

* ARM M0 variant based on [MM32L062PF](http://www.mindmotion.com.cn/en/download.aspx?cid=2564)
* ARM M0 variant based on [MM32W062](http://www.mindmotion.com.cn/en/download.aspx?cid=2564)
* RISC-V variant based on [CH582](https://www.wch.cn/products/CH583.html)

See specific pages regarding each version of the hardware below

### Display

The custom 11x44 LED dot matrix displays using persistence of vision.

Bitmaps are drawn using 24 pins in [charlieplexing](https://en.wikipedia.org/wiki/Charlieplexing) technique.

<img src="/assets/CharlieplexedArray4.png" alt="charlieplexing Original at https://en.wikipedia.org/wiki/Charlieplexing#/media/File:CharlieplexedArray4.png" style="height: width:700px;"/>

## Firmware

Manufacturer firmware is closed source. Creating an open source compatible firmware is the goal of this project.

### Functionalities

The badge stores bitmaps with preconfigured animation sequences.
It uses 8 memory banks that can be used alternatively.

The 2 buttons configuration:

* Top button
    * Switches device on or off (long press)
    * Changes to Bluetooth upload mode (short press when on)
* Bottom button
    * Switches to badge display instead of charging screen when plugged with USB
    * Switches memory bank when displayed
    * Changes brightness level (long press)

### Upload Protocol

Sorry, you'll have to dive into code here :/

See original [led-name-badge-ls32](https://github.com/fossasia/led-name-badge-ls32) for USB update protocol reverse engineering

See [Badge Magic app](https://github.com/fossasia/badgemagic-android) for Bluetooth update protocol

## Variant Specific Instructions

Currently, only the RISC-V version is targeted as it seems the only one produced at the moment.

### RISC-V CH582 Instructions 

Please go to [CH582 Badge Instructions and Reverse Engineering](CH582.md)


