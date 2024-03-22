## Badge Data Format for BLE

### Connection Information

The pixel data is transferred to the badge via BLE. The badge advertizes the name "LSLED" and uses a vendor specific service/characteristic:

- Service-UUID: 0000fee0-0000-1000-8000-00805f9b34fb
- Characteristic: 0000fee1-0000-1000-8000-00805f9b34fb

The characteristic accepts 16 byte long data packets.

### Data Format

The badge supports up to 8 bitmaps which can have various features enabled.

| offset    | byte-size | format       | name       | description  |
| --------- | --------- | ------------ | ---------- | ------------ |
| 00h       | 6         | uint8[6]     | header     | "wang\0\0" - magic number to indicate a new data frame begin |
| 06h       | 1         | uint8        | flash      | bitfield describing which of the 8 bitmaps are flashing |
| 07h       | 1         | uint8        | marquee    | bitfield describing which of the 8 bitmaps feature an animated border |
| 08h       | 8         | uint8[8]     | modes      | speed+mode for each of the 8 bitmaps. See below. |
| 10h       | 16        | uint16[8]    | sizes      | count of 8 pixel wide chunks for each bitmap (Big endian) |
| 20h       | 6         | uint8[6]     | padding    | 6 bytes zeroes of padding
| 26h       | 6         | uint8[6]     | timestamp  | one byte each for (year % 256), month, day, hour, minute, second. Purpose unclear.
| 2Ch       | 4         | uint8[4]     | padding    | 4 bytes zeroes of padding
| 30h       | 16        | uint8[16]    | separator  | zeroes to separate header from bitmap data |
| 40h       | 11*N      | uint8[11\*N] | bitmapdata | image data for all bitmaps. Each bitmap consists of a number of 11 byte chunks describing an 8x11 bitmap, each byte describes 8 pixels, starting with the most significant bit. The number of chunks is listed in the "sizes" fields above, N = sum(sizes). |
| 40h+11*N  | ?         | uint8[?]     | padding    | the last 16-byte block of bitmap data is padded with zeroes if necessary. |

Because the characteristic data value is fixed at 16 bytes, it only be able to
receive 16 bytes once each time, so the big fat data frame will be split into
16-byte chunks and will be sent out in sequence. For example, 16 bytes from
offset 00h-0Fh will be sent first, followed by the next 16 bytes from offset
10h-1Fh, and so on. For the last 16-byte of bitmap data, there would be some
cases, where some last bytes were not used, these should be filled with zeros.

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

