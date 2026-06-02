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

int16_t mic_adc()
{
    ADC_ChannelCfg(11);
    uint16_t sig_max = 0;
    uint16_t sig_min = 4095;

    for (int i = 0; i < 64; i++) {
        uint16_t sample = ADC_ExcutSingleConver();
        if (sample > sig_max) sig_max = sample;
        if (sample < sig_min) sig_min = sample;
    }

    int16_t amplitude = sig_max - sig_min;

    char buf[64];
    int len = snprintf(buf, sizeof(buf), "amp=%d\r\n", amplitude);
    cdc_tx_poll((uint8_t *)buf, len, 10);

    return amplitude;
}

void mic_init()
{
    GPIOA_ModeCfg(GPIO_Pin_7, GPIO_ModeIN_Floating);
    ADC_ExtSingleChSampInit(SampleFreq_3_2, ADC_PGA_2);
    ADC_ChannelCfg(11);

    DelayMs(500);

    uint64_t sum = 0;
    for (int i = 0; i < 64; i++) sum += ADC_ExcutSingleConver();
    mic_baseline = (int16_t)(sum / 64);

    char buf[64];
    int len = snprintf(buf, sizeof(buf), "\r\nbaseline=%d\r\n", mic_baseline);
    cdc_tx_poll((uint8_t *)buf, len, 10);
}