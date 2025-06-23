#include <memory.h>
#include <stdlib.h>

#include "setup.h"
#include "CH58xBLE_LIB.h"
#include "CH58x_common.h"

#ifndef BLE_BUFF_LEN
// MTU = 64 but clients should request new MTU, otherwise default will be 23
#define BLE_BUFF_LEN (64 + 4)
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
#define BLE_BUFF_NUM        (512 / 23)
#endif

#ifndef PERIPHERAL_MAX_CONNECTION
#define PERIPHERAL_MAX_CONNECTION           1
#endif

static __attribute__((aligned(4), section(".noinit")))
uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];

if (badge_cfg.ble_always_on) {
        ble_enable_advertise();
    } else {
        ble_disable_advertise();
    }
	
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

void ble_hardwareInit(void)
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

	cfg.SelRTCClock = 1 << 7;

	cfg.rcCB = lsi_calib;

	uint8_t m[6];
	GetMACAddress(m);
	memcpy(cfg.MacAddr, m, 6);

	BLE_LibInit(&cfg);
}
