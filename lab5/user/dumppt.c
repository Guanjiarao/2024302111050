#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  printf("DUMPPT COMMAND\n");
  dumppt();
  exit(0);
}
