#ifndef __USB_DEBUG_H__
#define __USB_DEBUG_H__

#include "CH58x_common.h"
#include "../debug.h"

#ifdef USB_DEBUG_ENABLED

void print_setuppk(USB_SETUP_REQ *setup_req_pk);
void print_status_reg();
void print_intflag_reg();

#else /* USB_DEBUG_ENABLED */

#undef PRINT()
#define PRINT(X...) do {} while(0)
#undef _TRACE()
#define _TRACE() do {} while(0)

#define print_setuppk(x) do {} while(0)
#define print_status_reg() do {} while(0)
#define print_intflag_reg() do {} while(0)


#endif /* USB_DEBUG_ENABLED */

#endif /* __USB_DEBUG_H__ */
