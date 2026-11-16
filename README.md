# Lab4

在 Ubuntu 虚拟机运行：

```bash
cd ~/lab4-complete/code
make clean && make qemu
```

看到 initproc 和时钟输出后，先按 Ctrl+A，再按 X 退出 QEMU。执行 `make grade` 运行课程评分。实验报告见 [report/report.md](report/report.md)。

调试时，终端一执行 `make debug`，终端二执行 `make gdb`，可在 `proc_run` 或 `switch_to` 设置硬件断点。
