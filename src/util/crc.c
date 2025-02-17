#include "crc.h"

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