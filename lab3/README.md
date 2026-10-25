# 操作系统实践 Lab3

本目录是学号 `2024302111050` 的 Lab3 工程，基于 Lab2 增量实现 SV39 三级页表、物理页分配器、用户进程隔离、受控拷贝和 EAGER `sbrk`。用户程序仍由 Makefile 编译并嵌入内核；不包含磁盘文件系统。

个性化参数来自 `kernel/course_sid.h`：`LAB3_ALLOC_ORDER=2`（中点优先）、`LAB3_GUARD_PAGES=1`、`LAB3_MEMCAP_DELTA=10`。实现决策和验收要点见 [设计与验收笔记](设计与验收笔记.md)。

## 构建与运行

在 WSL2 Ubuntu 终端运行：

```bash
cd '/mnt/d/操作系统实践/lab3'
make clean
make
make qemu
```

启动后检查 Lab1 Banner、初始 `DUMP-PAGETABLE` 块和 `sh> ` 提示符。输入 `spin &` 可在后台启动忙循环进程，再输入 `hi` 验证 Shell 与 `spin` 轮流运行。输入 `dumppt` 可重放当前进程页表；输入 `badaccess`、`guardtest`、`copyedge` 和 `memcap` 可分别观察越权访问终止、保护页、跨页拷贝及内存配额行为。`memcap` 会分配并触碰大量页面，正常情况下需要几十秒。

退出 QEMU：按 `Ctrl-a`，松开后按 `x`。

## 自动化回归

```bash
make test
```

该目标通过 Lab2 的 UART 注入工具依次运行冒烟、跨页拷贝、guard、官方 `badaccess`、页表重放和 `memcap` 检查。测试日志默认写到 `lab3/inject_uart.log`，属于本地运行产物。

## 提交

实验要求的基线标签为 `lab3-start`，验收版本标签为 `lab3-submit`。在验收版本提交后生成归档：

```bash
git archive --format=zip -o '提交-lab3-2024302111050.zip' lab3-submit
```
