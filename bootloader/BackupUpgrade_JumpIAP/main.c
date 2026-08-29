/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c   [DIRECT-XIP PING-PONG BOOT]
 * Description        : Reads the active-slot flag and jumps directly into
 *                       that slot's firmware ¡ª no erase/copy step. Includes
 *                       a simplified revert mechanism modeled after MCUboot's
 *                       direct-xip revert (docs.mcuboot.com/design.html
 *                       #direct-xip-ram-load-revert):
 *
 *                         1. Read which slot is flagged active.
 *                         2. If that slot has a pending (unconfirmed) boot
 *                            from a previous reset, it means the last boot
 *                            into that slot never confirmed itself healthy.
 *                            Revert: fall back to the other slot instead.
 *                         3. Otherwise, mark this boot as pending, and jump.
 *                            APP is responsible for clearing "pending" and
 *                            setting "confirmed" once it has verified it's
 *                            running correctly.
 *********************************************************************************/

#include "CH58x_common.h"
#include "OTA.h"

/* Records the currently active image */
unsigned char CurrImageFlag = 0xff;

/* Temporary buffer for flash operations */
__attribute__((aligned(8))) uint8_t block_buf[16];

#define jumpApp    ((void (*)(void))((int *)IMAGE_A_START_ADD))
#define jumpAppB   ((void (*)(void))((int *)IMAGE_B_START_ADD))

/*********************************************************************
 * @fn      WriteOTAInfo
 * @brief   Write a full OTADataFlashInfo_t record to dataflash
 *********************************************************************/
static void WriteOTAInfo(OTADataFlashInfo_t *info)
{
    uint8_t r_st, e_st, w_st;

    r_st = EEPROM_READ(OTA_DATAFLASH_ADD, (uint32_t *)&block_buf[0], 4);
    e_st = EEPROM_ERASE(OTA_DATAFLASH_ADD, EEPROM_PAGE_SIZE);
    tmos_memcpy(block_buf, info, sizeof(OTADataFlashInfo_t));
    w_st = EEPROM_WRITE(OTA_DATAFLASH_ADD, (uint32_t *)&block_buf[0], 4);

    PRINT("IAP WriteOTAInfo: flag=%02x pending=%02x ok=%02x r=%02x e=%02x w=%02x\n",
        info->ImageFlag, info->BootPending, info->BootOK, r_st, e_st, w_st);
}

/*********************************************************************
 * @fn      SwitchImageFlag
 * @brief   Kept for compatibility with existing peripheral.c calls.
 *          Switches ImageFlag only, leaves BootPending/BootOK untouched
 *          here ¡ª IAP is what actually sets BootPending, right before jump.
 *********************************************************************/
void SwitchImageFlag(uint8_t new_flag)
{
    OTADataFlashInfo_t info;
    EEPROM_READ(OTA_DATAFLASH_ADD, (uint32_t *)&info, sizeof(info));
    info.ImageFlag = new_flag;
    WriteOTAInfo(&info);
}

/*********************************************************************
 * @fn      jump_APP
 * @brief   Direct-XIP dispatch with simplified revert support.
 *********************************************************************/
void jump_APP(void)
{
    OTADataFlashInfo_t info;
    EEPROM_READ(OTA_DATAFLASH_ADD, (uint32_t *)&info, sizeof(info));

    PRINT("IAP jump_APP: flag=%02x pending=%02x ok=%02x\n",
        info.ImageFlag, info.BootPending, info.BootOK);

    uint8_t target_flag = info.ImageFlag;

    /* Revert check: was the flagged slot mid-test (pending) and never
     * confirmed OK on its last boot attempt? If so, distrust it and fall
     * back to the other slot, per MCUboot's revert procedure. */
    if(info.BootPending == BOOT_MARK_CONFIRMED /* i.e. not 0xFF -> was pending */
       && info.BootOK != BOOT_MARK_CONFIRMED)
    {
        PRINT("IAP: slot %02x never confirmed boot - reverting\n", target_flag);
        target_flag = (target_flag == IMAGE_A_FLAG) ? IMAGE_B_FLAG : IMAGE_A_FLAG;
    }

    /* Mark this boot attempt as pending; APP must confirm it. */
    info.ImageFlag    = target_flag;
    info.BootPending  = BOOT_MARK_CONFIRMED;   /* "a boot attempt is in flight" */
    info.BootOK       = BOOT_MARK_UNCONFIRMED; /* not yet confirmed by APP */
    WriteOTAInfo(&info);

    mDelaymS(50);
    __asm volatile("csrc mstatus, 8");

    if(target_flag == IMAGE_B_FLAG)
    {
        PRINT("IAP: jumping to slot B at %08lx\n", (unsigned long)IMAGE_B_START_ADD);
        jumpAppB();
    }
    else
    {
        PRINT("IAP: jumping to slot A at %08lx\n", (unsigned long)IMAGE_A_START_ADD);
        jumpApp();
    }
}

/*********************************************************************
 * @fn      main
 *********************************************************************/
int main(void)
{
    FLASH_ROM_SW_RESET();
#if(defined(DCDC_ENABLE)) && (DCDC_ENABLE == TRUE)
    PWR_DCDCCfg(ENABLE);
#endif
    SetSysClock(CLK_SOURCE_PLL_60MHz);
#if(defined(HAL_SLEEP)) && (HAL_SLEEP == TRUE)
    GPIOA_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
    GPIOB_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
#endif
#ifdef DEBUG
    GPIOA_SetBits(bTXD1);
    GPIOA_ModeCfg(bTXD1, GPIO_ModeOut_PP_5mA);
    UART1_DefInit();
#endif
    PRINT("\n=== IAP boot (direct-xip) ===\n");
    SYS_DisableAllIrq(NULL);
    jump_APP();
}

/******************************** endfile @ main ******************************/