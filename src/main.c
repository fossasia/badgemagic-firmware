#include "CH58x_common.h"
#include "CH58x_sys.h"
#include "CH58xBLE_LIB.h"

#include "leddrv.h"
#include "button.h"
#include "bmlist.h"
#include "resource.h"
#include "animation.h"
#include "font.h"
#include "font3x5.h"
#if HW_KEY_COUNT == 4
#include "auxbtn.h"
#endif
#include "game.h"

#include "power.h"
#include "data.h"
#include "config.h"
#include "debug.h"

#include "ble/setup.h"
#include "ble/profile.h"

#include "usb/usb.h"
#include "legacyctrl.h"

#define NEXT_STATE(v, min, max) \
				(v)++; \
				if ((v) >= (max)) \
					(v) = (min)

enum MODES {
	MENU = 0,
    NORMAL,
    DOWNLOAD,
    CLOCK,
	GAME,
	POWER_OFF,
    MODES_COUNT,
};

static int menu_cursor=0;

#define MENU_ITEMS_COUNT 5
static const char *menu_labels[] = {
	"ANIMATION",
	"BT-PAIRING",
	"CLOCK MODE",
	"SNAKE",
	"OFF"
};
#define MENU_IDX_ANIMATION 0
#define MENU_IDX_BLE       1
#define MENU_IDX_CLOCK     2
#define MENU_IDX_SNAKE     3
#define MENU_IDX_OFF       4

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
#define CLOCK_TICK          (1 << 5)
#define STOPWATCH_TICK  (1 << 6)

typedef enum {
    SW_STOPPED,
    SW_RUNNING,
} sw_state_t;

static sw_state_t sw_state = SW_STOPPED;
static uint32_t sw_centiseconds = 0;

static tmosTaskID common_taskid = INVALID_TASK_ID ;

volatile uint16_t fb[LED_COLS] = {0};
volatile int mode, is_play_sequentially = 1;
static int clock_active = 0;

__HIGH_CODE

static void mode_setup_download();
static void mode_setup_normal();
static void disp_clock();
static void disp_menu();
static void menu_up();
static void menu_down();
static void enter_clock_submenu();
static void disp_stopwatch();
void return_to_menu();

__HIGH_CODE
static void bm_transition()
{
	if (is_play_sequentially) {
		is_play_sequentially = 0;
		bmlist_gohead();
		return;
	}

	bmlist_gonext();
	if (bmlist_current() == bmlist_head()) {
		is_play_sequentially = 1;
		return;
	}
}
void play_splash(xbm_t *xbm, int col, int row, int spT)
{
	while (ani_xbm_scrollup_pad(xbm, 11, 11, 11, fb, 0, 0) != 0) {
		DelayMs(spT);
	}
}

void load_bmlist()
{
	data_legacy_t header;
	data_get_header(&header);
	if (memcmp(header.header, "wang", 5))
		return; // There is no bitmap stored in flash

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

		static int (*animations[])(bm_t *bm, uint16_t *fb) = {
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
			if (animations[LEGACY_GET_ANIMATION(bm->modes)](bm, fb) == 0
				&& is_play_sequentially) {
				bmlist_gonext();
			}

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
		if (bm->is_marquee) {
			ani_marque(bm, fb, marque_step);
		}

		return events ^ ANI_FLASH;
	}

	if (events & BLE_NEXT_STEP) {
		ani_xbm_next_frame(&bluetooth, fb, 10, 0);

		return events ^ BLE_NEXT_STEP;
	}
	
	if (events & CLOCK_TICK) {
		disp_clock();
		return events ^ CLOCK_TICK;
	}

	if (events & STOPWATCH_TICK) {
		sw_centiseconds++;
		disp_stopwatch();
		return events ^ STOPWATCH_TICK;
	}

	return 0;
}

void ble_setup()
{
	ble_hardwareInit();
	tmos_clockInit();

	peripheral_init();

	if (! badge_cfg.ble_always_on) {
		ble_disable_advertise();
	}

	devInfo_registerService();
	legacy_registerService();
	batt_registerService();
	ng_registerService();
}

static void spawn_tasks()
{
	common_taskid = TMOS_ProcessEventRegister(common_tasks);

	tmos_start_reload_task(common_taskid, ANI_MARQUE, ANI_MARQUE_SPEED_T / 625);
	tmos_start_reload_task(common_taskid, ANI_FLASH, ANI_FLASH_SPEED_T / 625);
	tmos_start_reload_task(common_taskid, SCAN_BOOTLD_BTN,
				SCAN_BOOTLD_BTN_SPEED_T / 625);
	tmos_start_task(common_taskid, ANI_NEXT_STEP, 500000 / 625);
}

