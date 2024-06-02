#ifndef __BUTTON_H__
#define __BUTTON_H__

#include "CH58x_common.h"

enum keys {
	KEY1 = 0,
	KEY2,
	KEY_INDEX,
};

#define KEY2_PIN (GPIO_Pin_22) // PB
#define KEY1_PIN (GPIO_Pin_1) // PA

#define isPressed(key) 		((key) ? \
				!GPIOB_ReadPortPin(KEY2_PIN) : \
				GPIOA_ReadPortPin(KEY1_PIN))

void btn_onOnePress(int key, void (*handler)(void));
void btn_onLongPress(int key, void (*handler)(void));
void btn_init();

#endif /* __BUTTON_H__ */
