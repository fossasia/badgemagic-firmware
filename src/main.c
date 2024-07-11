#include "CH58x_common.h"
#include "CH58x_sys.h"

#include "leddrv.h"
#include "button.h"
#include "fb.h"
#include "power.h"
#include "data.h"

#include "ble/setup.h"
#include "ble/profile.h"

#include "usb/usb.h"

#define FB_WIDTH 	(LED_COLS * 4)
#define SCROLL_IRATIO   (16)
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

volatile int mode, brightness;

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
static void fb_transition()
{
	fblist_gonext();
}

void draw_testfb()
{

	fb_t *curr_fb = fblist_currentfb();

	for (int i=0; i<8; i++) {
		fb_t *fb = flash2newfb(i);
		if (fb == NULL)
			continue;
		fb->modes = LEFT;
		fblist_append(fb);
	}
	fblist_gonext();

	fblist_drop(curr_fb);
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

void ble_start()
{
	ble_hardwareInit();
	tmos_clockInit();

	peripheral_init();
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

void handle_mode_transition()
{
	static int prev_mode;
	if (prev_mode == mode) return;

	switch (mode)
	{
	case DOWNLOAD:
		// Disable fb transition while in download mode
		btn_onOnePress(KEY2, NULL);

		// Take control of the current fb to display 
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

	fblist_init(FB_WIDTH);

	draw_testfb();

	btn_init();
	btn_onOnePress(KEY1, change_mode);
	btn_onOnePress(KEY2, fb_transition);
	btn_onLongPress(KEY1, change_brightness);

    while (1) {
		uint32_t i = 0;
		while (isPressed(KEY2)) {
			i++;
			if (i>10) {
				reset_jump();
			}
			DelayMs(200);
		}
		handle_mode_transition();
    }
}

__INTERRUPT
__HIGH_CODE
void TMR0_IRQHandler(void)
{
	static int i;

	if (TMR0_GetITFlag(TMR0_3_IT_CYC_END)) {

		fb_t *fb = fblist_currentfb();
		i += 1;
		if (i >= LED_COLS) {
			i = 0;
			if ((fb->modes & 0x0f) == LEFT) {
				fb->scroll++;
				if (fb->scroll >= (fb->width-LED_COLS)*SCROLL_IRATIO) {
					fb->scroll = 0;
				}
			}
		}
		
		if (i % 2) {
			if ((brightness + 1) % 2) 
				leds_releaseall();
		} else {
			if (i + fb->scroll/SCROLL_IRATIO >= fb->width) {
				leds_releaseall();
				return;
			}
			led_write2dcol(i/2, 
				fb->buf[i+ fb->scroll/SCROLL_IRATIO], 
				fb->buf[i+ fb->scroll/SCROLL_IRATIO + 1]);
		}

		TMR0_ClearITFlag(TMR0_3_IT_CYC_END);
	}
}
