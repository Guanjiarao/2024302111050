# Operating System Practice

This repository tracks the coursework based on xv6-riscv.

## Current progress

- WSL2 Ubuntu development environment configured and verified.
- RISC-V cross compiler and QEMU installed.
- The public xv6 source builds and reaches its shell successfully.
- Lab 0 control-flow, data-structure, and context-switch diagrams drafted.
- Personalized Lab 1 boot, privilege transition, UART, printf, and banner implemented.
- Lab 1 QEMU output matches `expect_banner.txt` byte for byte with no synchronous exceptions.

## Repository layout

- `xv6-riscv/xv6-riscv/`: public xv6 source tree and coursework notes.
- `xv6-riscv/xv6-riscv/doc/lab0/`: Lab 0 deliverable drafts.
- `xv6-riscv/xv6-riscv/doc/lab1-design-notes.md`: parameter-independent Lab 1 design notes.
- `202430211050-kernel/202430211050-kernel/`: personalized Lab 1 implementation for student `2024302111050` (the received package directory omitted one digit).

Course handouts, generated binaries, and another student's personalized baseline are intentionally excluded.

## Build

From Ubuntu under WSL2:

```bash
cd /mnt/d/操作系统实践/xv6-riscv/xv6-riscv
make qemu CPUS=1
```

Exit QEMU by pressing `Ctrl-a`, releasing the keys, and then pressing `x`.
