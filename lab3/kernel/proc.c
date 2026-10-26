#include "types.h"
#include "memlayout.h"
#include "riscv.h"
#include "course_sid.h"
#include "proc.h"
#include "defs.h"

#define NPROC_LAB3 4

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

static struct proc procs[NPROC_LAB3];
static struct proc *current;
static int nextpid = 1;

static struct proc *
next_runnable(struct proc *after)
{
  int start = (int)(after - procs);
  for (int step = 1; step <= NPROC_LAB3; step++) {
    struct proc *p = &procs[(start + step) % NPROC_LAB3];
    if (p->state == RUNNABLE)
      return p;
  }
  return 0;
}

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

struct proc *
myproc(void)
{
  return current;
}

static uint64 *
proc_pagetable(uint64 *trapframe)
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
proc_free_address_space(struct proc *p)
{
  if (p->pagetable)
    uvmfree(p->pagetable);
  if (p->trapframe)
    kfree(p->trapframe);
  p->pagetable = 0;
  p->trapframe = 0;
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

  uint64 *newtf = kalloc();
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

  struct proc *p = current;
  uint64 *oldpt = p->pagetable;
  struct trapframe *oldtf = p->trapframe;
  p->pagetable = newpt;
  p->trapframe = (struct trapframe *)newtf;
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
  for (int i = 0; i < NPROC_LAB3; i++) {
    memzero(&procs[i], sizeof(procs[i]));
    procs[i].state = UNUSED;
  }
  current = &procs[0];
  current->state = RUNNING;
  current->pid = nextpid++;
  current->trapframe = kalloc();
  if (!current->trapframe)
    return -1;
  if (proc_exec("sh") < 0) {
    proc_free_address_space(current);
    return -1;
  }
  dump_pagetable(current->pagetable);
  return 0;
}

int
proc_fork(void)
{
  struct proc *parent = current;
  struct proc *child = 0;
  for (int i = 0; i < NPROC_LAB3; i++)
    if (procs[i].state == UNUSED) { child = &procs[i]; break; }
  if (!child) {
    return -1;
  }

  uint64 *childtf = kalloc();
  if (!childtf) {
    return -1;
  }
  uint64 *childpt = proc_pagetable(childtf);
  if (!childpt) {
    kfree(childtf);
    return -1;
  }
  if (uvmcopy(parent->pagetable, childpt, parent->sz) < 0) {
    uvmfree(childpt);
    kfree(childtf);
    return -1;
  }

  memzero(child, sizeof(*child));
  child->state = RUNNING;
  child->pid = nextpid++;
  child->parent = parent;
  child->pagetable = childpt;
  child->trapframe = (struct trapframe *)childtf;
  child->sz = parent->sz;
  child->heap_base = parent->heap_base;
  child->guard_va = parent->guard_va;
  child->stack_base = parent->stack_base;
  *child->trapframe = *parent->trapframe;
  child->trapframe->a0 = 0;

  parent->state = RUNNABLE;
  current = child;
  parent->trapframe->a0 = child->pid;
  userenter();
}

void
proc_yield(void)
{
  struct proc *p = current;
  p->state = RUNNABLE;
  struct proc *next = next_runnable(p);
  if (!next) {
    p->state = RUNNING;
    return;
  }
  next->state = RUNNING;
  current = next;
  userenter();
}

int
proc_yield_read(void)
{
  struct proc *p = current;
  struct proc *next = next_runnable(p);
  if (!next)
    return 0;
  p->trapframe->epc -= 4;
  p->state = RUNNABLE;
  next->state = RUNNING;
  current = next;
  userenter();
}

void
proc_exit(int status)
{
  struct proc *dead = current;
  dead->xstate = status;
  if (!dead->parent)
    panic("init exited");

  struct proc *parent = dead->parent;
  proc_free_address_space(dead);
  if (parent->state == SLEEPING) {
    int result = dead->pid;
    if (parent->wait_status &&
        copyout(parent->pagetable, parent->wait_status,
                &dead->xstate, sizeof(dead->xstate)) < 0)
      result = -1;
    memzero(dead, sizeof(*dead));
    dead->state = UNUSED;
    parent->wait_status = 0;
    parent->trapframe->a0 = result;
    parent->state = RUNNING;
    current = parent;
    userenter();
  }

  dead->state = ZOMBIE;
  if (parent->state == RUNNABLE) {
    parent->state = RUNNING;
    current = parent;
    userenter();
  }
  struct proc *next = next_runnable(dead);
  if (!next)
    panic("no runnable process");
  next->state = RUNNING;
  current = next;
  userenter();
}

int
proc_wait(uint64 status)
{
  struct proc *parent = current;
  int have_child = 0;
  for (int i = 0; i < NPROC_LAB3; i++) {
    struct proc *child = &procs[i];
    if (child->parent != parent || child->state == UNUSED)
      continue;
    have_child = 1;
    if (child->state != ZOMBIE)
      continue;
    int xstate = child->xstate;
    int pid = child->pid;
    if (status && copyout(parent->pagetable, status, &xstate, sizeof(xstate)) < 0)
      return -1;
    memzero(child, sizeof(*child));
    child->state = UNUSED;
    return pid;
  }
  if (!have_child)
    return -1;

  parent->wait_status = status;
  parent->state = SLEEPING;
  struct proc *next = next_runnable(parent);
  if (!next) {
    parent->state = RUNNING;
    parent->wait_status = 0;
    return -1;
  }
  next->state = RUNNING;
  current = next;
  userenter();
}

uint64
proc_sbrk(int n)
{
  struct proc *p = current;
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
