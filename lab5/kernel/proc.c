#include "types.h"
#include "memlayout.h"
#include "riscv.h"
#include "course_sid.h"
#include "param.h"
#include "proc.h"
#include "defs.h"

struct spinlock wait_lock;
struct spinlock tickslock;
uint ticks;
int proc_ready;

static struct proc procs[NPROC];
static struct proc *initproc;
static int nextpid = 1;
static int rr_idx = -1;

struct uprog {
  uint64 start;
  uint64 end;
  uint64 memsz;
  uint64 text_end;
  uint64 data_start;
  char name[];
};

extern char _uprog_table[];
extern char trampoline[];

static void
memzero(void *dst, uint64 n)
{
  uchar *p = dst;
  while (n--)
    *p++ = 0;
}

static void
memcopy(void *dst, const void *src, uint64 n)
{
  uchar *d = dst;
  const uchar *s = src;
  while (n--)
    *d++ = *s++;
}

static int
nameeq(const char *a, const char *b)
{
  while (*a && *a == *b) { a++; b++; }
  return *b == 0 && (*a == 0 || (*a == '\n' && a[1] == 0) ||
                     (*a == '\r' && a[1] == 0));
}

static struct uprog *
findprog(const char *name)
{
  struct uprog *p = (struct uprog *)_uprog_table;
  while (p->start) {
    if (nameeq(name, p->name))
      return p;
    uint64 next = (uint64)p->name;
    while (*(char *)next)
      next++;
    p = (struct uprog *)((next + 8) & ~7L);
  }
  return 0;
}

static uint64 *
proc_pagetable(struct trapframe *trapframe)
{
  uint64 *pagetable = kalloc();
  if (!pagetable)
    return 0;
  if (mappages(pagetable, TRAMPOLINE, PGSIZE, (uint64)trampoline,
               PTE_R | PTE_X) < 0 ||
      mappages(pagetable, TRAPFRAME, PGSIZE, (uint64)trapframe,
               PTE_R | PTE_W) < 0) {
    uvmfree(pagetable);
    return 0;
  }
  return pagetable;
}

static void
freeproc(struct proc *p)
{
  if (p->pagetable)
    uvmfree(p->pagetable);
  if (p->trapframe)
    kfree(p->trapframe);
  if (p->kstack)
    kfree((void *)p->kstack);
  p->state = UNUSED;
  p->pid = 0;
  p->xstate = 0;
  p->killed = 0;
  p->cpu_ticks = 0;
  p->last_slice_ticks = 0;
  p->tickets = 0;
  p->mlfq_level = MLFQ_HIGH;
  p->chan = 0;
  p->parent = 0;
  p->trapframe = 0;
  p->pagetable = 0;
  p->sz = 0;
  p->heap_base = 0;
  p->guard_va = 0;
  p->stack_base = 0;
  p->kstack = 0;
  memzero(&p->context, sizeof(p->context));
}

static struct proc *
allocproc(void)
{
  struct proc *p;
  for (p = procs; p < &procs[NPROC]; p++) {
    acquire(&p->lock);
    if (p->state == UNUSED)
      break;
    release(&p->lock);
  }
  if (p == &procs[NPROC])
    return 0;

  p->state = USED;
  p->pid = nextpid++;
  p->xstate = 0;
  p->killed = 0;
  p->cpu_ticks = 0;
  p->last_slice_ticks = 0;
  p->tickets = 18;
  p->mlfq_level = MLFQ_HIGH;
  p->chan = 0;
  p->parent = 0;
  p->sz = 0;
  p->heap_base = 0;
  p->guard_va = 0;
  p->stack_base = 0;
  p->kstack = (uint64)kalloc();
  if (!p->kstack)
    goto bad;
  p->trapframe = kalloc();
  if (!p->trapframe)
    goto bad;
  p->pagetable = proc_pagetable(p->trapframe);
  if (!p->pagetable)
    goto bad;
  memzero(&p->context, sizeof(p->context));
  p->context.ra = (uint64)forkret;
  p->context.sp = p->kstack + PGSIZE;
  return p;

bad:
  freeproc(p);
  release(&p->lock);
  return 0;
}

static int
load_image(uint64 *pagetable, struct uprog *u)
{
  uint64 filesz = u->end - u->start;
  if (u->end < u->start || u->text_end != u->data_start ||
      (u->data_start & (PGSIZE - 1)) || u->data_start > u->memsz ||
      filesz > u->memsz || u->memsz >= TRAPFRAME - 2 * PGSIZE)
    return -1;
  if (uvmalloc(pagetable, 0, u->data_start,
               PTE_R | PTE_X | PTE_U) < 0 ||
      uvmalloc(pagetable, u->data_start, u->memsz,
               PTE_R | PTE_W | PTE_U) < 0)
    return -1;
  for (uint64 off = 0; off < filesz;) {
    uint64 va = off;
    pte_t *pte = walk(pagetable, va, 0);
    if (!pte || !(*pte & PTE_V))
      return -1;
    uint64 n = PGSIZE - (va & (PGSIZE - 1));
    if (n > filesz - off)
      n = filesz - off;
    memcopy((void *)(PTE2PA(*pte) + (va & (PGSIZE - 1))),
            (void *)(u->start + off), n);
    off += n;
  }
  return 0;
}

