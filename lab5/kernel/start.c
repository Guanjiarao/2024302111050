#include "types.h"
#include "riscv.h"
#include "course_sid.h"

void main(void);

static void
timerinit(void)
{
  // 允许 S 态直接使用 stimecmp
  w_menvcfg(r_menvcfg() | MENVCFG_STCE);

  // 置 1 后，S 态才能读取 time 计数器。
  w_mcounteren(r_mcounteren() | 2);

  w_stimecmp(r_time() + 1000000);
}

void
start(void)
{
  
  uint64 status = r_mstatus();
  status &= ~(1L << 3); // MSTATUS.MIE

  // 清掉旧 MPP，再设置为 S
  status &= ~MSTATUS_MPP_MASK;
  status |= MSTATUS_MPP_S;
  w_mstatus(status);

  w_mepc((uint64)main);

  //清零 satp，关闭地址转换和分页
  w_satp(0);

  w_medeleg(0xffff);
  w_mideleg(0xffff);
  w_sie(r_sie() | SIE_SEIE | SIE_STIE);

  w_pmpaddr0(0x3fffffffffffffull);
  w_pmpcfg0(0xf);

  // 启动第一次定时器；tp 保存当前 hart（CPU 核）编号。
  timerinit();
  w_tp(r_mhartid());

  // CPU 根据 mstatus.MPP 切换到 S 态，
  // 再跳到 mepc 保存的 main() 地址
  asm volatile("mret");
  __builtin_unreachable();
}
