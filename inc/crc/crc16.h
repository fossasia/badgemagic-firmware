#ifndef __CRC16_H__
#define __CRC16_H__

uint16_t crc16_ccitt_update (uint16_t crc, uint8_t data);
uint16_t crc16_cal(uint16_t crc, uint8_t *data, int len);

static inline uint16_t crc16_ccitt(uint16_t crc, uint8_t *data, int len)
{
	return crc16_cal(crc, data, len);
}

#endif /* __CRC16_H__ */
