#include "types.h"
#include "memlayout.h"
#include "proc.h"
#include "syscall.h"
#include "defs.h"

static uint64 argraw(int n)
{
  struct trapframe *t = &myproc()->trapframe;
  uint64 args[] = { t->a0, t->a1, t->a2, t->a3, t->a4, t->a5 };
  return n >= 0 && n < 6 ? args[n] : 0;
}
static uint64 sys_fork(void) { return proc_fork(); }
static uint64 sys_exit(void) { proc_exit((int)argraw(0)); }
static uint64 sys_wait(void) { return proc_wait(argraw(0)); }
static uint64 sys_read(void) {
  if ((int)argraw(0) != 0) return -1;
  return consoleread(argraw(1), (int)argraw(2));
}
static uint64 sys_write(void) {
  if ((int)argraw(0) != 1 && (int)argraw(0) != 2) return -1;
  return consolewrite(argraw(1), (int)argraw(2));
}
static uint64 sys_exec(void) {
  uint64 p = argraw(0);
  if (!proc_valid_range(p, 1)) return -1;
  uint64 end = myproc()->base + LAB2_USER_SLOT_SIZE;
  uint64 q = p;
  while (q < end && *(char *)q) q++;
  if (q == end) return -1;
  return proc_exec((const char *)p);
}
static uint64 sys_getpid(void) { return myproc()->pid; }

void syscall(void)
{
  struct proc *caller = myproc();
  int n = caller->trapframe.a7;
  uint64 ret = -1;
  switch (n) {
  case SYS_fork: ret = sys_fork(); break;
  case SYS_exit: ret = sys_exit(); break;
  case SYS_wait: ret = sys_wait(); break;
  case SYS_read: ret = sys_read(); break;
  case SYS_write: ret = sys_write(); break;
  case SYS_exec: ret = sys_exec(); break;
  case SYS_getpid: ret = sys_getpid(); break;
  default: ret = -1; break;
  }
  caller->trapframe.a0 = ret;
}
