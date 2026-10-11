# Lab1 GDB 实测与报告补齐

## 1. 启动与记录

在 Ubuntu 中进入实际的 `code` 目录。两个终端必须使用同一份构建产物。先编译，再在终端 A 运行：

```sh
make
make debug
```

终端 B 运行：

```sh
gdb-multiarch bin/kernel
```

在 GDB 中执行以下命令。日志保存到 `code/lab1-gdb.txt`，完成后可复制到 `report`；日志不应包含密码或 API 密钥。

```gdb
set pagination off
set logging file lab1-gdb.txt
set logging overwrite on
set logging enabled on
set architecture riscv:rv64
target remote localhost:1234
info registers pc
x/8i $pc
si
info registers pc
b *0x80200000
continue
x/6i $pc
info registers pc sp
p/x &bootstacktop
si
info registers pc sp
si
info registers pc sp
set logging enabled off
```

`la` 是伪指令，可能展开为多条机器指令；只有完成整段地址计算后，`sp` 才等于 `bootstacktop`。若两次 `si` 尚未完成栈设置，依据 `x/6i $pc` 继续单步，记录真实结果，不照抄预期值。

## 2. 可增加的固件阶段观察

若要更直接观察中间固件阶段，重新启动 QEMU，并在连接后先执行：

```gdb
x/8i 0x1000
```

根据实际反汇编观察复位 ROM 的寄存器准备和跳转。可尝试 `hbreak *0x80000000` 后 `continue`，记录固件入口；再删除该断点并设置内核入口断点。若环境不支持硬件断点，保留错误并使用复位指令单步和内核入口断点作为证据。不要对没有调试符号的固件直接逐行跟踪全部初始化代码。

采用 `-kernel` 时 QEMU 在 CPU 运行前装入镜像，不保证对 `0x80200000` 设置观察点能观察装入瞬间。

## 3. 报告必须来自输出的内容

- 连接时 PC 的实际值和最初几条指令。
- 初始指令如何准备 hart ID、设备树或交接信息，以及转移到哪个地址；逐项对照反汇编。
- 内核入口断点是否命中，PC 是否为 `0x80200000`。
- 栈初始化前后的 `sp` 与 `bootstacktop` 是否一致。
- `tail kern_init` 的实际反汇编及后续进入 C 初始化的观察。

保留至少两张清晰截图：复位入口反汇编，以及内核入口断点与栈寄存器。原来的端口监听截图作为辅助证据。

## 4. 补充版本的功能复测

把本次补充后的 `code` 放到 Ubuntu 的独立目录中，避免混用旧构建产物，再运行 `make clean && make` 和 `make qemu`。检查输出、连续命令、空行，以及输入 `abc` 后退格并输入 `d` 时是否得到 `abd`。终端退出 QEMU：先按 Ctrl+A，松开后按 X。

当前代码仍以忙轮询等待输入；它避免无输入状态刷屏，但不会使 CPU 进入低功耗等待。若粘贴产生 CRLF，可能多出现一行提示符；报告不应声称已经实现完整终端或命令解释器。
