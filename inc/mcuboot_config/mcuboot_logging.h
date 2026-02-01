#ifndef __MCUBOOT_LOGGING_H__
#define __MCUBOOT_LOGGING_H__

#include "common/debug.h"
#include "mcuboot_config.h"

#define MCUBOOT_LOG_ERR(_fmt, ...) PRINT(" [ERR] " _fmt "\n", ##__VA_ARGS__)
#define MCUBOOT_LOG_WRN(_fmt, ...) PRINT(" [WRN] " _fmt "\n", ##__VA_ARGS__)
#define MCUBOOT_LOG_INF(_fmt, ...) PRINT(" [INF] " _fmt "\n", ##__VA_ARGS__)
#define MCUBOOT_LOG_DBG(_fmt, ...) PRINT(" [DBG] " _fmt "\n", ##__VA_ARGS__)

#define MCUBOOT_LOG_MODULE_DECLARE(...)
#define MCUBOOT_LOG_MODULE_REGISTER(...)


#endif /* __MCUBOOT_LOGGING_H__ */
