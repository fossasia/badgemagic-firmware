#include "CH58x_common.h"

void print_setuppk(USB_SETUP_REQ *request)
{
	PRINT("Setup request:\n");

	char *dir[] = {"OUT", "IN"};
	char *type[] = {"Standard", "Class", "Vendor", "Reserved Type"};
	char *recipient[] = {
		"Device", "Interface", "Endpoint", "Other","Reserved Recipient"
	};
	char *bRequest[] = {
		"GET_STATUS",
		"CLEAR_FEATURE",
		"Reserved",
		"SET_FEATURE",
		"Reserved",
		"SET_ADDRESS",
		"GET_DESCRIPTOR",
		"SET_DESCRIPTOR",
		"GET_CONFIGURATION",
		"SET_CONFIGURATION",
		"GET_INTERFACE",
		"SET_INTERFACE",
		"SYNCH_FRAME",
	};

	uint8_t t = request->bRequestType;
	uint8_t req = request->bRequest;
	uint8_t type_val = (t & USB_REQ_TYP_MASK) >> 5;
	uint8_t recip_val = t & USB_REQ_RECIP_MASK;
	if (recip_val > 4)
		recip_val = 4;

	PRINT("\t- bRequestType: 0x%02x (%s|%s|%s)\n", t, dir[t>>7], 
				type[type_val],
				recipient[recip_val]);

	PRINT("\t- bRequest: 0x%02x (%s)\n", req, req > 12 ?
				"N/A" : bRequest[req]);
	PRINT("\t- wValue: 0x%04x\n", request->wValue);

	PRINT("\t- wIndex%s: 0x%04x\n", 
				recip_val == USB_REQ_RECIP_INTERF ? " (wInterfaceNumber)" : "",
				request->wIndex);

	PRINT("\t- wLength: 0x%04x\n", request->wLength);
}

void print_status_reg()
{
	char *token_type[] = {
		"OUT", "SOF","IN", "SETUP",
	};
	uint8_t reg = R8_USB_INT_ST;
	uint8_t is_setup = reg & RB_UIS_SETUP_ACT;
	uint8_t toggle = reg & RB_UIS_TOG_OK;
	uint8_t token = (reg & MASK_UIS_TOKEN) >> 4;
	uint8_t ep_num = reg & MASK_UIS_ENDP;
	PRINT("usb: Status reg: 0x%02x (%s|%s|%s|EP:%d)\n", reg, 
				is_setup ? "SETUP??" : "0",
				toggle ? "TOGGLE OK" : "0", token_type[token], ep_num);
}

void print_intflag_reg()
{
	uint8_t reg = R8_USB_INT_FG;
	uint8_t is_nak = reg & RB_U_IS_NAK;
	uint8_t tog_ok = reg & RB_U_TOG_OK;
	uint8_t sie = reg & RB_U_SIE_FREE;
	uint8_t overflow = reg & RB_UIF_FIFO_OV;
	uint8_t suspend = reg & RB_UIF_SUSPEND;
	uint8_t xfer_complete = reg & RB_UIF_TRANSFER;
	uint8_t bus_reset = reg & RB_UIF_BUS_RST;
	PRINT("usb: Interrupt reg: 0x%02x (%s|%s|%s|%s|%s|%s)\n", reg, 
				is_nak ? "NAK received" : "0",
				tog_ok ? "Toggle ok" : "0",
				sie ? "SIE" : "0",
				overflow ? "FIFO overflow" : "0",
				suspend ? "Suspend" : "0",
				xfer_complete ? "Xfer completed" : "0");
}