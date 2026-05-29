#ifndef __BASE64_H__
#define __BASE64_H__

#include <stddef.h>

size_t base64_decode_len(const char *in);

int base64_encode(uint8_t *dst, size_t dlen, size_t *olen, const uint8_t *src,
				size_t slen);

int base64_decode(uint8_t *dst, size_t dlen, size_t *olen, const uint8_t *src,
				size_t slen);

#endif /* __BASE64_H__ */
