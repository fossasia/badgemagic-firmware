#include "CH58x_common.h"

#include "leddrv.h"

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
