#ifndef __USB_H__
#define __USB_H__

#include <stdint.h>

void cdc_fill_IN(uint8_t *buf, uint8_t len);
int cdc_tx_poll(uint8_t *buf, int len, uint16_t timeout_ms);
void cdc_onWrite(void (*cb)(uint8_t *buf, uint16_t len));

void hiddev_fill_IN(uint8_t *buf, uint8_t len);
int hiddev_tx_poll(uint8_t *buf, int len, uint16_t timeout_ms);
void hiddev_onWrite(void (*cb)(uint8_t *buf, uint16_t len));

void usb_start();

#endif /* __USB_H__ */
