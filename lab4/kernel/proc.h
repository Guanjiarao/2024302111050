#ifndef PROC_H
#define PROC_H

#include "types.h"
#include "spinlock.h"

struct context {
  uint64 ra;
  uint64 sp;
  uint64 s0;
  uint64 s1;
  uint64 s2;
  uint64 s3;
  uint64 s4;
  uint64 s5;
  uint64 s6;
  uint64 s7;
  uint64 s8;
  uint64 s9;
  uint64 s10;
  uint64 s11;
};

struct trapframe {
  uint64 kernel_satp, kernel_sp, kernel_trap, epc, kernel_hartid;
  uint64 ra, sp, gp, tp, t0, t1, t2, s0, s1;
  uint64 a0, a1, a2, a3, a4, a5, a6, a7;
  uint64 s2, s3, s4, s5, s6, s7, s8, s9, s10, s11;
  uint64 t3, t4, t5, t6;
};

enum procstate { UNUSED, USED, RUNNABLE, RUNNING, SLEEPING, ZOMBIE };
enum mlfq_level { MLFQ_HIGH, MLFQ_LOW };

struct proc {
  struct spinlock lock;
  enum procstate state;
  int pid;
  int xstate;
  int killed;
  int cpu_ticks;
  int last_slice_ticks;
  int tickets;
  int mlfq_level;
  void *chan;
  struct proc *parent;
  struct trapframe *trapframe;
  uint64 *pagetable;
  uint64 sz;
  uint64 heap_base;
  uint64 guard_va;
  uint64 stack_base;
  uint64 kstack;
  struct context context;
};

struct cpu {
  struct proc *proc;
  struct context context;
  int noff;
  int intena;
};

#endif
