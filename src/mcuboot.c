#include <stdint.h>
#include <stddef.h>

#include <sysflash/sysflash.h>
#include <flash_map_backend/flash_map_backend.h>
#include <mcuboot_config/mcuboot_logging.h>

#include <CH58x_common.h>
#include <ISP583.h>
#include <CH58x_flash.h>

#define BOOTLD_OFFS (0x0)
#define BOOTLD_SIZE (64 * 1024)

#define APP_PRIMARY_OFFS (BOOTLD_OFFS + BOOTLD_SIZE)
#define APP_SIZE (200 * 1024)
#define APP_SECONDARY_OFFS (APP_PRIMARY_OFFS + APP_SIZE)

static const fa_t bootloader = {
	.fa_id = FLASH_AREA_BOOTLOADER,
	.fa_device_id = FLASH_DEVICE_INTERNAL_FLASH,
	.fa_off = BOOTLD_OFFS,
	.fa_size = BOOTLD_SIZE,
};

static const fa_t primary_img0 = {
	.fa_id = FLASH_AREA_IMAGE_PRIMARY(0),
	.fa_device_id = FLASH_DEVICE_INTERNAL_FLASH,
	.fa_off = APP_PRIMARY_OFFS,
	.fa_size = APP_SIZE,
};

static const fa_t secondary_img0 = {
	.fa_id = FLASH_AREA_IMAGE_SECONDARY(0),
	.fa_device_id = FLASH_DEVICE_INTERNAL_FLASH,
	.fa_off = APP_SECONDARY_OFFS,
	.fa_size = APP_SIZE,
};

static const fa_t *s_flash_areas[] = {
	&bootloader,
	&primary_img0,
	&secondary_img0,
};

#define ARRAY_SIZE(arr) sizeof(arr)/sizeof(arr[0])

static const fa_t *fa_lookup(uint8_t id)
{
	for (int i = 0; i < ARRAY_SIZE(s_flash_areas); i++) {
		if(id == s_flash_areas[i]->fa_id) {
			return s_flash_areas[i];
		}
	}
	return NULL;
}

int flash_area_open(uint8_t id, const fa_t **area_outp) 
{
	MCUBOOT_LOG_DBG("%s: ID=%d", __func__, (int)id);
	*area_outp = fa_lookup(id);
	// sys_safe_access_enable();
	// // enable flash Rom write/erase
	// R8_GLOB_ROM_CFG |= RB_ROM_CTRL_EN | RB_ROM_DATA_WE | RB_ROM_CODE_WE;
	// sys_safe_access_disable();
	// FLASH_ROM_START_IO();
	return area_outp != NULL ? 0 : -1;
}

void flash_area_close(const fa_t *area)
{

}

int flash_area_read(const fa_t *fa, uint32_t off, void *dst,
				uint32_t len)
{
	// MCUBOOT_LOG_DBG("%s", __func__);
	if (fa->fa_device_id != FLASH_DEVICE_INTERNAL_FLASH) {
		return -1;
	}

	const uint32_t end_offset = off + len;
	if (end_offset > fa->fa_size) {
		MCUBOOT_LOG_ERR("%s: Out of Bounds (0x%x vs 0x%x)", __func__, end_offset, fa->fa_size);
		return -1;
	}

	// TODO: find some way to access the flash over memory mapped address while
	// code flash is protected
	uint32_t addr = (fa->fa_off + off);
	FLASH_ROM_READ(addr, dst, len);
	// memcpy(dst, addr, len);

	return 0;
}

static int write_flash_align(uint32_t addr, const void *src, uint32_t len)
{
	int ret, i;
	uint32_t split_sz = EEPROM_PAGE_SIZE;
	__attribute__ ((aligned (4))) uint8_t src_aligned [split_sz];

	if ((uint32_t)src % 4 == 0) { // src is aligned to 4
		return - FLASH_ROM_WRITE(addr, src, len);
	}
	
	if (len <= split_sz) {
		memcpy(src_aligned, src, len);
		return - FLASH_ROM_WRITE(addr, src_aligned, len);
	}

	// if len > split_sz
	for (i = 0; i < len - split_sz; i += split_sz) {
		MCUBOOT_LOG_DBG("addr %08x, src_aligned %08x, len %08x", addr + i, (int)src_aligned, split_sz);
		memcpy(src_aligned, src + i, split_sz);
		ret = FLASH_ROM_WRITE(addr + i, src_aligned, split_sz);
		if (ret) {
			return -1;
		}
	}
	split_sz = i - len; // remain size
	i = len - split_sz;
	MCUBOOT_LOG_DBG("addr %08x, src_aligned %08x, len %08x", addr + i, (int)src_aligned, split_sz);
	memcpy(src_aligned, src + i, split_sz);
	return - FLASH_ROM_WRITE(addr + i, src_aligned, split_sz);
}

