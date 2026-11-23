# Lab5 测试说明

## 主测试脚本

```sh
python3 tools/inject_uart.py --tree . --script tests/lab5.script \
  --expect-timeout 30 --log lab5-test.log
```

该脚本依次运行课程测试 `cowpair`、`zerocopy`、`echo hi`，前序回归 `bench`、`memcap`，以及综合自测 `cowtest`。任何一个 `EXPECT` 不匹配都会以非零状态退出。

## 前序回归

```sh
python3 tools/inject_uart.py --tree . --script tests/lab4.script \
  --expect-timeout 30 --log lab4-regression.log
python3 tools/inject_uart.py --tree . --script tests/mlfq.script \
  --expect-timeout 30 --log mlfq-regression.log
```

课程指定的 Lab5 Shell 不再支持后台 `&` 语法，因此旧 `lab3.script` 中的 `spin &` 驱动方式不再适用；其余 Lab3 功能由 `lab4.script` 中的 `dumppt`、`badaccess`、`guardtest`、`copyedge` 和 Lab5 主脚本中的 `memcap` 覆盖。

## 综合自测覆盖

`cowtest` 验证：

1. 三个子进程同时共享并写入同一页，退出后共享页计数归零；
2. 写只读代码页时子进程被终止，内核继续运行；
3. `exec` 收到非法用户 `argv` 指针时返回 `-1`；
4. `exec` 接受恰好 `MAXARG` 个参数，并拒绝第 `MAXARG + 1` 个参数；
5. 子进程复制至少 234 个页面，达到个性化预算后打印一次告警并继续，父进程数据保持不变。
