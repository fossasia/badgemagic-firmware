#include "data.h"
#include "power.h"
#include "leddrv.h"
#include "debug.h"
#include "legacyctrl.h"
#include <stdlib.h>
#include <string.h>

typedef enum {
    LEGACY_BLE_RX_OK = 0,
    LEGACY_BLE_RX_ERR_WIDTH,
    LEGACY_BLE_RX_ERR_HEADER,
    LEGACY_BLE_RX_ERR_MEM,
    LEGACY_BLE_RX_ERR_COPY,
    LEGACY_BLE_RX_ERR_UNKNOWN
    // Add more error codes as needed
} legacy_ble_rx_error_t;

static volatile legacy_ble_rx_error_t legacy_ble_rx_error = LEGACY_BLE_RX_OK;

// Retrieve and clear the last error state
legacy_ble_rx_error_t legacy_ble_rx_get_last_error(void) {
    legacy_ble_rx_error_t err = legacy_ble_rx_error;
    legacy_ble_rx_error = LEGACY_BLE_RX_OK;
    return err;
}

void legacy_ble_rx(uint8_t *val, uint16_t len)
{
    _TRACE();
    static uint16_t c = 0, data_len = 0, n = 0;
    static uint8_t *data = NULL;

    // Always set error to OK at function start
    legacy_ble_rx_error = LEGACY_BLE_RX_OK;

    if (len != LEGACY_TRANSFER_WIDTH) {
        PRINT("Transfer width is not matched\n");
        legacy_ble_rx_error = LEGACY_BLE_RX_ERR_WIDTH;
        return;
    }

    PRINT("val[%d]: ", len);
    for (int i = 0; i < len; i++) {
        PRINT("%02X ", val[i]);
    }
    PRINT("\n");

    if (c == 0) {
        if (memcmp(val, "wang", 5)) {
            PRINT("Not a header\n");
            legacy_ble_rx_error = LEGACY_BLE_RX_ERR_HEADER;
            return;
        } else {
            free(data);
            data = malloc(sizeof(data_legacy_t));
            if (!data) {
                PRINT("insufficient memory\n");
                legacy_ble_rx_error = LEGACY_BLE_RX_ERR_MEM;
                return;
            }
        }
    } else { // Re attempt after a failed transfer
        if (!memcmp(val, "wang", 5)) {
            free(data);
            c = 0;
            data = malloc(sizeof(data_legacy_t));
            if (!data) {
                PRINT("insufficient memory\n");
                legacy_ble_rx_error = LEGACY_BLE_RX_ERR_MEM;
                return;
            }
        }
    }

    PRINT("Copying BLE value\n");
    // Defensive: check pointer before memcpy
    if (data == NULL) {
        legacy_ble_rx_error = LEGACY_BLE_RX_ERR_MEM;
        return;
    }
    memcpy(data + c * len, val, len);

    if (c == 1) {
        data_legacy_t *d = (data_legacy_t *)data;
        n = bigendian16_sum(d->sizes, 8);
        data_len = LEGACY_HEADER_SIZE + LED_ROWS * n;
        data = realloc(data, data_len);
        if (!data) {
            PRINT("insufficient memory\n");
            legacy_ble_rx_error = LEGACY_BLE_RX_ERR_MEM;
            return;
        }
    }

    if (c > 2 && ((c + 1) * LEGACY_TRANSFER_WIDTH) >= data_len) {
        PRINT("All bitmaps data received successfully\nWriting to flash.. ");
        if (data == NULL) {
            legacy_ble_rx_error = LEGACY_BLE_RX_ERR_MEM;
            return;
        }
        data_flatSave(data, data_len);
        free(data);
        data = NULL;
        handle_after_rx();
        PRINT("Done\n");
        c = 0;
        data_len = 0;
        n = 0;
        legacy_ble_rx_error = LEGACY_BLE_RX_OK;
        return;
    }

    c++;
    legacy_ble_rx_error = LEGACY_BLE_RX_OK;
}

// Example of a caller checking for error after calling legacy_ble_rx
void example_caller(uint8_t *val, uint16_t len) {
    legacy_ble_rx(val, len);
    legacy_ble_rx_error_t err = legacy_ble_rx_get_last_error();
    if (err != LEGACY_BLE_RX_OK) {
        PRINT("legacy_ble_rx failed with error code: %d\n", err);
        // Handle error accordingly
    }
}

// For thread safety in a multithreaded context, 
// use mutexes or atomic operations around legacy_ble_rx_error access.