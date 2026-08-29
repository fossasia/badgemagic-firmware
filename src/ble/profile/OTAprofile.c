/********************************** (C) COPYRIGHT *******************************
 * File Name          : OTAprofile.C
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2018/12/10
 * Description        : OTA firmware update communication interface
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "CH58xBLE_LIB.h"
#include "OTAprofile.h"
#include "../ota.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
// Simple GATT Profile Service UUID: 0xFEE0
const uint8_t OTAProfileServUUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(OTAPROFILE_SERV_UUID), HI_UINT16(OTAPROFILE_SERV_UUID)};

// Characteristic 1 UUID: 0xFEE1 (OTA command channel)
const uint8_t OTAProfilechar1UUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(OTAPROFILE_CHAR_UUID), HI_UINT16(OTAPROFILE_CHAR_UUID)};

// Characteristic 2 UUID: 0xFEE2 (slot info, read-only)
const uint8_t OTAProfileChar2UUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(OTAPROFILE_SLOT_CHAR_UUID), HI_UINT16(OTAPROFILE_SLOT_CHAR_UUID)};

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

static OTAProfileCBs_t *OTAProfile_AppCBs = NULL;

/*********************************************************************
 * Profile Attributes - variables
 */

// Simple Profile Service attribute
static const gattAttrType_t OTAProfileService = {ATT_BT_UUID_SIZE, OTAProfileServUUID};

// Characteristic 1 Properties (OTA command channel)
static uint8_t OTAProfileCharProps = GATT_PROP_READ | GATT_PROP_WRITE | GATT_PROP_WRITE_NO_RSP;

// Characteristic 1 Value
static uint8_t OTAProfileChar = 0;

// Characteristic 1 User Description
static uint8_t OTAProfileCharUserDesp[12] = "OTA Channel";

// Characteristic 2 Properties (slot info, read-only — no write, no staging needed)
static uint8_t OTAProfileSlotCharProps = GATT_PROP_READ;

// Characteristic 2 Value — placeholder; the read callback returns THIS_IMAGE_FLAG
// directly rather than reading this byte, so its stored content is unused.
static uint8_t OTAProfileSlotChar = 0;

// Characteristic 2 User Description
static uint8_t OTAProfileSlotCharUserDesp[10] = "Slot Info";

// write and read buffer (OTA command channel only)
static uint8_t OTAProfileReadLen;
static uint8_t OTAProfileReadBuf[IAP_LEN];
static uint8_t OTAProfileWriteLen;
static uint8_t OTAProfileWriteBuf[IAP_LEN];

/*********************************************************************
 * Profile Attributes - Table
 */

static gattAttribute_t OTAProfileAttrTbl[7] = {
    // Simple Profile Service
    {
        {ATT_BT_UUID_SIZE, primaryServiceUUID}, /* type */
        GATT_PERMIT_READ,                       /* permissions */
        0,                                      /* handle */
        (uint8_t *)&OTAProfileService           /* pValue */
    },

    // Characteristic 1 Declaration (OTA command channel)
    {
        {ATT_BT_UUID_SIZE, characterUUID},
        GATT_PERMIT_READ,
        0,
        &OTAProfileCharProps},

    // Characteristic 1 Value
    {
        {ATT_BT_UUID_SIZE, OTAProfilechar1UUID},
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        &OTAProfileChar},

    // Characteristic 1 User Description
    {
        {ATT_BT_UUID_SIZE, charUserDescUUID},
        GATT_PERMIT_READ,
        0,
        OTAProfileCharUserDesp},

    // Characteristic 2 Declaration (slot info, read-only)
    {
        {ATT_BT_UUID_SIZE, characterUUID},
        GATT_PERMIT_READ,
        0,
        &OTAProfileSlotCharProps},

    // Characteristic 2 Value (slot info)
    {
        {ATT_BT_UUID_SIZE, OTAProfileChar2UUID},
        GATT_PERMIT_READ,
        0,
        &OTAProfileSlotChar},

    // Characteristic 2 User Description
    {
        {ATT_BT_UUID_SIZE, charUserDescUUID},
        GATT_PERMIT_READ,
        0,
        OTAProfileSlotCharUserDesp},
};

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static bStatus_t OTAProfile_ReadAttrCB(uint16_t connHandle, gattAttribute_t *pAttr,
                                       uint8_t *pValue, uint16_t *pLen, uint16_t offset, uint16_t maxLen, uint8_t method);
static bStatus_t OTAProfile_WriteAttrCB(uint16_t connHandle, gattAttribute_t *pAttr,
                                        uint8_t *pValue, uint16_t len, uint16_t offset, uint8_t method);

/*********************************************************************
 * PROFILE CALLBACKS
 */
// OTA Profile Service Callbacks
gattServiceCBs_t OTAProfileCBs = {
    OTAProfile_ReadAttrCB,  // Read callback function pointer
    OTAProfile_WriteAttrCB, // Write callback function pointer
    NULL                    // Authorization callback function pointer
};

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      OTAProfile_AddService
 *
 * @brief   Initialize the OTA Profile
 *
 * @param   services    - services to add
 *
 * @return  init status
 */
bStatus_t OTAProfile_AddService(uint32_t services)
{
    uint8_t status = SUCCESS;

    if(services & OTAPROFILE_SERVICE)
    {
        // Register GATT attribute list and CBs with GATT Server App
        status = GATTServApp_RegisterService(OTAProfileAttrTbl,
                                             GATT_NUM_ATTRS(OTAProfileAttrTbl),
                                             GATT_MAX_ENCRYPT_KEY_SIZE,
                                             &OTAProfileCBs);
    }

    return (status);
}

