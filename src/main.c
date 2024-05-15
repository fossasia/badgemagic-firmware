#include "CH58x_common.h"
#include "CH58x_sys.h"

void led_init(void)
{
	GPIOA_SetBits(GPIO_Pin_10);
	GPIOA_ResetBits(GPIO_Pin_12);
	GPIOA_ModeCfg(GPIO_Pin_10, GPIO_ModeOut_PP_5mA);
	GPIOA_ModeCfg(GPIO_Pin_12, GPIO_ModeOut_PP_5mA);
}

void led_toggle(void)
{
	GPIOA_InverseBits(GPIO_Pin_10);
	GPIOA_InverseBits(GPIO_Pin_12);
}

int main()
{
	SetSysClock(CLK_SOURCE_PLL_60MHz);

	led_init();

	while(1)
	{
		mDelaymS(100);
		led_toggle();
	}
}

