# Lab0：一条命令的生命周期

Lab0 不修改内核代码。实验以 `echo hi` 为主线，通过阅读 xv6 源码和运行验证，梳理系统调用、陷入、地址空间与进程调度机制。

## 验收图纸

建议现场直接打开 SVG 文件，浏览器可缩放查看：

1. [`doc/lab0/01-echo-control-flow.svg`](doc/lab0/01-echo-control-flow.svg)：`echo hi` 系统控制流图
2. [`doc/lab0/02-exec-data-structures.svg`](doc/lab0/02-exec-data-structures.svg)：`exec` 完成时的数据结构快照
3. [`doc/lab0/03-timer-context-switch.svg`](doc/lab0/03-timer-context-switch.svg)：时钟中断与上下文切换时序图

同目录的 `.mmd` 文件是 Mermaid 图源。三张图依据课程 xv6 基线中的 `user/sh.c`、`user/init.c`、`kernel/exec.c`、`kernel/proc.c`、`kernel/trap.c`、`kernel/trampoline.S` 和 `kernel/swtch.S` 绘制。

## 运行验证

本地参考树位于教师材料目录，不纳入 Git。可在 WSL2 Ubuntu 中运行：

```bash
cd '/mnt/d/操作系统实践/materials/参考代码/xv6-riscv/xv6-riscv'
make clean
make qemu
```

进入 Shell 后执行：

```text
$ echo hi
hi
$
```

按 `Ctrl-a`，松开后按 `x` 退出 QEMU。

2026-09-28 在 WSL2 Ubuntu 中从干净状态编译并实测：

```text
xv6 kernel is booting
init: starting sh
$ echo hi
hi
$
```

该摘录验证了控制流图的外部可见主线：Shell 接收命令、子程序输出、退出回收，以及 Shell 再次进入下一轮输入。
