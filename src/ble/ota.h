/********************************** (C) COPYRIGHT *******************************
 * File Name          : ota.h
 * Author             : WCH
 * Version            : V1.10
 * Date               : 2018/12/14
 * Description        : OTA parameter configuration definitions
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/******************************************************************************/
#ifndef __OTA_H
#define __OTA_H

/* ------------------------------------------------------------------------------------------------
 *                                             OTA FLASH
 * ------------------------------------------------------------------------------------------------
 */

/* User code is split into large blocks: 4K, 216K, 216K, 12K, representing
 * imageA (APP), imageB (OTA staging), and imageIAP respectively. */

/* FLASH configuration */
#define FLASH_BLOCK_SIZE       EEPROM_BLOCK_SIZE
#define IMAGE_SIZE             216 * 1024

/* imageA configuration */
#define IMAGE_A_FLAG           0x01
#define IMAGE_A_START_ADD      4 * 1024
#define IMAGE_A_SIZE           IMAGE_SIZE

/* imageB configuration */
#define IMAGE_B_FLAG           0x02
#define IMAGE_B_START_ADD      (IMAGE_A_START_ADD + IMAGE_SIZE)
#define IMAGE_B_SIZE           IMAGE_SIZE

/* imageIAP configuration */
#define IMAGE_IAP_FLAG         0x03
#define IMAGE_IAP_START_ADD    (IMAGE_B_START_ADD + IMAGE_SIZE)
#define IMAGE_IAP_SIZE         12 * 1024

/* ------------------------------------------------------------------------------------------------
 *                                    DIRECT-XIP PING-PONG SUPPORT
 *  Modeled after MCUboot's "Equal slots (direct-xip)" scheme:
 *  https://docs.mcuboot.com/design.html#direct-xip
 *  Each slot holds fully independent, directly-bootable firmware (built with
 *  its own linker origin). The active slot is tracked via a flag in dataflash,
 *  not by copying images between slots.
 * ------------------------------------------------------------------------------------------------
 */

/* Compile-time: which slot is THIS firmware linked for (set via Makefile -DTHIS_SLOT_A/-DTHIS_SLOT_B) */
#if defined(THIS_SLOT_A)
    #define THIS_IMAGE_FLAG        IMAGE_A_FLAG
    #define OTA_TARGET_START_ADD   IMAGE_B_START_ADD
    #define OTA_TARGET_FLAG        IMAGE_B_FLAG
#elif defined(THIS_SLOT_B)
    #define THIS_IMAGE_FLAG        IMAGE_B_FLAG
    #define OTA_TARGET_START_ADD   IMAGE_A_START_ADD
    #define OTA_TARGET_FLAG        IMAGE_A_FLAG
#else
    #error "THIS_SLOT_A or THIS_SLOT_B must be defined by the build (see Makefile SLOT variable)"
#endif

/* Simplified revert/confirm mechanism, modeled after MCUboot's "Revert mechanism
 * in direct-xip and ram-load mode" (docs.mcuboot.com/design.html#direct-xip-ram-load-revert).
 * Reuses the existing OTADataFlashInfo_t reserved bytes, so no new dataflash
 * region is needed.
 *
 * BootPending : set by IAP right before jumping into a newly-flagged slot.
 *               Cleared by APP once it has confirmed a healthy boot.
 * BootOK      : written by APP after self-test passes; if BootPending is still
 *               set on the *next* boot (i.e. BootOK was never reached), IAP
 *               reverts the flag to the previously active slot instead of
 *               retrying the possibly-bad image.
 */
#define BOOT_MARK_UNCONFIRMED  0xFF   /* matches MCUboot's "unset" TLV convention */
#define BOOT_MARK_CONFIRMED    0x01   /* matches MCUboot's image_ok = 0x01 */

/* Number of milliseconds APP has to call ota_confirm_boot() before being
 * considered "failed to boot" on the next reset. Tune to your startup path. */
#define OTA_BOOT_CONFIRM_TIMEOUT_MS   5000

/* IAP commands */
/* The following are IAP download command definitions */
#define CMD_IAP_PROM           0x80               // IAP program command
#define CMD_IAP_ERASE          0x81               // IAP erase command
#define CMD_IAP_VERIFY         0x82               // IAP verify command
#define CMD_IAP_END            0x83               // IAP end flag
#define CMD_IAP_INFO           0x84               // IAP get device info

/* Data frame length definition */
#define IAP_LEN                247

/* Local DataFlash address; must not overlap the BLE stack's storage region */
#define OTA_DATAFLASH_ADD      0x00077000 - FLASH_ROM_MAX_SIZE

/* OTA info stored in local DataFlash */
typedef struct
{
    unsigned char ImageFlag;     /* Records the currently active image flag */
    unsigned char BootPending;   /* 0xFF = confirmed/none pending, else = awaiting self-test */
    unsigned char BootOK;        /* 0x01 = last boot into ImageFlag's slot confirmed good */
    unsigned char _reserved;
} OTADataFlashInfo_t;

/* OTA IAP communication protocol definition */
/* Address uses a 4x offset */
typedef union
{
    struct
    {
        unsigned char cmd;          /* Command code 0x81 */
        unsigned char len;          /* Data length */
        unsigned char addr[2];      /* Erase address */
        unsigned char block_num[2]; /* Erase count */

    } erase; /* Erase command */
    struct
    {
        unsigned char cmd;       /* Command code 0x83 */
        unsigned char len;       /* Data length */
        unsigned char status[2]; /* Reserved status return code bytes */
    } end;                       /* End command */
    struct
    {
        unsigned char cmd;              /* Command code 0x82 */
        unsigned char len;              /* Data length */
        unsigned char addr[2];          /* Verify address */
        unsigned char buf[IAP_LEN - 4]; /* Verify data */
    } verify;                           /* Verify command */
    struct
    {
        unsigned char cmd;              /* Command code 0x80 */
        unsigned char len;              /* Data length */
        unsigned char addr[2];          /* Address */
        unsigned char buf[IAP_LEN - 4]; /* Program data */
    } program;                          /* Program command */
    struct
    {
        unsigned char cmd;              /* Command code 0x84 */
        unsigned char len;              /* Data length */
        unsigned char buf[IAP_LEN - 2]; /* Reserved data */
    } info;                             /* Info command */
    struct
    {
        unsigned char buf[IAP_LEN]; /* Reserved data packet */
    } other;
} OTA_IAP_CMD_t;

/* Records the currently active image */
extern unsigned char CurrImageFlag;

#endif