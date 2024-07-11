
#include "CH58x_common.h"

#include "utils.h"
#include "debug.h"

extern void (*if_handlers[])(USB_SETUP_REQ *request);
extern void (*ep_handlers[])();

void handle_devreq(USB_SETUP_REQ *request);

static uint8_t address;

/* EP0 + EP4(IN + OUT) */
__attribute__((aligned(4))) uint8_t ep0buf[64 + 64 + 64];

void usb_set_address(uint8_t ad)
{
	address = ad;
}

void ctrl_start_load_block(void *buf, uint16_t len)
{
	usb_start_load_block(ep0buf, buf, len, 1);
}

static void route_interfaces(USB_SETUP_REQ *request)
{
	_TRACE();

	uint8_t ifn = request->wIndex & 0xff;
	PRINT("wInterfaceNumber: 0x%02x\n", ifn);

	if (if_handlers[ifn])
		if_handlers[ifn](request);
}

static void ep_handler()
{
	_TRACE();

	USB_SETUP_REQ *request = (USB_SETUP_REQ *)ep0buf;
	uint8_t req = request->bRequest;

	/* Each interface will have their own request handler */
	uint8_t recip = request->bRequestType & USB_REQ_RECIP_MASK;
	if (recip == USB_REQ_RECIP_INTERF) {
		route_interfaces(request);
		return;
	}

	uint8_t token = R8_USB_INT_ST & MASK_UIS_TOKEN;
	switch(token) {

	case UIS_TOKEN_SETUP:
		print_setuppk(req);

		if (recip == USB_REQ_RECIP_DEVICE) {
			handle_devreq(request);
		}
		break;

	case UIS_TOKEN_OUT:
		if (req == USB_CLEAR_FEATURE) {
			ctrl_ack();
		}

	case UIS_TOKEN_IN:
		R8_USB_DEV_AD = address;
		usb_load_next_chunk();
		break;
	
	default:
		break;
	}
}

void ctrl_init()
{
	ep_cb_register(0, ep_handler);
	dma_register(0, ep0buf);
}

static void route_enpoints()
{
	_TRACE();

	/* Workaround to solve a setup packet with EP 0x02 for unknown reason.
	This happens when return from the bootloader or after a sotfware reset. */
	uint8_t token = R8_USB_INT_ST & MASK_UIS_TOKEN;
	if (token == UIS_TOKEN_SETUP) {
		ep_handlers[0]();
	}

	uint8_t ep_num = R8_USB_INT_ST & MASK_UIS_ENDP;

	if (ep_num < 8 && ep_handlers[ep_num])
		ep_handlers[ep_num]();
}

static void handle_busReset()
{
	_TRACE();
	R8_USB_DEV_AD = 0;
	R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
	R8_UEP1_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
	R8_UEP2_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
	R8_UEP3_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
	R8_UEP4_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
	R8_UEP5_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
	R8_UEP6_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
	R8_UEP7_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
}

static void handle_powerChange()
{
	_TRACE();
	if (R8_USB_MIS_ST & RB_UMS_SUSPEND) {
		;// suspend
	}
	else {
		;// resume
	}
}

__INTERRUPT
__HIGH_CODE
void USB_IRQHandler(void) {
	uint8_t intflag = R8_USB_INT_FG;
	clear_handshake_sent_flag();
	PRINT("\nusb: new interrupt\n");
	print_intflag_reg();

	if (intflag & RB_UIF_TRANSFER) {
		PRINT("usb: RX Length reg: %d\n", R8_USB_RX_LEN);
		print_status_reg();

		route_enpoints();
	}
	else if (intflag & RB_UIF_BUS_RST) {
		handle_busReset();
	}
	else if (intflag & RB_UIF_SUSPEND) {
		handle_powerChange();
	}

	if (handshake_sent() == 0) {
		PRINT("WARN: This transaction is being IGNORED!\n");
	}
	R8_USB_INT_FG = intflag; // clear interrupt flags
}