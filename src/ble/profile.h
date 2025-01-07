#ifndef __BLE_UART_SERVICE_H__
#define __BLE_UART_SERVICE_H__

int legacy_registerService();
int devInfo_registerService();
int batt_registerService();
int ng_registerService();

bStatus_t ng_notify(uint8_t *pValue, uint8_t len);

#endif /* __BLE_UART_SERVICE_H__ */
