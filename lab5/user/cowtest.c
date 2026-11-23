#include "kernel/types.h"
#include "kernel/course_sid.h"
#include "kernel/param.h"
#include "user/user.h"

static int test_many_children(char *page);
static int test_readonly_fault(void);
static int test_bad_exec_argv(void);
static int test_budget(void);

int
main(void)
{
  char *page = sbrk(4096);
  if (page == (char *)-1)
    goto fail;
  page[0] = 0x11;

  if (test_many_children(page) < 0) {
    printf("COWTEST FAIL: many children\n");
    exit(1);
  }
  if (test_readonly_fault() < 0) {
    printf("COWTEST FAIL: readonly fault\n");
    exit(1);
  }
  if (test_bad_exec_argv() < 0) {
    printf("COWTEST FAIL: bad exec argv\n");
    exit(1);
  }
  {
    char *argv[MAXARG + 2];
    argv[0] = "hi";
    for (int i = 1; i <= MAXARG; i++)
      argv[i] = "x";

    for (int argc = MAXARG; argc <= MAXARG + 1; argc++) {
      argv[argc] = 0;
      int pid = fork();
      if (pid < 0) {
        printf("COWTEST FAIL: exec arg limit\n");
        exit(1);
      }
      if (pid == 0) {
        int result = exec("hi", argv);
        exit(argc == MAXARG || result != -1 ? 5 : 0);
      }
      int status = -1;
      if (wait(&status) < 0 || status != 0) {
        printf("COWTEST FAIL: exec arg limit\n");
        exit(1);
      }
      argv[argc] = "x";
    }
  }
  if (test_budget() < 0) {
    printf("COWTEST FAIL: budget\n");
    exit(1);
  }
  printf("COWTEST PASS: sharing, faults, argv bounds, budget\n");
  exit(0);

fail:
  printf("COWTEST FAIL: allocation\n");
  exit(1);
}

static int
test_many_children(char *page)
{
  int children = 3;
  for (int i = 0; i < children; i++) {
    int pid = fork();
    if (pid < 0)
      return -1;
    if (pid == 0) {
      pause(2);
      page[0] = (char)(0x20 + i);
      if (page[0] != (char)(0x20 + i))
        exit(2);
      exit(0);
    }
  }
  if (pmc(1) == 0)
    return -1;
  for (int i = 0; i < children; i++) {
    int status = -1;
    if (wait(&status) < 0 || status != 0)
      return -1;
  }
  return page[0] == 0x11 && pmc(1) == 0 ? 0 : -1;
}

static int
test_readonly_fault(void)
{
  int pid = fork();
  if (pid < 0)
    return -1;
  if (pid == 0) {
    *(volatile char *)0 = 0;
    exit(3);
  }
  int status = 0;
  return wait(&status) >= 0 && status != 0 ? 0 : -1;
}

static int
test_bad_exec_argv(void)
{
  int pid = fork();
  if (pid < 0)
    return -1;
  if (pid == 0) {
    int result = exec("hi", (char **)1);
    exit(result == -1 ? 0 : 4);
  }
  int status = -1;
  return wait(&status) >= 0 && status == 0 ? 0 : -1;
}

static int
test_budget(void)
{
  int pages = LAB5_COW_BUDGET;
  char *base = sbrk(pages * 4096);
  if (base == (char *)-1)
    return -1;
  for (int i = 0; i < pages; i++)
    base[i * 4096] = 0x5c;

  unsigned long long before = pmc(0);
  int pid = fork();
  if (pid < 0)
    return -1;
  if (pid == 0) {
    for (int i = 0; i < pages; i++)
      base[i * 4096] = (char)i;
    exit(0);
  }
  int status = -1;
  if (wait(&status) < 0 || status != 0)
    return -1;
  unsigned long long delta = pmc(0) - before;
  for (int i = 0; i < pages; i++)
    if (base[i * 4096] != 0x5c)
      return -1;
  return delta >= (unsigned long long)pages ? 0 : -1;
}
