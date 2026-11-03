#include <memory.h>
#include <stdlib.h>

#include "setup.h"
#include "CH58xBLE_LIB.h"
#include "CH58x_common.h"
#include "../debug.h"

#ifndef BLE_BUFF_LEN
// MTU = 128 but clients should request new MTU, otherwise default will be 23.
// 64 was too small for stream_bitmap: a full 44-column frame is 89 bytes and
// did not fit one ATT write, so every frame became a Write Long.
// Note for clients: F057 also accepts Write Command (GATT_PROP_WRITE_NO_RSP).
// That path has no ATT response and therefore no backpressure -- a client that
// writes as fast as its loop runs will flood the host queue, so it must pace
// itself. See BadgeBLE.md.
#define BLE_BUFF_LEN (128 + 4)
#endif

#ifndef BLE_TX_NUM_EVENT
#define BLE_TX_NUM_EVENT                    1
#endif

#ifndef BLE_TX_POWER
#define BLE_TX_POWER                        LL_TX_POWEER_6_DBM
// #define BLE_TX_POWER                        LL_TX_POWEER_MINUS_16_DBM
#endif

#ifndef BLE_MEMHEAP_SIZE
#define BLE_MEMHEAP_SIZE                    (1024 * 6)
#endif

#ifndef CENTRAL_MAX_CONNECTION
#define CENTRAL_MAX_CONNECTION              1
#endif

#ifndef BLE_BUFF_NUM
// The BLE lib automatically stack up Write Long messages in Write handler.
// A connection will be disconnected if this number is some how not enough.
// 512 / 23 was sized to reassemble a 512-byte Write Long at the default
// MTU of 23. With BLE_BUFF_LEN raised to 128+4 the same 22 buffers now hold
// ~2.7 KB of Write Long payload, so the count is generous rather than tight;
// it is left unchanged on purpose so the memory budget only moves in one
// direction and any shortfall shows up in BLE_LibInit() instead of at runtime.
#define BLE_BUFF_NUM        (512 / 23)
#endif

#ifndef PERIPHERAL_MAX_CONNECTION
#define PERIPHERAL_MAX_CONNECTION           1
#endif

static __attribute__((aligned(4), section(".noinit")))
uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];

static void lsi_calib(void)
{
	Calibration_LSI(Level_128);
}

void tmos_clockInit(void)
{
	sys_safe_access_enable();
	R8_CK32K_CONFIG &= ~(RB_CLK_OSC32K_XT | RB_CLK_XT32K_PON);
	sys_safe_access_enable();
	R8_CK32K_CONFIG |= RB_CLK_INT32K_PON;
	sys_safe_access_disable();
	lsi_calib();

	RTC_InitTime(2020, 1, 1, 0, 0, 0);
	TMOS_TimerInit(0);
}

int ble_hardwareInit(void)
{
	bleConfig_t cfg;
	memset(&cfg, 0, sizeof(bleConfig_t));

	cfg.MEMAddr = (uint32_t)MEM_BUF;
	cfg.MEMLen = (uint32_t)BLE_MEMHEAP_SIZE;
	cfg.BufMaxLen = (uint32_t)BLE_BUFF_LEN;
	cfg.BufNumber = (uint32_t)BLE_BUFF_NUM;
	cfg.TxNumEvent = (uint32_t)BLE_TX_NUM_EVENT;
	cfg.TxPower = (uint32_t)BLE_TX_POWER;
	cfg.ConnectNumber = (PERIPHERAL_MAX_CONNECTION & 3) | (CENTRAL_MAX_CONNECTION << 2);

	cfg.SelRTCClock = (1 << 7) | 0x02;

	cfg.rcCB = lsi_calib;

	uint8_t m[6];
	GetMACAddress(m);
	memcpy(cfg.MacAddr, m, 6);

	bStatus_t st = BLE_LibInit(&cfg);
	if (st != SUCCESS) {
		/* ERR_MEM_ALLOCATE_SIZE (0x02) means MEMLen is too small for
		 * BufNumber x BufMaxLen -- the one failure a larger BLE_BUFF_LEN can
		 * introduce. PRINT is empty in release builds, so the caller has to
		 * act on the status. On a chip whose factory MAC reads back, every
		 * remaining failure is a compile-time configuration error. */
		PRINT("ble: BLE_LibInit failed: 0x%02x\n", st);
		return st;
	}
	return 0;
}
