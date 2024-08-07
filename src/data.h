#ifndef __DATA_H__
#define __DATA_H__

#include <stdint.h>

#include "fb.h"

typedef struct {
	uint8_t header[6];
	uint8_t flash;
	uint8_t marquee;
	uint8_t modes[8];

	uint16_t sizes[8]; // big endian

	uint8_t pad6[6];
	uint8_t timestamp[6];
	uint8_t pad4[4];

	uint8_t separator[16];

	uint8_t *bitmapdata;
} data_legacy_t;

#define LEGACY_TRANSFER_WIDTH (16)
#define LEGACY_HEADER_SIZE (sizeof(data_legacy_t) - sizeof(uint8_t *))

static inline uint16_t bswap16(uint16_t i) {
	return (i >> 8) | (i << 8);
}
uint32_t bigendian16_sum(uint16_t *s, int len);

uint32_t data_flatSave(uint8_t *data, uint32_t len);
uint16_t data_flash2newmem(uint8_t **chunk, uint32_t n);

void chunk2buffer(uint8_t *chunk, uint16_t size, uint16_t *buf);
void chunk2fb(uint8_t *chunk, uint16_t size, fb_t *fb);

fb_t *chunk2newfb(uint8_t *chunk, uint16_t size);
fb_t *flash2newfb(uint32_t n);

#endif /* __DATA_H__ */
