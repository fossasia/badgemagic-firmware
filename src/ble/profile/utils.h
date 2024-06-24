#ifndef __UTILS_H__
#define __UTILS_H__

#include "CH58xBLE_LIB.h"

#define ATTR_DECLAR(uuid, uuid_size, permissions, val) \
				{ \
					{(uuid_size), (uint8_t *)(uuid)}, \
					(permissions), \
					0, \
					(uint8_t *)(val) \
				}

// Characteristic Declaration
#define CHAR_DECLAR(properties) \
	ATTR_DECLAR(characterUUID, 2, GATT_PERMIT_READ, properties)

// Characteristic Value Declaration
#define CHAR_VAL_DECLAR(uuid, uuid_size, permissions, val) \
	ATTR_DECLAR(uuid, uuid_size, permissions, val)

#endif /* __UTILS_H__ */
