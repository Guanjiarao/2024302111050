#include "types.h"
#include "memlayout.h"
#include "riscv.h"
#include "course_sid.h"
#include "proc.h"
#include "defs.h"

extern char uservec[], userret[], trampoline[];
extern char boot_stack[];
void kernelvec(void);

static uint timer_ticks;

// stvec 保存 S 态 trap 的入口地址。内核态发生 trap 时先进入 kernelvec。
void trapinit(void) { w_stvec((uint64)kernelvec); }


//准备返回用户态
static void prepare_return(void)
{
  struct proc *p = myproc();

  intr_off();

  p->trapframe->kernel_satp = MAKE_SATP(kernel_pagetable);
  p->trapframe->kernel_sp = (uint64)boot_stack + LAB1_STACK_KB * 1024;
  p->trapframe->kernel_trap = (uint64)usertrap;
  p->trapframe->kernel_hartid = 0;

  // SPP=0 表示下一次 sret 返回 U 态；SPIE=1 表示返回后允许中断。
  uint64 s = r_sstatus();
  s &= ~SSTATUS_SPP;
  s |= SSTATUS_SPIE;
  w_sstatus(s);

  // sepc 决定 sret 后用户程序从哪里继续；用户态 trap 入口改为 uservec。
  w_sepc(p->trapframe->epc);
  w_stvec(TRAMPOLINE + ((uint64)uservec - (uint64)trampoline));
}

//进入用户态
void userenter(void)
{
  prepare_return();
  // userret 恢复用户寄存器并执行 sret；参数是当前进程的 SV39 satp 值。
  uint64 userret_va = TRAMPOLINE + ((uint64)userret - (uint64)trampoline);
  ((void (*)(uint64))userret_va)(MAKE_SATP(myproc()->pagetable));
  __builtin_unreachable();
}

//处理从 U 态进入内核的陷阱
uint64 usertrap(void)
{
  // 当前已经进入内核，先把后续 S 态 trap 入口切回 kernelvec，
  // 避免内核执行期间再次发生中断却错误地走用户入口。
  w_stvec((uint64)kernelvec);
  struct proc *trapped = myproc();
  trapped->trapframe->epc = r_sepc();

  // scause 说明“为什么进入内核”。值 8 表示来自 U 态的 ecall。
  uint64 cause = r_scause();
  if (cause == 8) {
    // ecall 是 4 字节指令。返回地址必须加 4 跳过它，
    // 否则 sret 后会再次执行同一条 ecall，形成无限 trap。
    trapped->trapframe->epc += 4;
    syscall();
  } else {
    int which_dev = devintr();
    if (which_dev == 2)
      proc_yield();
    if (which_dev)
      goto usertrap_done;
    // 既不是系统调用也不是已知设备中断：打印诊断并只结束出错进程，
    // 不让一个用户程序错误拖垮整个内核。
    printf("user trap cause=%x epc=%x stval=%x\n", (uint)cause,
           (uint)r_sepc(), (uint)r_stval());
    proc_exit(-1);
  }
usertrap_done:
  prepare_return();
  return MAKE_SATP(trapped->pagetable);
}

//处理从 S 态进入内核的陷阱
void kerneltrap(void)
{
  // 内核态也可能被 UART/定时器中断。处理前保存 sepc、sstatus，
  // 因为 devintr() 及其调用链可能改变这些 CSR。
  uint64 epc = r_sepc();
  uint64 status = r_sstatus();
  if (!devintr()) panic("kernel trap");
  w_sepc(epc);
  w_sstatus(status);
}

//处理中断
int devintr(void)
{
  uint64 cause = r_scause();

  // 最高位为 1 表示 interrupt；低位 9 表示 S 态外部中断。
  if (cause == 0x8000000000000009L) {
    // 向 PLIC claim 当前 IRQ；若为 UART IRQ 10，则读取所有已到达字符；
    // 处理结束后 complete，告诉 PLIC 可以继续投递后续中断。
    int irq = plic_claim();
    if (irq == UART0_IRQ) uartintr();
    if (irq) plic_complete(irq);
    return 1;
  }
  // 低位 5 表示 S 态时钟中断。
  if (cause == 0x8000000000000005L) {
    timer_ticks++;
    // 当前闹钟已经响过，重新预约“当前时间 + 1000000”的下一次中断。
    w_stimecmp(r_time() + 1000000);
    // LAB2_TICK 决定轮转时间片跨越几个硬件定时器 tick。
    return timer_ticks % LAB2_TICK == 0 ? 2 : 1;
  }
  return 0;
}
