#ifndef __ERROR_H__
#define __ERROR_H__

typedef enum {
    BM_OK               = 0,
    BM_ERR_TOO_SHORT    = 1,
    BM_ERR_INVALID_CMD  = 2,
    BM_ERR_NOT_IMPL     = 3,
    BM_ERR_NO_MEM       = 4,
    BM_ERR_OUT_OF_RANGE = 5,
    BM_ERR_INVALID_SIZE = 6,
    BM_ERR_BAD_HEADER   = 7,
    BM_ERR_FLASH        = 8,
} bm_err_t;

typedef enum {
    BM_USB_OK           =  0,
    BM_USB_ERR_TIMEOUT  = -1,
    BM_USB_ERR_NO_MEM   = -2,
} bm_usb_err_t;

#endif 