#include <crc/crc8.h>
#include <crc/crc16.h>

// Polynomial: x^8 + x^2 + x + 1 (0xE0)
uint8_t crc8_ccitt_update (uint8_t inCrc, uint8_t inData)
{
	uint8_t data = inCrc ^ inData;
	for (int i = 0; i < 8; i++) {
		if (( data & 0x80 ) != 0 ) {
			data <<= 1;
			data ^= 0x07;
		} else {
			data <<= 1;
		}
	}
	return data;
}

uint8_t crc_cal(uint8_t *a, int len)
{
	uint8_t crc = 0x00;
	for (int i=0; i<len; i++) {
		crc = crc8_ccitt_update(crc, a[i]);
	}
	return crc;
}

#define POLY    (0x1021)
#define HI8(a)  (((a) >> 8) & 0xFF)
#define LO8(a)  ((a) & 0xFF)

uint16_t crc16_ccitt_update(uint16_t crc, uint8_t data)
{
	data ^= LO8(crc);
	data ^= data << 4;
	return ((((uint16_t)data << 8) | HI8(crc)) ^ (uint8_t)(data >> 4) 
		^ ((uint16_t)data << 3));
}

// uint16_t crc16_cal(uint16_t crc, uint8_t *data, int len)
// {
// 	for (int i=0; i<len; i++) {
// 		crc = crc16_ccitt_update(crc, data[i]);
// 	}
// 	return crc;
// }

uint16_t crc16_cal(uint16_t crc, uint8_t *data, int len)
{
	int i, j;
	uint16_t c;

	for (j = 0; j < len; j++, data++) {
		c = ((crc >> 8) ^ *data) << 8;
		for (i = 0; i < 8; i++) {
			if (c & 0x8000)
				c = (c << 1) ^ POLY;
			else
				c = c << 1;
		}
		crc = c ^ (crc << 8);
	}
	return crc;
}