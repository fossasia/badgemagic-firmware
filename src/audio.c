#include "CH58x_common.h"
#include <stdio.h>
#include "usb/usb.h"
#include "leddrv.h"
#include "audio.h"

const uint16_t amp_wav_lut[8] = {
	0b00000000000,
	0b00000100000,
	0b00001110000,
	0b00011111000,
	0b00111111100,
	0b01111111110,
	0b11111111111,
	0b11111111111,
};

const uint16_t amp_wav_lut_w1[8] = {
	0b00000100000,
	0b00001110000,
	0b00011111000,
	0b00111111100,
	0b01111111110,
	0b11111111111,
	0b11111111111,
	0b11111111111,
};

static int16_t mic_baseline = 2048;

int16_t mic_adc()
{
    ADC_ChannelCfg(11);
    int16_t peak = 0;
    for (int i = 0; i < 64; i++) {
        uint16_t raw = ADC_ExcutSingleConver();
        int16_t sample = (int16_t)raw - mic_baseline;
        int16_t abssample = sample < 0 ? -sample : sample;
        if (abssample > peak) peak = abssample;

        if (i == 0) {
            char buf[64];
            int len = snprintf(buf, sizeof(buf), "raw=%d base=%d\r\n", raw, mic_baseline);
            cdc_tx_poll((uint8_t *)buf, len, 10);
        }
    }
    return peak;
}

void mic_init()
{
    GPIOA_ModeCfg(GPIO_Pin_7, GPIO_ModeIN_Floating);
    ADC_ExtSingleChSampInit(SampleFreq_3_2, ADC_PGA_2);
    ADC_ChannelCfg(11);

    DelayMs(10);

    uint64_t sum = 0;
    for (int i = 0; i < 64; i++) sum += ADC_ExcutSingleConver();
    mic_baseline = (int16_t)(sum / 64);

    char buf[64];
    int len = snprintf(buf, sizeof(buf), "\r\nbaseline=%d\r\n", mic_baseline);
    cdc_tx_poll((uint8_t *)buf, len, 10);
}
