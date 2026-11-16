# Lab4 模块规格与复核提示词

下面按本次实现整理复核提示词，供报告说明和复习使用。

| 模块 | 规格 | 复核提示词 |
|---|---|---|
| 页表操作 | 支持建立、替换及撤销 4 KiB 页映射，保持 PTE、页引用和 TLB 一致 | 检查 page_insert 与 page_remove 的同页更新、换页、重复删除和内存不足四条路径。说明每条路径如何更新引用计数和 TLB。 |
| 线程创建 | 初始化 PCB，建立 idleproc 和 initproc，并在失败时释放已分配资源 | 对照 proc_struct 和 trapframe 检查 do_fork。逐步说明栈、PID、链表、状态和首次运行入口如何设置。 |
| 调度 | 在本核关中断区间选择可运行线程，切换页表和寄存器 | 对照 struct context 检查 switch_to 的寄存器偏移，并追踪 idleproc 首次切到 initproc 时的 forkret、forkrets、sret 和线程入口。 |
