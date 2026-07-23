#ifndef __LEGACYCTRL_H__
#define __LEGACYCTRL_H__

#include <stdint.h>

void handle_after_rx();
int legacy_ble_rx(uint8_t *val, uint16_t len);
int legacy_usb_rx(uint8_t *buf, uint16_t len);

void legacy_set_auth_code(uint16_t code);  // called from main.c on BT-PAIRING entry
void legacy_reset_auth();                   // called from peripheral.c on disconnect
void legacy_bypass_auth();

#endif /* __LEGACYCTRL_H__ */