static void start_ble_animation()
{
	tmos_stop_task(common_taskid, ANI_NEXT_STEP);
	tmos_stop_task(common_taskid, ANI_MARQUE);
	tmos_stop_task(common_taskid, ANI_FLASH);
	memset(fb, 0, sizeof(fb));

	tmos_start_reload_task(common_taskid, BLE_NEXT_STEP, 500000 / 625);
}

static void start_normal_animation()
{
	tmos_start_reload_task(common_taskid, ANI_MARQUE, ANI_MARQUE_SPEED_T / 625);
	tmos_start_reload_task(common_taskid, ANI_FLASH, ANI_FLASH_SPEED_T / 625);
	tmos_start_task(common_taskid, ANI_NEXT_STEP, 500000 / 625);
	tmos_stop_task(common_taskid, BLE_NEXT_STEP);
}

static void resume_from_streaming()
{
	if (badge_cfg.ble_always_on) {
		start_normal_animation();
	} else {
		start_ble_animation();
	}
}

static void stop_all_animation()
{
	tmos_stop_task(common_taskid, ANI_NEXT_STEP);
	tmos_stop_task(common_taskid, ANI_MARQUE);
	tmos_stop_task(common_taskid, ANI_FLASH);
	tmos_stop_task(common_taskid, BLE_NEXT_STEP);
	memset(fb, 0, sizeof(fb));
}

int streaming_enabled;

uint8_t streaming_setting(uint8_t *params, uint16_t len)
{
	if (params[0] == 0x00) { // enter streaming mode
		stop_all_animation();
		streaming_enabled = 1;
	} else if (params[0] == 0x01) { // return to normal mode
		resume_from_streaming();
		streaming_enabled = 0;
	}
	return 0;
}

uint8_t stream_bitmap(uint8_t *params, uint16_t len)
{
	if (! streaming_enabled) {
		return -1;
	}

	tmos_memcpy(fb, params, min(LED_COLS * 2, len));
	return 0;
}

static void debug_init()
{
	GPIOA_SetBits(GPIO_Pin_9);
	GPIOA_ModeCfg(GPIO_Pin_8, GPIO_ModeIN_PU);
	GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA);
	UART1_DefInit();
	UART1_BaudRateCfg(921600);
}

static void disp_bat_stt(int bat_percent, int col, int row)
{
	if (bat_percent < 0) {
		xbm2fb(&batwarn_xbm, fb, col, row);
		return;
	}

	xbm2fb(&bat_xbm, fb, col, row);
	bat_percent /= 10;
	for (int i=1; i <= bat_percent; i++) {
		fb[col + i] = fb[col];
	}
}

static void fb_putchar(char c, int col, int row)
{
	for (int i=0; i < 6; i++) {
		if (col + i >= LED_COLS) break;
		fb[col + i] = (fb[col + i] & ~(0x7f << row))
				| (font5x7[c - ' '][i] << row);
	}
}

static void fb_puts(char *s, int len, int col, int row)
{
	while (*s && len--) {
		fb_putchar(*s, col, row);
		col += 6;
		s++;
	}
}

static void fb_putchar_small(char c, int col, int row)
{
    for (int i = 0; i < 4; i++) {
        if (col + i >= LED_COLS) break;
        fb[col + i] = (fb[col + i] & ~(0x1f << row)) | (font3x5[c - ' '][i] << row);
    }
}

static void fb_puts_small(char *s, int len, int col, int row)
{
    while (*s && len--) {
        fb_putchar_small(*s, col, row);
        col += 4;
        s++;
    }
}

static void disp_clock()
{
    uint16_t year, month, day, hour, minute, second;
    RTC_GetTime(&year, &month, &day, &hour, &minute, &second);
    memset(fb, 0, sizeof(fb));

    char buf[6];
    buf[0] = '0' + hour / 10;
    buf[1] = '0' + hour % 10;
    buf[2] = ':';
    buf[3] = '0' + minute / 10;
    buf[4] = '0' + minute % 10;
    buf[5] = '\0';

    fb_puts(buf, 5, 2, 2);
}

static void disp_menu()
{
    memset(fb, 0, sizeof(fb));

    int page = menu_cursor / 2;
    int item0 = page * 2;
    int item1 = page * 2 + 1;

    // top item
    if (menu_cursor == item0)
        fb_putchar_small('>', 0, 0);
    fb_puts_small((char *)menu_labels[item0],
                  strlen(menu_labels[item0]), 4, 0);

    // bottom item
    if (item1 < MENU_ITEMS_COUNT) {
        if (menu_cursor == item1)
            fb_putchar_small('>', 0, 6);
        fb_puts_small((char *)menu_labels[item1],
                      strlen(menu_labels[item1]), 4, 6);
    }
}

