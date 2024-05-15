/********************************** (C) COPYRIGHT *******************************
 * File Name          : CH58x_adc.c
 * Author             : WCH
 * Version            : V1.2
 * Date               : 2021/11/17
 * Description
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#include "CH58x_common.h"

/*********************************************************************
 * @fn      ADC_DataCalib_Rough
 *
 * @brief   采样数据粗调,获取偏差值,必须先配置ADC后调用此函数获取校准值
 *
 * @param   none
 *
 * @return  偏差
 */
signed short ADC_DataCalib_Rough(void) // 采样数据粗调,获取偏差值
{
    uint16_t i;
    uint32_t sum = 0;
    uint8_t  ch = 0;   // 备份通道

    ch = R8_ADC_CHANNEL;

    ADC_ChannelCfg(1);                                          // ADC校准通道请选择通道1
    R8_ADC_CFG |= RB_ADC_OFS_TEST; // 进入测试模式
    R8_ADC_CONVERT = RB_ADC_START;
    while(R8_ADC_CONVERT & RB_ADC_START);
    for(i = 0; i < 16; i++)
    {
        R8_ADC_CONVERT = RB_ADC_START;
        while(R8_ADC_CONVERT & RB_ADC_START);
        sum += (~R16_ADC_DATA) & RB_ADC_DATA;
    }
    sum = (sum + 8) >> 4;
    R8_ADC_CFG &= ~RB_ADC_OFS_TEST; // 关闭测试模式

    R8_ADC_CHANNEL = ch;

    return (2048 - sum);
}

/*********************************************************************
 * @fn      ADC_ExtSingleChSampInit
 *
 * @brief   外部信号单通道采样初始化
 *
 * @param   sp  - refer to ADC_SampClkTypeDef
 * @param   ga  - refer to ADC_SignalPGATypeDef
 *
 * @return  none
 */
void ADC_ExtSingleChSampInit(ADC_SampClkTypeDef sp, ADC_SignalPGATypeDef ga)
{
    R8_TKEY_CFG &= ~RB_TKEY_PWR_ON;
    R8_ADC_CFG = RB_ADC_POWER_ON | RB_ADC_BUF_EN | (sp << 6) | (ga << 4);
}

/*********************************************************************
 * @fn      ADC_ExtDiffChSampInit
 *
 * @brief   外部信号差分通道采样初始化
 *
 * @param   sp  - refer to ADC_SampClkTypeDef
 * @param   ga  - refer to ADC_SignalPGATypeDef
 *
 * @return  none
 */
void ADC_ExtDiffChSampInit(ADC_SampClkTypeDef sp, ADC_SignalPGATypeDef ga)
{
    R8_TKEY_CFG &= ~RB_TKEY_PWR_ON;
    R8_ADC_CFG = RB_ADC_POWER_ON | RB_ADC_DIFF_EN | (sp << 6) | (ga << 4);
}

/*********************************************************************
 * @fn      ADC_InterTSSampInit
 *
 * @brief   内置温度传感器采样初始化
 *
 * @param   none
 *
 * @return  none
 */
void ADC_InterTSSampInit(void)
{
    R8_TKEY_CFG &= ~RB_TKEY_PWR_ON;
    R8_TEM_SENSOR = RB_TEM_SEN_PWR_ON;
    R8_ADC_CHANNEL = CH_INTE_VTEMP;
    R8_ADC_CFG = RB_ADC_POWER_ON | RB_ADC_DIFF_EN | (3 << 4);
}

/*********************************************************************
 * @fn      ADC_InterBATSampInit
 *
 * @brief   内置电池电压采样初始化
 *
 * @param   none
 *
 * @return  none
 */
void ADC_InterBATSampInit(void)
{
    R8_TKEY_CFG &= ~RB_TKEY_PWR_ON;
    R8_ADC_CHANNEL = CH_INTE_VBAT;
    R8_ADC_CFG = RB_ADC_POWER_ON | RB_ADC_BUF_EN | (0 << 4); // 使用-12dB模式，
}

