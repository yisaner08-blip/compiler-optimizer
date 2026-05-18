/**
 * @file optimizer.h
 * @brief 优化器所需的所有数据结构、全局变量和函数声明。
 */

#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include <stdio.h>

// --- 全局变量声明 ---

extern const char *INPUT_PATH;       // 默认输入文件路径
extern const char *OUTPUT_PATH;      // 默认输出文件路径
extern int globalInstrIdCounter; // 全局指令ID计数器，用于生成新的临时变量和指令ID
extern FILE *outFile;              // 全局文件指针，用于日志和最终结果的输出

// --- 自定义数据结构 ---

/** @brief 自定义的可动态扩展的字符串 */
typedef struct {
  char *data;     // 字符串内容
  int length;   // 当前长度
  int capacity; // 分配的容量
} MyString;

// ** 字符串操作函数 **
MyString *string_create();
void string_free(MyString *s);
void string_append(MyString *s, const char *str);
MyString *string_copy(const MyString *src);
int string_compare(const MyString *s1, const MyString *s2);
const char *string_cstr(const MyString *s);
void string_clear(MyString *s);

/** @brief 自定义的动态数组（向量），可存储任意类型的指针 */
typedef struct {
  void **data;    // 数据指针数组
  int size;       // 当前元素数量
  int capacity;   // 分配的容量
} Vector;

// ** 向量操作函数 **
Vector *vector_create();
void vector_free(Vector *v, void (*free_func)(void *));
void *vector_get(const Vector *v, int index);
void vector_push_back(Vector *v, void *item);
void vector_insert(Vector *v, int index, void *item);

/** @brief 二叉搜索树（BST）节点，是Set和Map的基础 */
typedef struct BSTNode {
  char *key;              // 键（字符串）
  void *value;            // 值（用于Map）
  struct BSTNode *left;
  struct BSTNode *right;
} BSTNode;

/** @brief 基于BST实现的字符串集合 */
typedef struct {
  BSTNode *root; // 集合的根节点
  int size;      // 集合大小
} Set;

// ** 集合操作函数 **
Set *set_create();
void set_free(Set *s);
int str_equal(const char *s1, const char *s2);
BSTNode *set_find(BSTNode *root, const char *key);
BSTNode *set_insert_recursive(BSTNode *root, const char *key);
void set_insert(Set *s, const char *key);
int set_contains(const Set *s, const char *key);
void set_inorder_traverse(BSTNode *root, void (*visit)(const char *));
void set_inorder_traverse_insert(BSTNode *root, Set *dest);

/** @brief 基于BST实现的字符串到指针的映射 */
typedef struct {
  BSTNode *root; // 映射的根节点
  int size;      // 映射大小
} Map;

// ** 映射操作函数 **
Map *map_create();
void map_free(Map *m);
BSTNode *map_find(BSTNode *root, const char *key);
BSTNode *map_insert_recursive(BSTNode *root, const char *key, void *value);
void map_insert(Map *m, const char *key, void *value);
void *map_get(const Map *m, const char *key, void *default_value);
int map_contains(const Map *m, const char *key);

// --- 核心代码表示 ---

/** @brief 四元式结构，表示一条中间代码指令 */
typedef struct {
  int id;           // 指令的唯一ID
  MyString *op;     // 操作符
  MyString *arg1;   // 参数1
  MyString *arg2;   // 参数2
  MyString *arg3;   // 参数3 (用于二维数组等)
  MyString *result; // 结果
  int removed;      // 标记该指令是否在优化中被删除
} Quad;

/** @brief 基本块结构 */
typedef struct {
  int id;                      // 基本块的唯一ID
  Vector *instructions;        // 块内的指令序列
  Vector *predecessors;        // 前驱基本块ID列表
  Vector *successors;          // 后继基本块ID列表
  Vector *preHeaderInstructions; // 用于循环不变量外提的指令
  // 数据流分析集合
  Set *in;    // IN 集合
  Set *out;   // OUT 集合
  Set *gen;   // GEN 集合
  Set *kill;  // KILL 集合
} BasicBlock;

// --- 函数声明 ---

