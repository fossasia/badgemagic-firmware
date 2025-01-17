#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdio.h>

#ifdef  DEBUG
#define PRINT(X...) printf(X)
#else
#define PRINT(X...)
#endif

#define _TRACE() PRINT("> "); PRINT(__FILE__); PRINT(": "); PRINT(__func__); \
				PRINT("()\n")

#endif /* __DEBUG_H__ */
