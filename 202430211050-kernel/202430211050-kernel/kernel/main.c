#include "types.h"
#include "course_sid.h"

#define STRINGIFY_INNER(x) #x
#define STRINGIFY(x) STRINGIFY_INNER(x)

void consoleinit(void);
uint console_checksum(void);
void printf(const char *fmt, ...);

void
main(void)
{
  consoleinit();

  printf("OSLAB1 sid=%s mod97=%x\n", STRINGIFY(COURSE_SID),
         (uint)(COURSE_SID % 97));

#if LAB1_BANNER_PROTOCOL == 2
  uint checksum = console_checksum();
  printf("[chk=%d]\n", checksum);
#endif

  for (;;)
    asm volatile("wfi");
}
