#include "types.h"
#include "memlayout.h"
#include "riscv.h"
#include "course_sid.h"
#include "proc.h"
#include "defs.h"

extern char uservec[], userret[];
extern char boot_stack[];
void kernelvec(void);
static struct trapframe *shared_tf = (struct trapframe *)TRAPFRAME;
static uint timer_ticks;

void trapinit(void) { w_stvec((uint64)kernelvec); }

static void prepare_return(void)
{
  struct proc *p = myproc();
  intr_off();
  *shared_tf = p->trapframe;
  shared_tf->kernel_satp = 0;
  shared_tf->kernel_sp = (uint64)boot_stack + LAB1_STACK_KB * 1024;
  shared_tf->kernel_trap = (uint64)usertrap;
  shared_tf->kernel_hartid = 0;
  uint64 s = r_sstatus();
  s &= ~SSTATUS_SPP;
  s |= SSTATUS_SPIE;
  w_sstatus(s);
  w_sepc(shared_tf->epc);
  w_stvec((uint64)uservec);
}

void userenter(void)
{
  prepare_return();
  ((void (*)(uint64))userret)(0);
  __builtin_unreachable();
}

uint64 usertrap(void)
{
  w_stvec((uint64)kernelvec);
  struct proc *trapped = myproc();
  trapped->trapframe = *shared_tf;
  trapped->trapframe.epc = r_sepc();
  uint64 cause = r_scause();
  if (cause == 8) {
    trapped->trapframe.epc += 4;
    syscall();
  } else if (!devintr()) {
    printf("user trap cause=%x epc=%x stval=%x\n", (uint)cause,
           (uint)r_sepc(), (uint)r_stval());
    proc_exit(-1);
  }
  prepare_return();
  return 0;
}

void kerneltrap(void)
{
  uint64 epc = r_sepc();
  uint64 status = r_sstatus();
  if (!devintr()) panic("kernel trap");
  w_sepc(epc);
  w_sstatus(status);
}

int devintr(void)
{
  uint64 cause = r_scause();
  if (cause == 0x8000000000000009L) {
    int irq = plic_claim();
    if (irq == UART0_IRQ) uartintr();
    if (irq) plic_complete(irq);
    return 1;
  }
  if (cause == 0x8000000000000005L) {
    timer_ticks++;
    if (timer_ticks % LAB2_TICK == 0)
      asm volatile("nop"); // Lab4 replaces this fake time-slice boundary.
    w_stimecmp(r_time() + 1000000);
    return 2;
  }
  return 0;
}
