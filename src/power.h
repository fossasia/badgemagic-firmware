#ifndef __RESET_H__
#define __RESET_H__

static inline void reset_jump()
{
	asm volatile("j 0x00");
}

void poweroff();

#endif /* __RESET_H__ */
