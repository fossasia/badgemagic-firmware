#ifndef __COMMON_DEBUG_H__
#define __COMMON_DEBUG_H__

#include <stdio.h>

#ifdef  DEBUG
#define PRINT(X...) printf(X)
#else
#define PRINT(X...)
#endif

#define _TRACE() PRINT("> "); PRINT(__FILE__); PRINT(": "); PRINT(__func__); \
				PRINT("()\n")

void dump_mem(void *ptr, int len);

#endif /* __COMMON_DEBUG_H__ */
