#include "types.h"
#include "riscv.h"
#include "course_sid.h"

void main(void);

static void
timerinit(void)
{
  w_menvcfg(r_menvcfg() | MENVCFG_STCE);
  w_mcounteren(r_mcounteren() | 2);
  w_stimecmp(r_time() + 1000000);
}

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

  // Delegate traps and enable supervisor timer/external interrupts for Lab2.
  w_medeleg(0xffff);
  w_mideleg(0xffff);
  w_sie(r_sie() | SIE_SEIE | SIE_STIE);

  // Give S/U modes read, write, and execute access to physical memory.
  w_pmpaddr0(0x3fffffffffffffull);
  w_pmpcfg0(0xf);

  timerinit();
  w_tp(r_mhartid());

  asm volatile("mret");
  __builtin_unreachable();
}
