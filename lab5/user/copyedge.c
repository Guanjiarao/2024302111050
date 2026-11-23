#include "kernel/types.h"
#include "user/user.h"

#define PGSIZE 4096

int
main(void)
{
  char *base = sbrk(3 * PGSIZE);
  if (base == (char *)-1) {
    printf("COPYEDGE FAIL: sbrk\n");
    exit(1);
  }
  uint64 boundary = ((uint64)base + PGSIZE - 1) & ~(PGSIZE - 1);
  char *cross = (char *)(boundary + PGSIZE - 4);
  const char expected[] = "ABCDEFGH";
  for (int i = 0; i < 8; i++)
    cross[i] = expected[i];
  if (write(1, cross, 8) == 8)
    printf("\nCOPYEDGE PASS\n");
  else
    printf("\nCOPYEDGE FAIL: write\n");
  exit(0);
}
