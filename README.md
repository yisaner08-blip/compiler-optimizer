# 编译原理课程大作业 — C语言编译优化器

## 项目简介

编译原理课程小组大作业——数据流分析与循环优化部分。读取四元式中间代码，通过多种优化技术生成优化后的四元式。

**GitHub**: https://github.com/yisaner08-blip/compiler-optimizer

## 目录结构

```
bianyi/
├── src/
│   ├── main.c              # 主入口，调度优化管线
│   ├── main_c_fixed.c      # 核心实现 (~3000行)
│   ├── optimizer.h         # 头文件
│   └── bitset.h            # BitSet 位集模块
├── test/
│   ├── input/              # 测试输入 (10个)
│   └── expected/           # 参考输出 (4个)
├── output/                 # 优化输出（运行时生成）
├── build/                  # 编译产物
├── Makefile                # 构建脚本
├── docs/tiaojian.md        # 实验要求文档
└── README.md
```

## 编译与运行

```bash
# 编译
make
# 或手动编译
gcc -O2 -Wall -Wextra -o build/optimizer.exe src/main.c src/main_c_fixed.c

# 运行
./build/optimizer.exe test/input/input1.txt output/out1.txt

# 调试（输出详细日志到 optimizer.log）
./build/optimizer.exe test/input/input1.txt output/out1.txt --log

# 运行全部测试
make test

# 清理
make clean
```

## 优化管线

```
cross_block_cse → constant_propagation → peephole_optimize → constant_propagation (第二轮)
→ LICM → SR → loop_unrolling → IVE
→ simplify_instructions → peephole_optimize (第二轮)
→ eliminate_dead_stores → eliminate_dead_code → dead_block_elimination
→ merge_basic_blocks → move_preheader_to_start → renumber_instructions
```

## 优化能力一览

### 数据流分析（4种）
| 分析 | 方向 | 汇合方式 |
|------|------|---------|
| 到达-定值分析 | 前向 | ∪ |
| 活跃变量分析 | 后向 | ∪ |
| 可用表达式分析 | 前向 | ∩ |
| 预期执行表达式 | 后向 | ∩ |

### 循环优化（4种）
| 优化 | 说明 |
|------|------|
| 循环不变量外提 (LICM) | 单次扫描，最外层循环，数组写感知，多定值安全检查 |
| 强度削弱 (SR) | iv×const → 累加变量，factor×stride 增量 |
| 循环展开 (Unroll) | 单基本块小常量迭代次数（≤4）全展开，IV常量替换 |
| 归纳变量消除 (IVE) | 显式乘法+步长比(排除基本IV)，替换循环条件，安全删除IV |

### 全局优化（9种）
| 优化 | 说明 |
|------|------|
| 跨基本块 CSE | 基于AE的in集合，单前驱块间冗余计算替换 |
| 常量传播 | 入口块记录+has_later_def检查+全局替换，多轮迭代 |
| 常量折叠 | int64_t 双常量操作数求值(加减乘除除零比较)，溢出检测 |
| 分支折叠 | if(1)→goto, if(0)→删除，自动清理goto的arg1 |
| **窥孔优化** | 13条代数恒等规则：x+0→x, x-0→x, x*1→x, x/1→x, x*0→0, 0/x→0, x-x→0, x/x→1, 0+x→x, 1*x→x, x=x删除等 |
| **无用存储消除** | 同一变量连续赋值覆盖写入消除 |
| **基本块合并** | 相邻fallthrough单前驱块合并，减少CFG碎片 |
| 死块删除 | DFS可达性分析，删除不可达基本块 |
| 死代码消除 | 迭代删除未使用定义 |
| 指令简化 | (op,a,b,t);(=,t,_,c)→(op,a,b,c) 安全合并 |

> **加粗**项为 v2.0 新增优化。

### 基础设施
| 模块 | 说明 |
|------|------|
| BitSet | 64位字对齐位集，并/交/差 O(n_words) |
| 支配树 | Cooper-Harvey-Kennedy 算法 |
| 自然循环检测 | 支配边判断 + 反向BFS收集循环体 |
| 统一求解器 | dataflow_solve_forward/backward |
| 自定义容器 | MyString, Vector, Set(BST), Map(BST) |

## 测试用例

### 原始测试（4个）
| 输入 | 优化前 | 优化后 | 缩减 | 主要优化 |
|------|--------|--------|------|---------|
| input1 | 13行 | 11行 | -15% | DCE删除死存储 |
| input2 | 33行 | 34行 | — | SR累加 + IVE条件替换 |
| input3 | 50行 | 32行 | **-36%** | 3分支折叠 + 死块删除 + CP33次 |
| input4 | 102行 | 100行 | -2% | LICM外提 + SR累加 + IVE |

### 针对性测试（6个，v2.0 新增）
| 输入 | 内容 | 演示优化 |
|------|------|---------|
| input5_peephole | 13条代数恒等表达式 | 窥孔优化 + 常量传播 |
| input6_deadstore | 连续覆盖写入 | 无用存储消除 |
| input7_unroll | i<3 小迭代循环 | 强度削弱 SR |
| input8_merge | 直行fallthrough块链 | 基本块合并 |
| input9_copyprop | val→copy1→copy2→copy3 链 | 复制传播 + 常量传播 |
| input10_composite | 综合循环+复制+常量 | 多pass协同优化 |

## 更新日志

### v2.0 (2026-05)
- **新增4个优化pass**: 窥孔优化、无用存储消除、基本块合并、循环展开(完整实现)
- **优化管线升级**: 常量传播两轮迭代，窥孔两轮调用
- **Bug修复**: 
  - `fclose(outFile)` 输出文件关闭
  - `is_constant()` 全字符串校验（修复 `is_constant("5abc")=true` ）
  - `try_const_fold()` int64_t 溢出防护 + INT_MAX/INT_MIN 范围检查
  - BranchFold 后 `goto` 的 arg1 残留常量值修复
  - 移除未使用的 `bitset.h` include 和 200+ 行死代码
- **新增6个测试用例**，测试覆盖从4→10个
- **新增 Makefile** 支持 `make` / `make test` / `make clean`
- **添加 `<assert.h>`, `<limits.h>` 头文件**

### v1.0 (2025-12)
- 初始版本，实现4种数据流分析 + 4种循环优化 + 6种全局优化

## 参考项目
- [compiler-ref-optimization-passes](https://github.com/baziotis/compiler-optimization) — BitSet, 支配树, LVN
- [chibicc](https://github.com/rui314/chibicc) — 简洁C编译器架构
- [acwj](https://github.com/DoctorWkt/acwj) — 编译原理教程
