/********************************** (C) COPYRIGHT *******************************
 * File Name          : OTAprofile.C
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2018/12/10
 * Description        : OTA升级蓝牙通讯接口
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "CONFIG.h"
#include "OTAprofile.h"
#include "CH583SFR.h"
#include "ota.h"

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
// Simple GATT Profile Service UUID: 0xFFF0
const uint8_t OTAProfileServUUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(OTAPROFILE_SERV_UUID), HI_UINT16(OTAPROFILE_SERV_UUID)};

// Characteristic 1 UUID: 0xFFF1
const uint8_t OTAProfilechar1UUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(OTAPROFILE_CHAR_UUID), HI_UINT16(OTAPROFILE_CHAR_UUID)};

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

// Simple Profile Characteristic 1 Properties
static uint8_t OTAProfileCharProps = GATT_PROP_READ | GATT_PROP_WRITE | GATT_PROP_WRITE_NO_RSP;

// Characteristic 1 Value
static uint8_t OTAProfileChar = 0;

// Simple Profile Characteristic 1 User Description
static uint8_t OTAProfileCharUserDesp[12] = "OTA Channel";

// write and read buffer
static uint8_t OTAProfileReadLen;
static uint8_t OTAProfileReadBuf[IAP_LEN];
static uint8_t OTAProfileWriteLen;
static uint8_t OTAProfileWriteBuf[IAP_LEN];

/*********************************************************************
 * Profile Attributes - Table
 */

static gattAttribute_t OTAProfileAttrTbl[4] = {
    // Simple Profile Service
    {
        {ATT_BT_UUID_SIZE, primaryServiceUUID}, /* type */
        GATT_PERMIT_READ,                       /* permissions */
        0,                                      /* handle */
        (uint8_t *)&OTAProfileService           /* pValue */
    },

    // Characteristic Declaration
    {
        {ATT_BT_UUID_SIZE, characterUUID},
        GATT_PERMIT_READ,
        0,
        &OTAProfileCharProps},

    // Characteristic Value
    {
        {ATT_BT_UUID_SIZE, OTAProfilechar1UUID},
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        &OTAProfileChar},

    // Characteristic User Description
    {
        {ATT_BT_UUID_SIZE, charUserDescUUID},
        GATT_PERMIT_READ,
        0,
        OTAProfileCharUserDesp},
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
 * @brief   OTA Profile初始化
 *
 * @param   services    - 服务控制字
 *
 * @return  初始化的状态
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
 * @brief   OTA Profile读写回调函数注册
 *
 * @param   appCallbacks    - 函数结构体指针
 *
 * @return  函数执行状态
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
            default:
            {
                // Should never get here! (characteristics 3 and 4 do not have read permissions)
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
    //uint8_t notifyApp = 0xFF;

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
                // Should never get here! (characteristics 2 and 4 do not have write permissions)
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
 * @brief   OTA Profile通道发送数据
 *
 * @param   paramID     - OTA通道选择
 * @param   p_data      - 数据指针
 * @param   send_len    - 发送数据长度
 *
 * @return  函数执行状态
 */
bStatus_t OTAProfile_SendData(unsigned char paramID, unsigned char *p_data, unsigned char send_len)
{
    bStatus_t status = SUCCESS;

    /* 数据长度超出范围 */
    if(send_len > 20)
        return 0xfe;

    OTAProfileReadLen = send_len;
    tmos_memcpy(OTAProfileReadBuf, p_data, OTAProfileReadLen);

    return status;
}

/*********************************************************************
*********************************************************************/
