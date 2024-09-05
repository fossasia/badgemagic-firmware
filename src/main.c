#include "CH58x_common.h"
#include "CH58x_sys.h"
#include "CH58xBLE_LIB.h"

#include "leddrv.h"
#include "button.h"
#include "bmlist.h"
#include "resource.h"
#include "animation.h"

#include "power.h"
#include "data.h"

#include "ble/setup.h"
#include "ble/profile.h"

#include "usb/usb.h"

#define SCAN_F          (2000)
#define SCAN_T          (FREQ_SYS / SCAN_F)

#define NEXT_STATE(v, min, max) \
				(v)++; \
				if ((v) >= (max)) \
					(v) = (min)

enum MODES {
	NORMAL = 0,
	DOWNLOAD,
	POWER_OFF,
	MODES_COUNT,
};

#define BRIGHTNESS_LEVELS   (4)

#define ANI_BASE_SPEED_T      (200000) // uS
#define ANI_MARQUE_SPEED_T    (100000) // uS
#define ANI_FLASH_SPEED_T     (500000) // uS
#define SCAN_BOOTLD_BTN_SPEED_T         (200000) // uS
#define ANI_SPEED_STRATEGY(speed_level) \
				(ANI_BASE_SPEED_T - ((speed_level) \
				* ANI_BASE_SPEED_T / 8))

#define ANI_NEXT_STEP       (1 << 0)
#define ANI_MARQUE          (1 << 1)
#define ANI_FLASH           (1 << 2)
#define SCAN_BOOTLD_BTN     (1 << 3)
#define BLE_NEXT_STEP       (1 << 4)

static tmosTaskID common_taskid = INVALID_TASK_ID ;

volatile uint16_t fb[LED_COLS] = {0};
volatile int mode, brightness = 0;

__HIGH_CODE
static void change_brightness()
{
	NEXT_STATE(brightness, 0, BRIGHTNESS_LEVELS);
	led_setDriveStrength(brightness / 2);
}

__HIGH_CODE
static void change_mode()
{
	NEXT_STATE(mode, 0, MODES_COUNT);
}

__HIGH_CODE
static void bm_transition()
{
	bmlist_gonext();
}
void play_splash(xbm_t *xbm, int col, int row)
{
	while (ani_xbm_scrollup_pad(xbm, 11, 11, 11, fb, 0, 0) != 0) {
		DelayMs(30);
	}
}

void load_bmlist()
{
	if (data_get_header(0) == 0) // There is no bitmap stored in flash
		return; // skip

	bm_t *curr_bm = bmlist_current();

	for (int i=0; i<8; i++) {
		bm_t *bm = flash2newbm(i);
		if (bm == NULL)
			continue;
		bmlist_append(bm);
	}
	bmlist_gonext();

	bmlist_drop(curr_bm);
}

void poweroff()
{
	// Stop wasting energy
	GPIOA_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_Floating);
	GPIOB_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_Floating);

	// Configure wake-up
	GPIOA_ModeCfg(KEY1_PIN, GPIO_ModeIN_PD);
	GPIOA_ITModeCfg(KEY1_PIN, GPIO_ITMode_RiseEdge);
	PFIC_EnableIRQ(GPIO_A_IRQn);
	PWR_PeriphWakeUpCfg(ENABLE, RB_SLP_GPIO_WAKE, Long_Delay);

	/* Good bye */
	LowPower_Shutdown(0);
}

static uint16_t common_tasks(tmosTaskID task_id, uint16_t events)
{
	static int marque_step, flash_step;

	if(events & SYS_EVENT_MSG) {
		uint8 *pMsg = tmos_msg_receive(common_taskid);
		if(pMsg != NULL)
		{
			tmos_msg_deallocate(pMsg);
		}
		return (events ^ SYS_EVENT_MSG);
	}

	if(events & ANI_NEXT_STEP) {

		static void (*animations[])(bm_t *bm, uint16_t *fb) = {
			ani_scroll_left,
			ani_scroll_right,
			ani_scroll_up,
			ani_scroll_down,
			ani_fixed,
			ani_animation,
			ani_snowflake,
			ani_picture,
			ani_laser
		};

		bm_t *bm = bmlist_current();
		if (animations[LEGACY_GET_ANIMATION(bm->modes)])
			animations[LEGACY_GET_ANIMATION(bm->modes)](bm, fb);

		if (bm->is_flash) {
			ani_flash(bm, fb, flash_step);
		}
		if (bm->is_marquee) {
			ani_marque(bm, fb, marque_step);
		}

		uint32_t t = ANI_SPEED_STRATEGY(LEGACY_GET_SPEED(bm->modes));
		tmos_start_task(common_taskid, ANI_NEXT_STEP, t / 625);

		return events ^ ANI_NEXT_STEP;
	}

	if (events & ANI_MARQUE) {
		bm_t *bm = bmlist_current();
		marque_step++;
		if (bm->is_marquee) {
			ani_marque(bm, fb, marque_step);
		}

		return events ^ ANI_MARQUE;
	}

	if (events & SCAN_BOOTLD_BTN) {
		static uint32_t hold;
		hold = isPressed(KEY2) ? hold + 1 : 0;
		if (hold > 10) {
			reset_jump();
		}

		return events ^ SCAN_BOOTLD_BTN;
	}

	if (events & ANI_FLASH) {
		bm_t *bm = bmlist_current();
		flash_step++;

		if (bm->is_flash) {
			ani_flash(bm, fb, flash_step);
		}
		/* After flash is applied, it will potentialy overwrite the marque
		effect after it just wrote, results in flickering. So here apply the
		marque effect again */
		if (bm->is_marquee) {
			ani_marque(bm, fb, marque_step);
		}

		return events ^ ANI_FLASH;
	}

	if (events & BLE_NEXT_STEP) {
		ani_xbm_next_frame(&bluetooth, fb, 10, 0);

		return events ^ BLE_NEXT_STEP;
	}

	return 0;
}