/*********************************************************************
 * @fn      OTAProfile_RegisterAppCBs
 *
 * @brief   Register OTA Profile read/write callbacks
 *
 * @param   appCallbacks    - pointer to callback struct
 *
 * @return  execution status
 */
bStatus_t OTAProfile_RegisterAppCBs(OTAProfileCBs_t *appCallbacks)
{
    if(appCallbacks)
    {
        OTAProfile_AppCBs = appCallbacks;

        return (SUCCESS);
    }
    else
    {
        return (bleAlreadyInRequestedMode);
    }
}

/*********************************************************************
 * @fn          OTAProfile_ReadAttrCB
 *
 * @brief       Read an attribute.
 *
 * @param       connHandle - connection message was received on
 * @param       pAttr - pointer to attribute
 * @param       pValue - pointer to data to be read
 * @param       pLen - length of data to be read
 * @param       offset - offset of the first octet to be read
 * @param       maxLen - maximum length of data to be read
 *
 * @return      Success or Failure
 */
static bStatus_t OTAProfile_ReadAttrCB(uint16_t connHandle, gattAttribute_t *pAttr,
                                       uint8_t *pValue, uint16_t *pLen, uint16_t offset, uint16_t maxLen, uint8_t method)
{
    bStatus_t status = SUCCESS;

    if(pAttr->type.len == ATT_BT_UUID_SIZE)
    {
        // 16-bit UUID
        uint16_t uuid = BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1]);

        switch(uuid)
        {
            case OTAPROFILE_CHAR_UUID:
            {
                *pLen = 0;
                if(OTAProfileReadLen)
                {
                    *pLen = OTAProfileReadLen;
                    tmos_memcpy(pValue, OTAProfileReadBuf, OTAProfileReadLen);
                    OTAProfileReadLen = 0;
                    if(OTAProfile_AppCBs && OTAProfile_AppCBs->pfnOTAProfileRead)
                    {
                        OTAProfile_AppCBs->pfnOTAProfileRead(OTAPROFILE_CHAR);
                    }
                }
                break;
            }
            case OTAPROFILE_SLOT_CHAR_UUID:
            {
                // Always-current, no write/staging required — directly reflects
                // which slot this running firmware was linked for.
                pValue[0] = THIS_IMAGE_FLAG;
                *pLen = 1;
                break;
            }
            default:
            {
                // Should never get here! (declaration/description attrs are read-only structural entries)
                *pLen = 0;
                status = ATT_ERR_ATTR_NOT_FOUND;
                break;
            }
        }
    }
    else
    {
        // 128-bit UUID
        *pLen = 0;
        status = ATT_ERR_INVALID_HANDLE;
    }

    return (status);
}

/*********************************************************************
 * @fn      OTAProfile_WriteAttrCB
 *
 * @brief   Validate attribute data prior to a write operation
 *
 * @param   connHandle - connection message was received on
 * @param   pAttr - pointer to attribute
 * @param   pValue - pointer to data to be written
 * @param   len - length of data
 * @param   offset - offset of the first octet to be written
 *
 * @return  Success or Failure
 */
static bStatus_t OTAProfile_WriteAttrCB(uint16_t connHandle, gattAttribute_t *pAttr,
                                        uint8_t *pValue, uint16_t len, uint16_t offset, uint8_t method)
{
    bStatus_t status = SUCCESS;

    if(pAttr->type.len == ATT_BT_UUID_SIZE)
    {
        // 16-bit UUID
        uint16_t uuid = BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1]);

        switch(uuid)
        {
            case OTAPROFILE_CHAR_UUID:
            {
                //Write the value
                if(status == SUCCESS)
                {
                    uint16_t i;
                    uint8_t *p_rec_buf;

                    OTAProfileWriteLen = len;
                    p_rec_buf = pValue;
                    for(i = 0; i < OTAProfileWriteLen; i++)
                        OTAProfileWriteBuf[i] = p_rec_buf[i];
                }
                break;
            }

            default:
                // Should never get here! (slot-info characteristic and structural
                // attrs do not have write permissions)
                status = ATT_ERR_ATTR_NOT_FOUND;
                break;
        }
    }
    else
    {
        // 128-bit UUID
        status = ATT_ERR_INVALID_HANDLE;
    }

    if(OTAProfileWriteLen && OTAProfile_AppCBs && OTAProfile_AppCBs->pfnOTAProfileWrite)
    {
        OTAProfile_AppCBs->pfnOTAProfileWrite(OTAPROFILE_CHAR, OTAProfileWriteBuf, OTAProfileWriteLen);
        OTAProfileWriteLen = 0;
    }

    return (status);
}

/*********************************************************************
 * @fn      OTAProfile_SendData
 *
 * @brief   Stage data to be returned on the next read of the OTA command channel
 *
 * @param   paramID     - OTA channel selector
 * @param   p_data      - data pointer
 * @param   send_len    - data length
 *
 * @return  execution status
 */
bStatus_t OTAProfile_SendData(unsigned char paramID, unsigned char *p_data, unsigned char send_len)
{
    bStatus_t status = SUCCESS;

    /* Data length out of range */
    if(send_len > 20)
        return 0xfe;

    OTAProfileReadLen = send_len;
    tmos_memcpy(OTAProfileReadBuf, p_data, OTAProfileReadLen);

    return status;
}

/*********************************************************************
*********************************************************************/