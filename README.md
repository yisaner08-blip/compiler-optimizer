# 编译原理课程大作业 — C语言编译优化器

## 项目简介

本项目是编译原理课程小组大作业中的数据流分析与循环优化部分。程序读取四元式中间代码，通过多种数据流分析和循环优化技术生成优化后的四元式。

## 目录结构

```
bianyi/
├── src/                          # 源代码
│   ├── main.c                    # 主入口，解析参数，调度优化流程
│   ├── main_c_fixed.c            # 核心实现：数据结构、数据流分析、循环优化
│   └── optimizer.h               # 头文件：数据结构定义和函数声明
├── test/
│   ├── input/                    # 测试输入文件
│   │   ├── input1.txt            # 简单 while 循环
│   │   ├── input2.txt            # 数组操作 + 条件分支（对应测试用例4）
│   │   ├── input3.txt            # 复杂控制流 + 嵌套循环
│   │   └── input4.txt            # 多循环 + 二维数组 + 复杂表达式
│   └── expected/                 # 原始参考输出
├── output/                       # 优化输出（运行时生成）
├── build/                        # 编译产物
├── log/                          # 分析日志
├── docs/
│   └── tiaojian.md               # 实验要求文档（算法参考 + 测试用例）
├── .gitignore
└── README.md
```

## 已实现功能

### 数据流分析
- 到达-定值分析（前向迭代，gen/kill）
- 活跃变量分析（后向迭代，use/def）
- 可用表达式分析（前向迭代，eval/kill，交集交汇）
- 预期执行表达式分析（后向迭代，eval/kill，交集交汇）

### 循环优化
- **循环不变量外提（LICM）** — 单次扫描标记，同块内定值顺序感知，满足外提条件检查
- **强度削弱** — `iv * const` → 累加变量，正确计算增量（factor × stride）
- **循环展开** — 检测循环结构
- **归纳变量消除** — 显式乘法 + 步长比例双模式识别，替换循环条件

### 其他优化
- 跨块公共子表达式消除
- 指令简化（临时变量合并：`(op,a,b,t);(=,t,_,c)` → `(op,a,b,c)`）
- 死代码消除（迭代删除直到收敛）

## 编译与运行

```bash
# 编译（0 warnings）
gcc -o build/optimizer.exe src/main.c src/main_c_fixed.c -O2

# 运行
./build/optimizer.exe test/input/input1.txt output/out1.txt

# 调试模式（输出详细分析日志到 optimizer.log）
./build/optimizer.exe test/input/input1.txt output/out1.txt --log
```

## 输入输出格式

四元式：`(id) (op, arg1, arg2, result)`

```
(0) (func, main, _, _)
(1) (=, 10, _, n)
(2) (+, i, 1, t1)
(3) (label, _, _, L1)
(4) (if, t0, _, L2)
```

- `_` 表示空参数，内部转为 `null`
- 数组读：`([]=, 数组名, 下标, 结果)`
- 数组写：`(=[], 值, 下标, 数组名)`

## 优化示例（input2）

```
输入:                              输出:
i = 0                             i = 0
L1: i < 100                       t2 = 0          ← 初始化累加变量
    ...                           L1: t2 < 200    ← 循环条件替换
    t2 = i * 2                        ...
    a[i] = t2                         a[i] = t2
    ...                               ...
    i = i + 1                         i = i + 1
    goto L1                           t2 = t2 + 2  ← 强度削弱
                                      goto L1
```

## 核心数据结构

| 结构 | 说明 |
|------|------|
| `Quad` | 四元式（ID、操作符、arg1/2/3、result、removed标记） |
| `BasicBlock` | 基本块（指令列表、前驱/后继、IN/OUT/GEN/KILL集） |
| `Set` / `Map` | 基于二叉搜索树的字符串集合/映射 |
| `Vector` | 动态指针数组 |
