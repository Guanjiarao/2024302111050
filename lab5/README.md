# Lab5 系统调用与写时复制 fork

- 学号：2024302111050
- `LAB5_REFCNT_SCHEME = 2`：全局引用计数作为回收依据，并分析页表反向扫描方案
- `LAB5_KILL_SEMANTICS = 0`：已标记终止的进程不再执行 COW 复制
- `LAB5_COW_BUDGET = 234`：单进程达到预算时告警一次并继续运行

## 实现内容

本实验在 `lab4-submit` 基线上完成：

1. 物理页引用计数、归零回收和共享页统计；
2. `fork` 只读共享、写缺页分离和 `refcnt == 1` 原地恢复；
3. `copyout` 对 COW 页的透明写入；
4. `pmc(0)` 累计实际 COW 复制次数，`pmc(1)` 统计当前共享物理页；
5. `exec(name, argv)` 的参数提取、用户栈装配、NULL 哨兵和 16 字节对齐；
6. COW 双核临界区串行化、分配失败回滚及单进程复制预算降级；
7. 课程测试、前序回归和综合自测 `cowtest`。

## 构建与运行

在 WSL Ubuntu 中执行：

```sh
cd /mnt/d/操作系统实践/lab5
make clean
make
make qemu
```

退出 QEMU：按 `Ctrl-A`，再按 `X`。

## 自动测试

```sh
python3 tools/inject_uart.py --tree . --script tests/lab5.script \
  --expect-timeout 30
python3 tools/inject_uart.py --tree . --script tests/lab4.script \
  --expect-timeout 30
python3 tools/inject_uart.py --tree . --script tests/mlfq.script \
  --expect-timeout 30
```

`tests/lab5.script` 验证以下判据：

- `cowpair`：`copies delta = 2`，父子数据隔离；
- `zerocopy`：本次实测 `delta = 1`，且要求始终不超过 2；
- `echo hi`：`argc = 2`、`argv[argc] = NULL`、`argv` 16 字节对齐；
- `bench`、`memcap`：调度与物理页回收回归通过；
- `cowtest`：多子进程、只读页非法写、非法 `argv`、参数数量边界和预算降级全部通过。

## 标签与归档

- `lab5-start`：Lab4 基线与 Lab5 v2 增量包合并完成；
- `lab5-submit`：最终实现与测试说明；
- 归档命令：

```sh
git archive --format=zip -o 提交-lab5-2024302111050.zip lab5-submit
```