static void menu_up(){
	menu_cursor--;
    if (menu_cursor < 0) menu_cursor = MENU_ITEMS_COUNT - 1;
    disp_menu();
}

static void menu_down(){
	menu_cursor++;
	if(menu_cursor > MENU_ITEMS_COUNT - 1) menu_cursor = 0;
	disp_menu();
}

static void menu_select(){
    if (menu_cursor == MENU_IDX_ANIMATION) {
        mode = NORMAL;
        btn_onOnePress(KEY1, NULL);
        btn_onOnePress(KEY2, bm_transition);
        mode_setup_normal();
    } else if (menu_cursor == MENU_IDX_BLE) {
        mode = DOWNLOAD;
        btn_onOnePress(KEY1, NULL);
        btn_onOnePress(KEY2, NULL);
        ble_enable_advertise();
        start_ble_animation();
    } else if (menu_cursor == MENU_IDX_CLOCK) {
        enter_clock_submenu();
    }

    else if (menu_cursor == MENU_IDX_SNAKE) {
        mode = GAME;
        stop_all_animation();
        game_start((uint16_t *)fb);
    }

    else if (menu_cursor == MENU_IDX_OFF) {
        mode = POWER_OFF;
        poweroff();
    }
}

static void disp_stopwatch()
{
    uint32_t cs = sw_centiseconds;
    uint32_t minutes = cs / 6000;
    uint32_t seconds = (cs % 6000) / 100;
    uint32_t centis  = cs % 100;

    char buf[8];
    buf[0] = '0' + minutes % 10;  // M:SS:cs - to be able to fit it on the badge at the usual font size 5x7
    buf[1] = ':';
    buf[2] = '0' + seconds / 10;
    buf[3] = '0' + seconds % 10;
    buf[4] = ':';
    buf[5] = '0' + centis / 10;
    buf[6] = '0' + centis % 10;
    buf[7] = '\0';

    memset(fb, 0, sizeof(fb));
    fb_puts(buf, 7, 2, 2);  
}

static void sw_startstop()
{
    if (sw_state == SW_STOPPED) {
        sw_state = SW_RUNNING;
        tmos_start_reload_task(common_taskid, STOPWATCH_TICK, 10000 / 625);
    } else {
        sw_state = SW_STOPPED;
        tmos_stop_task(common_taskid, STOPWATCH_TICK);
    }
}

static void sw_reset()
{
    sw_centiseconds = 0;
    disp_stopwatch();
}

static void sw_back()
{
    sw_state = SW_STOPPED;
    tmos_stop_task(common_taskid, STOPWATCH_TICK);
    enter_clock_submenu();
}

// Clock submenu: 0 = Time, 1 = Stopwatch
static int clock_submenu_sel = 0;

static void disp_clock_submenu()
{
    memset(fb, 0, sizeof(fb));
    if (clock_submenu_sel == 0)
        fb_putchar_small('>', 0, 0);
    else
        fb_putchar_small('>', 0, 6);

    fb_puts_small("TIME", 4, 4, 0);
    fb_puts_small("STOPWATCH", 9, 4, 6);
}

static void clock_submenu_nav()
{
    clock_submenu_sel ^= 1;
    disp_clock_submenu();
}

static void clock_submenu_select()
{
    if (clock_submenu_sel == 0) {
        // Enter Time mode
        clock_active = 1;
        tmos_start_reload_task(common_taskid, CLOCK_TICK, 1000000 / 625);
        btn_onOnePress(KEY1, NULL);
        btn_onOnePress(KEY2, NULL);
		btn_onLongPress(KEY1, NULL);
        btn_onLongPress(KEY2, enter_clock_submenu);
    } else {
        // Enter Stopwatch mode
        sw_state = SW_STOPPED;
        sw_centiseconds = 0;
        disp_stopwatch();
        btn_onOnePress(KEY1, sw_startstop);
        btn_onOnePress(KEY2, sw_reset);
		btn_onLongPress(KEY1, NULL);
        btn_onLongPress(KEY2, sw_back);
    }
}

static void enter_clock_submenu()
{
    clock_active = 0;
    tmos_stop_task(common_taskid, CLOCK_TICK);
    stop_all_animation();
    clock_submenu_sel = 0;

    btn_onOnePress(KEY1, clock_submenu_nav);
    btn_onOnePress(KEY2, clock_submenu_nav);
    btn_onLongPress(KEY1, clock_submenu_select);
    btn_onLongPress(KEY2, return_to_menu);

    disp_clock_submenu();
}

void return_to_menu()
{
    stop_all_animation();
    tmos_stop_task(common_taskid, CLOCK_TICK);
	tmos_stop_task(common_taskid, STOPWATCH_TICK);
	sw_state=SW_STOPPED;
    clock_active = 0;

    mode = MENU;
    btn_onOnePress(KEY1, menu_up);
    btn_onOnePress(KEY2, menu_down);
    btn_onLongPress(KEY1, menu_select);
#if HW_KEY_COUNT == 2
    btn_onLongPress(KEY2, NULL);
#endif
    disp_menu();
}