// ** 核心结构创建与销毁 **
Quad *quad_create();
Quad *quad_copy(const Quad *src);
void quad_free(Quad *q);
int quad_is_calculation(const Quad *q);
int quad_is_branch(const Quad *q);
MyString *quad_to_string(const Quad *q);
BasicBlock *basic_block_create();
void basic_block_free(BasicBlock *bb);

// ** 全局状态管理 **
extern Vector *blocks;       // 存储所有基本块的全局向量
extern Map *labelToBlockId;  // 存储标签名到基本块ID映射的全局Map

/** @brief 初始化所有全局变量 */
void init_global_vars();

/** @brief 释放所有全局变量占用的内存 */
void free_global_vars();

// ** 工具与辅助函数 **

/** @brief 检查一个字符串是否为数字常量 */
int is_constant(const char *str);

/** @brief 检查一个字符串是否为变量名 */
int is_variable(const char *str);

/** @brief 从一个计算类型的四元式中提取其表达式字符串，如 "a + b" */
MyString *get_expression(const Quad *q);

/** @brief 打印一个Set集合的内容到文件，用于调试 */
void print_set(FILE *file, const Set *s, int max_items);

// ** 数据流分析核心算法 **

/** @brief 为到达-定值分析计算每个基本块的gen和kill集合 */
void compute_gen_kill();

/** @brief 计算两个集合的并集 */
Set *set_union(const Set *s1, const Set *s2);

/** @brief 计算两个集合的差集 (s1 - s2) */
Set *set_difference(const Set *s1, const Set *s2);

/** @brief 检查两个集合是否相等 */
int set_equal(const Set *s1, const Set *s2);

/** @brief 执行到达-定值分析 */
void reaching_definitions();

/** @brief 为活跃变量分析计算每个基本块的use和def集合 */
void compute_use_def();

/** @brief 执行活跃变量分析 */
void live_variables();

/** @brief 为可用表达式分析计算每个基本块的eval和kill集合 */
void compute_eval_kill_available();

/** @brief 计算两个集合的交集 */
Set *set_intersection(const Set *s1, const Set *s2);

/** @brief 执行可用表达式分析 */
void available_expressions();

/** @brief 为预期执行表达式分析计算每个基本块的eval和kill集合 */
void compute_eval_kill_very_busy();

/** @brief 执行预期执行的表达式（非常忙表达式）分析 */
void very_busy_expressions();

/** @brief 运行所有数据流分析的主函数 */
void perform_all_analyses();

/** @brief 基于可用表达式分析的跨基本块公共子表达式消除 */
void cross_block_cse();

/** @brief 常量传播、常量折叠和无用分支删除 */
void constant_propagation();

// ** 循环分析与优化 **

/** @brief 在CFG中通过寻找回边来识别所有循环 */
Map *find_loops();

/** @brief 检查一条指令在给定的循环中是否为循环不变量 */
int is_loop_invariant(Quad *q, int loop_header, int loop_back, Map *invariant_map);

/** @brief 检查一个变量在循环中是否有多个定义 */
int has_multiple_defs_in_loop(const char *var, int loop_header, int loop_back);

/** @brief 执行循环不变量外提（LICM）优化 */
void optimize_invariant_code_motion();

/** @brief 执行强度削弱优化，将循环中的乘法替换为加法 */
void optimize_strength_reduction();

/** @brief 执行简单的循环展开 */
void optimize_loop_unrolling();

/** @brief 在强度削弱后，尝试删除无用的基本归纳变量 */
void optimize_induction_variable_elimination();

/** @brief 修正循环回边，确保它们指向循环头而不是preheader */
void finalize_preheaders();

// ** 文件IO与最终处理 **

/** @brief 清理从文件中读取的行，去除括号和逗号 */
void clean_line(char *line, char *clean);

/** @brief 从文件加载四元式，并构建基本块和CFG */
void load_file(const char *path);

/** @brief 删除未被使用的变量定义（死代码） */
void eliminate_dead_code();

/** @brief 将所有外提的指令移动到程序的入口块 */
void move_preheader_to_start();

/** @brief 在所有优化完成后，重新为指令编号 */
void renumber_instructions();

/** @brief 打印最终优化后的四元式到输出文件 */
void print_final_result();

/** @brief 执行指令合并，减少临时变量的使用 */
void simplify_instructions();

#endif // OPTIMIZER_H
