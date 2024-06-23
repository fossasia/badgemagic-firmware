#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "ISP583.h"

#include "data.h"
#include "leddrv.h"

uint32_t bigendian16_sum(uint16_t *s, int len)
{
	uint32_t sum = 0;
	while (len-- > 0) {
		sum += bswap16(s[len]);
	}
	return sum;
}

uint32_t data_flatSave(uint8_t *data, uint32_t len)
{
	uint32_t r = EEPROM_ERASE(0, len);
	if (r) 
		return r;
	return EEPROM_WRITE(0, data, len);
}

uint16_t data_flash2newmem(uint8_t **chunk, uint32_t n)
{
	data_legacy_t header;
	EEPROM_READ(0, &header, LEGACY_HEADER_SIZE);

	uint16_t size = bswap16(header.sizes[n]) * LED_ROWS;
	if (size == 0)
		return 0;

	uint16_t offs = LEGACY_HEADER_SIZE
		+ bigendian16_sum(header.sizes, n) * LED_ROWS;

	*chunk = malloc(size);
	EEPROM_READ(offs, *chunk, size);
	return size;
}

static void __chunk2buffer(uint16_t *fb, uint8_t *chunk, int col) 
{
	uint16_t tmpfb[8] = {0};
	for (int i=0; i<8; i++) {
		for (int j=0; j<11; j++) {
			tmpfb[i] |= ((chunk[j] >> (7-i)) & 0x01) << j;
		}
	}
	for (int i=0; i<8; i++) {
		fb[col+i] = tmpfb[i];
	}
}

void chunk2buffer(uint8_t *chunk, uint16_t size, uint16_t *buf)
{
	for (int i=0; i<size/11; i++) {
		__chunk2buffer(buf, &chunk[i*11], 8 * i);
	}
}

void chunk2fb(uint8_t *chunk, uint16_t size, fb_t *fb)
{
	chunk2buffer(chunk, size, fb->buf);
}

fb_t *chunk2newfb(uint8_t *chunk, uint16_t size)
{
	fb_t *fb = fb_new((size*8)/11);
	chunk2fb(chunk, size, fb);
	return fb;
}

fb_t *flash2newfb(uint32_t n)
{
	uint8_t *buf;
	uint16_t size = data_flash2newmem(&buf, n);
	if (size == 0)
		return NULL;
	return chunk2newfb(buf, size);
	free(buf);
}
