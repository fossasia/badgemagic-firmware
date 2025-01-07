#ifndef __NG_CTRL_H__
#define __NG_CTRL_H__

#include <stdint.h>

// enum NG_STATUS {
	
// };

uint8_t next_packet(uint8_t *val, uint16_t len);
uint8_t power_switch(uint8_t *val, uint16_t len);
uint8_t ble_setting(uint8_t *val, uint16_t len);
uint8_t streaming_setting(uint8_t *val, uint16_t len);
uint8_t stream_bitmap(uint8_t *val, uint16_t len);
uint8_t flash_splash_screen(uint8_t *val, uint16_t len);

extern const uint8_t (*cmd_lut[])(uint8_t *val, uint16_t len);
uint8_t ng_parse(uint8_t *val, uint16_t len);

#endif /* __NG_CTRL_H__ */
