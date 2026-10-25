# Lab2：物理内存与页分配实验报告

| 项目 | 内容 |
|---|---|
| 小组 | 2413000 李亚杰、2414072 贾申奥 |
| 分工 | 沿用小组共同完成方式，未提供独立模块分工，不编造个人贡献 |
| 整理日期 | 2026-10-19 |
| 状态 | 实现与 Ubuntu 验证完成；课程评分 20/20；Buddy、SLUB、GDB 和压力测试通过 |
| 初始代码 | 老师提供的 lab2.zip，目录 lab2_new |

## 一、目的与整体逻辑

Lab1 能启动并输出，Lab2 进一步把“物理内存”转化为可分配资源。入口先保存 hart ID 与 DTB 地址，建立 Sv39 映射；C 入口解析设备树。`pmm_init → init_pmm_manager → page_init → init_memmap` 把真实内存范围组织为描述符和空闲块。`alloc_pages/free_pages` 通过同一接口分派给不同算法。默认 Best-Fit 满足课程基础测试，Buddy 演示二次幂拆合，SLUB 在页分配之上提供字节粒度对象。

## 二、环境及 AI 协作

实际验证环境为 Ubuntu 虚拟机，riscv64-unknown-elf-gcc 13.2.0、QEMU 8.2.2、OpenSBI 1.3、gdb-multiarch 15.0.50；版本输出见 logs/environment.txt。本次本机额外使用已有 Clang 15 / LLD 生成 RV64GC 目标并链接，使用 Windows 宿主编译器执行真实分配器源码测试。小组使用的 AI 辅助开发环境为 Claude Code + DeepSeek API，底层模型为 DeepSeek V4，用于辅助理解实验要求、生成代码和设计测试；后续补齐、检查及验证工作另有 Codex 协助。

## 三、物理内存初始化

`static void page_init(void)` 从 `get_memory_base/get_memory_size` 取得范围。框架 `page2ppn` 固定加 `nbase=DRAM_BASE/PGSIZE`，因此拒绝不匹配的物理基址，而不是把不兼容范围静默映射到错误描述符。检查 size 为零和 base+size 溢出；可管理上界限制到 PHYSICAL_MEMORY_END，向下页对齐。

- VA：开启分页后 C 代码使用的高半区地址。
- PA：硬件物理地址，`PADDR(VA)=VA-va_pa_offset`。
- PFN：物理地址除 4096 的页号；`npage` 是绝对上界，不是 DRAM 页数。
- `Page*`：元数据地址。第 i 个描述符对应 PFN `i+nbase`。

`pages=ROUNDUP(end,PGSIZE)` 放在内核镜像之后；为 `npage-nbase` 个描述符检查空间。全部先标为 reserved，防止 OpenSBI、内核和元数据进入分配器。描述符尾部向上页对齐得到 free_pa，仅把 `[free_pa, limit)` 的完整页面交给管理器。没有完整页时不调用 init_memmap；若元数据本身放不下则 panic。

限制：当前框架管理单一 DRAM 区间，管理上界为 128 MiB 默认 RAM 对应的 PHYSICAL_MEMORY_END；不解析多段 RAM、设备树 reserved-memory 或热插拔。本实验不在初始化后继续依赖原 DTB 数据。课程 page_init 注入测试按默认 RAM 检查，不宣称超出框架上限的配置也能通过该测试。

## 四、First-Fit 分析与 Best-Fit 实现

First-Fit 的 init 初始化空链表和计数；init_memmap 清理可用页状态并把块按地址插入；alloc 扫描首个满足长度的块、分割余块；free 按地址插入并检查前后是否连续，连续则合并。按地址排序让相邻合并无需全表查找。

改进空间：First-Fit 的线性搜索会随碎片增多而变慢，可采用游标形成 Next-Fit、按大小分桶、平衡树或边界标记。Next-Fit 只改变搜索起点，不保证更少碎片。必须权衡查找成本、元数据及局部性。

Best-Fit 复用地址有序链表，但扫描所有块找出 `property>=n` 的最小块；相同长度因遍历顺序保留低地址块。分割、释放及合并与连续区间语义一致。修正了参考实现尾插后继续遍历的细节，显式 break，避免新节点又参与当前插入遍历。注册 best_fit_pmm_manager，并默认在 pmm.c 接入。

原 `basic_check`、`best_fit_check` 没有修改。它们临时隔离 free_list/nr_free，检查单页分配、空表失败、精确最小块选择、连续块回收以及最终统计恢复。Best-Fit 仍是 O(空闲块数) 搜索和插入，可能制造很多小碎片；按长度树或大小类可改进检索，但仍需另一结构处理地址合并，不保证消除外部碎片。

