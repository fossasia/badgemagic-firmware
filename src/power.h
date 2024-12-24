#ifndef __RESET_H__
#define __RESET_H__

#define CHARGE_STT_PIN      GPIO_Pin_0 // PA0

static inline void reset_jump()
{
	asm volatile("j 0x00");
}

void power_init();
void poweroff();
int batt_raw();
int charging_status();
int batt_raw2percent(int r);

#endif /* __RESET_H__ */
