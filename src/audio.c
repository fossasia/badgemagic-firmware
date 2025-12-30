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

// TODO: setup an interrupt instead of polling, now is just for POC
int16_t mic_adc()
{
	ADC_ChannelCfg(11);
	uint64_t sum = 0;
	for (int i = 0; i < 64; i++) {
		sum += ADC_ExcutSingleConver();
		// mDelayuS(1);
	}

	// adc idle level is 2048. Normalizing this to 0
	return sum / 64 - 2048;
}

void mic_init()
{
	/* adc 11 - pa7 */
	GPIOA_ModeCfg(GPIO_Pin_7, GPIO_ModeIN_Floating);
	ADC_ExtSingleChSampInit(SampleFreq_3_2, ADC_PGA_0);
	ADC_ChannelCfg(11);
}
