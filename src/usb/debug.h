#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "CH58x_common.h"


#define _TRACE() PRINT("> "); PRINT(__FILE__);PRINT(": "); PRINT(__func__); \
				PRINT("()\n")

void print_setuppk(USB_SETUP_REQ *setup_req_pk);
void print_status_reg();
void print_intflag_reg();

#endif /* __DEBUG_H__ */
