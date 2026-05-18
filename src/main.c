/**
 * @file main.c
 * @brief 优化器的主入口文件。
 * 
 * 负责解析命令行参数，初始化全局变量，
 * 按顺序调用各个分析和优化阶段，并最终输出优化后的四元式。
 */

#include "optimizer.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
  // --- 1. 初始化与参数解析 ---

  // 默认输入输出文件路径，可以被命令行参数覆盖
  const char *input_file = INPUT_PATH;
  const char *output_file = OUTPUT_PATH;

  // 从命令行参数获取输入和输出文件路径
  if (argc >= 2) {
    input_file = argv[1];
  }
  if (argc >= 3) {
    output_file = argv[2];
  }

  // 初始化全局数据结构，如基本块列表等
  init_global_vars();

  // --- 2. 日志文件设置 ---

  // 检查是否传入了 "--log" 参数以启用详细的分析日志
  int enable_log = 0;
  if (argc >= 4 && strcmp(argv[3], "--log") == 0) enable_log = 1;

  // 根据是否启用日志，将日志输出重定向到 "optimizer.log" 文件或空设备
#ifdef _WIN32
  const char* sink = enable_log ? "optimizer.log" : "NUL";
#else
  const char* sink = enable_log ? "optimizer.log" : "/dev/null";
#endif
  outFile = fopen(sink, "w");
  if (!outFile) {
    fprintf(stderr, "无法打开日志输出目标 %s\n", sink);
    free_global_vars();
    return 1;
  }

  // --- 3. 加载与分析 ---

  // 从输入文件加载四元式，并构建基本块和控制流图(CFG)
  load_file(input_file);

  // 执行所有数据流分析（到达-定值、活跃变量、可用表达式等）
  perform_all_analyses();

  // --- 4. 优化阶段 ---

  // 执行跨基本块的公共子表达式消除
  cross_block_cse();

  // 常量传播与折叠（在循环优化之前，以简化循环条件）
  constant_propagation();
  dead_block_elimination();     // 删除常量折叠后产生的不可达块

  // 执行循环优化
  optimize_invariant_code_motion();      // 循环不变量外提
  optimize_strength_reduction();         // 强度削弱
  optimize_loop_unrolling();             // 循环展开
  optimize_induction_variable_elimination(); // 归纳变量删除

  // 执行其他全局优化
  simplify_instructions();      // 合并冗余的临时变量赋值
  eliminate_dead_code();        // 删除死代码
  /* dead_block_elimination 待修复后启用 */

  // --- 5. 清理与输出 ---

  // 将所有外提的 preheader 指令统一移动到程序的入口块
  move_preheader_to_start();
  // 重新为所有指令编号，保持连续性
  renumber_instructions();

  // 关闭日志文件
  fclose(outFile);
  
  // 重新打开输出文件，准备写入最终的优化结果
  outFile = fopen(output_file, "w");
  if (!outFile) {
    fprintf(stderr, "无法写入 %s\n", output_file);
    free_global_vars();
    return 1;
  }
  // 打印优化后的四元式到输出文件
  print_final_result();

  // --- 6. 资源释放 ---

  // 释放所有动态分配的内存
  free_global_vars();

  printf("优化完成，请查看: %s\n", output_file);
  return 0;
}
