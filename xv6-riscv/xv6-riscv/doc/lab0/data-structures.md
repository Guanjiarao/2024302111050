# Lab0：核心数据结构快照（草稿）

## 输入路径

| 结构 | 关键字段 | 作用 |
|---|---|---|
| `cons` | `buf[128]`, `r`, `w`, `e`, `lock` | UART 输入环形缓冲及读写游标 |
| `struct proc` | `state`, `pid`, `parent`, `pagetable`, `trapframe`, `context`, `ofile[]`, `cwd` | 描述 Shell/echo 进程及其调度、地址空间、资源 |
| `trapframe` | `epc`, `sp`, `a0/a1`, `kernel_sp`, `kernel_trap` | 用户态陷入时保存寄存器和返回信息 |

## fork/exec/exit 生命周期

```text
Shell proc
  ├─ kfork -> child proc
  │   ├─ pagetable: uvmcopy(parent)
  │   ├─ trapframe.a0 = 0
  │   ├─ ofile[]: filedup
  │   └─ state = RUNNABLE
  ├─ parent: wait_lock + kwait()
  └─ child: kexec()
      ├─ 新 pagetable
      ├─ ELF text/data 映射
      ├─ guard page + user stack
      ├─ argv 字符串及指针数组
      └─ trapframe.epc = ELF entry, sp = user stack

child exit:
  kexit -> 关闭文件、释放 cwd、reparent children、唤醒 parent
        -> state = ZOMBIE
  parent kwait -> 复制 xstate -> freeproc -> 回收 child
```

## 地址空间要点

- 用户程序的代码、数据和栈位于用户虚拟地址空间。
- 陷入后通过 trampoline 切换到内核页表和当前进程内核栈。
- `trapframe` 位于每个进程地址空间的固定高地址区域附近，用于保存 U 态现场。
