/********************************** (C) COPYRIGHT *******************************
 * File Name          : OTAprofile.h
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2018/12/11
 * Description        :
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#ifndef OTAPROFILE_H
#define OTAPROFILE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * INCLUDES
 */

/*********************************************************************
 * CONSTANTS
 */

// OTA Profile channel index definitions
#define OTAPROFILE_CHAR         0
#define OTAPROFILE_SLOT_CHAR    1

// OTA service UUID
#define OTAPROFILE_SERV_UUID    0xFEE0

// OTA communication channel UUID (write commands / staged read responses)
#define OTAPROFILE_CHAR_UUID       0xFEE1

// OTA slot-info channel UUID (read-only, always returns current active slot flag)
#define OTAPROFILE_SLOT_CHAR_UUID  0xFEE2

// Simple Keys Profile Services bit fields
#define OTAPROFILE_SERVICE      0x00000001

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * Profile Callbacks
 */

typedef void (*OTAProfileRead_t)(unsigned char paramID);
typedef void (*OTAProfileWrite_t)(unsigned char paramID, unsigned char *p_data, unsigned char w_len);

typedef struct
{
    OTAProfileRead_t  pfnOTAProfileRead;
    OTAProfileWrite_t pfnOTAProfileWrite;
} OTAProfileCBs_t;

/*********************************************************************
 * API FUNCTIONS
 */

bStatus_t OTAProfile_AddService(uint32_t services);

bStatus_t OTAProfile_RegisterAppCBs(OTAProfileCBs_t *appCallbacks);

bStatus_t OTAProfile_SendData(unsigned char paramID, unsigned char *p_data, unsigned char send_len);

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif