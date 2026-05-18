#include "CH58x_common.h"
#include "CH58xBLE_LIB.h"
#include <stdbool.h>
#include "auxbtn.h"
#include "debug.h"

#define AUX_ADC_CHANNEL         (3)
#define BATT_ADC_CHANNEL        (1)
#define ADC_SAMPLE_COUNT        (20)

#define NO_PRESS_LOW            (3000)
#define KEY3_HIGH               (500)
#define KEY4_LOW                (501)
#define KEY4_HIGH               (2999)
#define DEBOUNCE_THRES          (5)

enum aux_state {
    AUX_NO_PRESS = 0,
    AUX_KEY3,
    AUX_KEY4,
    AUX_UNDEFINED,
};

static volatile void (*onePressHandler[2])(void) = { NULL };
static volatile bool onePressPending[2] = { false };

static tmosTaskID auxbtn_task_id = INVALID_TASK_ID;
#define AUXBTN_POLL (1 << 0)
#define AUXBTN_POLL_FREQ (50) //same as button.c

static int aux_adc_read()
{
    ADC_ChannelCfg(AUX_ADC_CHANNEL);

    int ret = 0;
    for (int i = 0; i < ADC_SAMPLE_COUNT; i++) {
        ret += ADC_ExcutSingleConver();
    }

    ADC_ChannelCfg(BATT_ADC_CHANNEL);
    return ret / ADC_SAMPLE_COUNT;
}

static enum aux_state classify(int raw)
{
    if (raw > NO_PRESS_LOW)
        return AUX_NO_PRESS;
    if (raw < KEY3_HIGH)
        return AUX_KEY3;
    if (raw > KEY4_LOW && raw < KEY4_HIGH)
        return AUX_KEY4;
    return AUX_UNDEFINED;
}

static void auxbtn_poll()
{
    static enum aux_state last = AUX_NO_PRESS;
    static int count = 0;

    enum aux_state curr = classify(aux_adc_read());

    if (curr == AUX_UNDEFINED) {
        count = 0;
        return;
    }

    if (curr == AUX_NO_PRESS) {
        last = AUX_NO_PRESS;
        count = 0;
        return;
    }

    if (curr == last) {
        count++;
    } else {
        last = curr;
        count = 1;
        return;
    }

    if (count == DEBOUNCE_THRES) {
        if (curr == AUX_KEY3)
            onePressPending[0] = true;
        else if (curr == AUX_KEY4)
            onePressPending[1] = true;
    }
}

static void auxbtn_tick()
{
    for (int i = 0; i < 2; i++) {
        if (onePressPending[i]) {
            onePressPending[i] = false;
            if (onePressHandler[i])
                onePressHandler[i]();
        }
    }
}

static uint16_t auxbtn_task(tmosTaskID id, uint16_t events)
{
    if (events & SYS_EVENT_MSG) {
        uint8 *pMsg = tmos_msg_receive(auxbtn_task_id);
        if (pMsg != NULL)
            tmos_msg_deallocate(pMsg);
        return (events ^ SYS_EVENT_MSG);
    }

    if (events & AUXBTN_POLL) {
        auxbtn_poll();
        auxbtn_tick();
        return (events ^ AUXBTN_POLL);
    }

    return events;
}

void auxbtn_init_task(void)
{
    auxbtn_task_id = TMOS_ProcessEventRegister(auxbtn_task);
    tmos_start_reload_task(auxbtn_task_id, AUXBTN_POLL,
                1000000 / AUXBTN_POLL_FREQ / 625);
}

void auxbtn_init(void)
{
    GPIOA_ModeCfg(GPIO_Pin_13, GPIO_ModeIN_Floating);
}

void auxbtn_onOnePress(int key, void (*handler)(void))
{
    if (key == KEY3)
        onePressHandler[0] = handler;
    else if (key == KEY4)
        onePressHandler[1] = handler;
}