## Badge Data Format for BLE

### Connection Information

The pixel data is transferred to the badge via BLE. The badge advertizes the name "LSLED" and uses a vendor specific service/characteristic:

- Service-UUID: 0000fee0-0000-1000-8000-00805f9b34fb
- Characteristic: 0000fee1-0000-1000-8000-00805f9b34fb

The characteristic accepts 16 byte long data packets.

### Data Format

The badge supports up to 8 bitmaps which can have various features enabled.

| format    | data      | description  |
| --------- | --------- | ------------ |
| uint8[6]  | header    | fixed header: "wang\0\0" |
| uint8     | flash     | bitfield describing which of the 8 bitmaps are flashing |
| uint8     | marquee   | bitfield describing which of the 8 bitmaps feature an animated border |
| uint8[8]  | modes     | speed+mode for each of the 8 bitmaps. See below. |
| uint16[8] | sizes     | count of 8 pixel wide chunks for each bitmap (Big endian) |
| uint8[6]  | padding   | 6 bytes zeroes of padding
| uint8[6]  | timestamp | one byte each for (year % 256), month, day, hour, minute, second. Purpose unclear.
| uint8[4]  | padding   | 4 bytes zeroes of padding
| uint8[16] | separator | zeroes to separate header from bitmap data |
| uint8[11]\*N | bitmapdata | image data for all bitmaps. Each bitmap consists of a number of 11 byte chunks describing an 8x11 bitmap, each byte describes 8 pixels, starting with the most significant bit. The number of chunks is listed in the "sizes" fields above, N = sum(sizes). |
| uint8[?] | padding | the last 16-byte block of bitmap data is padded with zeroes if necessary. |


#### Modes

The "mode" bytes are a combination of two 4 bit values. The high nibble describes the animation speed in the range from 0 (slowest) to 7 (highest). The low nibble describes the animation to use:

| mode | animation         |
|------|-------------------|
| 0x00 | scroll leftwards  |
| 0x01 | scroll rightwards |
| 0x02 | scroll up         |
| 0x03 | scroll down       |
| 0x04 | fixed             |
| 0x05 | "snowflake"       |
| 0x06 | "picture"         |
| 0x07 | "animation"       |
| 0x08 | "laser"           |

