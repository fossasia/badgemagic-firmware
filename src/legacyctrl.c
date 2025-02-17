#include "data.h"
#include "power.h"
#include "leddrv.h"
#include "debug.h"
#include "legacyctrl.h"

int legacy_ble_rx(uint8_t *val, uint16_t len)
{
	_TRACE();
	static uint16_t c, data_len, n;
	static uint8_t *data;

	if (len != LEGACY_TRANSFER_WIDTH) {
		PRINT("Transfer width is not matched\n");
		return -1;
	}

	PRINT("val[%d]: ", len);
	for (int i=0; i<len; i++) {
		PRINT("%02X ", val[i]);
	}
	PRINT("\n");

	if (c == 0) {
		if (memcmp(val, "wang", 5)) {
			PRINT("Not a header\n");
			return -2;
		} else {
			free(data);
			data = malloc(sizeof(data_legacy_t));
			if (!data) {
				PRINT("insufficient memory\n");
				return -3;
			}
		}
	} else { // Re attempt after a failed transfer
		if (!memcmp(val, "wang", 5)) {
			free(data);
			c = 0;
			data = malloc(sizeof(data_legacy_t));
			if (!data) {
				PRINT("insufficient memory\n");
				return -3;
			}
		}
	}

	PRINT("Copying BLE value\n");
	memcpy(data + c * len, val, len);

	if (c == 1) {
		data_legacy_t *d = (data_legacy_t *)data;
		n = bigendian16_sum(d->sizes, 8);
		data_len = LEGACY_HEADER_SIZE + LED_ROWS * n;
		PRINT("Data len: %d\n", data_len);
		data = realloc(data, data_len);
		if (!data) {
			PRINT("insufficient memory\n");
			return -3;
		}
	}

	if (c > 2 && ((c+1) * LEGACY_TRANSFER_WIDTH) >= data_len) {
		PRINT("All bitmaps data received successfully\nWriting to flash.. ");
		data_flatSave(data, data_len);
		free(data);
		data = NULL;
		handle_after_rx();
		PRINT("Done\n");
	}

	c++;
	return 0;
}

int legacy_usb_rx(uint8_t *buf, uint16_t len)
{
	static uint16_t rx_len, data_len;
	static uint8_t *data;

	PRINT("dump first 8 bytes: %02x %02x %02x %02x %02x %02x %02x %02x\n",
				buf[0], buf[1], buf[2], buf[3],
				buf[4], buf[5], buf[6], buf[7]);

	if (rx_len == 0) {
		if (memcmp(buf, "wang", 5))
			return -1;

		int init_len = len > LEGACY_HEADER_SIZE ? len : sizeof(data_legacy_t);
		init_len += MAX_PACKET_SIZE;
		data = malloc(init_len);
	}

	memcpy(data + rx_len, buf, len);
	rx_len += len;

	if (!data_len) {
		data_legacy_t *d = (data_legacy_t *)data;
		uint16_t n = bigendian16_sum(d->sizes, 8);
		data_len = LEGACY_HEADER_SIZE + LED_ROWS * n;
		data = realloc(data, data_len);
	}

	if ((rx_len > LEGACY_HEADER_SIZE) && rx_len >= data_len) {
		data_flatSave(data, data_len);
		free(data);
		handle_after_rx();
	}
	return 0;
}