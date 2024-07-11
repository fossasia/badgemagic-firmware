#ifndef __UTILS_H__
#define __UTILS_H__

#include "CH58x_common.h"

enum USB_RESPONSE_TYPE {
	USB_ACK = 0,
	USB_NO_RESP,
	USB_NAK,
	USB_STALL,
};

void cfg_desc_append(void *desc);
int ep_cb_register(int ep_num, void (*cb)());
int if_cb_register(uint8_t if_num, void (*cb)(USB_SETUP_REQ *request));
void dma_register(uint8_t endpoint_number, void *buf_addr);

uint16_t
usb_start_load_block(void *ep_IN_buf, void *buf, uint16_t len, int tog);
void ctrl_start_load_block(void *buf, uint16_t len);
int usb_load_next_chunk();
void usb_flush();

void set_handshake(uint8_t ep_num, int type, int tog, uint8_t len);
void ctrl_ack();

void clear_handshake_sent_flag();
int handshake_sent();

#endif /* __UTILS_H__ */
