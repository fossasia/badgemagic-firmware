#include "CH58xBLE_LIB.h"
#include "setup.h"
#include "../config.h"

#define ADV_UUID    (0xFEE0)

static uint8 taskid = INVALID_TASK_ID;

typedef struct
{
	uint16 connHandle; // Connection handle of current connection
	uint16 connInterval;
	uint16 connSlaveLatency;
	uint16 connTimeout;
} peripheralConnItem_t;

#define SBP_PARAM_UPDATE_DELAY 1600 // Parameter update delay (unit of 0.625ms)
#define SLAVE_LATENCY       0

#define MIN_ADV_INTERVAL    100 // Advertising interval (units of 0.625ms)
#define MAX_ADV_INTERVAL    200 // Advertising interval (units of 0.625ms)

#define MIN_CONN_INTERVAL   20 // Connection interval (units of 1.25ms)
#define MAX_CONN_INTERVAL   100 // Connection interval (units of 1.25ms)

#define CONN_TIMEOUT        100 // Supervision timeout (units of 10ms)

// GAP - Advertisement data (max size = 31 bytes)
// keep short, save energy, save the planet
static uint8 advertData[] = {
	0x02, // section length
	GAP_ADTYPE_FLAGS,
	GAP_ADTYPE_FLAGS_GENERAL | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,

	// advertise UUID
	0x03, // section length
	GAP_ADTYPE_16BIT_MORE,
	LO_UINT16(ADV_UUID),
	HI_UINT16(ADV_UUID)
};

// Connection item list
static peripheralConnItem_t conn_list;

static void initConn(peripheralConnItem_t *conn)
{
	conn->connHandle = GAP_CONNHANDLE_INIT;
	conn->connInterval = 0;
	conn->connSlaveLatency = 0;
	conn->connTimeout = 0;
}

static void processTMOSMsg(tmos_event_hdr_t *pMsg)
{
	switch(pMsg->event) {
		default:
			break;
	}
}

static void enable_advertising(uint8_t enable)
{
	GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8), &enable);
}

static void link_onEstablished(gapRoleEvent_t *pe)
{
	gapEstLinkReqEvent_t *e = (gapEstLinkReqEvent_t *)pe;

	// If already connected
	if(conn_list.connHandle != GAP_CONNHANDLE_INIT) {
		// Only allow 1 connection, so the new connection will be dropped:
		GAPRole_TerminateLink(e->connectionHandle);
		GAPRole_PeripheralConnParamUpdateReq(e->connectionHandle,
					MIN_CONN_INTERVAL,
					MAX_CONN_INTERVAL,
					SLAVE_LATENCY,
					CONN_TIMEOUT,
					taskid);
		return;
	}
	conn_list.connHandle = e->connectionHandle;
	conn_list.connInterval = e->connInterval;
	conn_list.connSlaveLatency = e->connLatency;
	conn_list.connTimeout = e->connTimeout;
	enable_advertising(FALSE);
}

static void link_onTerminated(gapRoleEvent_t *pe)
{
	gapTerminateLinkEvent_t *event = (gapTerminateLinkEvent_t *)pe;
	GAPRole_TerminateLink(pe->linkCmpl.connectionHandle);
	enable_advertising(TRUE);

	if(event->connectionHandle == conn_list.connHandle) {
		conn_list.connHandle = GAP_CONNHANDLE_INIT;
		conn_list.connInterval = 0;
		conn_list.connSlaveLatency = 0;
		conn_list.connTimeout = 0;
	} else {
		// Requested connection is not existed in connection list
	}
}

static void gap_onParamUpdate(uint16 connHandle, uint16 connInterval,
				uint16 connSlaveLatency, uint16 connTimeout)
{
	conn_list.connHandle = connHandle;
	conn_list.connInterval = connInterval;
	conn_list.connSlaveLatency = connSlaveLatency;
	conn_list.connTimeout = connTimeout;
}

static void gap_onStateChange(gapRole_States_t newState, gapRoleEvent_t *pEvent)
{
	switch(newState & GAPROLE_STATE_ADV_MASK) {
	case GAPROLE_ADVERTISING:
		if(pEvent->gap.opcode == GAP_LINK_TERMINATED_EVENT) {
			link_onTerminated(pEvent);
		}
		break;

	case GAPROLE_CONNECTED:
		if(pEvent->gap.opcode == GAP_LINK_ESTABLISHED_EVENT) {
			link_onEstablished(pEvent);
		}
		break;

	case GAPROLE_WAITING:
		if(pEvent->gap.opcode == GAP_LINK_TERMINATED_EVENT) {
			link_onTerminated(pEvent);
		} else {
			// refer to pEvent->gap.opcode
		}
		break;

	default:
		break;
	}
}

// GAP Role Callbacks
static gapRolesCBs_t gap_handlers = {
	gap_onStateChange,
	NULL,
	gap_onParamUpdate
};

