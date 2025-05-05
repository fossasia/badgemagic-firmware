#ifndef __HAL_SYSTEM_H__
#define __HAL_SYSTEM_H__

#include <CH58x_common.h>

static inline void hal_system_reset()
{
	SYS_ResetExecute();
}

#endif /* __HAL_SYSTEM_H__ */
