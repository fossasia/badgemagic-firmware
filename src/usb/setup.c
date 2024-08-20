#include <stdlib.h>

#include "CH58x_common.h"

#include "utils.h"

uint8_t *cfg_desc;

// FIXME: here wasting 1KiB of ram
void (*if_handlers[256])(USB_SETUP_REQ *request);
void (*ep_handlers[7])();

/* Configuration Descriptor template */
USB_CFG_DESCR cfg_static = {
	.bLength = sizeof(USB_CFG_DESCR),
	.bDescriptorType = 0x02,
	.wTotalLength = sizeof(USB_CFG_DESCR), // will be updated on cfg_desc_add()
	.bNumInterfaces = 0, // will be updated on cfg_desc_add()
	.bConfigurationValue = 0x01,
	.iConfiguration = 4,
	.bmAttributes = 0xA0,
	.MaxPower = 50 // mA
};

void cfg_desc_append(void *desc)
{
	uint8_t cfg_len = 0;
	uint8_t len = ((uint8_t *)desc)[0];
	if (cfg_desc) // attempting to add a non-Configuration Descriptor
		cfg_len = ((USB_CFG_DESCR *)cfg_desc)->wTotalLength;
	uint8_t newlen = cfg_len + len;

	cfg_desc = realloc(cfg_desc, newlen); // TODO: add a safe check here

	memcpy(cfg_desc + cfg_len, desc, len);

	((USB_CFG_DESCR *)cfg_desc)->wTotalLength = newlen;
	((USB_CFG_DESCR *)cfg_desc)->bNumInterfaces += ((uint8_t *)desc)[1] == 0x04;
}

int ep_cb_register(int ep_num, void (*cb)())
{
	if (ep_num > 8)
		return -1;
	if (ep_handlers[ep_num]) // already registerd
		return -1;

	ep_handlers[ep_num] = cb;
	return 0;
}

int if_cb_register(uint8_t if_num, void (*cb)(USB_SETUP_REQ *request))
{
	if (if_handlers[if_num]) // already registered
		return -1;

	if_handlers[if_num] = cb;
	return 0;
}

void dma_register(uint8_t ep_num, void *buf)
{
	if (ep_num > 7) // CH582 support only 8 endpoint
		return;

	volatile uint16_t *p_regs[] = {
		&R16_UEP0_DMA,
		&R16_UEP1_DMA,
		&R16_UEP2_DMA,
		&R16_UEP3_DMA,
		NULL,
		&R16_UEP5_DMA,
		&R16_UEP6_DMA,
		&R16_UEP7_DMA,
	};
	volatile uint8_t *ctrl_regs[] = {
		&R8_UEP0_CTRL,
		&R8_UEP1_CTRL,
		&R8_UEP2_CTRL,
		&R8_UEP3_CTRL,
		&R8_UEP4_CTRL,
		&R8_UEP5_CTRL,
		&R8_UEP6_CTRL,
		&R8_UEP7_CTRL,
	};
	// ep4's dma buffer is hardcoded pointing to 
	// the next ep0
	if (ep_num != 4)
		*p_regs[ep_num] = (uint16_t)(uint32_t)buf;

	*ctrl_regs[ep_num] = UEP_R_RES_ACK | UEP_T_RES_NAK;
}

static void init(void)
{
	R8_USB_CTRL = 0x00;

	R8_UEP4_1_MOD = RB_UEP4_RX_EN | RB_UEP4_TX_EN |
				RB_UEP1_RX_EN | RB_UEP1_TX_EN;
	R8_UEP2_3_MOD = RB_UEP2_RX_EN | RB_UEP2_TX_EN |
				RB_UEP3_RX_EN | RB_UEP3_TX_EN;
	R8_UEP567_MOD = RB_UEP7_RX_EN | RB_UEP7_TX_EN |
				RB_UEP6_RX_EN | RB_UEP6_TX_EN |
				RB_UEP5_RX_EN | RB_UEP5_TX_EN;

	R16_PIN_ANALOG_IE |= RB_PIN_USB_IE | RB_PIN_USB_DP_PU;
	R8_UDEV_CTRL = RB_UD_PD_DIS | RB_UD_PORT_EN;

	R8_USB_DEV_AD = 0x00;
	R8_USB_INT_FG = 0xFF;
	R8_USB_CTRL = RB_UC_DEV_PU_EN | RB_UC_INT_BUSY | RB_UC_DMA_EN;
	R8_USB_INT_EN = RB_UIE_SUSPEND | RB_UIE_BUS_RST | RB_UIE_TRANSFER;
}

void ctrl_init();
void hiddev_init();
void cdc_acm_init();

void usb_start() {
	cfg_desc_append(&cfg_static);

	ctrl_init();

	/* This should be placed first, the python script always looks
	for the first interface (not the interface number) */
	hiddev_init();

	cdc_acm_init();

	init();
	PFIC_EnableIRQ(USB_IRQn);
}