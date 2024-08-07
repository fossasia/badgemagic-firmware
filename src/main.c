#include "CH58x_common.h"
#include "CH58x_sys.h"

#include "leddrv.h"
#include "button.h"
#include "fb.h"
#include "power.h"
#include "data.h"

#include "ble/setup.h"
#include "ble/profile.h"

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

int main()
{
	SetSysClock(CLK_SOURCE_PLL_60MHz);

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