void ble_setup()
{
	ble_hardwareInit();
	tmos_clockInit();

	peripheral_init();
	ble_disable_advertise();

	devInfo_registerService();
	legacy_registerService();
}

static void usb_receive(uint8_t *buf, uint16_t len)
{
	static uint16_t rx_len, data_len;
	static uint8_t *data;

	PRINT("dump first 8 bytes: %02x %02x %02x %02x %02x %02x %02x %02x\n",
				buf[0], buf[1], buf[2], buf[3],
				buf[4], buf[5], buf[6], buf[7]);

	if (rx_len == 0) {
		if (memcmp(buf, "wang", 5))
			return;

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
		SYS_ResetExecute();
	}
}

void spawn_tasks()
{
	common_taskid = TMOS_ProcessEventRegister(common_tasks);

	tmos_start_reload_task(common_taskid, ANI_MARQUE, ANI_MARQUE_SPEED_T / 625);
	tmos_start_reload_task(common_taskid, ANI_FLASH, ANI_FLASH_SPEED_T / 625);
	tmos_start_reload_task(common_taskid, SCAN_BOOTLD_BTN, 
				SCAN_BOOTLD_BTN_SPEED_T / 625);
	tmos_start_task(common_taskid, ANI_NEXT_STEP, 500000 / 625);
}

void ble_start()
{
	ble_enable_advertise();

	tmos_stop_task(common_taskid, ANI_NEXT_STEP);
	tmos_stop_task(common_taskid, ANI_MARQUE);
	tmos_stop_task(common_taskid, ANI_FLASH);
	memset(fb, 0, sizeof(fb));

	tmos_start_reload_task(common_taskid, BLE_NEXT_STEP, 500000 / 625);
}

void handle_mode_transition()
{
	static int prev_mode;
	if (prev_mode == mode) return;

	switch (mode)
	{
	case DOWNLOAD:
		// Disable bitmap transition while in download mode
		btn_onOnePress(KEY2, NULL);

		// Take control of the current bitmap to display 
		// the Bluetooth animation
		ble_start();
		while (mode == DOWNLOAD) {
			TMOS_SystemProcess();
		}
		// If not being flashed, pressing KEY1 again will 
		// make the badge goes off:
		
		// fallthrough
	case POWER_OFF:
		poweroff();
		break;
	
	default:
		break;
	}
	prev_mode = mode;
}

static void debug_init()
{
	GPIOA_SetBits(GPIO_Pin_9);
	GPIOA_ModeCfg(GPIO_Pin_8, GPIO_ModeIN_PU);
	GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA);
	UART1_DefInit();
	UART1_BaudRateCfg(921600);
}

int main()
{
	SetSysClock(CLK_SOURCE_PLL_60MHz);

	debug_init();
	PRINT("\nDebug console is on UART%d\n", DEBUG);

	cdc_onWrite(usb_receive);
	hiddev_onWrite(usb_receive);
	usb_start();

	led_init();
	TMR0_TimerInit(SCAN_T / 2);
	TMR0_ITCfg(ENABLE, TMR0_3_IT_CYC_END);
	PFIC_EnableIRQ(TMR0_IRQn);

	bmlist_init(LED_COLS * 4);
	
	play_splash(&splash, 0, 0);

	load_bmlist();

	btn_init();
	btn_onOnePress(KEY1, change_mode);
	btn_onOnePress(KEY2, bm_transition);
	btn_onLongPress(KEY1, change_brightness);

	ble_setup();

	spawn_tasks();
	while (1) {
		handle_mode_transition();
		TMOS_SystemProcess();
	}
}

__INTERRUPT
__HIGH_CODE
void TMR0_IRQHandler(void)
{
	static int i;

	if (TMR0_GetITFlag(TMR0_3_IT_CYC_END)) {
		i += 1;
		if (i >= LED_COLS) {
			i = 0;
		}
		
		if (i % 2) {
			if ((brightness + 1) % 2) 
				leds_releaseall();
		} else {
			led_write2dcol(i/2, fb[i], fb[i + 1]);
		}

		TMR0_ClearITFlag(TMR0_3_IT_CYC_END);
	}
}
