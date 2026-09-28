# 操作系统实践 Lab1

本仓库是学号 `2024302111050` 的个人 Lab1 工程。仓库根目录即为可编译、可验收的工程目录，不包含公共 xv6 参考树。

## 个人参数

- `COURSE_SID = 2024302111050`
- `LAB1_BANNER_PROTOCOL = 0`
- `LAB1_STACK_KB = 4`
- `COURSE_SID % 97 = 34 = 0x22`
- UART 节流间隔：`16 + COURSE_SID % 16 = 26`

启动后必须精确输出一行（末尾包含换行）：

```text
OSLAB1 sid=2024302111050 mod97=0x22
```

## 构建与运行

构建环境为 WSL2 Ubuntu。请在 Ubuntu 终端中从仓库根目录执行：

```bash
cd '/mnt/d/操作系统实践'
make clean
make
make qemu
```

退出 QEMU：按 `Ctrl-a`，松开后再按 `x`。

可单独检查期望 Banner 文件及个人参数公式：

```bash
python3 check_expect.py 2024302111050
```

## 目录说明

- `kernel/`：Lab1 内核源码与课程预置头文件、链接脚本
- `doc/`：验收演示与问答文档
- `tests/`：格式化输出测试用例说明
- `expect_banner.txt`：逐字节验收所用的期望输出
- `我的参数.txt`：当前实验的个性化参数记录

## 后续实验

Lab2–Lab6 的参数来自后续对应增量包。旧基线包曾包含错误学号，不能从少量样本推测公式，也不能沿用本仓库的 Lab1 结果。每个后续实验开始前，必须按教师发布的对应增量包重新核对参数、公式和不可修改文件，验证后再记录结果。
