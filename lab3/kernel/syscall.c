#include "types.h"
#include "memlayout.h"
#include "proc.h"
#include "syscall.h"
#include "defs.h"

static uint64 argraw(int n)
{
  struct trapframe *t = myproc()->trapframe;
  // RISC-V ABI 规定前 6 个函数/系统调用参数位于 a0-a5。
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
  char name[64];
  if (copyinstr(myproc()->pagetable, name, argraw(0), sizeof(name)) < 0)
    return -1;
  return proc_exec(name);
}
static uint64 sys_getpid(void) { return myproc()->pid; }
static uint64 sys_sbrk(void) { return proc_sbrk((int)argraw(0)); }
static uint64 sys_dumppt(void) { dump_pagetable(myproc()->pagetable); return 0; }

void syscall(void)
{
  struct proc *caller = myproc();
  // 用户系统调用桩把调用号放在 a7，参数放在 a0-a5。
  int n = caller->trapframe->a7;
  uint64 ret = -1;
  switch (n) {
  case SYS_fork: ret = sys_fork(); break;
  case SYS_exit: ret = sys_exit(); break;
  case SYS_wait: ret = sys_wait(); break;
  case SYS_read: ret = sys_read(); break;
  case SYS_write: ret = sys_write(); break;
  case SYS_exec: ret = sys_exec(); break;
  case SYS_getpid: ret = sys_getpid(); break;
  case SYS_sbrk: ret = sys_sbrk(); break;
  case SYS_dumppt: ret = sys_dumppt(); break;
  // 未实现或非法的调用号只返回 -1，不允许用户错误导致内核 panic。
  default: ret = -1; break;
  }
  // RISC-V ABI 使用 a0 传回返回值；userret 恢复寄存器后用户函数即可得到它。
  caller->trapframe->a0 = ret;
}
