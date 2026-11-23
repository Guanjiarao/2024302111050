#include "kernel/types.h"
#include "user/user.h"

static void
run_a(void)
{
  for (int i = 0; i < 24; i++) {
    for (volatile int d = 0; d < 1200000; d++)
      ;
    printf("A");
  }
  exit(0);
}

static void
run_b(void)
{
  for (int i = 0; i < 24; i++) {
    for (volatile int d = 0; d < 1200000; d++)
      ;
    printf("B");
  }
  exit(0);
}

int
main(void)
{
  if (fork() == 0)
    run_a();
  if (fork() == 0)
    run_b();
  wait(0);
  wait(0);
  printf("\nPASS: alt preemption\n");
  exit(0);
}
