#include "CH58x_common.h"
#include <bootutil/bootutil.h>
#include <common/debug.h>

// /* Import a binary file */
// #define IMPORT_BIN(sect, file, sym) asm (\
//     ".section "#sect "\n" \
// 	".balign 4\n"                           /* Word alignment */\
//     ".global " #sym "\n"                    /* Export the object address */\
//     #sym ":\n"                              /* Define the object label */\
//     ".incbin \"" file "\"\n"                /* Import the file */\
//     ".global _sizeof_" #sym "\n"            /* Export the object size */\
//     ".set _sizeof_" #sym ", . - " #sym "\n" /* Define the object size */\
//     ".balign 4\n"                           /* Word alignment */\
//     )
// 	// __attribute__((section(".highcode")))
// IMPORT_BIN(.app, "build/badgemagic-ch582.signed.bin", main_bin);
// /* Declaration of symbols (any type can be used) */
// extern const unsigned char main_bin[], _sizeof_main_bin[];

static void debug_init()
{
	GPIOA_SetBits(GPIO_Pin_9);
	GPIOA_ModeCfg(GPIO_Pin_8, GPIO_ModeIN_PU);
	GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA);
	UART1_DefInit();
	UART1_BaudRateCfg(921600);
}

void start_app(uint32_t pc, uint32_t sp) {
	// __asm volatile ("la sp, %0" : : "r" (sp) : );

	PRINT("Attempting jump pc=%x, sp=%x\n", pc, sp);
	void (*application_reset_handler)(void) = (void *)pc;
	((  void  (*)  ( void ))  ((int*)pc))();
}

void do_boot(struct boot_rsp *rsp) {
	PRINT("Starting Main Application");
	PRINT("  Image Start Offset: 0x%x", (int)rsp->br_image_off);

	// We run from internal flash. Base address of this medium is 0x0
	uint32_t vector_table = 0x0 + rsp->br_image_off + rsp->br_hdr->ih_hdr_size;

	uint32_t *app_code = (uint32_t *)vector_table;
	uint32_t app_sp = app_code[0];
	uint32_t app_start = app_code[1];

	PRINT("  Vector Table Start Address: 0x%x. PC=0x%x, SP=0x%x",
	(int)vector_table, app_start, app_sp);

	// // We need to move the vector table to reflect the location of the main application
	// volatile uint32_t *vtor = (uint32_t *)0xE000ED08;
	// *vtor = (vector_table & 0xFFFFFFF8);

	start_app(vector_table, app_sp);
}

int main() {
	SetSysClock(CLK_SOURCE_PLL_60MHz);

	debug_init();
	PRINT("\nDebug console is on UART%d\n", DEBUG);

	struct boot_rsp rsp;
	int rv = boot_go(&rsp);

	if (rv == 0) {
		// 'rsp' contains the start address of the image
		do_boot(&rsp);
	}
	PRINT("\rv failed\n");

	while(1);
}