#include "types.h"
#include "memlayout.h"

//初始化 PLIC，设置UART中断的优先级为1
void plicinit(void)
{
  *(volatile uint32 *)(PLIC + UART0_IRQ * 4) = 1;
}

//为当前 hart（CPU 核）配置中断接收条件
void plicinithart(void)
{
  // 在 hart 0 的 S 态中断使能位图中打开 UART0_IRQ；阈值设为 0，
  // 因此优先级为 1 的 UART 中断可以被投递。
  *(volatile uint32 *)PLIC_SENABLE(0) = 1 << UART0_IRQ;
  *(volatile uint32 *)PLIC_SPRIORITY(0) = 0;
}

int plic_claim(void)
{
  // 读取 claim 寄存器可取得当前最高优先级 IRQ 编号；0 表示没有待处理中断。
  return *(volatile uint32 *)PLIC_SCLAIM(0);
}

void plic_complete(int irq)
{
  // 把相同 IRQ 写回 claim/complete 寄存器，通知 PLIC 本次中断已经处理完成。
  *(volatile uint32 *)PLIC_SCLAIM(0) = irq;
}