## 五、Buddy System

以物理 PFN 对齐，维护 0 至 15 阶空闲链表。初始化将非二次幂区间分解成对齐块。申请 n 页向上取二次幂；高阶块递归拆分，余半块进入低阶链表。释放依据保存的阶信息，使用 `PFN XOR (1<<order)` 寻找唯一伙伴，只有同阶空闲伙伴才合并。请求 3 页实际占 4 页，free_pages 传回原请求 3 即可，不支持部分释放。

本实现复用 Page.ref 保存 Buddy 私有分配头阶数+1，内部页为 -1；因此它不是通用引用计数语义，不能直接与未来依赖 page_ref 的共享页管理混用。当前 Lab2 仅在独立宏配置运行，不执行 Best-Fit 假定 ref=0 的测试。

测试覆盖零请求、超量、3→4 页取整、不重叠、计数恢复、64 单页交错释放、空闲块对齐和链表计数。宿主测试增加完整耗尽 2048 页、交错释放后重新取得完整 2048 页，以及确定性随机操作。它们能检查重复分配、丢页、拆合错误，但不能证明多 CPU 安全。

## 六、简化 SLUB

底层按页，上层管理 16、32、64、128、256、512、1024、2048 字节大小类。每个 slab 页含对象位图和使用计数，复用空槽，空页立即归还底层。大于 2048 字节的对象独立申请连续页，记录请求页数，释放时兼容 Buddy 的向上取整。返回地址对齐到 16 字节；极大请求检查溢出；空分配和 NULL 释放遵循文档约定。

这体现了页级与对象级两层管理，不包含 Linux 完整 SLUB 的 per-CPU 缓存、NUMA、并发锁或构造器。大小类带来内部碎片，空闲 slab 搜索为线性；大对象也受连续物理页和当前框架容量限制。无效或重复释放通过断言/ panic 拒绝，不作为正常 API 行为。

测试遍历大小类两侧边界及大对象，写入并核对全部请求字节，检查对齐；分配 300 个小对象迫使跨页，交错释放并重新分配，最终核对页数恢复。在宿主中分别使用 Best-Fit、Buddy 作后端；内核 `make slub` 默认使用 Best-Fit。

## 七、启动集成与迭代记录

1. 检查初始包：缺 page_init 和 Best-Fit 管理函数；原入口只保留高半区根项，satp 切换后当前物理 PC 缺恒等映射。补入根项 2 的 1 GiB 恒等映射并保留根项 511 高半区映射；入口使用独立 .text.kern_entry 保证链接放在前面。该额外映射仅用于教学启动，非最终安全隔离方案。
2. 按 specification 实现初始化和 Best-Fit，不改原测试；新增 Buddy 和 SLUB 模块及独立目标。规范、实现任务提示和测试任务提示见 [prompt.md](prompt.md)。没有记录逐字模型思维或虚构错误日志。
3. 宿主运行原分配器测试通过，并扩展随机操作及耗尽恢复。交叉检查时 Clang 将框架 GNU 旧式指定初始化语法视为警告，额外检查命令用 -Wno-gnu-designator 兼容；未为此修改原库。课程 GCC 是否通过以 Ubuntu 输出为准。
4. 适配当前环境的 QEMU -kernel 启动和 gdb-multiarch。课程评分脚本只改变镜像启动参数，未改变判分逻辑、注入器和测试断言。

## 八、验证结果

| 项目 | 当前结果 |
|---|---|
| 原 Best-Fit 测试源码保留 | 已做逐段文本比对 |
| 实际 C 分配器宿主测试 | 已通过，含两种 PMM 的 40,000 次随机操作与 SLUB 测试 |
| RV64GC 全源码编译及链接 | 本机 Clang/LLD 通过，默认、Buddy、SLUB 宏配置及评分注入检查分别验证 |
| Ubuntu GCC make | 干净构建成功，见 logs/best-fit-build.txt |
| 默认 QEMU 与 make grade | 实际启动、自检通过，课程两项检查 OK，总分 20/20，见 logs/grade.txt |
| QEMU Buddy、SLUB | 两种独立配置实际运行，均输出自检成功，见 logs/buddy.txt 与 logs/slub.txt |
| GDB 与运行截图 | 已连接并验证高半区入口、Sv39、虚实偏移及保留描述符；现场复验截图与补充 GDB 日志截图见下方 |

具体命令与保存位置见 [验证记录.md](验证记录.md)。宿主测试不覆盖设备树、SBI、页表和平台启动，因此另外执行了真实 QEMU 与 GDB 验证。`python3 tests/verify_qemu.py` 可完整复现验证，测试结束恢复默认 Best-Fit。

