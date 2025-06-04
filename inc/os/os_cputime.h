#ifndef __OS_CPUTIME_H__
#define __OS_CPUTIME_H__

#include <CH58x_sys.h>

static inline void os_cputime_delay_usecs(int t)
{
	mDelayuS(t);
}

#endif /* __OS_CPUTIME_H__ */
