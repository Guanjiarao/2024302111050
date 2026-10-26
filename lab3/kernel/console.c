#include "types.h"
#include "riscv.h"
#include "course_sid.h"
#include "proc.h"
#include "defs.h"

#define BACKSPACE 0x100
#define C(x) ((x) - '@')
#define THROTTLE_INTERVAL (16 + COURSE_SID % 16)

static struct {
  char buf[LAB2_BUF_SIZE];
  // r：下一读取位置；w：已经提交给 read 的边界；e：当前编辑位置。
  // 三者使用单调递增计数，访问数组时再取模，从而构成环形缓冲区。
  uint r, w, e;
} input;
static uint emitted;

void consputc(int c)
{
  // 屏幕上的“退格”需要依次输出：光标左移、空格擦除、再次左移。
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
  //配置 UART 硬件寄存器
  uartinit();
}

void consoleintr(int c)
{
  // Ctrl-U：删除当前尚未提交的一整行。不能越过 w，因为 w 之前已可被 read 消费。
  if (c == C('U')) {
    while (input.e != input.w && input.buf[(input.e - 1) % LAB2_BUF_SIZE] != '\n') {
      input.e--;
      consputc(BACKSPACE);
    }
  // Ctrl-H 或 DEL：删除一个尚未提交的字符。
  } else if (c == C('H') || c == 0x7f) {
    if (input.e != input.w) {
      input.e--;
      consputc(BACKSPACE);
    }
  // 缓冲区未满时接收普通字符。e-r 是“尚未被读取”的字符总数。
  } else if (c && input.e - input.r < LAB2_BUF_SIZE) {
    // 统一把回车 CR 转成换行 LF，并立即回显到终端。
    c = c == '\r' ? '\n' : c;
    consputc(c);
    input.buf[input.e++ % LAB2_BUF_SIZE] = c;
#if LAB2_BUF_SEMANTICS == 0
    // 个性化配置为行缓冲：只有收到换行或缓冲区满时，
    // 才令 w=e，把当前输入正式提交给阻塞中的 read。
    if (c == '\n' || input.e - input.r == LAB2_BUF_SIZE)
      input.w = input.e;
#else
    // 字符流模式下，每收到一个字符就立刻允许 read 读取。
    input.w = input.e;
#endif
  }
}

int consoleread(uint64 dst, int n)
{
  int count = 0;
  if (n < 0)
    return -1;
  while (count < n) {
    // 修改 r/w/e 前短暂关中断，避免 UART 中断同时修改缓冲区索引。
    intr_off();
    while (input.r == input.w) {
      // 没有已提交字符时必须重新开中断再 wfi；否则 UART 中断进不来，
      // 新字符永远无法写入，read 会永久睡眠。
      intr_on();
      if (proc_yield_read())
        continue;
      asm volatile("wfi");
      intr_off();
    }
    // 取出一个已提交字符并推进读取位置；取模实现环形复用。
    int c = input.buf[input.r % LAB2_BUF_SIZE];
    if (copyout(myproc()->pagetable, dst + count, &c, 1) < 0)
    {
      intr_on();
      return count ? count : -1;
    }
    input.r++;
    intr_on();
    count++;
    // 行缓冲 read 读到换行就返回这一整行。
    if (c == '\n')
      break;
  }
  return count;
}

int consolewrite(uint64 src, int n)
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
