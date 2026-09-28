# Lab2：系统调用与控制台中断

本目录是学号 `2024302111050` 的独立 Lab2 工程，基于 Lab1 增量实现用户态陷阱、系统调用、UART/时钟中断、控制台输入和最小进程机制。

个性化参数：`LAB2_TICK=3`、`LAB2_BUF_SEMANTICS=0`（行缓冲）、`LAB2_BUF_SIZE=96`。

```bash
cd '/mnt/d/操作系统实践/lab2'
make clean
make
make qemu
```

启动应保留 Lab1 Banner，随后显示 `sh> `。内嵌命令包括 `hi`、`spin`、`badecall` 和 `bufstorm`。
