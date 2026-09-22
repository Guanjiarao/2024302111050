#include "types.h"
#include "memlayout.h"
#include "riscv.h"
#include "course_sid.h"

#define UART_THR 0
#define UART_LSR 5
#define UART_LSR_THRE 0x20
#define THROTTLE_INTERVAL (16 + COURSE_SID % 16)

static uint emitted;
static uint checksum;

static volatile uint8 *
uart_reg(uint offset)
{
  return (volatile uint8 *)(UART0 + offset);
}

void
consoleinit(void)
{
  emitted = 0;
  checksum = 0;
}

void
uartputc_sync(int c)
{
  while ((*uart_reg(UART_LSR) & UART_LSR_THRE) == 0)
    ;
  io_fence();
  *uart_reg(UART_THR) = (uint8)c;
  io_fence();
}

void
consputc(int c)
{
  uartputc_sync(c);
  checksum += (uint8)c;
  emitted++;

  if (emitted % THROTTLE_INTERVAL == 0) {
    // Keep the delay personalized without embedding a generated literal.
    for (volatile uint i = 0; i < (COURSE_SID % 256) + 1; i++)
      asm volatile("nop");
  }
}

uint
console_checksum(void)
{
  return checksum;
}

