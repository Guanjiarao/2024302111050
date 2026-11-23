#include "types.h"
#include "riscv.h"
#include "param.h"
#include "proc.h"
#include "defs.h"

struct cpu cpus[NCPU];

int
cpuid(void)
{
  return (int)r_tp();
}

struct cpu *
mycpu(void)
{
  return &cpus[cpuid()];
}

struct proc *
myproc(void)
{
  struct proc *p;
  push_off();
  p = mycpu()->proc;
  pop_off();
  return p;
}

void
initlock(struct spinlock *lk, const char *name)
{
  lk->name = name;
  lk->locked = 0;
  lk->cpu = 0;
}

int
holding(struct spinlock *lk)
{
  return lk->locked && lk->cpu == mycpu();
}

void
push_off(void)
{
  int enabled = intr_get();
  intr_off();
  struct cpu *c = mycpu();
  if (c->noff == 0)
    c->intena = enabled;
  c->noff++;
}

void
pop_off(void)
{
  struct cpu *c = mycpu();
  if (intr_get())
    panic("pop_off - interruptible");
  if (c->noff < 1)
    panic("pop_off");
  c->noff--;
  if (c->noff == 0 && c->intena)
    intr_on();
}

void
acquire(struct spinlock *lk)
{
  push_off();
  if (holding(lk))
    panic("acquire");
  while (__sync_lock_test_and_set(&lk->locked, 1) != 0)
    ;
  __sync_synchronize();
  lk->cpu = mycpu();
}

void
release(struct spinlock *lk)
{
  if (!holding(lk))
    panic("release");
  lk->cpu = 0;
  __sync_synchronize();
  __sync_lock_release(&lk->locked);
  pop_off();
}
