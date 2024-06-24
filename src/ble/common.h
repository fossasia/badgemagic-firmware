/**
 * This file contain high-level data structures for differents BLE chip.
 * 
 */

#ifndef __BLE_H__
#define __BLE_H__

#include <stdint.h>
#include <stddef.h>

typedef struct {
	uint8_t *bytes;
	int size;
} byte_t;

typedef struct {
	byte_t          val; // Values
	uint8_t         props; // Properties //TODO: add enum
	byte_t          uuid;
} ble_char_t;

#endif /* __BLE_H__ */