__HIGH_CODE
int flash_area_write(const fa_t *fa, uint32_t off, const void *src,
				uint32_t len)
{
	MCUBOOT_LOG_DBG("%s", __func__);
	if (fa->fa_device_id != FLASH_DEVICE_INTERNAL_FLASH) {
		return -1;
	}

	const uint32_t end_offset = off + len;
	if (end_offset > fa->fa_size) {
		MCUBOOT_LOG_ERR("%s: Out of Bounds (0x%x vs 0x%x)", __func__, end_offset, fa->fa_size);
		return -1;
	}
	
	uint32_t addr = fa->fa_off + off;
	MCUBOOT_LOG_DBG("%s: Addr: 0x%08x Length: %d", __func__, addr, len);
	if (len == 0) {
		MCUBOOT_LOG_WRN("Zero length");
		return 0;
	}

	_TRACE();
	uint32_t pirqv;
	MCUBOOT_LOG_DBG("addr %08x, src %08x, len %08x", addr, (int)src, len);
	// FLASH_ROM_WRITE(addr, src, len);
	int ret = write_flash_align(addr, src, len);
	dump_mem(addr, len);
	// mDelaymS(1);
	_TRACE();
	
	#if VALIDATE_PROGRAM_OP
	if (FLASH_ROM_VERIFY(addr, src, len) != 0) {
		MCUBOOT_LOG_ERR("%s: Program Failed", __func__);
		assert(0);
	}
	#endif

	return ret;
}

int flash_area_erase(const fa_t *fa, uint32_t off, uint32_t len)
{
	if (fa->fa_device_id != FLASH_DEVICE_INTERNAL_FLASH) {
		return -1;
	}

	if ((len % EEPROM_BLOCK_SIZE) != 0 || (off % EEPROM_BLOCK_SIZE) != 0) {
		MCUBOOT_LOG_ERR("%s: Not aligned on sector Offset: 0x%x Length: 0x%x", __func__,
						(int)off, (int)len);
		return -1;
	}

	const uint32_t start_addr = fa->fa_off + off;
	MCUBOOT_LOG_DBG("%s: Addr: 0x%08x Length: %d", __func__, (int)start_addr, (int)len);

	uint32_t *val = (void *)start_addr;
	// dump_mem(val, 100);
	if (FLASH_ROM_ERASE(start_addr, len)) {
		MCUBOOT_LOG_ERR("Flash erase failed");
		return -1;
	}

	// dump_mem(val, 100);
	// #if VALIDATE_PROGRAM_OP
	for (size_t i = 0; i < (len / 4) - 1; i++) {
		val++;
		// dump_mem(val, 4);
		if (*val != 0xf3f9bda9) {
			MCUBOOT_LOG_ERR("%s: Erase at 0x%x Failed", __func__, (int)val);
			while(1);
		}
	}
	// #endif

	return 0;
}

size_t flash_area_align(const fa_t *area)
{
	return 4;
}

uint8_t flash_area_erased_val(const fa_t *area)
{
	return 0xff;
}

int flash_area_get_sectors(int fa_id, uint32_t *count,
				struct flash_sector *sectors)
{
	MCUBOOT_LOG_DBG("%s", __func__);
	const fa_t *fa = fa_lookup(fa_id);
	if (fa->fa_device_id != FLASH_DEVICE_INTERNAL_FLASH) {
		return -1;
	}

	const size_t sector_size = EEPROM_BLOCK_SIZE;
	uint32_t total_count = 0;
	for (size_t off = 0; off < fa->fa_size; off += sector_size) {
		// Note: Offset here is relative to flash area, not device
		sectors[total_count].fs_off = off;
		sectors[total_count].fs_size = sector_size;
		total_count++;
	}

	*count = total_count;
	return 0;
}

int flash_area_id_from_multi_image_slot(int image_index, int slot)
{
	MCUBOOT_LOG_DBG("%s", __func__);
	switch (slot) {
		case 0:
			return FLASH_AREA_IMAGE_PRIMARY(image_index);
		case 1:
			return FLASH_AREA_IMAGE_SECONDARY(image_index);
	}

	MCUBOOT_LOG_ERR("Unexpected Request: image_index=%d, slot=%d",
				image_index, slot);
	return -1;
}

int flash_area_id_from_image_slot(int slot)
{
	return flash_area_id_from_multi_image_slot(0, slot);
}


/*< Obtains ID of the flash area characterized by `fa` */
int flash_area_get_id(const fa_t *fa)
{
	return fa->fa_id;
}

/*< Obtains ID of a device the flash area `fa` described region resides on */
int flash_area_get_device_id(const fa_t *fa)
{
	return fa->fa_device_id;
}

/*< Obtains offset, from the beginning of a device, the flash area described
* region starts at */
uint32_t flash_area_get_off(const fa_t *fa)
{
	return fa->fa_off;
}

/*< Obtains size, from the offset, of the flash area `fa` characterized region */
uint32_t flash_area_get_size(const fa_t *fa)
{
	return fa->fa_size;
}

int flash_area_to_sectors(int idx, int *cnt, fa_t *ret)
{
	MCUBOOT_LOG_DBG("%s: idx=%d cnt=%d", __func__, idx, *cnt);

	fa_t *fa = fa_lookup(idx);
	if (fa->fa_device_id != FLASH_DEVICE_INTERNAL_FLASH) {
		return -1;
	}

	*cnt = fa->fa_size / EEPROM_BLOCK_SIZE;
	memcpy(ret, fa, sizeof(fa_t));

	return 0;
}