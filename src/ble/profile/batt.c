#include "utils.h"

#include "../../power.h"

static const uint16_t ServiceUUID = 0x180F;
static const gattAttrType_t service = {2, (uint8_t *)&ServiceUUID};

static const uint16_t battLv_CharUUID = 0x2A19;
static uint8 battLv_CharProps = GATT_PROP_READ;
static uint8 battLv_Val[1];

static gattAttribute_t attr_table[] = {
	ATTR_DECLAR(primaryServiceUUID, 2, GATT_PERMIT_READ, &service),

	CHAR_DECLAR(&battLv_CharProps),
	CHAR_VAL_DECLAR(&battLv_CharUUID, 2, GATT_PERMIT_READ, battLv_Val),
};

static bStatus_t read_handler(uint16_t connHandle, gattAttribute_t *pAttr,
				uint8_t *p_value, uint16_t *pLen, uint16_t offset,
				uint16_t maxLen, uint8_t method)
{
	uint16_t uuid = BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1]);

	if (uuid != battLv_CharUUID) {
		*pLen = 0;
		return ATT_ERR_ATTR_NOT_FOUND;
	}

	*pLen = 1;
	battLv_Val[0] = batt_raw2percent(batt_raw());
	tmos_memcpy(p_value, battLv_Val, *pLen);

	return SUCCESS;
}

static gattServiceCBs_t service_handlers = {
	read_handler,
	NULL,
	NULL
};

int batt_registerService()
{
	uint8 status = SUCCESS;

	status = GATTServApp_RegisterService(attr_table,
				GATT_NUM_ATTRS(attr_table),
				GATT_MAX_ENCRYPT_KEY_SIZE,
				&service_handlers);
	return (status);
}