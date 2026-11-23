#include "types.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

// UART 使用 MMIO：读写 0x10000000 开始的地址，实际上是在操作硬件寄存器。
// volatile 强制编译器每次都真实访问设备，不能把读写缓存或优化掉。
#define REG(n) ((volatile uint8 *)(UART0 + (n)))
#define RHR 0
#define THR 0
#define IER 1
#define FCR 2
#define LCR 3
#define LSR 5
#define RX_READY 0x01
#define TX_IDLE 0x20

//初始化串口
void uartinit(void)
{
  *REG(IER) = 0;
  *REG(LCR) = 0x80;
  *REG(0) = 0x03;
  *REG(1) = 0;
  *REG(LCR) = 0x03;
  *REG(FCR) = 0x07;
  *REG(IER) = 0x01;
}

//同步发送字符
void uartputc_sync(int c)
{
  // 轮询线路状态寄存器，直到发送保持寄存器空闲，再写入一个字符。
  while ((*REG(LSR) & TX_IDLE) == 0)
    ;
  io_fence();
  *REG(THR) = (uint8)c;
  io_fence();
}

//中断处理函数
void uartintr(void)
{
  // UART 可能一次积累多个字符：只要 RX_READY 就不断从 RHR 取出，
  // UART 只负责字节运输，退格/换行/缓冲语义交给 consoleintr。
  while (*REG(LSR) & RX_READY)
    consoleintr(*REG(RHR));
}
