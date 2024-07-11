#include <stdint.h>
#include <memory.h>

#include "CH58x_common.h"

#include "../utils.h"
#include "../debug.h"

#define EP_NUM   (1)
#define IF_NUM   (0)

static __attribute__((aligned(4))) uint8_t ep_buf[64 + 64]; 
static uint8_t *const ep_out = ep_buf;
static uint8_t *const ep_in = ep_buf + 64;

static void (*on_write)(uint8_t *buf, uint16_t len);

static USB_ITF_DESCR if_desc = {
	.bLength = sizeof(USB_ITF_DESCR),
	.bDescriptorType = USB_DESCR_TYP_INTERF,
	.bInterfaceNumber = IF_NUM,
	.bAlternateSetting = 0,
	.bNumEndpoints = 2, /* One for read, one for write */
	.bInterfaceClass = 0x03, /* HID class */
	.bInterfaceSubClass = 0, /* No subclass */
	.bInterfaceProtocol = 0, /* Not a Mouse nor Keyboard */
	.iInterface = 0 /* Index of string descriptor */
};

static USB_HID_DESCR hid_desc = {
	.bLength = sizeof(USB_HID_DESCR),
	.bDescriptorType = USB_DESCR_TYP_HID,
	.bcdHID = 0x0100,
	.bCountryCode = 0x00,
	.bNumDescriptors = 0x01,
	.bDescriptorTypeX = 0x22,
	.wDescriptorLengthL = 0x22,
	.wDescriptorLengthH = 0x00
};

static USB_ENDP_DESCR read_ep_desc = {
	.bLength = sizeof(USB_ENDP_DESCR),
	.bDescriptorType = USB_DESCR_TYP_ENDP,
	.bEndpointAddress = 0x80 | EP_NUM, /* IN endpoint */
	.bmAttributes = 0x03, /* exchange data over Interrupt */
	.wMaxPacketSize = MAX_PACKET_SIZE, /* bytes */
	.bInterval = 0xff /* polling interval */
};

static USB_ENDP_DESCR write_ep_desc = {
	.bLength = sizeof(USB_ENDP_DESCR),
	.bDescriptorType = USB_DESCR_TYP_ENDP,
	.bEndpointAddress = EP_NUM, /* IN endpoint */
	.bmAttributes = 0x03, /* exchange data over Interrupt */
	.wMaxPacketSize = MAX_PACKET_SIZE, /* bytes */
	.bInterval = 8 /* polling interval */
};

static const uint8_t report_desc[] = {
	0x06, 0x00, 0xFF,  // Usage Page (Vendor Defined 0xFF00)
	0x09, 0x01,        // Usage (0x01)
	0xA1, 0x01,        // Collection (Application)

	/* IN */
	0x09, 0x02,        //   Usage (0x02)
	0x15, 0x00,        //   Logical Minimum (0)
	0x26, 0x00, 0xFF,  //   Logical Maximum (-256)
	0x75, 0x08,        //   Report Size (8)
	0x95, 0x40,        //   Report Count (64)
	0x81, 0x06,        //   INPUT

	/* OUT */
	0x09, 0x02,        //   Usage (0x02)
	0x15, 0x00,        //   Logical Minimum (0)
	0x26, 0x00, 0xFF,  //   Logical Maximum (-256)
	0x75, 0x08,        //   Report Size (8)
	0x95, 0x40,        //   Report Count (64)
	0x91, 0x06,        //   OUTPUT

	0xC0,              // End Collection
};

static void hid_request_handler(USB_SETUP_REQ * request)
{
	_TRACE();
	static uint8_t report_val, idle_val;
	uint8_t req = request->bRequest;
	uint16_t type = request->wValue >> 8;

	switch(req) {
	case USB_GET_DESCRIPTOR:
		PRINT("- USB_GET_DESCRIPTOR\n");
		PRINT("bDescriptorType: 0x%02x\n", type);
		switch (type)
		{
		case USB_DESCR_TYP_REPORT:
			PRINT("- USB_DESCR_TYP_REPORT\n");
			ctrl_start_load_block(report_desc, sizeof(report_desc));
			break;

		default:
			break;
		}
		break;

	case HID_GET_REPORT:
		PRINT("- HID_GET_REPORT\n");
		break;

	case HID_GET_IDLE:
		PRINT("- HID_GET_IDLE\n");
		ctrl_start_load_block(&idle_val, 1);
		break;

	case HID_GET_PROTOCOL:
		PRINT("- HID_GET_PROTOCOL\n");
		ctrl_start_load_block(&report_val, 1);
		break;

	case HID_SET_REPORT:
		PRINT("- HID_SET_REPORT\n");
		ctrl_ack();
		break;

	case HID_SET_IDLE:
		PRINT("- HID_SET_IDLE\n");
		ctrl_ack();
		break;

	case HID_SET_PROTOCOL:
		PRINT("- HID_SET_PROTOCOL\n");
		break;

	default:
		break;
	}
}

static volatile uint16_t transferred;

static void ep_handler()
{
	_TRACE();
	static int tog;
	uint8_t token = R8_USB_INT_ST & MASK_UIS_TOKEN;

	switch(token) {
	case UIS_TOKEN_OUT:
		if (on_write)
			on_write(ep_out, R8_USB_RX_LEN);
		set_handshake(EP_NUM, USB_ACK, tog, 0);
		tog = !tog;
		break;

	case UIS_TOKEN_IN:
		if (transferred == 0) {
			transferred = 1;
		} else {
			set_handshake(EP_NUM, USB_NAK, 1, 0);
		}
		break;
		break;
	
	default:
		break;
	}
}

// In case we want to send something to the host,
// or want to see the log over hidraw by `cat /dev/hidrawX`
void hiddev_fill_IN(uint8_t *buf, uint8_t len)
{
	if (len > read_ep_desc.wMaxPacketSize)
		return;

	static int tog;

	memcpy(ep_in, buf, len);
	set_handshake(EP_NUM, USB_ACK, tog, len);

	tog = !tog;
	transferred = 0;
}

static int wait_until_sent(uint16_t timeout_ms)
{
	while(timeout_ms--) {
		if (transferred) {
			return 0;
		}
		DelayMs(1);
	}
	return -1;
}

int hiddev_tx_poll(uint8_t *buf, int len, uint16_t timeout_ms)
{
	int i = 0;
	while (len > read_ep_desc.wMaxPacketSize) {
		hiddev_fill_IN(buf + i, read_ep_desc.wMaxPacketSize);
		if (wait_until_sent(timeout_ms))
			return -1;

		i += read_ep_desc.wMaxPacketSize;
		len -= read_ep_desc.wMaxPacketSize;
	}
	hiddev_fill_IN(buf + i, len);
	if (wait_until_sent(timeout_ms))
		return -1;

	return 0;
}

void hiddev_onWrite(void (*cb)(uint8_t *buf, uint16_t len))
{
	on_write = cb;
}

void hiddev_init()
{
	cfg_desc_append(&if_desc);
	cfg_desc_append(&hid_desc);
	cfg_desc_append(&read_ep_desc);
	cfg_desc_append(&write_ep_desc);

	if_cb_register(IF_NUM, hid_request_handler);
	ep_cb_register(EP_NUM, ep_handler);

	dma_register(EP_NUM, ep_out);
}
