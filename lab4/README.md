# Lab4 进程上下文切换与两级 MLFQ

- 学号：2024302111050
- `LAB4_POLICY_ID = 0`
- 高优先级时间片：`LAB2_TICK = 3` 个硬件时钟 tick
- 低优先级时间片：6 个硬件时钟 tick
- 默认票数记录值：`7 + COURSE_SID % 13 = 18`（MLFQ 不据此选进程）

## 功能概述

本目录从提交版 Lab3 独立建立，保留 SV39、用户地址校验、guard page、`dumppt` 和内存上限测试。Lab4 新增每进程锁、`wait_lock`、每 CPU 调度器上下文、独立内核栈、`swtch` 执行流切换、标准 `sleep/wakeup`、阻塞式 `pause`、时钟抢占、两级 MLFQ 以及 `schedstat/time_rdtime/setickets`。进程表容量为 64，QEMU 默认单核。

## 构建与启动

在 WSL Ubuntu 中执行：

```sh
cd /mnt/d/操作系统实践/lab4
make clean
make
make qemu
```

退出 QEMU：按 `Ctrl-A`，再按 `X`。

## 手工测试

进入 `sh>` 后可执行：

```text
hi
dumppt
badaccess
guardtest
copyedge
memcap
alt
bench
mlfq_test
```

`bench` 应输出 `RESULT concurrent_children = 8` 和 `PASS: bench`。`mlfq_test` 应输出 `RESULT mlfq_slices = high:3 low:6 interactive_max:0` 和 `PASS: mlfq`。`lottery_test` 与 `stride_test` 会编译，但份额断言只适用于其规定的策略 ID，不用于证明本人的 ID 0 MLFQ。

## 自动测试

```sh
python3 tools/inject_uart.py --tree . --script tests/lab4.script
python3 tools/inject_uart.py --tree . --script tests/mlfq.script
python3 tools/inject_uart.py --tree . --script tests/lab3.script
python3 tools/inject_uart.py --tree . --script tests/stress.script
```

## 标签与归档

- `lab4-start`：合并增量包并通过基线构建、Lab3 回归后的提交。
- `lab4-submit`：最终实现与验收文档提交。
- 归档命令：`git archive --format=zip -o 提交-lab4-2024302111050.zip lab4-submit`。
