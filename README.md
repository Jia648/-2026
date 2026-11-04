# Lab3

只包含今年必做部分，未启用旧版 Challenge。

```bash
cd ~/lab3-complete/code
make clean && make qemu
make grade
```

正常启动后每 100 次时钟中断输出一次 `100 ticks`，累计 10 次后关机。

GDB：终端一 `make debug`，终端二 `make gdb`，设置 `hbreak trap` 后 `continue`，使用 `p *tf` 查看 Trap 上下文。
