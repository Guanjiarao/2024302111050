#include "types.h"
#include "course_sid.h"
#include "riscv.h"
#include "defs.h"

#define STRINGIFY_INNER(x) #x
#define STRINGIFY(x) STRINGIFY_INNER(x)

void
main(void)
{
  consoleinit();

  printf("OSLAB1 sid=%s mod97=%x\n", STRINGIFY(COURSE_SID),
         (uint)(COURSE_SID % 97));

  trapinit();
  plicinit();
  plicinithart();
  if (procinit() < 0)
    panic("cannot load sh");
  intr_on();
  userenter();
}

void panic(const char *msg)
{
  intr_off();
  printf("panic: %s\n", msg);
  for (;;) asm volatile("wfi");
}
