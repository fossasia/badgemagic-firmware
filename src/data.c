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

void data_get_header(data_legacy_t *buf)
{
	EEPROM_READ(0, buf, LEGACY_HEADER_SIZE);
}

uint16_t data_flash2newmem(uint8_t **chunk, uint32_t n)
{
	data_legacy_t header;
	data_get_header(&header);

	if (memcmp(header.header, "wang", 5))
		return 0;

	uint16_t size = bswap16(header.sizes[n]) * LED_ROWS;
	if (size == 0)
		return 0;

	uint16_t offs = LEGACY_HEADER_SIZE
		+ bigendian16_sum(header.sizes, n) * LED_ROWS;

	*chunk = malloc(size);
	EEPROM_READ(offs, *chunk, size);
	return size;
}

static void __chunk2buffer(uint16_t *bm, uint8_t *chunk, int col) 
{
	uint16_t tmpbm[8] = {0};
	for (int i=0; i<8; i++) {
		for (int j=0; j<11; j++) {
			tmpbm[i] |= ((chunk[j] >> (7-i)) & 0x01) << j;
		}
	}
	for (int i=0; i<8; i++) {
		bm[col+i] = tmpbm[i];
	}
}

void chunk2buffer(uint8_t *chunk, uint16_t size, uint16_t *buf)
{
	for (int i=0; i<size/11; i++) {
		__chunk2buffer(buf, &chunk[i*11], 8 * i);
	}
}

void chunk2bm(uint8_t *chunk, uint16_t size, bm_t *bm)
{
	chunk2buffer(chunk, size, bm->buf);
}

bm_t *chunk2newbm(uint8_t *chunk, uint16_t size)
{
	bm_t *bm = bm_new((size*8)/11);
	chunk2bm(chunk, size, bm);
	return bm;
}

bm_t *flash2newbm(uint32_t n)
{
	uint8_t *buf;
	uint16_t size = data_flash2newmem(&buf, n);
	if (size == 0)
		return NULL;

	bm_t *bm = chunk2newbm(buf, size);
	data_legacy_t header;
	data_get_header(&header);

	bm->is_flash = (header.flash & (1 << n)) != 0;
	bm->is_marquee = (header.marquee & (1 << n)) != 0;
	bm->modes = header.modes[n];

	free(buf);
	return bm;
}
