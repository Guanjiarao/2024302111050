#include "common.h"

static void
cpu_child(unsigned long dl)
{
  burn_until(dl);
  exit(0);
}

static void
interactive_child(unsigned long dl)
{
  while (time_rdtime() < dl) {
    for (volatile int i = 0; i < 20000; i++)
      ;
    pause(1);
  }
  exit(0);
}

int
main(void)
{
  unsigned long dl = deadline_after(70);
  int cpu1 = fork();
  if (cpu1 == 0) cpu_child(dl);
  int cpu2 = fork();
  if (cpu2 == 0) cpu_child(dl);
  int io = fork();
  if (io == 0) interactive_child(dl);
  ASSERT(cpu1 > 0 && cpu2 > 0 && io > 0, "fork failed");

  int saw3 = 0, saw6 = 0, io_max = 0;
  for (int sample = 0; sample < 50; sample++) {
    pause(1);
    int cpu, slice;
    if (schedstat(cpu1, &cpu, &slice) == 0) {
      if (slice == 3) saw3 = 1;
      if (slice == 6) saw6 = 1;
    }
    if (schedstat(cpu2, &cpu, &slice) == 0) {
      if (slice == 3) saw3 = 1;
      if (slice == 6) saw6 = 1;
    }
    if (schedstat(io, &cpu, &slice) == 0 && slice > io_max)
      io_max = slice;
  }

  wait(0);
  wait(0);
  wait(0);
  printf("RESULT mlfq_slices = high:%d low:%d interactive_max:%d\n",
         saw3 ? 3 : 0, saw6 ? 6 : 0, io_max);
  ASSERT(saw3, "missing 3-tick high-priority slice");
  ASSERT(saw6, "missing 6-tick low-priority slice");
  ASSERT(io_max < 3, "interactive process exhausted a high-priority slice");
  PASS("mlfq");
  return 0;
}
