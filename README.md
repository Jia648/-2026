# Lab2：物理内存管理

成员：2413000 李亚杰、2414072 贾申奥。代码来源为老师发放的 `lab2.zip`（内层目录 `lab2_new`）。原压缩包保留于工作区，本目录为 Lab2 完成版，按 `code/`、`report/` 结构存放于仓库的 `lab2` 分支；`lab1` 分支保留原实验。

## 在 Ubuntu 上运行

将整个 `lab2-complete` 复制到 Ubuntu 主目录，确保结构为 `~/lab2-complete/code/Makefile`，不要再嵌套一层同名目录。

```sh
cd ~/lab2-complete/code
make clean && make
make qemu
```

默认 Best-Fit；应观察设备树内存范围、`memory management: best_fit_pmm_manager` 和 `check_alloc_page() succeeded!`。退出 QEMU：Ctrl+A，松开后 X。

```sh
make grade
```

课程评分已在 Ubuntu 实测通过：Page Init / Best-Fit 两项 OK，`Total Score: 20/20`。如果提示 `bc: not found`，安装 `bc` 后重试。评分在临时副本注入原测试，不改变交付源码中的测试函数。

```sh
make buddy
# 退出后再运行下一项
make slub
```

Buddy 目标预期输出 `buddy_check() succeeded!`；SLUB 目标使用 Best-Fit 页分配器，预期输出 `slub_check() succeeded!`。切换目标会自动 clean；从 Buddy 切回默认版本须 `make clean && make qemu`，避免复用不同宏定义的目标文件。

## GDB

两个终端先后运行 `make clean && make debug` 和 `make gdb`。Lab2 入口会建立 Sv39 映射，不能照搬 Lab1 只用物理地址解释所有后续符号。

```gdb
set pagination off
b *0x80200000
continue
x/18i $pc
hbreak kern_init
continue
info registers pc sp satp
b page_init
continue
finish
p/x va_pa_offset
p/x npage
p/x pages
```

`satp` 应启用 Sv39；高半区与物理地址偏移为 `0xffffffff40000000`。`npage` 是管理上界的绝对页号，不是空闲页面数。具体地址以本次构建输出为准。

## 已执行的本机验证

- 使用 Windows 上已有的 Clang 15 生成 RV64GC 对象并由 LLD 链接；这是额外交叉检查，不代替课程 GCC/Makefile 验证。
- 执行真实 C 分配器源码的宿主测试：保留不变的 Best-Fit 原测试、Buddy 和 SLUB 测试，40,000 次随机分配释放、不重叠与计数检查，以及 Buddy 全内存耗尽和回收。
- Ubuntu GCC 构建、QEMU 默认/Buddy/SLUB、课程评分 20/20、实际 GDB 及 Ubuntu 原生压力测试均已运行通过；原始结果保存在 report/logs。

可在有本机 C 编译器的环境复现宿主测试：`python3 tests/run_host_tests.py`。它不验证 DTB、SBI 或页表。

详见 `report/report.md`、`report/prompt.md` 和 `report/验证记录.md`。

## 一键复验

```sh
cd ~/lab2-complete/code
python3 tests/verify_qemu.py
```

该脚本依次构建、启动、评分、检查 Buddy/SLUB、运行宿主压力测试并通过 GDB 检查 Sv39，最后恢复默认构建。默认测试 RAM 为 128 MiB；仅终止脚本自己启动的进程。正常最后输出 `ALL PASS`。每次启动后的 SIGTERM 是脚本主动结束无限循环内核，不能将此当成内核崩溃。
