#include <stdint.h>
#include <memory.h>

#include "CH58x_common.h"

#include "../utils.h"
#include "../debug.h"

#define NOTI_EP_NUM   (2)
#define DATA_EP_NUM   (3)
#define ACM_IF_NUM    (1)

#define USB_DESCTYPE_CS_INTERFACE 0x24

static __attribute__((aligned(4))) uint8_t noti_ep_buf[64 + 64]; 
static uint8_t *const noti_ep_out = noti_ep_buf;
static uint8_t *const noti_ep_in = noti_ep_buf + 64;

static __attribute__((aligned(4))) uint8_t data_ep_buf[64 + 64]; 
static uint8_t *const data_ep_out = data_ep_buf;
static uint8_t *const data_ep_in = data_ep_buf + 64;

static void (*on_write)(uint8_t *buf, uint16_t len);

/* CDC Communication interface */
static USB_ITF_DESCR acm_if_desc = {
	.bLength = sizeof(USB_ITF_DESCR),
	.bDescriptorType = USB_DESCR_TYP_INTERF,
	.bInterfaceNumber = ACM_IF_NUM,
	.bAlternateSetting = 0,
	.bNumEndpoints = 3, // A Notification and RX/TX Endpoint

	.bInterfaceClass = 0x02, /* Communications and CDC Control */
	.bInterfaceSubClass = 2, /* ACM subclass */
	.bInterfaceProtocol = 1, /* AT Command V.250 protocol */

	.iInterface = 0 /* No string descriptor */
};

/* Header Functional descriptor */
static uint8_t header_func_desc[] = {
	5,                          /* bLength */
	USB_DESCTYPE_CS_INTERFACE,  /* bDescriptortype */
	0x00,       /* bDescriptorsubtype, HEADER */
	0x10, 0x01, /* bcdCDC */
};

/* ACM Functional descriptor */
static uint8_t acm_func_desc[] = {
	4,                          /* bLength */
	USB_DESCTYPE_CS_INTERFACE,  /* bDescriptortype */
	0x02,    /* bDescriptorsubtype, ABSTRACT CONTROL MANAGEMENT */
	0x02,    /* bmCapabilities: Supports subset of ACM commands */
};

/* Call Management Functional descriptor */
static uint8_t callmgr_func_desc[] = {
	4,                          /* bLength */
	USB_DESCTYPE_CS_INTERFACE,  /* bDescriptortype */
	0x01,    /* bDescriptorsubtype, CALL MANAGEMENT */
	0x03,    /* bmCapabilities, DIY */
};

/* Notification Endpoint descriptor */
static USB_ENDP_DESCR noti_ep_desc = {
	.bLength = sizeof(USB_ENDP_DESCR),
	.bDescriptorType = USB_DESCR_TYP_ENDP,
	.bEndpointAddress = 0x80 | NOTI_EP_NUM, /* IN endpoint */
	.bmAttributes = 0x03,       /* Interrupt transfer */
	.wMaxPacketSize = 64,       /* bytes */
	.bInterval = 0xff
};

/* Data TX Endpoint descriptor */
static USB_ENDP_DESCR tx_ep_desc = {
	.bLength = sizeof(USB_ENDP_DESCR),
	.bDescriptorType = USB_DESCR_TYP_ENDP,
	.bEndpointAddress = 0x80 | DATA_EP_NUM, /* IN endpoint */
	.bmAttributes = 0x02,       /* Bulk */
	.wMaxPacketSize = MAX_PACKET_SIZE,       /* bytes */
	.bInterval = 0xff
};

/* Data RX Endpoint descriptor */
static USB_ENDP_DESCR rx_ep_desc = {
	.bLength = sizeof(USB_ENDP_DESCR),
	.bDescriptorType = USB_DESCR_TYP_ENDP,
	.bEndpointAddress = DATA_EP_NUM, /* OUT endpoint */
	.bmAttributes = 0x02,       /* Bulk */
	.wMaxPacketSize = MAX_PACKET_SIZE,       /* bytes */
	.bInterval = 0xff
};

static void cdc_request_handler(USB_SETUP_REQ * request)
{
	_TRACE();
	// Handle CDC Class Request here
}

static void noti_ep_handler()
{
	_TRACE();
	// Handle Subclass Request here
}

static volatile uint16_t transferred;

static void data_ep_handler()
{
	_TRACE();
	static int tog;

	uint8_t token = R8_USB_INT_ST & MASK_UIS_TOKEN;
	switch(token) {
	case UIS_TOKEN_OUT:
		if (on_write)
			on_write(data_ep_out, R8_USB_RX_LEN);
		tog = !tog;
		set_handshake(DATA_EP_NUM, USB_ACK, tog, 0);
		break;

	case UIS_TOKEN_IN:
		if (transferred == 0) {
			transferred = 1;
		} else {
			set_handshake(DATA_EP_NUM, USB_NAK, 1, 0);
		}
		break;
	
	default:
		break;
	}
}

// In case we want to send something to the host,
// or want to see the log over ttyACMx
void cdc_fill_IN(uint8_t *buf, uint8_t len)
{
	if (len > tx_ep_desc.wMaxPacketSize)
		return;

	static int tog;

	memcpy(data_ep_in, buf, len);
	set_handshake(DATA_EP_NUM, USB_ACK, tog, len);

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

int cdc_tx_poll(uint8_t *buf, int len, uint16_t timeout_ms)
{
	int i = 0;
	while (len > tx_ep_desc.wMaxPacketSize) {
		cdc_fill_IN(buf + i, tx_ep_desc.wMaxPacketSize);
		if (wait_until_sent(timeout_ms))
			return -1;

		i += tx_ep_desc.wMaxPacketSize;
		len -= tx_ep_desc.wMaxPacketSize;
	}
	cdc_fill_IN(buf + i, len);
	if (wait_until_sent(timeout_ms))
		return -1;

	return 0;
}

void cdc_onWrite(void (*cb)(uint8_t *buf, uint16_t len))
{
	on_write = cb;
}

void cdc_acm_init()
{
	cfg_desc_append(&acm_if_desc);
	cfg_desc_append(header_func_desc);
	cfg_desc_append(acm_func_desc);
	cfg_desc_append(callmgr_func_desc);

	cfg_desc_append(&noti_ep_desc);
	cfg_desc_append(&rx_ep_desc);
	cfg_desc_append(&tx_ep_desc);

	if_cb_register(ACM_IF_NUM, cdc_request_handler);
	ep_cb_register(NOTI_EP_NUM, noti_ep_handler);
	ep_cb_register(DATA_EP_NUM, data_ep_handler);

	dma_register(NOTI_EP_NUM, noti_ep_buf);
	dma_register(DATA_EP_NUM, data_ep_buf);
}