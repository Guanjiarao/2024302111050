#include "types.h"
#include "riscv.h"

void main(void);

void
start(void)
{
  // Return from M mode into main() in S mode with interrupts disabled.
  uint64 status = r_mstatus();
  status &= ~(1L << 3); // MSTATUS.MIE
  status &= ~MSTATUS_MPP_MASK;
  status |= MSTATUS_MPP_S;
  w_mstatus(status);
  w_mepc((uint64)main);

  // Lab 1 uses physical addresses directly; paging is introduced later.
  w_satp(0);

  // Delegate later S-mode traps, but do not enable interrupts in this lab.
  w_medeleg(0xffff);
  w_mideleg(0xffff);
  w_mie(0);
  w_sie(0);

  // Give S/U modes read, write, and execute access to physical memory.
  w_pmpaddr0(0x3fffffffffffffull);
  w_pmpcfg0(0xf);

  asm volatile("mret");
  __builtin_unreachable();
}
