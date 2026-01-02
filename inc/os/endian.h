#ifndef __OS_ENDIAN_H__
#define __OS_ENDIAN_H__

#include <machine/endian.h>

static inline uint32_t htonl(uint32_t x)
{
	return __htonl(x);
}

static inline uint16_t htons(uint16_t x)
{
	return __htons(x);
}

static inline uint32_t ntohl(uint32_t x)
{
	return __ntohl(x);
}

static inline uint16_t ntohs(uint16_t x)
{
	return __ntohs(x);
}


#endif /* __OS_ENDIAN_H__ */
