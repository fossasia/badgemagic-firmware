#include "../common.h"
#include "utils.h"

// TODO: Automatic assign these on build
static const uint8_t    systemId_val[] = {0, 0, 0, 0, 0, 0, 0, 0};
static const uint16_t   systemId_UUID = SYSTEM_ID_UUID;

#ifdef USBC_VERSION
#define USBC_VER_PREFIX "(C) "
#define USBC_VER_SUFIX "-C"
#else
#define USBC_VER_PREFIX ""
#define USBC_VER_SUFIX ""
#endif

static const uint8_t    modelNumber_val[] = "BM1144" USBC_VER_SUFIX;
static const uint16_t   modelNumber_UUID = MODEL_NUMBER_UUID;

const uint16_t          serialNumber_UUID = SERIAL_NUMBER_UUID;
static const uint8_t    serialNumber_val[] = "N/A";

const uint16_t          firmwareRev_UUID = FIRMWARE_REV_UUID;
static const uint8_t    firmwareRev_val[] = USBC_VER_PREFIX VERSION;

const uint16_t          hardwareRev_UUID = HARDWARE_REV_UUID;
static const uint8_t    hardwareRev_val[] = "20240908";

const uint16_t          softwareRev_UUID = SOFTWARE_REV_UUID;
static const uint8_t    softwareRev_val[] = "N/A";

uint16_t                mfr_name_UUID = MANUFACTURER_NAME_UUID;
static const uint8_t    mfr_name_val[] = "FOSSASIA";

static const ble_char_t dev_info[7] = {
	{
		{systemId_val, sizeof(systemId_val)}, 
		GATT_PROP_READ, { (uint8_t*)&systemId_UUID, 2}
	}, {
		{modelNumber_val, sizeof(modelNumber_val) - 1}, 
		GATT_PROP_READ, { (uint8_t*)&modelNumber_UUID, 2}
	}, {
		{serialNumber_val, sizeof(serialNumber_val) - 1}, 
		GATT_PROP_READ, { (uint8_t*)&serialNumber_UUID, 2}
	}, {
		{firmwareRev_val, sizeof(firmwareRev_val) - 1}, 
		GATT_PROP_READ, { (uint8_t*)&firmwareRev_UUID, 2}
	}, {
		{hardwareRev_val, sizeof(hardwareRev_val) - 1}, 
		GATT_PROP_READ, { (uint8_t*)&hardwareRev_UUID, 2}
	}, {
		{softwareRev_val, sizeof(softwareRev_val) - 1}, 
		GATT_PROP_READ, { (uint8_t*)&softwareRev_UUID, 2}
	}, {
		{mfr_name_val, sizeof(mfr_name_val) - 1},
		GATT_PROP_READ,
		{ (uint8_t*)&mfr_name_UUID, 2},
	}
};

const uint16_t service_UUID = DEVINFO_SERV_UUID;
static const gattAttrType_t service = {ATT_BT_UUID_SIZE, (uint8_t *)&service_UUID};

static bStatus_t read_handler(uint16_t connHandle, gattAttribute_t *pAttr,
				uint8_t *p_value, uint16_t *pLen, uint16_t offset,
				uint16_t maxLen, uint8_t method)
{
	uint16_t uuid = BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1]);

	if (uuid < SYSTEM_ID_UUID || uuid > MANUFACTURER_NAME_UUID ) {
		*pLen = 0;
		return ATT_ERR_ATTR_NOT_FOUND;
	}

	uint16_t i = uuid - SYSTEM_ID_UUID;
	if(offset >= dev_info[i].val.size) {
		return ATT_ERR_INVALID_OFFSET;
	} else {
		*pLen = MIN(maxLen, (dev_info[i].val.size - offset));
		tmos_memcpy(p_value, &dev_info[i].val.bytes[offset], *pLen);
	}
	return SUCCESS;
}

gattServiceCBs_t devInfoCBs = {
	read_handler,
	NULL,
	NULL
};

int devInfo_registerService()
{	
	static gattAttribute_t attr_table[] = {
		ATTR_DECLAR(primaryServiceUUID, 2, GATT_PERMIT_READ, &service),
		
		CHAR_DECLAR(&dev_info[0].props),
		CHAR_VAL_DECLAR(dev_info[0].uuid.bytes, dev_info[0].uuid.size, \
					GATT_PERMIT_READ, dev_info[0].val.bytes),

		CHAR_DECLAR(&dev_info[1].props),
		CHAR_VAL_DECLAR(dev_info[1].uuid.bytes, dev_info[1].uuid.size, \
					GATT_PERMIT_READ, dev_info[1].val.bytes),

		CHAR_DECLAR(&dev_info[2].props),
		CHAR_VAL_DECLAR(dev_info[2].uuid.bytes, dev_info[2].uuid.size, \
					GATT_PERMIT_READ, dev_info[2].val.bytes),

		CHAR_DECLAR(&dev_info[3].props),
		CHAR_VAL_DECLAR(dev_info[3].uuid.bytes, dev_info[3].uuid.size, \
					GATT_PERMIT_READ, dev_info[3].val.bytes),

		CHAR_DECLAR(&dev_info[4].props),
		CHAR_VAL_DECLAR(dev_info[4].uuid.bytes, dev_info[4].uuid.size, \
					GATT_PERMIT_READ, dev_info[4].val.bytes),

		CHAR_DECLAR(&dev_info[5].props),
		CHAR_VAL_DECLAR(dev_info[5].uuid.bytes, dev_info[5].uuid.size, \
					GATT_PERMIT_READ, dev_info[5].val.bytes),

		CHAR_DECLAR(&dev_info[6].props),
		CHAR_VAL_DECLAR(dev_info[6].uuid.bytes, dev_info[6].uuid.size, \
					GATT_PERMIT_READ, dev_info[6].val.bytes),
	};

	return GATTServApp_RegisterService(attr_table,
				GATT_NUM_ATTRS(attr_table),
				GATT_MAX_ENCRYPT_KEY_SIZE,
				&devInfoCBs);
}