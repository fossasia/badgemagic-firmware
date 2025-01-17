#include "utils.h"

#include "CH583SFR.h"

#include "../../power.h"
#include "../../ngctrl.h"
#include "../../debug.h"

static const uint16_t ServiceUUID = 0xF055;
static const gattAttrType_t service = {2, (uint8_t *)&ServiceUUID};

static const uint16_t TxCharUUID = 0xF056;
static uint8_t TxCharProps = GATT_PROP_READ | GATT_PROP_NOTIFY;
static uint8_t TxCharVal[256];
static uint16_t TxLen;

static const uint16_t RxCharUUID = 0xF057;
static uint8_t RxCharProps = GATT_PROP_WRITE;
#define RxCharVal TxCharVal
static gattCharCfg_t TxCCCD[1];

static gattAttribute_t attr_table[] = {
	ATTR_DECLAR(primaryServiceUUID, 2, GATT_PERMIT_READ, &service),

	CHAR_DECLAR(&RxCharProps),
	CHAR_VAL_DECLAR(&RxCharUUID, 2, GATT_PERMIT_WRITE, RxCharVal),

	CHAR_DECLAR(&TxCharProps),
	CHAR_VAL_DECLAR(&TxCharUUID, 2, GATT_PERMIT_READ, TxCharVal),
	ATTR_DECLAR(clientCharCfgUUID, 2, GATT_PERMIT_READ | GATT_PERMIT_WRITE, TxCCCD),
};

#define notiAttr attr_table[4]


static bStatus_t write_handler(uint16 connHandle, gattAttribute_t *pAttr,
				uint8 *pValue, uint16 len, uint16 offset, uint8 method)
{
	PRINT("ble: write_handler(): connHandle: %04X\n", connHandle);

	if (!gattPermitWrite(pAttr->permissions)) {
		return ATT_ERR_WRITE_NOT_PERMITTED;
	}
	
	uint16_t uuid = BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1]);
	if(uuid == GATT_CLIENT_CHAR_CFG_UUID) {
		bStatus_t ret = GATTServApp_ProcessCCCWriteReq(connHandle, pAttr, pValue, len,
				offset, GATT_CLIENT_CFG_NOTIFY);
		PRINT("ble: CCCD changed: %02X\n", TxCCCD->value);
		return ret;
	}

	if (uuid == RxCharUUID) {
		return ng_parse(pValue, len);
	}

	return ATT_ERR_ATTR_NOT_FOUND;
}

static bStatus_t read_handler(uint16_t connHandle, gattAttribute_t *pAttr,
				uint8_t *pValue, uint16_t *pLen, uint16_t offset,
				uint16_t maxLen, uint8_t method)
{
	if (!gattPermitRead(pAttr->permissions)) {
		return ATT_ERR_READ_NOT_PERMITTED;
	}

	uint16_t uuid = BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1]);
	if(uuid == GATT_CLIENT_CHAR_CFG_UUID) {
		*pLen = 2;
		tmos_memcpy(pValue, pAttr->pValue, *pLen);
		return SUCCESS;
	}

	if (uuid == TxCharUUID) {
		*pLen = MIN(TxLen-offset, maxLen);
		tmos_memcpy(pValue, &pAttr->pValue[offset], *pLen);
		return SUCCESS;
	}

	return ATT_ERR_ATTR_NOT_FOUND;
}

static gattServiceCBs_t service_handlers = {
	read_handler,
	write_handler,
	NULL
};

static void connStatus_handler(uint16 connHandle, uint8 changeType)
{
	if(connHandle == LOOPBACK_CONNHANDLE)
		return;

	// Reset ClientCharConfig if connection has dropped
	if((changeType == LINKDB_STATUS_UPDATE_REMOVED)
				|| ((changeType == LINKDB_STATUS_UPDATE_STATEFLAGS)
				&& (!linkDB_Up(connHandle)))) {
		GATTServApp_InitCharCfg(connHandle, TxCCCD);
	}
}

int ng_registerService()
{
	uint8 status = SUCCESS;
	GATTServApp_InitCharCfg(INVALID_CONNHANDLE, TxCCCD);
	linkDB_Register(connStatus_handler);

	status = GATTServApp_RegisterService(attr_table,
				GATT_NUM_ATTRS(attr_table),
				GATT_MAX_ENCRYPT_KEY_SIZE,
				&service_handlers);
	return (status);
}

static uint8 isNotifyEnabled(uint16 connHandle)
{
	uint16_t val = GATTServApp_ReadCharCfg(connHandle, TxCCCD);
	return val & GATT_CLIENT_CFG_NOTIFY;
}
/**
 * @brief   Send notify to client. Currently support one client connection
 *          only.
 * 
 * @param	val Value to be sent
 * @param	len length of val. This should not be larger than MTU.
 * @return	bStatus_t
 */
bStatus_t ng_notify(uint8_t *val, uint8_t len)
{
	if(!isNotifyEnabled(TxCCCD->connHandle)) {
		PRINT("ble: ng_notify() notify is not enabled\n");
		return bleIncorrectMode;
	}
	if(len > ATT_GetMTU(TxCCCD->connHandle)) {
		return bleInvalidRange;
	}

	attHandleValueNoti_t noti = {
		.handle = notiAttr.handle,
		.len = len
	};
	noti.pValue = GATT_bm_alloc(TxCCCD->connHandle, ATT_HANDLE_VALUE_NOTI,
				len, NULL, 0);
	if (noti.pValue == NULL) {
		return bleMemAllocError;
	}

	tmos_memcpy(noti.pValue, val, len);

	bStatus_t ret = GATT_Notification(TxCCCD->connHandle, &noti, FALSE);
	GATT_bm_free((gattMsg_t *)&noti, ATT_HANDLE_VALUE_NOTI);
	if (ret != SUCCESS) {
		PRINT("ble: noti sending failed\n");
		return ret;
	}

	PRINT("ble: noti sent\n");
	return SUCCESS;
}
