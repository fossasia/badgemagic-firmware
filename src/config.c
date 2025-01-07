#include "config.h"
#include "resource.h"
#include "res/foss-asia-2.xbm"

#include "ISP583.h"
#include <stdlib.h>

#define __ble_name "LED Badge Magic"
// static sfmem_t ble_name = {sizeof(__ble_name) - 1, (uint8_t *)__ble_name};

// Default configuration
badge_cfg_t badge_cfg = {
	.ble.always_on = 1,
	.ble.devname.len = sizeof(__ble_name) - 1,
	.ble.devname.ptr = __ble_name,

	.led.brightness = 0,
	.led.scan_freq = 2000,

	.splash.bm.bits = foss_asia_2_bits,
	.splash.bm.w = 44,
	.splash.bm.h = 22,
	.splash.bm.fh = 11,
	.splash.speed_t = 30, // ms
};

// uint32_t cfglen(badge_cfg_t cfg)
// {
// 	uint32_t len = sizeof(badge_cfg_t);
// 	uint32_t len = sizeof(splash_cfg_t);
// 	uint32_t len = sizeof(led_cfg_t);
// 	uint32_t len = sizeof(ble_cfg_t);
// }

static int flash_write(uint8_t *buf, uint32_t base_addr, uint16_t len)
{
	return EEPROM_WRITE(base_addr, buf, len);
}

static int flash_read(uint8_t *buf, uint32_t base_addr, uint16_t len)
{
	return EEPROM_READ(base_addr, buf, len);
}

static int ble_cfg_write(ble_cfg_t *cfg, uint32_t base_addr)
{
	int ret;
	ret = flash_write(cfg, base_addr, sizeof(ble_cfg_t));
	if (ret) {
		return ret;
	}

	base_addr += sizeof(ble_cfg_t);
	ret = flash_write(cfg->devname.ptr, base_addr, cfg->devname.len);
	if (ret) {
		return ret;
	}

	return 0;
}

static int ble_cfg_read(ble_cfg_t *cfg, uint32_t base_addr)
{
	int ret;
	ret = flash_read(cfg, base_addr, sizeof(ble_cfg_t));
	if (ret) {
		return ret;
	}

	uint32_t devname_offs = (uint32_t)cfg->devname.ptr;
	ret = flash_read(cfg->devname.ptr, devname_offs, cfg->devname.len);
	if (ret) {
		return ret;
	}

	return 0;
}

#define XBM_BYTE_PER_ROW(xbm) xbm.w / 8 + ((xbm.w % 8)>0) // Number of bytes in a row

static int splash_cfg_read(splash_cfg_t *cfg, uint32_t base_addr)
{
	int ret;
	ret = flash_read(cfg, base_addr, sizeof(splash_cfg_t));
	if (ret) {
		return ret;
	}

	uint32_t devname_offs = (uint32_t) cfg->bm.bits;
	int len = XBM_BYTE_PER_ROW(cfg->bm) * cfg->bm.h;
	ret = flash_read(cfg->bm.bits, devname_offs, len);
	if (ret) {
		return ret;
	}

	return 0;
}

// cfg.bm.bits will be written next to cfg header and its pointer will be
// modified to be cfg.xbm.bits = base_addr + sizeof(splash_cfg_t) instead of
// pointer in memory
static int splash_cfg_write(splash_cfg_t *cfg, uint32_t base_addr)
{
	int ret;

	// write bits next to splash cfg
	int len = XBM_BYTE_PER_ROW(cfg->bm) * (cfg->bm.h);
	int bits_offs = base_addr + sizeof(splash_cfg_t);
	ret = flash_write(cfg->bm.bits, bits_offs, len);
	if (ret) {
		return ret;
	}

	// point to eeprom offset instead of mem
	cfg->bm.bits = base_addr + sizeof(splash_cfg_t);

	// write splash cfg
	ret = flash_write(cfg, base_addr, sizeof(splash_cfg_t));
	if (ret) {
		return ret;
	}

	return 0;
}

int cfg_read(badge_cfg_t *cfg, uint32_t base_addr)
{
	int ret = flash_read(cfg, base_addr, sizeof(badge_cfg_t));
	if (ret) {
		return ret;
	}
	return 0;
}

int cfg_init()
{
	
}

int cfg_read_alloc(badge_cfg_t **buf, uint32_t base_addr)
{
	int ret;
	*buf = malloc(sizeof(badge_cfg_t));
	ret = cfg_read(base_addr, *buf);
	if (ret) {
		return ret;
	}

	(*buf)->ble.devname.ptr = malloc(sizeof((*buf)->ble.devname.len));
	ret = ble_cfg_read((*buf)->ble.devname.ptr, (*buf)->ble.devname.len);
	if (ret) {
		return ret;
	}

	int rb = XBM_BYTE_PER_ROW((*buf)->splash.bm);
	(*buf)->splash.bm.bits = malloc(rb);
	ret = splash_cfg_read((*buf)->splash.bm.bits, rb);
	if (ret) {
		return ret;
	}

	return 0;
}