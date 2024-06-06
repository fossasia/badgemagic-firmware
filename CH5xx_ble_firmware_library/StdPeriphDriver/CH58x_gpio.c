/********************************** (C) COPYRIGHT *******************************
 * File Name          : CH58x_gpio.c
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
 * @fn      GPIOA_ModeCfg
 *
 * @brief   GPIOA端口引脚模式配置
 *
 * @param   pin     - PA0-PA15
 * @param   mode    - 输入输出类型
 *
 * @return  none
 */
void GPIOA_ModeCfg(uint32_t pin, GPIOModeTypeDef mode)
{
    switch(mode)
    {
        case GPIO_ModeIN_Floating:
            R32_PA_PD_DRV &= ~pin;
            R32_PA_PU &= ~pin;
            R32_PA_DIR &= ~pin;
            break;

        case GPIO_ModeIN_PU:
            R32_PA_PD_DRV &= ~pin;
            R32_PA_PU |= pin;
            R32_PA_DIR &= ~pin;
            break;

        case GPIO_ModeIN_PD:
            R32_PA_PD_DRV |= pin;
            R32_PA_PU &= ~pin;
            R32_PA_DIR &= ~pin;
            break;

        case GPIO_ModeOut_PP_5mA:
            R32_PA_PD_DRV &= ~pin;
            R32_PA_DIR |= pin;
            break;

        case GPIO_ModeOut_PP_20mA:
            R32_PA_PD_DRV |= pin;
            R32_PA_DIR |= pin;
            break;

        default:
            break;
    }
}

/*********************************************************************
 * @fn      GPIOB_ModeCfg
 *
 * @brief   GPIOB端口引脚模式配置
 *
 * @param   pin     - PB0-PB23
 * @param   mode    - 输入输出类型
 *
 * @return  none
 */
void GPIOB_ModeCfg(uint32_t pin, GPIOModeTypeDef mode)
{
    switch(mode)
    {
        case GPIO_ModeIN_Floating:
            R32_PB_PD_DRV &= ~pin;
            R32_PB_PU &= ~pin;
            R32_PB_DIR &= ~pin;
            break;

        case GPIO_ModeIN_PU:
            R32_PB_PD_DRV &= ~pin;
            R32_PB_PU |= pin;
            R32_PB_DIR &= ~pin;
            break;

        case GPIO_ModeIN_PD:
            R32_PB_PD_DRV |= pin;
            R32_PB_PU &= ~pin;
            R32_PB_DIR &= ~pin;
            break;

        case GPIO_ModeOut_PP_5mA:
            R32_PB_PD_DRV &= ~pin;
            R32_PB_DIR |= pin;
            break;

        case GPIO_ModeOut_PP_20mA:
            R32_PB_PD_DRV |= pin;
            R32_PB_DIR |= pin;
            break;

        default:
            break;
    }
}

/*********************************************************************
 * @fn      GPIOA_ITModeCfg
 *
 * @brief   GPIOA引脚中断模式配置
 *
 * @param   pin     - PA0-PA15
 * @param   mode    - 触发类型
 *
 * @return  none
 */
void GPIOA_ITModeCfg(uint32_t pin, GPIOITModeTpDef mode)
{
    switch(mode)
    {
        case GPIO_ITMode_LowLevel: // 低电平触发
            R16_PA_INT_MODE &= ~pin;
            R32_PA_CLR |= pin;
            break;

        case GPIO_ITMode_HighLevel: // 高电平触发
            R16_PA_INT_MODE &= ~pin;
            R32_PA_OUT |= pin;
            break;

        case GPIO_ITMode_FallEdge: // 下降沿触发
            R16_PA_INT_MODE |= pin;
            R32_PA_CLR |= pin;
            break;

        case GPIO_ITMode_RiseEdge: // 上升沿触发
            R16_PA_INT_MODE |= pin;
            R32_PA_OUT |= pin;
            break;

        default:
            break;
    }
    R16_PA_INT_IF = pin;
    R16_PA_INT_EN |= pin;
}

/*********************************************************************
 * @fn      GPIOB_ITModeCfg
 *
 * @brief   GPIOB引脚中断模式配置
 *
 * @param   pin     - PB0-PB23
 * @param   mode    - 触发类型
 *
 * @return  none
 */
