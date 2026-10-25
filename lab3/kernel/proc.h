struct trapframe {
  // 返回用户态前由内核填写：内核页表、内核栈、C 级 trap 处理函数、
  // 用户返回地址 epc，以及当前 hart 编号。
  uint64 kernel_satp, kernel_sp, kernel_trap, epc, kernel_hartid;

  // 用户态被 trap 打断时保存的通用寄存器。恢复这些寄存器后，
  // 用户程序才能从被打断处继续运行。a0-a5 还用于参数，a7 用于系统调用号。
  uint64 ra, sp, gp, tp, t0, t1, t2, s0, s1;
  uint64 a0, a1, a2, a3, a4, a5, a6, a7;
  uint64 s2, s3, s4, s5, s6, s7, s8, s9, s10, s11;
  uint64 t3, t4, t5, t6;
};

enum procstate { UNUSED, RUNNABLE, RUNNING, SLEEPING, ZOMBIE };
struct proc {
  // Process identity and parent/exit state.
  enum procstate state;
  int pid;
  int xstate;
  struct proc *parent;
  // User memory and saved execution context are owned by this process.
  struct trapframe *trapframe;
  uint64 *pagetable;
  uint64 sz;
  uint64 heap_base;
  uint64 guard_va;
  uint64 stack_base;
  uint64 wait_status;
};