static void disp_charging()
{
	int blink = 0;
	while (1) {
		btn_tick();
		int percent = batt_raw2percent(batt_raw());

		if (charging_status()) {
			disp_bat_stt(blink ? percent : 0, 2, 2);
			if (ani_xbm_next_frame(&fabm_xbm, fb, 16, 0) == 0) {
				fb_puts(VERSION_ABBR, sizeof(VERSION_ABBR), 16, 2);
				fb_putchar(' ', 40, 2);
			}
			blink = !blink;
			DelayMs(500);
		} else {
			disp_bat_stt(percent, 7, 2);
			DelayMs(500);
			return;
		}
	}
}

static void mode_setup_download()
{
	// Disable bitmap transition while in download mode
	btn_onOnePress(KEY2, NULL);

#if HW_KEY_COUNT == 2
	// No aux KEY4 on 2-key hardware: give KEY2 long-press a way back to menu
	btn_onLongPress(KEY1, NULL);
	btn_onLongPress(KEY2, return_to_menu);
#endif

	// Take control of the current bitmap to display
	// the Bluetooth animation
	ble_enable_advertise();
	start_ble_animation();
}

void clean_bmlist()
{
	bm_t *curr_bm = bmlist_current();
	while (curr_bm->next != curr_bm)
		bmlist_drop(curr_bm->next);
}

void reload_bmlist()
{
	clean_bmlist();
	load_bmlist();
}

static void mode_setup_normal()
{
	btn_onOnePress(KEY2, bm_transition);
#if HW_KEY_COUNT == 2
	// No aux KEY4 on 2-key hardware: give KEY2 long-press a way back to menu
	btn_onLongPress(KEY1, NULL);
	btn_onLongPress(KEY2, return_to_menu);
#endif
	reload_bmlist();
	start_normal_animation();
}

void handle_after_rx()
{
    if (badge_cfg.reset_rx) {
        SYS_ResetExecute();
    } else {
        tmos_stop_task(common_taskid, CLOCK_TICK);
        tmos_stop_task(common_taskid, STOPWATCH_TICK);
        sw_state = SW_STOPPED;
        clock_active = 0;
        mode_setup_normal();
    }
}

int main()
{
	SetSysClock(CLK_SOURCE_PLL_60MHz);
	
	debug_init();
	PRINT("\nDebug console is on UART%d\n", DEBUG);

	cdc_onWrite(legacy_usb_rx);
	hiddev_onWrite(legacy_usb_rx);
	usb_start();

	led_init();
	TMR0_TimerInit((FREQ_SYS / 2000) / 2);
	TMR0_ITCfg(ENABLE, TMR0_3_IT_CYC_END);
	PFIC_EnableIRQ(TMR0_IRQn);

	bmlist_init(LED_COLS * 4);

	btn_init();
	btn_onOnePress(KEY1, menu_up);
	btn_onOnePress(KEY2, menu_down);
	btn_onLongPress(KEY1, menu_select);

#if HW_KEY_COUNT == 4
	auxbtn_init();
	auxbtn_onOnePress(KEY3, menu_select);
	auxbtn_onOnePress(KEY4, return_to_menu);
#endif

	power_init();
	disp_charging();
	cfg_init();
	xbm_t spl = {
		.bits = &(badge_cfg.splash_bm_bits),
		.w = badge_cfg.splash_bm_w,
		.h = badge_cfg.splash_bm_h,
		.fh = badge_cfg.splash_bm_fh,
	};
	play_splash(&spl, 0, 0, badge_cfg.splash_speedT);

	load_bmlist();

	ble_setup();

	spawn_tasks();
	btn_init_task();
#if HW_KEY_COUNT == 4
	auxbtn_init_task();
#endif
	game_init();
	stop_all_animation();

	mode = MENU;
	disp_menu();
	while (1) {
		TMOS_SystemProcess();
	}
}

__INTERRUPT
__HIGH_CODE
void TMR0_IRQHandler(void)
{
	static int i;
	int state;

	if (TMR0_GetITFlag(TMR0_3_IT_CYC_END)) {
		i++;
		state = i&3;

		if (state == 0) {
			if ((i >> 1) >= LED_COLS)
				i = 0;
			led_write2dcol(i >> 2, fb[i >> 1], fb[(i >> 1) + 1]);
		}
		else if (state > (badge_cfg.led_brightness&3))
			leds_releaseall();

		TMR0_ClearITFlag(TMR0_3_IT_CYC_END);
	}
}