#include "ngctrl.h"

#include "CH58xBLE_LIB.h"
#include "CH583SFR.h"

#include "ble/profile.h"
#include "power.h"
#include "debug.h"

uint8_t next_packet(uint8_t *val, uint16_t len)
{
	PRINT(__func__);
	PRINT(": to be implemented\n");
	return 0;
}

uint8_t power_switch(uint8_t *val, uint16_t len)
{
	PRINT(__func__);
	PRINT(": to be implemented\n");
	poweroff();
	return 0;
}

uint8_t ble_setting(uint8_t *val, uint16_t len)
{
	PRINT(__func__);
	PRINT(": to be implemented\n");
	return 0;
}

uint8_t flash_splash_screen(uint8_t *val, uint16_t len)
{
	PRINT(__func__);
	PRINT(": to be implemented\n");
	return 0;
}

const uint8_t (*cmd_lut[])(uint8_t *val, uint16_t len) = {
	next_packet,
	power_switch,
	streaming_setting,
	stream_bitmap,
	ble_setting, // always-on, adv name
	flash_splash_screen
};

#define CMD_LUT_LEN (sizeof(cmd_lut) / sizeof(cmd_lut[0]))

uint8_t ng_parse(uint8_t *val, uint16_t len)
{
	uint8_t cmd = val[0];
	PRINT("LUT_LEN: %02x \n", CMD_LUT_LEN);
	if (cmd >= CMD_LUT_LEN) {
		PRINT("invalid command!\n");
		return bleInvalidRange;
	}

	if (cmd_lut[cmd]) {
		PRINT("executing [cmd %02x] \n", cmd);
		uint8_t ret = (*cmd_lut[cmd])(val + 1, len - 1);
		ng_notify(&ret, 1); // response to the client app
	} else {
		PRINT("function is not defined!\n");
	}
	return SUCCESS;
}