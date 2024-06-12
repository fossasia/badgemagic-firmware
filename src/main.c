#include "CH58x_common.h"
#include "CH58x_sys.h"

#include "leddrv.h"
#include "button.h"

#define FB_WIDTH 	(LED_COLS * 4)
#define SCROLL_IRATIO   (16)
#define FB_NUM_SPARE    (8)
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

uint16_t fb[FB_NUM_SPARE][FB_WIDTH];

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

volatile int fb_sel, fb_num;
volatile int mode, brightness;
volatile uint64_t tick;

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
static void change_fb()
{
	NEXT_STATE(fb_sel, 0, fb_num);
}

int main()
{
	SetSysClock(CLK_SOURCE_PLL_60MHz);

	led_init();
	draw2fb(fb[0], 0, 8*(5-1));
	draw2fb(fb[0], 1, 8*(6-1));
	draw2fb(fb[0], 2, 8*(7-1));
	draw2fb(fb[0], 3, 8*(8-1));
	draw2fb(fb[0], 4, 8*(9-1));
	draw2fb(fb[0], 5, 8*(10-1));
	draw2fb(fb[0], 6, 8*(11-1));
	draw2fb(fb[0], 7, 8*(12-1));

	draw2fb(fb[1], 4, 8*5);
	draw2fb(fb[1], 5, 8*6);
	draw2fb(fb[1], 6, 8*7);
	draw2fb(fb[1], 7, 8*8);
	draw2fb(fb[1], 0, 8*9);
	draw2fb(fb[1], 1, 8*10);
	draw2fb(fb[1], 2, 8*11);
	draw2fb(fb[1], 3, 8*12);
	fb_num = 2;

	TMR0_TimerInit(SCAN_T / 2);
	TMR0_ITCfg(ENABLE, TMR0_3_IT_CYC_END);
	PFIC_EnableIRQ(TMR0_IRQn);

	btn_init();
	btn_onOnePress(KEY1, change_mode);
	btn_onOnePress(KEY2, change_fb);
	btn_onLongPress(KEY1, change_brightness);

    while (1) {
		int i = 0;
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
	static int i, scroll;

	if (TMR0_GetITFlag(TMR0_3_IT_CYC_END)) {

		i += 1;
		if (i >= LED_COLS) {
			i = 0;
			scroll++;
			if (scroll >= (FB_WIDTH-LED_COLS)*SCROLL_IRATIO) {
				scroll = 0;
			}
		}
		
		if (i % 2) {
			if ((brightness + 1) % 2) 
				leds_releaseall();
		} else {
			led_write2dcol(i/2, 
				fb[fb_sel][i+scroll/SCROLL_IRATIO], 
				fb[fb_sel][i+scroll/SCROLL_IRATIO+1]);
		}

		TMR0_ClearITFlag(TMR0_3_IT_CYC_END);
	}
}
