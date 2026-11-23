#include "types.h"
#include "riscv.h"
#include "course_sid.h"
#include "proc.h"
#include "defs.h"

#define BACKSPACE 0x100
#define C(x) ((x) - '@')
#define THROTTLE_INTERVAL (16 + COURSE_SID % 16)

static struct {
  struct spinlock lock;
  char buf[LAB2_BUF_SIZE];
  uint r;
  uint w;
  uint e;
} input;
static uint emitted;

void
consputc(int c)
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

void
consoleinit(void)
{
  initlock(&input.lock, "console");
  input.r = input.w = input.e = emitted = 0;
  uartinit();
}

void
consoleintr(int c)
{
  acquire(&input.lock);
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
    if (input.e == input.w)
      wakeup(&input);
  }
  release(&input.lock);
}

int
consoleread(uint64 dst, int n)
{
  int count = 0;
  if (n < 0)
    return -1;
  while (count < n) {
    acquire(&input.lock);
    while (input.r == input.w)
      sleep(&input, &input.lock);
    int c = input.buf[input.r % LAB2_BUF_SIZE];
    input.r++;
    release(&input.lock);
    if (copyout(myproc()->pagetable, dst + count, &c, 1) < 0)
      return count ? count : -1;
    count++;
    if (c == '\n')
      break;
  }
  return count;
}

int
consolewrite(uint64 src, int n)
{
  char buf[64];
  int done = 0;
  if (n < 0)
    return -1;
  while (done < n) {
    int chunk = n - done;
    if (chunk > (int)sizeof(buf))
      chunk = sizeof(buf);
    if (copyin(myproc()->pagetable, buf, src + done, chunk) < 0)
      return done ? done : -1;
    for (int i = 0; i < chunk; i++)
      consputc(buf[i]);
    done += chunk;
  }
  return done;
}
