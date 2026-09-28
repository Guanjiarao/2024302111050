struct trapframe {
  uint64 kernel_satp, kernel_sp, kernel_trap, epc, kernel_hartid;
  uint64 ra, sp, gp, tp, t0, t1, t2, s0, s1;
  uint64 a0, a1, a2, a3, a4, a5, a6, a7;
  uint64 s2, s3, s4, s5, s6, s7, s8, s9, s10, s11;
  uint64 t3, t4, t5, t6;
};

enum procstate { UNUSED, RUNNING, ZOMBIE };
struct proc {
  enum procstate state;
  int pid;
  int xstate;
  struct proc *parent;
  uint64 base;
  uint64 size;
  struct trapframe trapframe;
};
