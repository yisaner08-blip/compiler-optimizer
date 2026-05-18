# 编译原理课程大作业 — C语言编译优化器

## 项目简介

编译原理课程小组大作业——数据流分析与循环优化部分。读取四元式中间代码，通过多种优化技术生成优化后的四元式。

**GitHub**: https://github.com/yisaner08-blip/compiler-optimizer

## 目录结构

```
bianyi/
├── src/
│   ├── main.c              # 主入口，调度优化管线
│   ├── main_c_fixed.c      # 核心实现 (~2800行)
│   ├── optimizer.h         # 头文件
│   └── bitset.h            # BitSet 位集模块
├── test/
│   ├── input/              # 测试输入 (4个)
│   └── expected/           # 参考输出 (4个)
├── output/                 # 优化输出（运行时生成）
├── build/                  # 编译产物
├── log/                    # 分析日志
├── docs/tiaojian.md        # 实验要求文档
└── README.md
```

## 编译与运行

```bash
# 编译
gcc -o build/optimizer.exe src/main.c src/main_c_fixed.c -O2

# 运行
./build/optimizer.exe test/input/input1.txt output/out1.txt

# 调试（输出详细日志到 optimizer.log）
./build/optimizer.exe test/input/input1.txt output/out1.txt --log
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
| 循环不变量外提 (LICM) | 单次扫描，最外层循环，数组写感知 |
| 强度削弱 (SR) | iv×const → 累加变量，factor×stride 增量 |
| 循环展开 | 检测结构（完整展开待实现） |
| 归纳变量消除 (IVE) | 显式乘法+步长比(排除基本IV)，替换循环条件 |

### 全局优化（6种）
| 优化 | 说明 |
|------|------|
| 常量传播 | 入口块记录+has_later_def检查+全局替换 |
| 常量折叠 | 双常量操作数求值(加减乘除比较) |
| 分支折叠 | if(1)→goto, if(0)→删除 |
| 死块删除 | DFS可达性分析，删除不可达基本块 |
| 死代码消除 | 迭代删除未使用定义 |
| 指令简化 | (op,a,b,t);(=,t,_,c)→(op,a,b,c) 安全合并 |

### 基础设施
| 模块 | 说明 |
|------|------|
| BitSet | 64位字对齐位集，并/交/差 O(n_words) |
| 支配树 | Cooper-Harvey-Kennedy 算法 |
| 自然循环检测 | 支配边判断 + 反向BFS收集循环体 |
| 统一求解器 | dataflow_solve_forward/backward |

## 测试效果

| 输入 | 优化前 | 优化后 | 缩减 | 主要优化 |
|------|--------|--------|------|---------|
| input1 | 13行 | 12行 | -7% | DCE |
| input2 | 33行 | 35行 | — | SR累加 + IVE条件替换 |
| input3 | 50行 | 33行 | **-34%** | 3分支折叠 + 5死块删除 |
| input4 | 102行 | 101行 | -1% | LICM外提 + SR累加 |

## 参考项目
- [compiler-ref-optimization-passes](https://github.com/baziotis/compiler-optimization) — BitSet, 支配树, LVN
- [chibicc](https://github.com/rui314/chibicc) — 简洁C编译器架构
- [acwj](https://github.com/DoctorWkt/acwj) — 编译原理教程
