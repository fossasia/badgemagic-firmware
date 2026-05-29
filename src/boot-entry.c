#include "CH58x_common.h"
#include <bootutil/bootutil.h>
#include <common/debug.h>
#include <boot_serial/boot_serial.h>
#include <stdlib.h>
#include <lwrb/lwrb.h>

#include "usb/usb.h"

static void debug_init()
{
	GPIOA_SetBits(GPIO_Pin_9);
	GPIOA_ModeCfg(GPIO_Pin_8, GPIO_ModeIN_PU);
	GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA);
	UART1_DefInit();
	UART1_BaudRateCfg(921600);
}

__INTERRUPT
__HIGH_CODE
void HardFault_Handler(void)
{
	PRINT("HardFault happen\n");
	while(1);
}

void do_boot(struct boot_rsp *rsp) {
	PRINT(" Image Start Offset: 0x%x", (int)rsp->br_image_off);

	int fw_address = 0x0 + rsp->br_image_off + rsp->br_hdr->ih_hdr_size;

	PRINT("Attempting jump pc=%x\n", fw_address);
	((void (*) ( void )) ((int*)fw_address))();
}

lwrb_t fifo;
uint8_t fifo_buf[1024];

void rx_to_fifo(uint8_t *buf, uint16_t len)
{
	PFIC_DisableAllIRQ();
	lwrb_write(&fifo, buf, len);
	PFIC_EnableAllIRQ();
}

int read(char *str, int cnt, int *newline)
{
	int rb = lwrb_read(&fifo, str, cnt);
	if (rb > 0 && str[rb-1] == '\n') {
		*newline = 1;
		PRINT("> read: %d %s\n", rb, str);
	} else {
		*newline = 0;
	}
	return rb;
}

void write(const char *ptr, int cnt)
{
	PRINT("> write: %s %d\n", ptr, cnt);
	cdc_tx_poll(ptr, cnt, 1000);
}

struct boot_uart_funcs boot_uart = {
	read, write,
};

int boot() {
	SetSysClock(CLK_SOURCE_PLL_60MHz);
	debug_init();
	PRINT("\nDebug console is on UART%d\n", DEBUG);

	lwrb_init(&fifo, fifo_buf, sizeof(fifo_buf));
	cdc_onWrite(rx_to_fifo);
	usb_start();
	boot_serial_start(&boot_uart);

	struct boot_rsp rsp;
	int rv = boot_go(&rsp);

	if (rv == 0) {
		do_boot(&rsp);
	}
	PRINT("\rv failed\n");

	abort();
}