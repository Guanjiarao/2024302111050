#include "types.h"
#include "memlayout.h"

void plicinit(void)
{
  *(volatile uint32 *)(PLIC + UART0_IRQ * 4) = 1;
}

void plicinithart(void)
{
  *(volatile uint32 *)PLIC_SENABLE(0) = 1 << UART0_IRQ;
  *(volatile uint32 *)PLIC_SPRIORITY(0) = 0;
}

int plic_claim(void)
{
  return *(volatile uint32 *)PLIC_SCLAIM(0);
}

void plic_complete(int irq)
{
  *(volatile uint32 *)PLIC_SCLAIM(0) = irq;
}
