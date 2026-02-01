#ifndef __UTIL_CRC_H__
#define __UTIL_CRC_H__

#include <stdint.h>

uint8_t crc8_ccitt_update (uint8_t inCrc, uint8_t inData);
uint8_t crc_cal(uint8_t *a, int len);

#endif /* __UTIL_CRC_H__ */
