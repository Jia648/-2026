# Lab2 实现规范与复现提示词

本文件记录本次实现采用的约束与可复现任务提示，并区分实现与测试任务。它不是对未保存的用户历史对话的逐字还原。所有测试结果以验证记录为准。

## 一、page_init 的 specification / prompt

阅读 `entry.S`、`memlayout.h`、`pmm.h`、`dtb.c` 和课程 `test_lab2.c`。仅在 `pmm.c` 对应 lab2 标记实现 `static void page_init(void)`，将 YOUR CODE 替换为 2413000 2414072。保留课程测试。框架描述符索引固定以 DRAM_BASE 为基准；设备树 base 不匹配、size 为零、范围溢出时 panic。设置虚实偏移；将可管理上界限制到 PHYSICAL_MEMORY_END 并向下页对齐；npage 保存绝对上界 PFN。把 pages 放在链接符号 end 之后页对齐处，为 npage-nbase 个描述符检查空间。初始化所有描述符为保留、引用为零；将内核及描述符之后的完整页交给 init_memmap；没有可用页不得调用 init_memmap。分别解释 VA、PA、PFN、Page*，输出物理范围和空闲页数。

## 二、Best-Fit specification / prompt

阅读 default_pmm.c 和 struct pmm_manager，保持 best_fit_pmm.c 中 basic_check、best_fit_check 完全不变。使用 free_area、free_list、nr_free 兼容原测试。空闲链表按地址递增；每次分配扫描全部块，选择能够满足 n 的最小块，同大小选择低地址；分割时原位保留余块；释放按地址插入并合并前后邻块，维护首节点 PageProperty 和总空闲页数。链表尾插入后立即退出遍历。定义 best_fit_pmm_manager，在 pmm.c 默认接入。不得修改测试以让错误实现通过。

## 三、Buddy 实现 prompt

从现有 pmm_manager 接口设计独立 Buddy 页分配器。按真实物理页号对齐，以 0 至 15 阶链表管理空闲块；任意初始区间分解为对齐的二次幂块。请求 n 向上取二次幂，按阶拆分；记录分配阶数，释放时以 PFN XOR 找唯一伙伴，仅合并同阶空闲伙伴，防止越界合并。free_pages 必须接受原请求 n，不支持部分释放。空闲计数体现实际占用的二次幂页数。用 USE_BUDDY 宏独立接入，不改变默认 Best-Fit 的课程评分行为。单 CPU 启动环境，不宣称支持并发。

## 四、Buddy 测试 prompt

为实际 Buddy 模块编写独立测试，不复用 First-Fit 的精确页数假设。覆盖 0 请求、超量请求、非二次幂向上取整、分配区间不重叠、拆分、乱序释放、空闲计数恢复、块对齐及链表计数一致性。在宿主模拟中额外完整耗尽 2048 页，确认失败，再交错释放并要求可重新分配整个 2048 页块。用确定性随机序列同时检查活跃区间互不重叠和 used+free 守恒。测试触发断言即失败，不把仅打印成功当验证。

## 五、SLUB 实现 prompt

实现教学简化 SLUB，不声称实现 Linux 完整 SLUB。底层使用 alloc_pages/free_pages，上层按 16 至 2048 字节二次幂大小类管理 slab 页面，页头记录所属页、对象大小、使用数和 bitmap。对象 16 字节对齐；优先复用空位、满页创建新 slab、空页归还页分配器。大对象按整页数另行分配，页头保存原请求页数，使 Buddy 后端也能正确释放。kmalloc(0) 返回 NULL；大小算术溢出返回 NULL；kfree(NULL) 无操作；拒绝内部指针或无效释放；不提供锁或每 CPU 缓存。用 RUN_SLUB 宏在默认 Best-Fit 下执行测试。

## 六、SLUB 测试 prompt

针对真实 kmalloc/kfree，测试所有大小类边界和 2048 以上大对象；写满每个请求长度，再验证数据未被其他对象破坏；检查 16 字节对齐，0 和极大请求、NULL 释放。连续分配 300 个 16 字节对象，迫使多 slab；交错释放、复用空洞，最终检查所有页归还。宿主中分别使用 Best-Fit 和 Buddy 作后端执行同一组测试。内核默认仅运行明确启用的测试，避免污染官方默认管理器测试。

## 七、集成复核 prompt

保持原始评分注入器和断言不变；只调整评分 QEMU 启动参数为适配本环境的 -kernel。检查 satp 写入后的当前物理指令地址有恒等映射，并同时有高半区映射；将入口放入链接脚本优先的 .text.kern_entry。用固定寄存器约束实现 legacy SBI 调用。实际构建、运行、截图分别记录，不把宿主模拟或交叉链接等同于 QEMU 已通过。