int
proc_exec(const char *name)
{
  struct uprog *u = findprog(name);
  if (!u)
    return -1;
  struct proc *p = myproc();
  struct trapframe *newtf = kalloc();
  if (!newtf)
    return -1;
  uint64 *newpt = proc_pagetable(newtf);
  if (!newpt) {
    kfree(newtf);
    return -1;
  }
  if (load_image(newpt, u) < 0) {
    uvmfree(newpt);
    kfree(newtf);
    return -1;
  }

  uint64 image_end = PGROUNDUP(u->memsz);
  uint64 guard_va = image_end;
  uint64 stack_base = guard_va + LAB3_GUARD_PAGES * PGSIZE;
  uint64 new_sz = stack_base + PGSIZE;
  if (new_sz >= TRAPFRAME ||
      uvmalloc(newpt, stack_base, new_sz,
               PTE_R | PTE_W | PTE_U) < 0) {
    uvmfree(newpt);
    kfree(newtf);
    return -1;
  }

  uint64 *oldpt = p->pagetable;
  struct trapframe *oldtf = p->trapframe;
  p->pagetable = newpt;
  p->trapframe = newtf;
  p->sz = new_sz;
  p->heap_base = new_sz;
  p->guard_va = guard_va;
  p->stack_base = stack_base;
  memzero(p->trapframe, sizeof(*p->trapframe));
  p->trapframe->epc = 0;
  p->trapframe->sp = new_sz;
  if (oldpt)
    uvmfree(oldpt);
  if (oldtf)
    kfree(oldtf);
  return 0;
}

int
procinit(void)
{
  initlock(&wait_lock, "wait");
  initlock(&tickslock, "ticks");
  ticks = 0;
  proc_ready = 0;
  for (int i = 0; i < NPROC; i++) {
    initlock(&procs[i].lock, "proc");
    procs[i].state = UNUSED;
  }
  struct proc *p = allocproc();
  if (!p)
    return -1;
  initproc = p;
  mycpu()->proc = p;
  if (proc_exec("sh") < 0) {
    mycpu()->proc = 0;
    freeproc(p);
    release(&p->lock);
    return -1;
  }
  mycpu()->proc = 0;
  dump_pagetable(p->pagetable);
  p->state = RUNNABLE;
  proc_ready = 1;
  release(&p->lock);
  return 0;
}

void
scheduler(void)
{
  struct cpu *c = mycpu();
  for (;;) {
    intr_on();
    int best = -1;
    for (int level = MLFQ_HIGH; level <= MLFQ_LOW && best < 0; level++) {
      for (int step = 1; step <= NPROC; step++) {
        int i = (rr_idx + step) % NPROC;
        struct proc *candidate = &procs[i];
        acquire(&candidate->lock);
        int runnable = candidate->state == RUNNABLE &&
                       candidate->mlfq_level == level;
        release(&candidate->lock);
        if (runnable) {
          best = i;
          break;
        }
      }
    }
    if (best < 0) {
      intr_on();
      asm volatile("wfi");
      continue;
    }
    struct proc *p = &procs[best];
    acquire(&p->lock);
    if (p->state != RUNNABLE) {
      release(&p->lock);
      continue;
    }
    rr_idx = best;
    p->state = RUNNING;
    p->last_slice_ticks = 0;
    c->proc = p;
    swtch(&c->context, &p->context);
    c->proc = 0;
    release(&p->lock);
  }
}

void
sched(void)
{
  struct proc *p = myproc();
  struct cpu *c = mycpu();
  if (!holding(&p->lock))
    panic("sched p->lock");
  if (c->noff != 1)
    panic("sched locks");
  if (p->state == RUNNING)
    panic("sched running");
  if (intr_get())
    panic("sched interruptible");
  int intena = c->intena;
  swtch(&p->context, &c->context);
  c->intena = intena;
}

void
yield(void)
{
  struct proc *p = myproc();
  acquire(&p->lock);
  p->state = RUNNABLE;
  sched();
  release(&p->lock);
}

void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  if (!p || !lk)
    panic("sleep");
  if (lk != &p->lock) {
    acquire(&p->lock);
    release(lk);
  }
  p->chan = chan;
  p->state = SLEEPING;
  p->mlfq_level = MLFQ_HIGH;
  sched();
  p->chan = 0;
  if (lk != &p->lock) {
    release(&p->lock);
    acquire(lk);
  }
}

