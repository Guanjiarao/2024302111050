# Lab0：`echo hi` 系统控制流图（草稿）

```mermaid
flowchart TD
  A[用户键入 echo hi\nUART 接收] --> B[UART 中断]
  B --> C[devintr -> uartintr -> consoleintr]
  C --> D[写入 cons.buf\ncons.w 更新]
  D --> E[wakeup(&cons.r)]
  E --> F[Shell 的 consoleread 返回]
  F --> G[user/sh.c getcmd\nparsecmd]
  G --> H[fork 系统调用]
  H --> I[kfork\n复制地址空间/寄存器/文件描述符]
  I --> J[子进程 RUNNABLE]
  J --> K[scheduler -> forkret]
  K --> L[子进程 exec("echo", argv)]
  L --> M[kexec\n读取 ELF、建立新页表、用户栈]
  M --> N[user/echo.c main]
  N --> O[write(1, "hi\\n", 3)]
  O --> P[sys_write -> filewrite]
  P --> Q[consolewrite -> uartwrite]
  Q --> R[终端显示 hi]
  N --> S[exit]
  S --> T[kexit -> ZOMBIE\nwakeup(parent)]
  T --> U[Shell wait -> kwait\n回收子进程]
  U --> V[Shell 再次 write("$ ")]
```

关键状态：Shell 在等待输入时通过 `consoleread()` 睡眠；回车经 UART 中断写入控制台缓冲区并唤醒它。Shell fork 后父进程 wait，子进程进入 RUNNABLE，经调度后执行 exec。echo 退出后成为 ZOMBIE，父进程 kwait 回收。
