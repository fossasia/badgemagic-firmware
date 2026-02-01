#include "CH58xBLE_LIB.h"
#include "button.h"
#include <stdbool.h>

#define DEBOUNCE_HIGH_THRES (200) // 0-255
#define DEBOUNCE_LOW_THRES  (55)  // 0-255
#define LONGPRESS_THRES     (25)  // Hz
#define BUTTON_SCAN_FREQ    (50)  // Hz

static volatile void (*onePressHandler[KEY_INDEX])(void) = { NULL };
static volatile void (*longPressHandler[KEY_INDEX])(void) = { NULL };

static volatile bool onePressPending[KEY_INDEX] = { false };
static volatile bool longPressPending[KEY_INDEX] = { false };

static tmosTaskID button_task_id = INVALID_TASK_ID;
#define BTN_PRESS (1 << 0)
static uint16_t btn_task(tmosTaskID, uint16_t);

void btn_init()
{
	GPIOA_ModeCfg(KEY1_PIN, GPIO_ModeIN_PD);
	GPIOB_ModeCfg(KEY2_PIN, GPIO_ModeIN_PU);

	TMR3_TimerInit(FREQ_SYS/BUTTON_SCAN_FREQ);
	TMR3_ITCfg(ENABLE, TMR0_3_IT_CYC_END);
	PFIC_EnableIRQ(TMR3_IRQn);
}

void btn_init_task(void)
{
	button_task_id = TMOS_ProcessEventRegister(btn_task);
}

void btn_onOnePress(int key, void (*handler)(void))
{
	if (key >= KEY_INDEX) return;
	onePressHandler[key] = handler;
}

void btn_onLongPress(int key, void (*handler)(void))
{
	if (key >= KEY_INDEX) return;
	longPressHandler[key] = handler;
}

void btn_tick(void)
{
	for (int i = 0; i < KEY_INDEX; i++) {
		if (onePressPending[i]) {
			onePressPending[i] = false;
			if (onePressHandler[i]) onePressHandler[i]();
		}
		if (longPressPending[i]) {
			longPressPending[i] = false;
			if (longPressHandler[i]) longPressHandler[i]();
		}
	}
}

static uint16_t btn_task(tmosTaskID id, uint16_t events)
{

	if(events & SYS_EVENT_MSG) {
		uint8 *pMsg = tmos_msg_receive(button_task_id);
		if(pMsg != NULL)
		{
			tmos_msg_deallocate(pMsg);
		}
		return (events ^ SYS_EVENT_MSG);
	}

	if (events & BTN_PRESS) {
		btn_tick();
		return (events ^ BTN_PRESS);
	}

	return events;
}


__HIGH_CODE
static int debounce(int key, int is_press)
{
	static int y[KEY_INDEX], flag[KEY_INDEX];
	if (key >= KEY_INDEX) return 0;
	
	// RC filter
	y[key] -=  y[key] >> 2;
	y[key] += is_press ? 0x3F : 0;

	// Schmitt trigger
	if ((y[key] > DEBOUNCE_HIGH_THRES) && (flag[key] == 0))
		flag[key] = 1;
	if ((y[key] < DEBOUNCE_LOW_THRES) && (flag[key] == 1))
		flag[key] = 0;

	return flag[key];
}

__HIGH_CODE
static void check(int k)
{
	static int hold[KEY_INDEX], is_longpress[KEY_INDEX];
	if (k >= KEY_INDEX) return; // TODO: assert instead

	if (debounce(k, isPressed(k))) {
		hold[k]++;
		if (hold[k] >= LONGPRESS_THRES && is_longpress[k] == 0) {
			is_longpress[k] = 1;
			longPressPending[k] = true;
			if (button_task_id != INVALID_TASK_ID) {
				tmos_set_event(button_task_id, BTN_PRESS);
			}
		}
	} else {
		if (hold[k] > 0 && hold[k] < LONGPRESS_THRES) {
			onePressPending[k] = true;
			if (button_task_id != INVALID_TASK_ID) {
				tmos_set_event(button_task_id, BTN_PRESS);
			}
		}
		is_longpress[k] = 0;
		hold[k] = 0;
	}
}

__HIGH_CODE
static void check_keys()
{
	for (int k=0; k<KEY_INDEX; k++) {
		check(k);
	}
}

__INTERRUPT
__HIGH_CODE
void TMR3_IRQHandler(void)
{
	if (TMR3_GetITFlag(TMR0_3_IT_CYC_END)) {		
		check_keys();
		TMR3_ClearITFlag(TMR0_3_IT_CYC_END);
	}
}
