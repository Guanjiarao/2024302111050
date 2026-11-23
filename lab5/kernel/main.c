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

  scheduler();
}

void panic(const char *msg)
{
  intr_off();
  printf("panic: %s\n", msg);
  for (;;) asm volatile("wfi");
}
