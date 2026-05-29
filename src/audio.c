#include "CH58x_common.h"

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

static int16_t mic_baseline = 2048; // calibrated at init

int16_t mic_adc()
{
    ADC_ChannelCfg(11);
    int16_t peak = 0;
    for (int i = 0; i < 64; i++) {
        int16_t sample = (int16_t)ADC_ExcutSingleConver() - mic_baseline;
        int16_t abssample = sample < 0 ? -sample : sample;
        if (abssample > peak) peak = abssample;
    }
    return peak;
}

void mic_init()
{
    GPIOA_ModeCfg(GPIO_Pin_7, GPIO_ModeIN_Floating);
    ADC_ExtSingleChSampInit(SampleFreq_3_2, ADC_PGA_0);
    ADC_ChannelCfg(11);

    uint64_t sum = 0;
    for (int i = 0; i < 64; i++) sum += ADC_ExcutSingleConver();
    mic_baseline = (int16_t)(sum / 64);
}
