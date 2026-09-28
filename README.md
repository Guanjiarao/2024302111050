# 操作系统实践

学号：`2024302111050`

本仓库按实验分目录保存，每个实验均可在自己的目录内独立构建和验收。

## 目录结构

- `lab1/`：启动、特权级切换、UART 输出与个性化 Banner
- `lab2/`：用户态陷阱、系统调用、时钟与 UART 中断、控制台输入及最小进程机制

非工程材料统一保存在本地 `materials/`，该目录已被 Git 忽略，不会上传到 GitHub。

## 构建

在 WSL2 Ubuntu 中进入对应实验目录：

```bash
cd '/mnt/d/操作系统实践/lab1'
make clean
make
make qemu
```

Lab2 完成后使用相同方式进入 `lab2/` 构建。

## 提交标签

- `lab0-submit`：Lab0 提交节点
- `lab1-submit`：Lab1 提交节点
- `lab2-submit`：Lab2 提交节点
