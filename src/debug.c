#include "debug.h"
#include "CH583SFR.h"

#ifdef DEBUG

#define Debug_UART0        0
#define Debug_UART1        1
#define Debug_UART2        2
#define Debug_UART3        3

int _write(int fd, char *buf, int size)
{
    int i;
    for(i = 0; i < size; i++)
    {
#if DEBUG == Debug_UART0
        while(R8_UART0_TFC == UART_FIFO_SIZE);
        R8_UART0_THR = *buf++;
#elif DEBUG == Debug_UART1
        while(R8_UART1_TFC == UART_FIFO_SIZE);
        R8_UART1_THR = *buf++;
#elif DEBUG == Debug_UART2
        while(R8_UART2_TFC == UART_FIFO_SIZE);
        R8_UART2_THR = *buf++;
#elif DEBUG == Debug_UART3       
        while(R8_UART3_TFC == UART_FIFO_SIZE);
        R8_UART3_THR = *buf++;
#else
#error "Please specify a debug port for uart"
#endif
    }
    return size;
}

#endif