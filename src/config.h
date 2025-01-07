#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <stdint.h>

#include "xbm.h"

// safe mem
typedef struct {
	uint16_t len;
	uint8_t *ptr;
} __attribute__((packed)) sfmem_t;

typedef struct {
	uint8_t always_on; // keep BLE on in normal mode
	// Turn on Bluetooth while in Normal mode and disable Downloads mode
	sfmem_t devname;
} __attribute__((packed)) ble_cfg_t;

typedef struct {
	xbm_t bm;
	// Speed in period of micro second. The lower value, the higher speed
	uint16_t speed_t;
} __attribute__((packed)) splash_cfg_t;

typedef struct {
	uint8_t brightness;
	uint16_t scan_freq; // Scan frequency
} __attribute__((packed)) led_cfg_t;

typedef struct {
	ble_cfg_t ble;
	splash_cfg_t splash;
	led_cfg_t led;
	// uint8_t reset_after_complete_download;
} __attribute__((packed)) badge_cfg_t;

extern badge_cfg_t badge_cfg;

#endif /* __CONFIG_H__ */