实际默认模式探测到 128 MiB RAM，初始空闲页为 31929。GDB 在 `kern_init` 停下时，PC 为 `0xffffffffc02000da`，SP 为 `0xffffffffc0205000`，satp 为 `0x8000000000080205`，MODE=8，页表物理基址为 `0x80205000`。初始化后 `va_pa_offset=0xffffffff40000000`，`npage=0x88000`（绝对上界 PFN），`pages=0xffffffffc0207000`，`pages[0].flags=1`，说明低端页仍被保留。地址依赖本次构建，不应作为跨版本固定常量。

原先的远程测试包装器误把编译文件名 `panic.c` 当成发生内核 panic；日志本身已有自检成功输出。已修正验证脚本，按真正的 `kernel panic at` 检查，并完整重跑全部验证成功。这是测试包装器问题，没有修改分配器去规避断言。

以下五张为小组于 2026-10-19 自行运行并截取的现场画面，保留命令和结果，原图未作内容修改。它们分别验证默认 Best-Fit、课程评分、Buddy、SLUB 及一键完整复验。截图已核对与已有原始日志一致。

### 8.1 默认 Best-Fit 启动与自检

执行 `make clean && make qemu`，探测到 128 MiB RAM，使用 Best-Fit，初始空闲页数 31929，页面分配检查成功。

![Best-Fit 现场运行](images/01_best_fit.png)

### 8.2 课程评分

执行 `make grade`，Page Init 与 Best-Fit 两项均为 OK，总分 20/20。

![课程评分 20/20](images/02_grade.png)

### 8.3 Buddy System

执行 `make buddy`，管理器切换为 Buddy，`buddy_check()` 与页面分配检查均成功。本构建内核较小，初始空闲页数为 31930；与其他模式差一页来自镜像/元数据布局，不代表漏记页面。

![Buddy 现场自检](images/03_buddy.png)

### 8.4 简化 SLUB

执行 `make slub`，底层页管理器仍为 Best-Fit，上层对象分配测试 `slub_check()` 成功。这是两层管理结构，而不是遗漏切换页分配器。

![SLUB 现场自检](images/04_slub.png)

### 8.5 完整复验

执行 `python3 tests/verify_qemu.py`，默认启动、课程评分、Buddy、SLUB、原生分配器压力测试及 GDB/Sv39/页描述符检查全部 PASS，最后恢复默认 Best-Fit 构建。

![完整复验 ALL PASS](images/05_all_pass.png)

### 8.6 GDB 补充证据

以下为先前真实 GDB 日志的终端展示截图（仅截取终端区域），与上面的现场复验截图区分。完整输出见 `logs/gdb.txt`。

![GDB 与分页状态日志](images/02-gdb-no-cursor.png)

## 九、思考题与 OS 原理联系

无法预知内存范围时，可读取固件提供的内存地图：RISC-V/ARM 常用设备树 reg 与保留区域，PC 可用 UEFI GetMemoryMap 或 BIOS E820，并结合固件保留区及硬件窗口排除不可用区域。盲目逐地址写入探测可能访问 MMIO 或破坏固件，不是通用安全方法。实际实现应支持多区间、地址 cell 数和大小 cell 数及边界校验，本框架仅简化支持 QEMU 的一个范围。

| 实验知识点 | 对应 OS 原理及差异 |
|---|---|
| Page 元数据及保留页 | 资源记账与内核自举；元数据自身也消耗受管理 RAM |
| First/Best-Fit | 连续分配和外部碎片；能找到总量不代表能找到足够连续块 |
| Buddy | 二次幂快速拆合；减少搜索复杂性但产生内部碎片 |
| SLUB | 对象缓存与分层分配；本实现未提供每 CPU 缓存及并发 |
| Sv39 与 VA/PA | 地址翻译；本实验用大页静态映射，没有动态用户页表、缺页或换页 |

重要但未实现：进程调度、用户地址空间隔离、按需分页与交换、锁及多核内存管理、文件系统。AI 协作中应先写清不变量（计数、对齐、保留区域、不重叠），再用独立测试和真实平台运行验证；仅出现成功字符串不能代替测试逻辑正确性。

## 来源

- 老师发放的初始 lab2.zip，保留原测试。
- 指导书 Lab2 的练习与报告要求：<http://8.135.34.58/lab2026/_book/lab2/lab2_2_1_exercise.html>、<http://8.135.34.58/lab2026/_book/lab2/lab2_5_labs_requirement.html>（2026-10-19 已读取）。
- 课程主站：<http://oslab.mobisys.top/lab2026/_book/>；老师代码入口：<http://oslab.mobisys.top/lab2026/labcode.html>。