void GPIOB_ITModeCfg(uint32_t pin, GPIOITModeTpDef mode)
{
    uint32_t Pin = pin | ((pin & (GPIO_Pin_22 | GPIO_Pin_23)) >> 14);
    switch(mode)
    {
        case GPIO_ITMode_LowLevel: // 低电平触发
            R16_PB_INT_MODE &= ~Pin;
            R32_PB_CLR |= pin;
            break;

        case GPIO_ITMode_HighLevel: // 高电平触发
            R16_PB_INT_MODE &= ~Pin;
            R32_PB_OUT |= pin;
            break;

        case GPIO_ITMode_FallEdge: // 下降沿触发
            R16_PB_INT_MODE |= Pin;
            R32_PB_CLR |= pin;
            break;

        case GPIO_ITMode_RiseEdge: // 上升沿触发
            R16_PB_INT_MODE |= Pin;
            R32_PB_OUT |= pin;
            break;

        default:
            break;
    }
    R16_PB_INT_IF = Pin;
    R16_PB_INT_EN |= Pin;
}

/*********************************************************************
 * @fn      GPIOPinRemap
 *
 * @brief   外设功能引脚映射
 *
 * @param   s       - 是否使能映射
 * @param   perph   - RB_RF_ANT_SW_EN -  RF antenna switch control output on PB16/PB17/PB18/PB19/PB20/PB21
 *                    RB_PIN_U0_INV -  RXD0/RXD0_/TXD0/TXD0_ invert input/output
 *                    RB_PIN_INTX   -  INTX: INT24/INT25 PB8/PB9 -> INT24_/INT25_ PB22/PB23
 *                    RB_PIN_MODEM  -  MODEM: PB1/PB5 -> PB14/PB15
 *                    RB_PIN_I2C    -  I2C: PB13/PB12 -> PB21/PB20
 *                    RB_PIN_PWMX   -  PWMX: PA12/PA13/PB4/PB6/PB7 -> PA6/PA7/PB1/PB2/PB3
 *                    RB_PIN_SPI0   -  SPI0:  PA12/PA13/PA14/PA15 -> PB12/PB13/PB14/PB15
 *                    RB_PIN_UART3  -  UART3: PA4/PA5 ->  PB20/PB21
 *                    RB_PIN_UART2  -  UART2: PA6/PA7 ->  PB22/PB23
 *                    RB_PIN_UART1  -  UART1: PA8/PA9 ->  PB12/PB13
 *                    RB_PIN_UART0  -  UART0: PB4/PB7 ->  PA15/PA14
 *                    RB_PIN_TMR3   -  TMR2:  PA9 ->  PB23
 *                    RB_PIN_TMR2   -  TMR2:  PA11 ->  PB11
 *                    RB_PIN_TMR1   -  TMR1:  PA10 ->  PB10
 *                    RB_PIN_TMR0   -  TMR0:  PA9 ->  PB23
 *
 * @return  none
 */
void GPIOPinRemap(FunctionalState s, uint16_t perph)
{
    if(s)
    {
        R16_PIN_ALTERNATE |= perph;
    }
    else
    {
        R16_PIN_ALTERNATE &= ~perph;
    }
}

/*********************************************************************
 * @fn      GPIOAGPPCfg
 *
 * @brief   模拟外设GPIO引脚功能控制
 *
 * @param   s       -   ENABLE  - 打开模拟外设功能，关闭数字功能
 *                      DISABLE - 启用数字功能，关闭模拟外设功能
 * @param   perph   -   RB_PIN_ADC8_9_IE  - ADC/TKEY 9/8通道
 *                      RB_PIN_ADC6_7_IE  - ADC/TKEY 7/6通道
 *                      RB_PIN_ADC10_IE   - ADC/TKEY 10通道
 *                      RB_PIN_ADC11_IE   - ADC/TKEY 11 通道
 *                      RB_PIN_USB2_DP_PU - USB2 U2D+引脚内部上拉电阻
 *                      RB_PIN_USB2_IE    - USB2引脚
 *                      RB_PIN_USB_DP_PU  - USB UD+引脚内部上拉电阻
 *                      RB_PIN_USB_IE     - USB 引脚
 *                      RB_PIN_ADC0_IE    - ADC/TKEY 0 通道
 *                      RB_PIN_ADC1_IE    - ADC/TKEY 1 通道
 *                      RB_PIN_ADC12_IE   - ADC/TKEY 12 通道
 *                      RB_PIN_ADC13_IE   - ADC/TKEY 13 通道
 *                      RB_PIN_XT32K_IE   - 32KHz晶振LSE引脚
 *                      RB_PIN_ADC2_3_IE  - ADC/TKEY 2/3 通道
 *                      RB_PIN_ADC4_5_IE  - ADC/TKEY 4/5 通道
 *
 * @return  none
 */
void GPIOAGPPCfg(FunctionalState s, uint16_t perph)
{
    if(s)
    {
        R16_PIN_ANALOG_IE |= perph;
    }
    else
    {
        R16_PIN_ANALOG_IE &= ~perph;
    }
}
