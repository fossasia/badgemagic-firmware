#include "CH58x_common.h"
#include "CH58x_sys.h"

#include "leddrv.h"
#include "button.h"
#include "fb.h"

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

uint8_t test_font[][11] = {
	0x00, 0xFE, 0x66, 0x62, 0x68, 0x78, 0x68, 0x60, 0x60, 0xF0, 0x00, // F
	0x00, 0x7C, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0x7C, 0x00, // O
	0x00, 0x7C, 0xC6, 0xC6, 0x60, 0x38, 0x0C, 0xC6, 0xC6, 0x7C, 0x00, // S
	0x00, 0x7C, 0xC6, 0xC6, 0x60, 0x38, 0x0C, 0xC6, 0xC6, 0x7C, 0x00, // S
	0x00, 0x38, 0x6C, 0xC6, 0xC6, 0xFE, 0xC6, 0xC6, 0xC6, 0xC6, 0x00, // A
	0x00, 0x7C, 0xC6, 0xC6, 0x60, 0x38, 0x0C, 0xC6, 0xC6, 0x7C, 0x00, // S
	0x00, 0x3C, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00, // I
	0x00, 0x38, 0x6C, 0xC6, 0xC6, 0xFE, 0xC6, 0xC6, 0xC6, 0xC6, 0x00, // A
};

void draw2fb(uint16_t *fb, int c, int col) 
{
	uint16_t tmpfb[8] = {0};
	for (int i=0; i<8; i++) {
		for (int j=0; j<11; j++) {
			tmpfb[i] |= ((test_font[c][j] >> (8-i)) & 0x01) << j;
		}
	}
	for (int i=0; i<8; i++) {
		fb[col+i] = tmpfb[i];
	}
}

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
	curr_fb->modes = LEFT;

	for (int i=0; i<8; i++) {
		draw2fb(curr_fb->buf, i, 8 * (i + 4));
	}

	fb_t *fb_next = fb_new(8*12);
	fblist_append(fb_next);
	fb_next->modes = LEFT;
	
	for (int i=4; i<12; i++) {
		draw2fb(fb_next->buf, i % 8, 8 * (i + 1));
	}
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
				asm volatile("j 0x00");
			}
			DelayMs(200);
		}
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
			led_write2dcol(i/2, 
				fb->buf[i+ fb->scroll/SCROLL_IRATIO], 
				fb->buf[i+ fb->scroll/SCROLL_IRATIO + 1]);
		}

		TMR0_ClearITFlag(TMR0_3_IT_CYC_END);
	}
}