void
wakeup(void *chan)
{
  struct proc *current = myproc();
  for (struct proc *p = procs; p < &procs[NPROC]; p++) {
    if (p == current)
      continue;
    acquire(&p->lock);
    if (p->state == SLEEPING && p->chan == chan) {
      p->state = RUNNABLE;
      p->mlfq_level = MLFQ_HIGH;
    }
    release(&p->lock);
  }
}

void
forkret(void)
{
  struct proc *p = myproc();
  release(&p->lock);
  userenter();
}

int
proc_fork(void)
{
  struct proc *parent = myproc();
  acquire(&wait_lock);
  struct proc *child = allocproc();
  if (!child) {
    release(&wait_lock);
    return -1;
  }
  if (uvmcopy(parent->pagetable, child->pagetable, parent->sz) < 0) {
    freeproc(child);
    release(&child->lock);
    release(&wait_lock);
    return -1;
  }
  child->sz = parent->sz;
  child->heap_base = parent->heap_base;
  child->guard_va = parent->guard_va;
  child->stack_base = parent->stack_base;
  *child->trapframe = *parent->trapframe;
  child->trapframe->a0 = 0;
  child->parent = parent;
  child->mlfq_level = MLFQ_HIGH;
  child->state = RUNNABLE;
  int pid = child->pid;
  release(&child->lock);
  release(&wait_lock);
  return pid;
}

void
proc_exit(int status)
{
  struct proc *p = myproc();
  if (p == initproc)
    panic("init exited");

  acquire(&wait_lock);
  for (struct proc *child = procs; child < &procs[NPROC]; child++) {
    int reparented = 0;
    if (child == p)
      continue;
    acquire(&child->lock);
    if (child->parent == p) {
      child->parent = initproc;
      reparented = 1;
    }
    release(&child->lock);
    if (reparented)
      wakeup(initproc);
  }
  wakeup(p->parent);
  acquire(&p->lock);
  p->xstate = status;
  p->state = ZOMBIE;
  release(&wait_lock);
  sched();
  panic("zombie returned");
}

int
proc_wait(uint64 status)
{
  struct proc *parent = myproc();
  acquire(&wait_lock);
  for (;;) {
    int have_children = 0;
    for (struct proc *child = procs; child < &procs[NPROC]; child++) {
      if (child == parent)
        continue;
      acquire(&child->lock);
      if (child->parent != parent || child->state == UNUSED) {
        release(&child->lock);
        continue;
      }
      have_children = 1;
      if (child->state == ZOMBIE) {
        int pid = child->pid;
        int xstate = child->xstate;
        release(&child->lock);
        if (status && copyout(parent->pagetable, status, &xstate,
                              sizeof(xstate)) < 0) {
          release(&wait_lock);
          return -1;
        }
        acquire(&child->lock);
        if (child->parent == parent && child->state == ZOMBIE) {
          freeproc(child);
          release(&child->lock);
          release(&wait_lock);
          return pid;
        }
        release(&child->lock);
        break;
      }
      release(&child->lock);
    }
    if (!have_children) {
      release(&wait_lock);
      return -1;
    }
    sleep(parent, &wait_lock);
  }
}

int
proc_pause(int n)
{
  if (n < 0)
    return -1;
  acquire(&tickslock);
  uint start = ticks;
  while ((uint)(ticks - start) < (uint)n)
    sleep(&ticks, &tickslock);
  release(&tickslock);
  return 0;
}

int
proc_schedstat(int pid, int *cpu_count, int *slice_count)
{
  struct proc *self = myproc();
  if (pid == -1) {
    acquire(&self->lock);
    if (self->state == UNUSED || self->state == ZOMBIE) {
      release(&self->lock);
      return -1;
    }
    *cpu_count = self->cpu_ticks;
    *slice_count = self->last_slice_ticks;
    release(&self->lock);
    return 0;
  }
  for (struct proc *p = procs; p < &procs[NPROC]; p++) {
    acquire(&p->lock);
    if (p->pid == pid && p->state != UNUSED && p->state != ZOMBIE) {
      *cpu_count = p->cpu_ticks;
      *slice_count = p->last_slice_ticks;
      release(&p->lock);
      return 0;
    }
    release(&p->lock);
  }
  return -1;
}

uint64
proc_sbrk(int n)
{
  struct proc *p = myproc();
  uint64 oldsz = p->sz;
  uint64 newsz;
  if (n > 0) {
    newsz = oldsz + (uint64)n;
    if (newsz < oldsz || newsz >= TRAPFRAME ||
        uvmalloc(p->pagetable, oldsz, newsz,
                 PTE_R | PTE_W | PTE_U) < 0)
      return (uint64)-1;
  } else if (n < 0) {
    uint64 shrink = (uint64)(-(int)n);
    if (shrink > oldsz - p->heap_base)
      return (uint64)-1;
    newsz = oldsz - shrink;
    uvmdealloc(p->pagetable, oldsz, newsz);
  } else {
    return oldsz;
  }
  p->sz = newsz;
  return oldsz;
}
