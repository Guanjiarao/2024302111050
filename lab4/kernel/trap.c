#include "types.h"
#include "memlayout.h"
#include "riscv.h"
#include "course_sid.h"
#include "proc.h"
#include "defs.h"

extern char uservec[], userret[], trampoline[];
void kernelvec(void);

void
trapinit(void)
{
  w_stvec((uint64)kernelvec);
}

static void
prepare_return(void)
{
  struct proc *p = myproc();
  intr_off();
  p->trapframe->kernel_satp = MAKE_SATP(kernel_pagetable);
  p->trapframe->kernel_sp = p->kstack + PGSIZE;
  p->trapframe->kernel_trap = (uint64)usertrap;
  p->trapframe->kernel_hartid = cpuid();

  uint64 s = r_sstatus();
  s &= ~SSTATUS_SPP;
  s |= SSTATUS_SPIE;
  w_sstatus(s);
  w_sepc(p->trapframe->epc);
  w_stvec(TRAMPOLINE + ((uint64)uservec - (uint64)trampoline));
}

void
userenter(void)
{
  prepare_return();
  uint64 userret_va = TRAMPOLINE + ((uint64)userret - (uint64)trampoline);
  ((void (*)(uint64))userret_va)(MAKE_SATP(myproc()->pagetable));
  __builtin_unreachable();
}

uint64
usertrap(void)
{
  w_stvec((uint64)kernelvec);
  struct proc *p = myproc();
  p->trapframe->epc = r_sepc();
  uint64 cause = r_scause();
  if (cause == 8) {
    p->trapframe->epc += 4;
    syscall();
  } else {
    int which_dev = devintr();
    if (which_dev == 2) {
      acquire(&p->lock);
      p->cpu_ticks++;
      p->last_slice_ticks++;
      int quantum = p->mlfq_level == MLFQ_HIGH
                      ? LAB2_TICK : 2 * LAB2_TICK;
      int expired = p->last_slice_ticks >= quantum;
      if (expired && p->mlfq_level == MLFQ_HIGH)
        p->mlfq_level = MLFQ_LOW;
      release(&p->lock);
      if (expired)
        yield();
    } else if (which_dev == 0) {
      printf("user trap cause=%x epc=%x stval=%x\n", (uint)cause,
             (uint)r_sepc(), (uint)r_stval());
      proc_exit(-1);
    }
  }
  prepare_return();
  return MAKE_SATP(myproc()->pagetable);
}

void
kerneltrap(void)
{
  uint64 epc = r_sepc();
  uint64 status = r_sstatus();
  if (!devintr()) {
    printf("kernel trap scause=%x sepc=%x stval=%x\n",
           (uint)r_scause(), (uint)epc, (uint)r_stval());
    panic("kernel trap");
  }
  w_sepc(epc);
  w_sstatus(status);
}

static void
clockintr(void)
{
  if (proc_ready) {
    acquire(&tickslock);
    ticks++;
    wakeup(&ticks);
    release(&tickslock);
  }
  w_stimecmp(r_time() + 1000000);
}

int
devintr(void)
{
  uint64 cause = r_scause();
  if (cause == 0x8000000000000009L) {
    int irq = plic_claim();
    if (irq == UART0_IRQ)
      uartintr();
    if (irq)
      plic_complete(irq);
    return 1;
  }
  if (cause == 0x8000000000000005L) {
    clockintr();
    return 2;
  }
  return 0;
}
