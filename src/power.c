#include <CH58x_common.h>

#include "power.h"
#include "button.h"
#include "debug.h"

void poweroff()
{
	// Stop wasting energy
	GPIOA_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_Floating);
	GPIOB_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_Floating);

	// Configure wake-up sources
	// KEY1: Always enabled (button wake-up)
	GPIOA_ModeCfg(KEY1_PIN, GPIO_ModeIN_PD);
	GPIOA_ITModeCfg(KEY1_PIN, GPIO_ITMode_RiseEdge);

#ifndef USBC_VERSION
	// CHARGE_STT: Enable for micro-USB version
	// Note: Disabled on USB-C version because it interferes with KEY1 wake-up
	GPIOA_ModeCfg(CHARGE_STT_PIN, GPIO_ModeIN_PU);
	GPIOA_ITModeCfg(CHARGE_STT_PIN, GPIO_ITMode_FallEdge);
#endif

	PFIC_EnableIRQ(GPIO_A_IRQn);
	PWR_PeriphWakeUpCfg(ENABLE, RB_SLP_GPIO_WAKE, Long_Delay);

	/* Good bye */
	LowPower_Shutdown(0);
}

void power_init()
{
	GPIOA_ModeCfg(GPIO_Pin_5, GPIO_ModeIN_Floating);
	ADC_ExtSingleChSampInit(SampleFreq_3_2, ADC_PGA_0);

	int16_t adc_calib = ADC_DataCalib_Rough();
	PRINT("RoughCalib_Value = %d \n", adc_calib);

	ADC_ChannelCfg(1);
}

int batt_raw()
{
	int ret = 0;

	PRINT("ADC reading: \n");
	uint16_t buf[20];
	for(int i = 0; i < 20; i++) {
		uint16_t adc = ADC_ExcutSingleConver();
		ret += adc;
		PRINT("%d \n", adc);
	}

	return ret / 20;
}

int charging_status()
{
	GPIOA_ModeCfg(CHARGE_STT_PIN, GPIO_ModeIN_PU);
	return GPIOA_ReadPortPin(CHARGE_STT_PIN) == 0;
}

#define ZERO_PERCENT_THRES      (3.3)
#define _100_PERCENT_THRES      (4.2)
#define ADC_MAX_VAL             (4096.0) // 12 bit
#define ADC_MAX_VOLT            (2.1)   // Volt
#define R1                      (182.0) // kOhm
#define R2                      (100.0) // kOhm
#define PERCENT_RANGE           (_100_PERCENT_THRES - ZERO_PERCENT_THRES)
#define VOLT_DIV(v)             ((v) / (R1 + R2) * R2) // Voltage divider
#define VOLT_DIV_INV(v)         ((v) / R2 * (R1 + R2)) // .. Inverse
#define ADC2VOLT(raw)           ((raw) / ADC_MAX_VAL * ADC_MAX_VOLT)
#define VOLT2ADC(volt)          ((volt) / ADC_MAX_VOLT * ADC_MAX_VAL)

int batt_raw2percent(int r)
{
	float vadc = ADC2VOLT(r);
	float vbat = VOLT_DIV_INV(vadc);
	float strip = vbat - ZERO_PERCENT_THRES;
	if (strip < PERCENT_RANGE) {
		// Negative values meaning the battery is not connected or died
		return (int)(strip / PERCENT_RANGE * 100.0);
	}
	return 100;
}
