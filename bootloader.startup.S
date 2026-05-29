	.section	.init,"ax",@progbits
	.global	_start
	.align	1
_start:
	j	handle_reset

    .section    .vector,"ax",@progbits
    .align  1
_vector_base:
    .option norvc;

    .word   0
    .word   0
    .word   NMI_Handler
    .word   HardFault_Handler
    .word   0xF5F9BDA9
    .word   Ecall_M_Mode_Handler
    .word   0
    .word   0
    .word   Ecall_U_Mode_Handler
    .word   Break_Point_Handler
    .word   0
    .word   0
    .word   SysTick_Handler
    .word   0
    .word   SW_Handler
    .word   0

    /* External Interrupts */
    .word   TMR0_IRQHandler            /* 0:  TMR0 */
    .word   GPIOA_IRQHandler           /* GPIOA */
    .word   GPIOB_IRQHandler           /* GPIOB */
    .word   SPI0_IRQHandler            /* SPI0 */
    .word   BB_IRQHandler              /* BLEB */
    .word   LLE_IRQHandler             /* BLEL */
    .word   USB_IRQHandler             /* USB */
    .word   USB2_IRQHandler			   /* USB2 */
    .word   TMR1_IRQHandler            /* TMR1 */
    .word   TMR2_IRQHandler            /* TMR2 */
    .word   UART0_IRQHandler           /* UART0 */
    .word   UART1_IRQHandler           /* UART1 */
    .word   RTC_IRQHandler             /* RTC */
    .word   ADC_IRQHandler             /* ADC */
    .word   I2C_IRQHandler 			   /* I2C */
    .word   PWMX_IRQHandler            /* PWMX */
    .word   TMR3_IRQHandler            /* TMR3 */
    .word   UART2_IRQHandler           /* UART2 */
    .word   UART3_IRQHandler           /* UART3 */
    .word   WDOG_BAT_IRQHandler        /* WDOG_BAT */

    .option rvc;

    .section    .vector_handler, "ax", @progbits
    .weak   NMI_Handler
    .weak   HardFault_Handler
    .weak   Ecall_M_Mode_Handler
    .weak   Ecall_U_Mode_Handler
    .weak   Break_Point_Handler
    .weak   SysTick_Handler
    .weak   SW_Handler
    .weak   TMR0_IRQHandler
    .weak   GPIOA_IRQHandler
    .weak   GPIOB_IRQHandler
    .weak   SPI0_IRQHandler
    .weak   BB_IRQHandler
    .weak   LLE_IRQHandler
    .weak   USB_IRQHandler
    .weak   USB2_IRQHandler
    .weak   TMR1_IRQHandler
    .weak   TMR2_IRQHandler
    .weak   UART0_IRQHandler
    .weak   UART1_IRQHandler
    .weak   RTC_IRQHandler
    .weak   ADC_IRQHandler
    .weak   I2C_IRQHandler
    .weak   PWMX_IRQHandler
    .weak   TMR3_IRQHandler
    .weak   UART2_IRQHandler
    .weak   UART3_IRQHandler
    .weak   WDOG_BAT_IRQHandler

NMI_Handler:          1: j 1b
HardFault_Handler:    1: j 1b
Ecall_M_Mode_Handler: 1: j 1b
Ecall_U_Mode_Handler: 1: j 1b
Break_Point_Handler:  1: j 1b
SysTick_Handler:      1: j 1b
SW_Handler:           1: j 1b
TMR0_IRQHandler:      1: j 1b
GPIOA_IRQHandler:     1: j 1b
GPIOB_IRQHandler:     1: j 1b
SPI0_IRQHandler:      1: j 1b
BB_IRQHandler:        1: j 1b
LLE_IRQHandler:       1: j 1b
USB_IRQHandler:       1: j 1b
USB2_IRQHandler:      1: j 1b
TMR1_IRQHandler:      1: j 1b
TMR2_IRQHandler:      1: j 1b
UART0_IRQHandler:     1: j 1b
UART1_IRQHandler:     1: j 1b
RTC_IRQHandler:       1: j 1b
ADC_IRQHandler:       1: j 1b
I2C_IRQHandler:       1: j 1b
PWMX_IRQHandler:      1: j 1b
TMR3_IRQHandler:      1: j 1b
UART2_IRQHandler:     1: j 1b
UART3_IRQHandler:     1: j 1b
WDOG_BAT_IRQHandler:  1: j 1b

	.section	.handle_reset,"ax",@progbits
	.weak	handle_reset
	.align	1
handle_reset:
.option push 
.option	norelax 
	la gp, __global_pointer$
.option	pop 
1:
	la sp, _eusrstack 

/* Load highcode code  section from flash to RAM */
2:
    la a0, _highcode_lma
    la a1, _highcode_vma_start
    la a2, _highcode_vma_end
    bgeu a1, a2, 2f
1:
    lw t0, (a0)
    sw t0, (a1)
    addi a0, a0, 4
    addi a1, a1, 4
    bltu a1, a2, 1b

/* Load data section from flash to RAM */
2:
	la a0, _data_lma
	la a1, _data_vma
	la a2, _edata
	bgeu a1, a2, 2f
1:
	lw t0, (a0)
	sw t0, (a1)
	addi a0, a0, 4
	addi a1, a1, 4
	bltu a1, a2, 1b
2:
	/* clear bss section */
	la a0, _sbss
	la a1, _ebss
	bgeu a0, a1, 2f
1:
	sw zero, (a0)
	addi a0, a0, 4
	bltu a0, a1, 1b
2:

	/* Microprocessor Configuration Registers (corecfgr) */
	/*
	This register is mainly used to configure the microprocessor pipeline,
	instruction prediction and other related features, and generally does not
	need to be operated. The relevant MCU products are configured with default
	values in the startup file */
	li t0, 0x1f
	csrw 0xbc0, t0

	/* Interrupt System Control Register (INTSYSCR) */
	/* INESTEN = 1: Interrupt nesting function enabled */
	/* HWSTKEN = 1: HPE function enabled */
	li t0, 0x3
	csrw 0x804, t0

	/* Machine Mode Status Register (mstatus) */
	/* MPIE = 1: Interrupt enable state before entering interrupt */
	/* MIE = 1: Machine mode interrupt enable */
	li t0, 0x88
	csrs mstatus, t0

	csrs pmpaddr0, x0

	/* Configure The interrupt vector table base address to `_vector_base` */
	/* MODE1 = 1: Identify by absolute address, support full range, but must jump to 
	absolute address mode */
	/* MODE0 = 1: Address offset based on interrupt number *4. */
	la t0, _vector_base
	ori t0, t0, 3
	csrw mtvec, t0

	j boot