/*********************************************************************
 * @fn      TouchKey_ChSampInit
 *
 * @brief   触摸按键通道采样初始化
 *
 * @param   none
 *
 * @return  none
 */
void TouchKey_ChSampInit(void)
{
    R8_ADC_CFG = RB_ADC_POWER_ON | RB_ADC_BUF_EN | (2 << 4);
    R8_TKEY_CFG |= RB_TKEY_PWR_ON;
}

/*********************************************************************
 * @fn      ADC_ExcutSingleConver
 *
 * @brief   ADC执行单次转换
 *
 * @param   none
 *
 * @return  ADC转换后的数据
 */
uint16_t ADC_ExcutSingleConver(void)
{
    R8_ADC_CONVERT = RB_ADC_START;
    while(R8_ADC_CONVERT & RB_ADC_START);

    return (R16_ADC_DATA & RB_ADC_DATA);
}

/*********************************************************************
 * @fn      TouchKey_ExcutSingleConver
 *
 * @brief   TouchKey转换后数据
 *
 * @param   charg   - Touchkey充电时间,5bits有效, t=charg*Tadc
 * @param   disch   - Touchkey放电时间,3bits有效, t=disch*Tadc
 *
 * @return  当前TouchKey等效数据
 */
uint16_t TouchKey_ExcutSingleConver(uint8_t charg, uint8_t disch)
{
    R8_TKEY_COUNT = (disch << 5) | (charg & 0x1f);
    R8_TKEY_CONVERT = RB_TKEY_START;
    while(R8_TKEY_CONVERT & RB_TKEY_START);
    return (R16_ADC_DATA & RB_ADC_DATA);
}

/*********************************************************************
 * @fn      ADC_AutoConverCycle
 *
 * @brief   设置连续 ADC的周期
 *
 * @param   cycle   - 采样周期计算方法为(256-cycle)*16*Tsys
 *
 * @return  none
 */
void ADC_AutoConverCycle(uint8_t cycle)
{
    R8_ADC_AUTO_CYCLE = cycle;
}

/*********************************************************************
 * @fn      ADC_DMACfg
 *
 * @brief   配置DMA功能
 *
 * @param   s           - 是否打开DMA功能
 * @param   startAddr   - DMA 起始地址
 * @param   endAddr     - DMA 结束地址
 * @param   m           - 配置DMA模式
 *
 * @return  none
 */
void ADC_DMACfg(uint8_t s, uint16_t startAddr, uint16_t endAddr, ADC_DMAModeTypeDef m)
{
    if(s == DISABLE)
    {
        R8_ADC_CTRL_DMA &= ~(RB_ADC_DMA_ENABLE | RB_ADC_IE_DMA_END);
    }
    else
    {
        R16_ADC_DMA_BEG = startAddr;
        R16_ADC_DMA_END = endAddr;
        if(m)
        {
            R8_ADC_CTRL_DMA |= RB_ADC_DMA_LOOP | RB_ADC_IE_DMA_END | RB_ADC_DMA_ENABLE;
        }
        else
        {
            R8_ADC_CTRL_DMA &= ~RB_ADC_DMA_LOOP;
            R8_ADC_CTRL_DMA |= RB_ADC_IE_DMA_END | RB_ADC_DMA_ENABLE;
        }
    }
}

/*********************************************************************
 * @fn      adc_to_temperature_celsius
 *
 * @brief   Convert ADC value to temperature(Celsius)
 *
 * @param   adc_val - adc value
 *
 * @return  temperature (Celsius)
 */

int adc_to_temperature_celsius(uint16_t adc_val)
{
    uint32_t C25 = 0;
    int      temp;

    C25 = (*((PUINT32)ROM_CFG_TMP_25C));

    /* current temperature = standard temperature + (adc deviation * adc linearity coefficient) */ 
    temp = (((C25 >> 16) & 0xFFFF) ? ((C25 >> 16) & 0xFFFF) : 25) + \
        (adc_val - ((int)(C25 & 0xFFFF))) * 10 / 27; 

    return (temp);
}
