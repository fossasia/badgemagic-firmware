#ifndef __USB_DEBUG_H__
#define __USB_DEBUG_H__

#include "CH58x_common.h"
#include "debug.h"

void print_setuppk(USB_SETUP_REQ *setup_req_pk);
void print_status_reg(void);
void print_intflag_reg(void);

#endif /* __USB_DEBUG_H__ */