// Broadcast Callbacks
static gapRolesBroadcasterCBs_t broadcast_handlers = {
	NULL,
	NULL
};

// GAP Bond Manager Callbacks
static gapBondCBs_t bond_managers = {
	NULL,
	NULL
};


static uint16 peripheral_task(uint8 task_id, uint16 events)
{
	if(events & SYS_EVENT_MSG) { // Handle BLE stack message
		uint8 *pMsg = tmos_msg_receive(taskid);
		if(pMsg) {
			processTMOSMsg((tmos_event_hdr_t *)pMsg);
			tmos_msg_deallocate(pMsg);
		}
		return (events ^ SYS_EVENT_MSG);
	}

	return 0;
}

void ble_enable_advertise()
{
	uint8 e = TRUE;
	GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8), &e);
}

void ble_disable_advertise()
{
	uint8 e = FALSE;
	GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8), &e);
}

// len should not exceed 20 chars excluding null-terminate char. Otherwise it
// will be trimmed off and return -1
static int setup_scan_rsp(char *name, uint8_t len)
{
	int ret = 0;
	// GAP - SCAN RSP data (max size = 31 bytes)
	uint8_t scanRspData[31] = {
		// connection interval range
		0x05, // length of this section
		GAP_ADTYPE_SLAVE_CONN_INTERVAL_RANGE,
		LO_UINT16(MIN_CONN_INTERVAL),
		HI_UINT16(MIN_CONN_INTERVAL),
		LO_UINT16(MAX_CONN_INTERVAL),
		HI_UINT16(MAX_CONN_INTERVAL),

		// Tx power level
		0x02, // length of this data
		GAP_ADTYPE_POWER_LEVEL,
		9, // 9dBm

		len + 1,
		GAP_ADTYPE_LOCAL_NAME_COMPLETE,
	};
	
	if (len < (31 - 11)) {
		tmos_memcpy(&scanRspData[11], name, len);
		ret = -1;
	} else {
		tmos_memcpy(&scanRspData[11], name, 20);
	}
	int total_len = 11 + len;
	tmos_memset(&scanRspData[total_len], 0, 31 - total_len);
	GAPRole_SetParameter(GAPROLE_SCAN_RSP_DATA, total_len, scanRspData);
	return ret;
}

static void gap_init()
{
	GAPRole_PeripheralInit();

	uint16_t min_interval = 6;
	uint16_t max_interval = 500;

	// Set the GAP Role Parameters
	setup_scan_rsp(badge_cfg.ble_devname, 20);
	GAPRole_SetParameter(GAPROLE_ADVERT_DATA, sizeof(advertData), advertData);
	GAPRole_SetParameter(GAPROLE_MIN_CONN_INTERVAL, sizeof(uint16), &min_interval);
	GAPRole_SetParameter(GAPROLE_MAX_CONN_INTERVAL, sizeof(uint16), &max_interval);
	
	// Set the GAP Characteristics
	GGS_SetParameter(GGS_DEVICE_NAME_ATT, 20,
				badge_cfg.ble_devname);
	GAP_SetParamValue(TGAP_DISC_ADV_INT_MIN, MIN_ADV_INTERVAL);
	GAP_SetParamValue(TGAP_DISC_ADV_INT_MAX, MAX_ADV_INTERVAL);

	static uint32 passkey = 0; // passkey "000000"
	static uint8  pairMode = GAPBOND_PAIRING_MODE_NO_PAIRING;
	static uint8  mitm = FALSE;
	static uint8  bonding = FALSE;
	static uint8  ioCap = GAPBOND_IO_CAP_DISPLAY_ONLY;
	GAPBondMgr_SetParameter(GAPBOND_PERI_DEFAULT_PASSCODE, sizeof(uint32), &passkey);
	GAPBondMgr_SetParameter(GAPBOND_PERI_PAIRING_MODE, sizeof(uint8), &pairMode);
	GAPBondMgr_SetParameter(GAPBOND_PERI_MITM_PROTECTION, sizeof(uint8), &mitm);
	GAPBondMgr_SetParameter(GAPBOND_PERI_IO_CAPABILITIES, sizeof(uint8), &ioCap);
	GAPBondMgr_SetParameter(GAPBOND_PERI_BONDING_ENABLED, sizeof(uint8), &bonding);

	GAPRole_BroadcasterSetCB(&broadcast_handlers);

	GGS_AddService(GATT_ALL_SERVICES);
	GATTServApp_AddService(GATT_ALL_SERVICES);
}

void peripheral_init()
{
	taskid = TMOS_ProcessEventRegister(peripheral_task);

	initConn(&conn_list);
	gap_init();

	GAPRole_PeripheralStartDevice(taskid, &bond_managers, &gap_handlers);
}
