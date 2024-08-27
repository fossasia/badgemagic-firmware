#ifndef __BLE_SETUP_H__
#define __BLE_SETUP_H__

void tmos_clockInit(void);
void ble_hardwareInit(void);
void peripheral_init(void);

void ble_enable_advertise();
void ble_disable_advertise();

#endif /* __BLE_SETUP_H__ */
