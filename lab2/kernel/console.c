#include "types.h"
#include "riscv.h"
#include "course_sid.h"
#include "defs.h"

#define BACKSPACE 0x100
#define C(x) ((x) - '@')
#define THROTTLE_INTERVAL (16 + COURSE_SID % 16)

static struct {
  char buf[LAB2_BUF_SIZE];
  uint r, w, e;
} input;
static uint emitted;

void consputc(int c)
{
  if (c == BACKSPACE) {
    uartputc_sync('\b');
    uartputc_sync(' ');
    uartputc_sync('\b');
  } else {
    uartputc_sync(c);
  }
  if (++emitted % THROTTLE_INTERVAL == 0)
    for (volatile uint i = 0; i < (COURSE_SID % 256) + 1; i++)
      asm volatile("nop");
}

void consoleinit(void)
{
  input.r = input.w = input.e = emitted = 0;
  uartinit();
}

void consoleintr(int c)
{
  if (c == C('U')) {
    while (input.e != input.w && input.buf[(input.e - 1) % LAB2_BUF_SIZE] != '\n') {
      input.e--;
      consputc(BACKSPACE);
    }
  } else if (c == C('H') || c == 0x7f) {
    if (input.e != input.w) {
      input.e--;
      consputc(BACKSPACE);
    }
  } else if (c && input.e - input.r < LAB2_BUF_SIZE) {
    c = c == '\r' ? '\n' : c;
    consputc(c);
    input.buf[input.e++ % LAB2_BUF_SIZE] = c;
#if LAB2_BUF_SEMANTICS == 0
    if (c == '\n' || input.e - input.r == LAB2_BUF_SIZE)
      input.w = input.e;
#else
    input.w = input.e;
#endif
  }
}

int consoleread(uint64 dst, int n)
{
  int count = 0;
  if (n < 0 || !proc_valid_range(dst, n))
    return -1;
  while (count < n) {
    intr_off();
    while (input.r == input.w) {
      intr_on();
      asm volatile("wfi");
      intr_off();
    }
    int c = input.buf[input.r++ % LAB2_BUF_SIZE];
    intr_on();
    ((char *)dst)[count++] = c;
    if (c == '\n')
      break;
  }
  return count;
}

int consolewrite(uint64 src, int n)
{
  if (n < 0 || !proc_valid_range(src, n))
    return -1;
  for (int i = 0; i < n; i++)
    consputc(((char *)src)[i]);
  return n;
}
