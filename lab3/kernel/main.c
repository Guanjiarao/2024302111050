#include "types.h"
#include "course_sid.h"
#include "riscv.h"
#include "defs.h"

#define STRINGIFY_INNER(x) #x
#define STRINGIFY(x) STRINGIFY_INNER(x)

void
main(void)
{
  // 初始化控制台软件层，并在其中初始化底层 UART 串口硬件。
  consoleinit();

  printf("OSLAB1 sid=%s mod97=%x\n", STRINGIFY(COURSE_SID),
         (uint)(COURSE_SID % 97));

  kinit();
  kvminit();
  kvminithart();

  trapinit();
  plicinit();
  plicinithart();

  if (procinit() < 0)
    panic("cannot load sh");

  // 打开 S 态中断“总开关”。
  intr_on();

  // 准备当前 sh 进程的用户上下文，经 userret/sret 进入 U 态执行 shell。
  userenter();
}

void panic(const char *msg)
{
  intr_off();
  printf("panic: %s\n", msg);
  for (;;) asm volatile("wfi");
}
