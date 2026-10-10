# Lab1 实验提示词汇总

## 1. 初始环境配置
- 在VMware的Ubuntu 24虚拟机中完成操作系统实验环境配置，先将虚拟机名称修改为「操作系统」，允许删除虚拟机内旧内容，优先通过VSCode远程连接操作。
- 配置RISC-V实验所需的交叉编译器、QEMU模拟器，以及Claude Code + DeepSeek API的AI辅助开发环境。

## 2. 环境搭建过程
- 通过apt源安装git、build-essential、curl、wget等基础工具
- 通过Gitee镜像安装nvm，配置国内Node.js镜像，安装Node.js 22版本
- 通过Ubuntu apt源安装riscv64-unknown-elf交叉编译工具链、qemu-system-misc RISC-V模拟器
- 排查解决GitHub网络连接问题，优先使用国内镜像和apt源完成安装

## 3. 内核代码实现
- 排查新版QEMU OpenSBI内核加载地址问题，修改Makefile使用-kernel参数正确加载内核
- 补全SBI调用封装，实现sbi_console_putchar和sbi_console_getchar控制台输入输出函数
- 验证cprintf格式化输出功能，支持十进制、十六进制、字符、字符串等格式
- 修正getchar输入等待逻辑，解决无输入时空转刷屏问题
- 实现readline行编辑功能，支持字符回显、退格，回车读取完整输入行
- 实现最小交互式Shell循环，打印提示符，读取并回显用户输入的完整命令行

## 4. GDB调试体验
- 安装gdb-multiarch多架构调试器
- 修改Makefile debug目标，添加-s -S参数，实现QEMU启动即暂停、监听1234调试端口
- 验证调试端口监听状态，理解GDB远程调试的工作流程和使用方式

## 5. 实验收尾交付
- 完成lab1所有原理思考题，整理完整实验报告
- 初始化git仓库进行版本管理，按照课程要求整理仓库目录结构
- 将最终代码、实验报告、提示词和测试截图推送到小组GitHub仓库对应lab1分支
