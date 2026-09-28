#include "types.h"
#include "memlayout.h"
#include "proc.h"
#include "defs.h"

#define NPROC_LAB2 4

struct uprog { uint64 start, end; char name[]; };
extern char _uprog_table[];
static struct proc procs[NPROC_LAB2];
static struct proc *current;
static int nextpid = 1;

static void memzero(void *dst, uint64 n)
{
  for (uint64 i = 0; i < n; i++) ((char *)dst)[i] = 0;
}

static void memcopy(void *dst, const void *src, uint64 n)
{
  for (uint64 i = 0; i < n; i++) ((char *)dst)[i] = ((const char *)src)[i];
}

static int nameeq(const char *a, const char *b)
{
  while (*a && *a == *b) { a++; b++; }
  return *b == 0 && (*a == 0 || (*a == '\n' && a[1] == 0) ||
                     (*a == '\r' && a[1] == 0));
}

static struct uprog *findprog(const char *name)
{
  struct uprog *p = (struct uprog *)_uprog_table;
  while (p->start) {
    if (nameeq(name, p->name)) return p;
    uint64 next = (uint64)p->name;
    while (*(char *)next) next++;
    p = (struct uprog *)((next + 8) & ~7L);
  }
  return 0;
}

struct proc *myproc(void) { return current; }

int proc_valid_range(uint64 addr, uint64 len)
{
  if (!current || addr < current->base || addr + len < addr)
    return 0;
  return addr + len <= current->base + LAB2_USER_SLOT_SIZE;
}

int proc_exec(const char *name)
{
  struct uprog *u = findprog(name);
  if (!u || u->end < u->start || u->end - u->start > LAB2_USER_SLOT_SIZE - 4096)
    return -1;
  memzero((void *)current->base, LAB2_USER_SLOT_SIZE);
  current->size = u->end - u->start;
  memcopy((void *)current->base, (void *)u->start, current->size);
  memzero(&current->trapframe, sizeof(current->trapframe));
  current->trapframe.epc = current->base;
  current->trapframe.sp = current->base + LAB2_USER_SLOT_SIZE - 16;
  return 0;
}

int procinit(void)
{
  for (int i = 0; i < NPROC_LAB2; i++) {
    procs[i].state = UNUSED;
    procs[i].base = LAB2_USER_BASE + i * LAB2_USER_SLOT_SIZE;
  }
  current = &procs[0];
  current->state = RUNNING;
  current->pid = nextpid++;
  return proc_exec("sh");
}

int proc_fork(void)
{
  struct proc *child = 0;
  for (int i = 0; i < NPROC_LAB2; i++)
    if (procs[i].state == UNUSED) { child = &procs[i]; break; }
  if (!child) return -1;
  child->state = RUNNING;
  child->pid = nextpid++;
  child->parent = current;
  child->size = current->size;
  memcopy((void *)child->base, (void *)current->base, LAB2_USER_SLOT_SIZE);
  child->trapframe = current->trapframe;
  uint64 delta = child->base - current->base;
  child->trapframe.epc += delta;
  uint64 *reg = &child->trapframe.ra;
  uint64 *last = &child->trapframe.t6;
  for (; reg <= last; reg++)
    if (*reg >= current->base &&
        *reg < current->base + LAB2_USER_SLOT_SIZE)
      *reg += delta;
  child->trapframe.a0 = 0;
  int pid = child->pid;
  current = child;
  return pid;
}

void proc_exit(int status)
{
  struct proc *dead = current;
  dead->xstate = status;
  dead->state = ZOMBIE;
  if (!dead->parent)
    panic("init exited");
  current = dead->parent;
  userenter();
}

int proc_wait(uint64 status)
{
  if (status && !proc_valid_range(status, sizeof(int)))
    return -1;
  for (int i = 0; i < NPROC_LAB2; i++) {
    if (procs[i].parent == current && procs[i].state == ZOMBIE) {
      int pid = procs[i].pid;
      if (status)
        *(int *)status = procs[i].xstate;
      procs[i].state = UNUSED;
      procs[i].parent = 0;
      return pid;
    }
  }
  return -1;
}
