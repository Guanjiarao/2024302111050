#include "types.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

#define REG(n) ((volatile uint8 *)(UART0 + (n)))
#define RHR 0
#define THR 0
#define IER 1
#define FCR 2
#define LCR 3
#define LSR 5
#define RX_READY 0x01
#define TX_IDLE 0x20

void uartinit(void)
{
  *REG(IER) = 0;
  *REG(LCR) = 0x80;
  *REG(0) = 0x03;
  *REG(1) = 0;
  *REG(LCR) = 0x03;
  *REG(FCR) = 0x07;
  *REG(IER) = 0x01;
}

void uartputc_sync(int c)
{
  while ((*REG(LSR) & TX_IDLE) == 0)
    ;
  io_fence();
  *REG(THR) = (uint8)c;
  io_fence();
}

void uartintr(void)
{
  while (*REG(LSR) & RX_READY)
    consoleintr(*REG(RHR));
}
