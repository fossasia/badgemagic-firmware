#include "utils.h"

#include "../../data.h"
#include "../../power.h"
#include "../../leddrv.h"

static const uint16_t ServiceUUID = 0xFEE0;
static const gattAttrType_t service = {2, (uint8_t *)&ServiceUUID};

static const uint16_t RxCharUUID = 0xFEE1;
static uint8 RxCharProps = GATT_PROP_WRITE;
static uint8 RxCharVal[16];

static gattAttribute_t attr_table[] = {
	ATTR_DECLAR(primaryServiceUUID, 2, GATT_PERMIT_READ, &service),

	CHAR_DECLAR(&RxCharProps),
	CHAR_VAL_DECLAR(&RxCharUUID, 2, GATT_PERMIT_WRITE, RxCharVal),
};

static bStatus_t receive(uint8_t *val, uint16_t len)
{
	static uint16_t c, data_len, n;
	static uint8_t *data;
	if (len != LEGACY_TRANSFER_WIDTH) {
		return ATT_ERR_INVALID_VALUE_SIZE;
	}
	if (c == 0) {
		if (memcmp(val, "wang\0\0", 6)) {
			return ATT_ERR_INVALID_VALUE;
		} else {
			data = malloc(sizeof(data_legacy_t));
		}
	}

	memcpy(data + c * len, val, len);

	if (c == 1) {
		data_legacy_t *d = (data_legacy_t *)data;
		n = bigendian16_sum(d->sizes, 8);
		data_len = LEGACY_HEADER_SIZE + LED_ROWS * n;
		data = realloc(data, data_len);
	}

	if (c > 2 && ((c+1) * LEGACY_TRANSFER_WIDTH) >= data_len) {
		data_flatSave(data, data_len);
		reset_jump();
	}

	c++;
	return SUCCESS;
}

static bStatus_t write_handler(uint16 connHandle, gattAttribute_t *pAttr,
				uint8 *pValue, uint16 len, uint16 offset, uint8 method)
{
	if(gattPermitAuthorWrite(pAttr->permissions)) {
		return ATT_ERR_INSUFFICIENT_AUTHOR;
	}

	uint16_t uuid = BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1]);
	if(uuid == RxCharUUID) {
		return receive(pValue, len);
	}
	return ATT_ERR_ATTR_NOT_FOUND;
}

gattServiceCBs_t service_handlers = {
	NULL,
	write_handler,
	NULL
};

int legacy_registerService()
{
	uint8 status = SUCCESS;

	status = GATTServApp_RegisterService(attr_table,
				GATT_NUM_ATTRS(attr_table),
				GATT_MAX_ENCRYPT_KEY_SIZE,
				&service_handlers);
	return (status);
}