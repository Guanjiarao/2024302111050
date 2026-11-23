#include "kernel/types.h"
#include "user/user.h"

extern char end[];

int
main(void)
{
  int pid = fork();
  if (pid == 0) {
    uint64 guard = ((uint64)end + 4095) & ~4095UL;
    *(volatile int *)guard = 1;
    exit(0);
  }
  int status = 0;
  wait(&status);
  if (status != 0)
    printf("GUARD PASS\n");
  else
    printf("GUARD FAIL\n");
  exit(0);
}
