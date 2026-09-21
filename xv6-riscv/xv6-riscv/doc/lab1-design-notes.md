# Lab1 通用设计笔记（待个人参数包到位后核对）

## 目标与边界

本轮只实现从 QEMU 复位到内核 Banner 输出：不实现键盘输入、中断驱动控制台、进程和文件系统。五个待实现文件职责分别是：

| 文件 | 职责 |
|---|---|
| `entry.S` | M 态最早入口、栈初始化、从核停驻、跳转 `start` |
| `start.c` | M 态 CSR/PMP 配置，`mret` 切换到 S 态 `main` |
| `console.c` | UART 16550 轮询发送单字节 |
| `printf.c` | 格式化解析与整数/字符串输出 |
| `main.c` | 初始化并输出个性化 Banner |

## 启动状态机

```text
QEMU PC=0x1000 (Boot ROM)
  -> -bios none 跳入 kernel，链接/加载基址 0x80000000
  -> _entry (M 态): 关闭 mie；读取 mhartid
       hart != 0: wfi 自旋
       hart == 0: sp = bootstack + LAB1_STACK_KB*1024
  -> call start()
  -> 配置 mtvec(4 字节对齐)、PMP、medeleg/mideleg、satp=0
  -> mepc = main；mstatus.MPP = S；mret
  -> main (S 态) -> console/printf -> UART MMIO
```

PMP 必须覆盖允许执行的物理区域；课程环境建议使用 `pmpaddr0=0x3fffffffffffff`、`pmpcfg0=0xf`。如果降权后无输出，优先检查 PMP 和 `-d int` 中的 cause=1。

## UART 协议

QEMU virt UART 基址为 `0x10000000`：THR/RBR 偏移 0，LSR 偏移 5。发送前循环读取 `LSR & 0x20`，直到 THR 空闲，再向基址写入一个字节。硬件发送函数不承担学号协议修饰；协议应放在 Banner/上层字符输出逻辑。

## `printf` 测试边界

- `%d`：0、正数、负数、最大/最小 `int`；
- `%x`：0、无前导零、小写 `0x` 前缀；
- `%s`：空字符串、长字符串；
- `%%`：只输出一个 `%`；
- 连续格式字段和末尾换行。

## 交付前检查

1. `git diff` 确认课程标记为预置/不可修改的文件没有变化。
2. 根据个人 `course_sid.h` 生成 `expect_banner.txt`。
3. 运行 `python3 check_expect.py <个人学号>`。
4. QEMU 实跑 Banner，并检查 `-d int` 没有异常。
5. 每个阶段单独提交，最后建立 `lab1-submit` 标签。

本笔记刻意不写入任何他人学号的参数值；收到个人文件夹后只需补齐对应参数解释和精确期望输出。
