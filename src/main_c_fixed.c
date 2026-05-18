#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "optimizer.h"
#include "bitset.h"

// ==========================================
// 1. 全局配置
// ==========================================

// 默认文件路径
const char *INPUT_PATH = "test/input/input1.txt";
const char *OUTPUT_PATH = "output/output1.txt";

// 指令计数器，用于生成新指令ID
int globalInstrIdCounter = 2000;

// 输出文件指针
FILE *outFile;


// ==========================================
// 2. 自定义容器实现
// ==========================================

// 初始化字符串
MyString *string_create() {
  MyString *s = (MyString *)malloc(sizeof(MyString));
  s->data = (char *)malloc(32 * sizeof(char));
  s->data[0] = '\0';
  s->length = 0;
  s->capacity = 32;
  return s;
}

// 释放字符串
void string_free(MyString *s) {
  free(s->data);
  free(s);
}

// 连接字符串
void string_append(MyString *s, const char *str) {
  int len = strlen(str);
  while (s->length + len + 1 > s->capacity) {
    s->capacity *= 2;
    s->data = (char *)realloc(s->data, s->capacity * sizeof(char));
  }
  strcat(s->data, str);
  s->length += len;
}

// 复制字符串
MyString *string_copy(const MyString *src) {
  MyString *dest = string_create();
  string_append(dest, src->data);
  return dest;
}

// 比较字符串
int string_compare(const MyString *s1, const MyString *s2) {
  return strcmp(s1->data, s2->data);
}

// 转换为char*
const char *string_cstr(const MyString *s) { return s->data; }

// 清空字符串
void string_clear(MyString *s) {
  s->data[0] = '\0';
  s->length = 0;
}

// 初始化向量
Vector *vector_create() {
  Vector *v = (Vector *)malloc(sizeof(Vector));
  v->data = (void **)malloc(8 * sizeof(void *));
  v->size = 0;
  v->capacity = 8;
  return v;
}

// 释放向量
void vector_free(Vector *v, void (*free_func)(void *)) {
  if (free_func) {
    for (int i = 0; i < v->size; i++) {
      free_func(v->data[i]);
    }
  }
  free(v->data);
  free(v);
}

// 获取向量元素
void *vector_get(const Vector *v, int index) {
  if (index < 0 || index >= v->size) {
    return NULL;
  }
  return v->data[index];
}

// 添加元素到向量末尾
void vector_push_back(Vector *v, void *item) {
  if (v->size == v->capacity) {
    v->capacity *= 2;
    v->data = (void **)realloc(v->data, v->capacity * sizeof(void *));
  }
  v->data[v->size++] = item;
}

// 插入元素到向量指定位置
void vector_insert(Vector *v, int index, void *item) {
  if (index < 0 || index > v->size) {
    return;
  }
  if (v->size == v->capacity) {
    v->capacity *= 2;
    v->data = (void **)realloc(v->data, v->capacity * sizeof(void *));
  }
  for (int i = v->size; i > index; i--) {
    v->data[i] = v->data[i - 1];
  }
  v->data[index] = item;
  v->size++;
}

// 创建BST节点
static char* str_dup(const char* s) {
  size_t n = strlen(s) + 1;
  char* d = (char*)malloc(n);
  if (d) memcpy(d, s, n);
  return d;
}

BSTNode *bst_node_create(const char *key, void *value) {
  BSTNode *node = (BSTNode *)malloc(sizeof(BSTNode));
  node->key = str_dup(key);
  node->value = value;
  node->left = NULL;
  node->right = NULL;
  return node;
}

// 释放BST节点
void bst_node_free(BSTNode *node, void (*free_value)(void *)) {
  if (!node) return;
  bst_node_free(node->left, free_value);
  bst_node_free(node->right, free_value);
  free(node->key);
  if (free_value) {
    free_value(node->value);
  }
  free(node);
}

// 初始化Set
Set *set_create() {
  Set *s = (Set *)malloc(sizeof(Set));
  s->root = NULL;
  s->size = 0;
  return s;
}

// 释放Set
void set_free(Set *s) {
  bst_node_free(s->root, NULL);
  free(s);
}

// 比较字符串
int str_equal(const char *s1, const char *s2) { return strcmp(s1, s2) == 0; }

// 查找Set中的元素
BSTNode *set_find(BSTNode *root, const char *key) {
  if (!root) {
    return NULL;
  }
  int cmp = strcmp(key, root->key);
  if (cmp < 0) {
    return set_find(root->left, key);
  } else if (cmp > 0) {
    return set_find(root->right, key);
  } else {
    return root;
  }
}

// 插入Set中的元素
BSTNode *set_insert_recursive(BSTNode *root, const char *key) {
  if (!root) {
    return bst_node_create(key, NULL);
  }
  int cmp = strcmp(key, root->key);
  if (cmp < 0) {
    root->left = set_insert_recursive(root->left, key);
  } else if (cmp > 0) {
    root->right = set_insert_recursive(root->right, key);
  }
  return root;
}

// 插入Set
void set_insert(Set *s, const char *key) {
  if (!set_find(s->root, key)) {
    s->root = set_insert_recursive(s->root, key);
    s->size++;
  }
}

// 检查Set是否包含元素
int set_contains(const Set *s, const char *key) {
  return set_find(s->root, key) != NULL;
}

// Set的中序遍历
void set_inorder_traverse(BSTNode *root, void (*visit)(const char *)) {
  if (root) {
    set_inorder_traverse(root->left, visit);
    visit(root->key);
    set_inorder_traverse(root->right, visit);
  }
}

// Set的中序遍历，用于插入到另一个Set
void set_inorder_traverse_insert(BSTNode *root, Set *dest) {
  if (root) {
    set_inorder_traverse_insert(root->left, dest);
    set_insert(dest, root->key);
    set_inorder_traverse_insert(root->right, dest);
  }
}

// 初始化Map
Map *map_create() {
  Map *m = (Map *)malloc(sizeof(Map));
  m->root = NULL;
  m->size = 0;
  return m;
}

// 释放Map
void map_free(Map *m) {
  bst_node_free(m->root, NULL);
  free(m);
}

// 查找Map中的元素
BSTNode *map_find(BSTNode *root, const char *key) {
  if (!root) {
    return NULL;
  }
  int cmp = strcmp(key, root->key);
  if (cmp < 0) {
    return map_find(root->left, key);
  } else if (cmp > 0) {
    return map_find(root->right, key);
  } else {
    return root;
  }
}

// 插入Map中的元素
BSTNode *map_insert_recursive(BSTNode *root, const char *key, void *value) {
  if (!root) {
    return bst_node_create(key, value);
  }
  int cmp = strcmp(key, root->key);
  if (cmp < 0) {
    root->left = map_insert_recursive(root->left, key, value);
  } else if (cmp > 0) {
    root->right = map_insert_recursive(root->right, key, value);
  } else {
    // 更新现有值
    root->value = value;
  }
  return root;
}

// 插入Map
void map_insert(Map *m, const char *key, void *value) {
  m->root = map_insert_recursive(m->root, key, value);
  m->size++;
}

// 获取Map中的值
void *map_get(const Map *m, const char *key, void *default_value) {
  BSTNode *node = map_find(m->root, key);
  return node ? node->value : default_value;
}

// 检查Map是否包含键
int map_contains(const Map *m, const char *key) {
  return map_find(m->root, key) != NULL;
}

// ==========================================
// 3. 四元式和基本块
// ==========================================

// 创建四元式
Quad *quad_create() {
  Quad *q = (Quad *)malloc(sizeof(Quad));
  q->id = globalInstrIdCounter++;
  q->op = string_create();
  q->arg1 = string_create();
  q->arg2 = string_create();
  q->arg3 = string_create();
  q->result = string_create();
  q->removed = 0;
  return q;
}

// 复制四元式
Quad *quad_copy(const Quad *src) {
  Quad *dest = quad_create();
  dest->id = src->id;
  string_append(dest->op, string_cstr(src->op));
  string_append(dest->arg1, string_cstr(src->arg1));
  string_append(dest->arg2, string_cstr(src->arg2));
  string_append(dest->arg3, string_cstr(src->arg3));
  string_append(dest->result, string_cstr(src->result));
  dest->removed = src->removed;
  return dest;
}

// 释放四元式
void quad_free(Quad *q) {
  string_free(q->op);
  string_free(q->arg1);
  string_free(q->arg2);
  string_free(q->arg3);
  string_free(q->result);
  free(q);
}

// 检查四元式是否为计算指令
int quad_is_calculation(const Quad *q) {
  const char *op = string_cstr(q->op);
  return str_equal(op, "+") || str_equal(op, "-") || str_equal(op, "*") ||
         str_equal(op, "/") || str_equal(op, "<") || str_equal(op, ">") ||
         str_equal(op, "<=") || str_equal(op, ">=") || str_equal(op, "==") ||
         str_equal(op, "!=");
}

// 检查四元式是否为分支指令
int quad_is_branch(const Quad *q) {
  const char *op = string_cstr(q->op);
  return str_equal(op, "if") || str_equal(op, "goto") || str_equal(op, "ret");
}

// Helper function to get a string for printing, or "_" if it's null/empty.
static const char* get_printable_str(const MyString* s) {
    const char* c_str = string_cstr(s);
    if (c_str == NULL || c_str[0] == '\0' || strcmp(c_str, "null") == 0) {
        return "_";
    }
    return c_str;
}

// 四元式转换为字符串
MyString *quad_to_string(const Quad *q) {
  MyString *str = string_create();
  char buffer[512];

  const char *op = get_printable_str(q->op);
  const char *arg1 = get_printable_str(q->arg1);
  const char *arg2 = get_printable_str(q->arg2);
  const char *arg3 = get_printable_str(q->arg3);
  const char *result = get_printable_str(q->result);

  int is_array_op = (strcmp(op, "[]=") == 0) || (strcmp(op, "=[]") == 0);
  int has_third = (arg3 && strcmp(arg3, "_") != 0 && strcmp(arg3, "null") != 0);

  if (is_array_op && has_third) {
    // 五元式（二维数组）
    sprintf(buffer, "(%d) (%s, %s, %s, %s, %s)", q->id, op, arg1, arg2, arg3, result);
  } else {
    // 普通四元式
    sprintf(buffer, "(%d) (%s, %s, %s, %s)", q->id, op, arg1, arg2, result);
  }

  string_append(str, buffer);
  return str;
}

// 创建基本块
BasicBlock *basic_block_create() {
  BasicBlock *bb = (BasicBlock *)malloc(sizeof(BasicBlock));
  bb->id = 0;
  bb->instructions = vector_create();
  bb->predecessors = vector_create();
  bb->successors = vector_create();
  bb->preHeaderInstructions = vector_create();
  bb->in = set_create();
  bb->out = set_create();
  bb->gen = set_create();
  bb->kill = set_create();
  return bb;
}

// 释放基本块
void basic_block_free(BasicBlock *bb) {
  vector_free(bb->instructions, (void (*)(void *))quad_free);
  vector_free(bb->predecessors, free);
  vector_free(bb->successors, free);
  vector_free(bb->preHeaderInstructions, (void (*)(void *))quad_free);
  set_free(bb->in);
  set_free(bb->out);
  set_free(bb->gen);
  set_free(bb->kill);
  free(bb);
}

// ==========================================
// 4. 全局变量
// ==========================================

// 基本块列表
Vector *blocks;

// 标签到基本块ID的映射
Map *labelToBlockId;

// 辅助：按ID查找基本块，减少重复遍历
static BasicBlock *find_block_by_id(int id) {
  for (int i = 0; i < blocks->size; ++i) {
    BasicBlock *b = (BasicBlock *)vector_get(blocks, i);
    if (b->id == id) {
      return b;
    }
  }
  return NULL;
}

// 根据块指针查找其在 blocks 向量中的索引
static int find_block_index(const BasicBlock* blk) {
  for (int i = 0; i < blocks->size; ++i) {
    if ((BasicBlock*)vector_get(blocks, i) == blk) return i;
  }
  return -1;
}

// 在块内找到第一个 label 名称
static const char* get_block_label(BasicBlock* b) {
  for (int j = 0; j < b->instructions->size; ++j) {
    Quad *q = (Quad *)vector_get(b->instructions, j);
    if (str_equal(string_cstr(q->op), "label")) {
      return string_cstr(q->result);
    }
  }
  return NULL;
}

// 创建一个新的 label 指令四元式
static Quad* make_label_quad(const char* name) {
  Quad* q = quad_create();
  string_append(q->op, "label");
  string_append(q->arg1, "null");
  string_append(q->arg2, "null");
  string_append(q->arg3, "null");
  string_append(q->result, name);
  return q;
}

// 重新构建 labelToBlockId、pred/succ，并重编号 block.id
void rebuild_cfg() {
  // 清空旧映射
  map_free(labelToBlockId);
  labelToBlockId = map_create();

  // 重编号并清空 pred/succ
  for (int i = 0; i < blocks->size; ++i) {
    BasicBlock *b = (BasicBlock *)vector_get(blocks, i);
    b->id = i + 1;
    // 清理旧的 pred/succ
    vector_free(b->predecessors, free);
    vector_free(b->successors, free);
    b->predecessors = vector_create();
    b->successors = vector_create();
  }

  // 建 label 映射
  for (int i = 0; i < blocks->size; ++i) {
    BasicBlock *b = (BasicBlock *)vector_get(blocks, i);
    for (int j = 0; j < b->instructions->size; ++j) {
      Quad *q = (Quad *)vector_get(b->instructions, j);
      if (str_equal(string_cstr(q->op), "label")) {
        map_insert(labelToBlockId, string_cstr(q->result), (void*)(intptr_t)b->id);
      }
    }
  }

  // 按 load_file 的逻辑重建 CFG
  for (int i = 0; i < blocks->size; ++i) {
    BasicBlock *block = (BasicBlock *)vector_get(blocks, i);
    if (block->instructions->size == 0) continue;
    Quad *lastQ = (Quad *)vector_get(block->instructions, block->instructions->size - 1);

    if (!str_equal(string_cstr(lastQ->op), "goto") &&
        !str_equal(string_cstr(lastQ->op), "ret") && i < blocks->size - 1) {
      BasicBlock *nextBlock = (BasicBlock *)vector_get(blocks, i + 1);
      int *succ = (int *)malloc(sizeof(int)); *succ = nextBlock->id; vector_push_back(block->successors, succ);
      int *pred = (int *)malloc(sizeof(int)); *pred = block->id; vector_push_back(nextBlock->predecessors, pred);
    }

    const char *target = "";
    if (str_equal(string_cstr(lastQ->op), "goto") || str_equal(string_cstr(lastQ->op), "if")) {
      target = string_cstr(lastQ->result);
    }
    if (target[0] != '\0' && !str_equal(target, "null") && map_contains(labelToBlockId, target)) {
      int tid = (int)(intptr_t)map_get(labelToBlockId, target, (void*)-1);
      int *succ = (int *)malloc(sizeof(int)); *succ = tid; vector_push_back(block->successors, succ);
      for (int j = 0; j < blocks->size; ++j) {
        BasicBlock *b = (BasicBlock *)vector_get(blocks, j);
        if (b->id == tid) { int *pred = (int *)malloc(sizeof(int)); *pred = block->id; vector_push_back(b->predecessors, pred); break; }
      }
    }
  }
}

// 创建或获取"真正的一次性" preheader：插在 header 之前，并仅由循环外的前驱跳转到该 preheader，
// preheader 再顺序落到 header。来自循环内部的回边仍然跳到 header，不会触发 preheader。
BasicBlock* get_or_create_preheader_for_header(int header_id, int back_id) {
  BasicBlock *header = find_block_by_id(header_id);
  if (!header) return NULL;

  // 记录循环内部块的指针集合（在结构改动前获取）
  Vector *internal_blocks = vector_create();
  for (int i = 0; i < blocks->size; ++i) {
    BasicBlock *b = (BasicBlock *)vector_get(blocks, i);
    if (b->id >= header_id && b->id <= back_id) {
      vector_push_back(internal_blocks, b);
    }
  }

  // 获取/创建 header 的标签名；若当前 header 本身是 PREHDR_/PRE_ 块，则将 header 滚动到其后的真实头块
  const char *header_label = get_block_label(header);
  if (!header_label) {
    char auto_label[32]; sprintf(auto_label, "Lh%d", header_id);
    Quad *lbl = make_label_quad(auto_label);
    vector_insert(header->instructions, 0, lbl);
    header_label = string_cstr(lbl->result);
  }
  // 若 header_label 带有 PREHDR_/PRE_ 前缀，则将 header 指向下一个块，直到拿到真实头块的标签
  while (strncmp(header_label, "PREHDR_", 7) == 0 || strncmp(header_label, "PRE_", 4) == 0) {
    int hidx_tmp = find_block_index(header);
    if (hidx_tmp >= 0 && hidx_tmp + 1 < blocks->size) {
      header = (BasicBlock *)vector_get(blocks, hidx_tmp + 1);
      header_label = get_block_label(header);
      if (!header_label) {
        char auto_label2[32]; sprintf(auto_label2, "Lh%d_real", header->id);
        Quad *lbl2 = make_label_quad(auto_label2);
        vector_insert(header->instructions, 0, lbl2);
        header_label = string_cstr(lbl2->result);
      }
    } else {
      break;
    }
  }

  // 规范化 preheader 名称：PREHDR_<base>
  const char *base = header_label;
  if (strncmp(base, "PREHDR_", 7) == 0) base += 7;
  if (strncmp(base, "PRE_", 4) == 0) base += 4;
  char pre_label[300];
  snprintf(pre_label, sizeof(pre_label), "PREHDR_%s", base);

  // 若已存在，直接返回
  if (map_contains(labelToBlockId, pre_label)) {
    int pre_id = (int)(intptr_t)map_get(labelToBlockId, pre_label, (void*)-1);
    return find_block_by_id(pre_id);
  }

  // 在 header 之前插入 preheader
  BasicBlock *pre = basic_block_create();
  Quad *pre_lbl = make_label_quad(pre_label);
  vector_push_back(pre->instructions, pre_lbl);
  int hidx = find_block_index(header);
  if (hidx < 0) return NULL;
  vector_insert(blocks, hidx, pre);

  // 重定向：将所有"来自循环外"的显式跳转（goto/if）目标为 header_label 的边改为 preheader
  for (int i = 0; i < blocks->size; ++i) {
    BasicBlock *b = (BasicBlock *)vector_get(blocks, i);
    if (b->id >= header_id && b->id <= back_id) continue; // 跳过循环内部块
    for (int j = 0; j < b->instructions->size; ++j) {
      Quad *q = (Quad *)vector_get(b->instructions, j);
      if (str_equal(string_cstr(q->op), "goto") || str_equal(string_cstr(q->op), "if")) {
        if (!str_equal(string_cstr(q->result), "null") &&
            strcmp(string_cstr(q->result), header_label) == 0) {
          string_clear(q->result); string_append(q->result, pre_label);
        }
      }
    }
  }

  // 不修改 header.if 的真分支（仍然指向原来的循环体 body），preheader 只会被"来自循环外"的路径触发一次；
  // fallthrough 外部前驱（非显式分支）也会因为我们把 pre 插在 header 之前而自然落到 pre，再到 header。

  rebuild_cfg();

  // 追加：确保循环内部的回边不指向 preheader，而是指向 header
  const char *hdr_label2 = get_block_label(header);
  // 仅修正循环"内部块"的分支目标，避免修改来自循环外的进入边
  for (int i = 0; i < blocks->size; ++i) {
    BasicBlock *b = (BasicBlock *)vector_get(blocks, i);
    if (!b || b == pre) continue;
    int is_internal = 0;
    for (int t = 0; t < internal_blocks->size; ++t) {
      if (vector_get(internal_blocks, t) == b) { is_internal = 1; break; }
    }
    if (!is_internal) continue;

    for (int j = 0; j < b->instructions->size; ++j) {
      Quad *q = (Quad *)vector_get(b->instructions, j);
      if (str_equal(string_cstr(q->op), "goto") || str_equal(string_cstr(q->op), "if")) {
        const char *tgt = string_cstr(q->result);
        if (!str_equal(tgt, "null") && strcmp(tgt, pre_label) == 0) {
          string_clear(q->result);
          string_append(q->result, hdr_label2);
        }
      }
    }
  }

  vector_free(internal_blocks, NULL);
  return pre;
}

// ==========================================
// 5. 数据流分析
// ==========================================

// 表达式全集与唯一定义映射（用于 AE 初始化与跨块 CSE）
typedef struct { char *var; int blockId; int quadId; } ExprDef;
static Set *g_expr_universe = NULL;     // 所有出现过的表达式
static Map *g_expr_unique_def = NULL;   // 表达式 -> 唯一定义(若不唯一则标记为 (void*)-1)

static Set* set_clone(const Set* src) {
  Set* dst = set_create();
  set_inorder_traverse_insert(src->root, dst);
  return dst;
}

static void free_expr_maps() {
  if (g_expr_universe) { set_free(g_expr_universe); g_expr_universe = NULL; }
  if (g_expr_unique_def) { map_free(g_expr_unique_def); g_expr_unique_def = NULL; }
}

static void build_expression_universe_and_defs() {
  free_expr_maps();
  g_expr_universe = set_create();
  g_expr_unique_def = map_create();

  for (int i = 0; i < blocks->size; ++i) {
    BasicBlock *b = (BasicBlock *)vector_get(blocks, i);
    for (int j = 0; j < b->instructions->size; ++j) {
      Quad *q = (Quad *)vector_get(b->instructions, j);
      if (q->removed) continue;
      const char *op = string_cstr(q->op);
      if (quad_is_calculation(q) || str_equal(op, "[]=")) {
        MyString *expr = get_expression(q);
        const char *e = string_cstr(expr);
        if (e && e[0] != '\0') set_insert(g_expr_universe, e);

        const char *res = string_cstr(q->result);
        if (is_variable(res)) {
          // 记录唯一定义（若多个定义同一表达式，则记为歧义）
          BSTNode *node = map_find(g_expr_unique_def->root, e);
          if (!node) {
            ExprDef *d = (ExprDef*)malloc(sizeof(ExprDef));
            d->var = str_dup(res);
            d->blockId = b->id;
            d->quadId = q->id;
            map_insert(g_expr_unique_def, e, d);
          } else {
            if (node->value != (void*)(intptr_t)(-1)) {
              ExprDef *old = (ExprDef*)node->value;
              if (old->quadId != q->id || strcmp(old->var, res) != 0 || old->blockId != b->id) {
                node->value = (void*)(intptr_t)(-1); // 标记为不唯一
              }
            }
          }
        }
        string_free(expr);
      }
    }
  }
}

// 初始化全局变量
void init_global_vars() {
  blocks = vector_create();
  labelToBlockId = map_create();
}

// 释放全局变量
void free_global_vars() {
  // 释放基本块
  for (int i = 0; i < blocks->size; i++) {
    BasicBlock *b = (BasicBlock *)vector_get(blocks, i);
    basic_block_free(b);
  }
  vector_free(blocks, NULL);

  // 释放标签映射
  map_free(labelToBlockId);
}

// 检查是否为常量
int is_constant(const char *str) {
  return isdigit(str[0]) || (str[0] == '-' && isdigit(str[1]));
}

// 检查是否为变量
int is_variable(const char *str) { return isalpha(str[0]) || str[0] == 't'; }

// 获取表达式字符串
MyString *get_expression(const Quad *q) {
  MyString *expr = string_create();
  const char *op = string_cstr(q->op);

  if (str_equal(op, "[]=")) { // Array read expression: a[i] or a[i][j]
    char buffer[128];
    if (str_equal(string_cstr(q->arg3), "null")) {
      sprintf(buffer, "%s[%s]", string_cstr(q->arg1), string_cstr(q->arg2));
    } else {
      sprintf(buffer, "%s[%s][%s]", string_cstr(q->arg1), string_cstr(q->arg2), string_cstr(q->arg3));
    }
    string_append(expr, buffer);
  } else if (quad_is_calculation(q)) { // Standard calculation
    char buffer[128];
    sprintf(buffer, "%s %s %s", string_cstr(q->arg1), op, string_cstr(q->arg2));
    string_append(expr, buffer);
  }
  return expr;
}

// 打印Set
// Helper to print set with limit without using nested functions
static void bst_inorder_print_limit(BSTNode *node, FILE *file, int *count, int max_items) {
  if (!node || *count >= max_items) return;
  bst_inorder_print_limit(node->left, file, count, max_items);
  if (*count < max_items) {
    fprintf(file, "%s ", node->key);
    (*count)++;
  }
  bst_inorder_print_limit(node->right, file, count, max_items);
}

void print_set(FILE *file, const Set *s, int max_items) {
  int count = 0;
  bst_inorder_print_limit(s->root, file, &count, max_items);
  if (s->size > max_items) {
    fprintf(file, "... ");
  }
}

// 计算基本块的gen和kill集合
void compute_gen_kill() {
  for (int i = 0; i < blocks->size; ++i) {
    BasicBlock *b = (BasicBlock *)vector_get(blocks, i);

    // 初始化gen和kill集合
    b->gen = set_create();
    b->kill = set_create();

    // 遍历基本块中的每条指令
    for (int j = 0; j < b->instructions->size; ++j) {
      Quad *q = (Quad *)vector_get(b->instructions, j);
      if (q->removed) {
        continue;
      }

      // 获取当前指令的结果变量
      const char *result = string_cstr(q->result);
      if (!str_equal(result, "null") && !is_constant(result)) {
        // 生成定值字符串：id:var
        char def_str[64];
        sprintf(def_str, "%d:%s", q->id, result);

        // 添加到gen集合
        set_insert(b->gen, def_str);

        // 遍历所有指令，查找是否有其他指令定值了相同的变量
        for (int k = 0; k < blocks->size; ++k) {
          BasicBlock *other_b = (BasicBlock *)vector_get(blocks, k);
          for (int l = 0; l < other_b->instructions->size; ++l) {
            Quad *other_q = (Quad *)vector_get(other_b->instructions, l);
            if (other_q->removed) {
              continue;
            }

            const char *other_result = string_cstr(other_q->result);
            if (!str_equal(other_result, "null") &&
                !is_constant(other_result) && str_equal(other_result, result) &&
                (other_b->id != b->id || other_q->id != q->id)) {
              // 生成其他定值字符串
              char other_def_str[64];
              sprintf(other_def_str, "%d:%s", other_q->id, other_result);

              // 添加到kill集合
              set_insert(b->kill, other_def_str);
            }
          }
        }
      }
    }
  }
}

// 合并多个Set的并集
Set *set_union(const Set *s1, const Set *s2) {
  Set *result = set_create();
  set_inorder_traverse_insert(s1->root, result);
  set_inorder_traverse_insert(s2->root, result);
  return result;
}


// Set的差集：s1 - s2
// Helpers for set algebra without nested functions
static void bst_inorder_diff(BSTNode *node, const Set *s2, Set *result) {
  if (!node) return;
  bst_inorder_diff(node->left, s2, result);
  if (!set_contains(s2, node->key)) set_insert(result, node->key);
  bst_inorder_diff(node->right, s2, result);
}

Set *set_difference(const Set *s1, const Set *s2) {
  Set *result = set_create();
  bst_inorder_diff(s1->root, s2, result);
  return result;
}

// 检查两个Set是否相等
static void bst_inorder_check_equal(BSTNode *node, const Set *s2, int *equal) {
  if (!node || !*equal) return;
  bst_inorder_check_equal(node->left, s2, equal);
  if (!set_contains(s2, node->key)) {
    *equal = 0;
    return;
  }
  bst_inorder_check_equal(node->right, s2, equal);
}

int set_equal(const Set *s1, const Set *s2) {
  if (s1->size != s2->size) return 0;
  int equal = 1;
  bst_inorder_check_equal(s1->root, s2, &equal);
  return equal;
}

// ==========================================
// 通用数据流分析求解器
// ==========================================

/* 集合汇合操作类型 */
typedef Set *(*ConfluenceOp)(const Set *, const Set *);

/* 通用前向数据流求解器
 *   汇合: in[B] = 前驱out的并集/交集
 *   传递: out[B] = gen[B] ∪ (in[B] - kill[B])
 *   遍历: 正向（0→N-1）
 */
static void dataflow_solve_forward(int use_intersection) {
  int change = 1;
  while (change) {
    change = 0;
    for (int i = 0; i < blocks->size; ++i) {
      BasicBlock *b = (BasicBlock *)vector_get(blocks, i);

      /* 计算 in[B] = 汇合所有前驱的 out */
      Set *in = set_create();
      int first = 1;
      for (int j = 0; j < b->predecessors->size; ++j) {
        int pred_id = *(int *)vector_get(b->predecessors, j);
        BasicBlock *pred_b = find_block_by_id(pred_id);
        if (!pred_b) continue;
        if (first) {
          set_inorder_traverse_insert(pred_b->out->root, in);
          first = 0;
        } else {
          Set *temp;
          if (use_intersection) {
            temp = set_intersection(in, pred_b->out);
          } else {
            temp = set_union(in, pred_b->out);
          }
          set_free(in);
          in = temp;
        }
      }

      set_free(b->in);
      b->in = in;

      /* 计算 out[B] = gen[B] ∪ (in[B] - kill[B]) */
      Set *in_minus_kill = set_difference(b->in, b->kill);
      Set *new_out = set_union(b->gen, in_minus_kill);
      set_free(in_minus_kill);

      if (!set_equal(new_out, b->out)) {
        change = 1;
        set_free(b->out);
        b->out = new_out;
      } else {
        set_free(new_out);
      }
    }
  }
}

/* 通用后向数据流求解器
 *   汇合: out[B] = 后继in的并集/交集
 *   传递: in[B] = gen[B] ∪ (out[B] - kill[B])
 *   遍历: 逆序（N-1→0），简单但保证收敛
 */
static void dataflow_solve_backward(int use_intersection) {
  int n = blocks->size;
  int change = 1;
  while (change) {
    change = 0;
    /* 逆序遍历基本块 */
    for (int i = n - 1; i >= 0; --i) {
      BasicBlock *b = (BasicBlock *)vector_get(blocks, i);

      /* out[B] = 汇合所有后继的 in */
      Set *out = set_create();
      int first = 1;
      for (int j = 0; j < b->successors->size; ++j) {
        int succ_id = *(int *)vector_get(b->successors, j);
        BasicBlock *succ_b = find_block_by_id(succ_id);
        if (!succ_b) continue;
        if (first) {
          set_inorder_traverse_insert(succ_b->in->root, out);
          first = 0;
        } else {
          Set *temp;
          if (use_intersection) {
            temp = set_intersection(out, succ_b->in);
          } else {
            temp = set_union(out, succ_b->in);
          }
          set_free(out);
          out = temp;
        }
      }

      set_free(b->out);
      b->out = out;

      /* in[B] = gen[B] ∪ (out[B] - kill[B]) */
      Set *out_minus_kill = set_difference(b->out, b->kill);
      Set *new_in = set_union(b->gen, out_minus_kill);
      set_free(out_minus_kill);

      if (!set_equal(new_in, b->in)) {
        change = 1;
        set_free(b->in);
        b->in = new_in;
      } else {
        set_free(new_in);
      }
    }
  }
}

// 到达-定值分析（前向 ∪ 汇合）
void reaching_definitions() {
  fprintf(outFile, "\n[分析] 1.1 到达-定值分析...\n");

  compute_gen_kill();

  /* 初始化: out[Entry]=空 */
  for (int i = 0; i < blocks->size; ++i) {
    BasicBlock *b = (BasicBlock *)vector_get(blocks, i);
    b->out = set_create();
    b->in = set_create();
  }

  dataflow_solve_forward(0); /* 0 = union confluence */

  /* 打印结果 */
  for (int i = 0; i < blocks->size; ++i) {
    const BasicBlock *b = (BasicBlock *)vector_get(blocks, i);
    fprintf(outFile, "Block %d IN: { ", b->id);
    print_set(outFile, b->in, 10);
    fprintf(outFile, "}\n");
    fprintf(outFile, "Block %d OUT: { ", b->id);
    print_set(outFile, b->out, 10);
    fprintf(outFile, "}\n");
  }
}

// 计算基本块的use和def集合（用于活跃变量分析）
void compute_use_def() {
  for (int i = 0; i < blocks->size; ++i) {
    BasicBlock *b = (BasicBlock *)vector_get(blocks, i);

    // 为每个基本块创建use和def集合（暂时使用gen和kill集合的空间）
    // 注意：这里复用了gen和kill集合，因为它们在到达-定值分析后不再使用
    b->gen = set_create();  // 这里gen实际是use集合
    b->kill = set_create(); // 这里kill实际是def集合

    // 遍历基本块中的每条指令，从后向前遍历
    for (int j = b->instructions->size - 1; j >= 0; --j) {
      Quad *q = (Quad *)vector_get(b->instructions, j);
      if (q->removed) {
        continue;
      }

      const char *op = string_cstr(q->op);
      const char *arg1 = string_cstr(q->arg1);
      const char *arg2 = string_cstr(q->arg2);
      const char *arg3 = string_cstr(q->arg3);
      const char *result = string_cstr(q->result);

      if (str_equal(op, "[]=")) { // Array read: result = arg1[arg2] or result = arg1[arg2][arg3]
        // def: result
        if (is_variable(result) && !set_contains(b->gen, result)) set_insert(b->kill, result);
        // use: arg1, arg2, arg3
        if (is_variable(arg1) && !set_contains(b->kill, arg1)) set_insert(b->gen, arg1);
        if (is_variable(arg2) && !set_contains(b->kill, arg2)) set_insert(b->gen, arg2);
        if (is_variable(arg3) && !set_contains(b->kill, arg3)) set_insert(b->gen, arg3);
      } else if (str_equal(op, "=[]")) { // Array write: result[arg2] = arg1 or result[arg2][arg3] = arg1
        // def: result (the array)
        if (is_variable(result) && !set_contains(b->gen, result)) set_insert(b->kill, result);
        // use: arg1, arg2, arg3
        if (is_variable(arg1) && !set_contains(b->kill, arg1)) set_insert(b->gen, arg1);
        if (is_variable(arg2) && !set_contains(b->kill, arg2)) set_insert(b->gen, arg2);
        if (is_variable(arg3) && !set_contains(b->kill, arg3)) set_insert(b->gen, arg3);
      } else { // Default case for other instructions
        // def: result
        if (is_variable(result) && !set_contains(b->gen, result)) set_insert(b->kill, result);
        // use: arg1, arg2
        if (is_variable(arg1) && !set_contains(b->kill, arg1)) set_insert(b->gen, arg1);
        if (is_variable(arg2) && !set_contains(b->kill, arg2)) set_insert(b->gen, arg2);
      }
    }
  }
}

// 活跃变量分析（后向 ∪ 汇合）
void live_variables() {
  fprintf(outFile, "\n[分析] 1.2 活跃变量分析...\n");

  compute_use_def();

  /* 初始化: in[Exit]=空 */
  for (int i = 0; i < blocks->size; ++i) {
    BasicBlock *b = (BasicBlock *)vector_get(blocks, i);
    b->in = set_create();
    b->out = set_create();
  }

  dataflow_solve_backward(0); /* 0 = union confluence */

  /* 打印结果 */
  for (int i = 0; i < blocks->size; ++i) {
    const BasicBlock *b = (BasicBlock *)vector_get(blocks, i);
    fprintf(outFile, "Block %d IN: { ", b->id);
    print_set(outFile, b->in, 10);
    fprintf(outFile, "}\n");
    fprintf(outFile, "Block %d OUT: { ", b->id);
    print_set(outFile, b->out, 10);
    fprintf(outFile, "}\n");
  }
}

// 计算基本块的eval和kill集合（用于可用表达式分析）
void compute_eval_kill_available() {
  for (int i = 0; i < blocks->size; ++i) {
    BasicBlock *b = (BasicBlock *)vector_get(blocks, i);

    // 为每个基本块创建eval和kill集合
    b->gen = set_create();  // 这里gen实际是eval集合
    b->kill = set_create(); // 这里kill实际是kill集合

    // 遍历基本块中的每条指令
    for (int j = 0; j < b->instructions->size; ++j) {
      Quad *q = (Quad *)vector_get(b->instructions, j);
      if (q->removed) {
        continue;
      }

      // 检查是否是计算指令
      if (quad_is_calculation(q)) {
        // 获取表达式字符串
        MyString *expr = get_expression(q);
        const char *expr_str = string_cstr(expr);

        // 添加到eval集合
        set_insert(b->gen, expr_str);
        string_free(expr);
      }

      // 处理变量赋值，kill相关表达式
      const char *result = string_cstr(q->result);
      const char *op = string_cstr(q->op);

      // For array write `a[i] = x`, the result is `a`
      if ((!str_equal(result, "null") && is_variable(result)) || str_equal(op, "=[]")) {
        // 遍历所有基本块，查找包含该变量的表达式
        for (int k = 0; k < blocks->size; ++k) {
          BasicBlock *other_b = (BasicBlock *)vector_get(blocks, k);
          for (int l = 0; l < other_b->instructions->size; ++l) {
            Quad *other_q = (Quad *)vector_get(other_b->instructions, l);
            if (other_q->removed) {
              continue;
            }

            if (quad_is_calculation(other_q)) {
              MyString *other_expr = get_expression(other_q);
              const char *other_expr_str = string_cstr(other_expr);

              // 检查表达式中是否包含result变量
              if (strstr(other_expr_str, result) != NULL) {
                // 添加到kill集合
                set_insert(b->kill, other_expr_str);
              }
              string_free(other_expr);
            }
          }
        }
      }
    }
  }
}

// 合并多个Set的交集
static void bst_inorder_intersection(BSTNode *node, const Set *s2, Set *result) {
  if (!node) return;
  bst_inorder_intersection(node->left, s2, result);
  if (set_contains(s2, node->key)) set_insert(result, node->key);
  bst_inorder_intersection(node->right, s2, result);
}

Set *set_intersection(const Set *s1, const Set *s2) {
  Set *result = set_create();
  bst_inorder_intersection(s1->root, s2, result);
  return result;
}

// 可用表达式分析（前向 ∩ 汇合）
void available_expressions() {
  fprintf(outFile, "\n[分析] 1.3 可用表达式分析...\n");

  build_expression_universe_and_defs();
  compute_eval_kill_available();

  /* 初始化: out[Entry]=空, 其余out=全集 */
  for (int i = 0; i < blocks->size; ++i) {
    BasicBlock *b = (BasicBlock *)vector_get(blocks, i);
    b->in = set_create();
    if (i == 0) {
      b->out = set_create();
    } else {
      b->out = set_clone(g_expr_universe);
    }
  }

  dataflow_solve_forward(1); /* 1 = intersection confluence */

  /* 打印结果 */
  for (int i = 0; i < blocks->size; ++i) {
    const BasicBlock *b = (BasicBlock *)vector_get(blocks, i);
    fprintf(outFile, "Block %d IN: { ", b->id);
    print_set(outFile, b->in, 10);
    fprintf(outFile, "}\n");
    fprintf(outFile, "Block %d OUT: { ", b->id);
    print_set(outFile, b->out, 10);
    fprintf(outFile, "}\n");
  }
}

// 计算基本块的eval和kill集合（用于非常忙表达式分析）
void compute_eval_kill_very_busy() {
  for (int i = 0; i < blocks->size; ++i) {
    BasicBlock *b = (BasicBlock *)vector_get(blocks, i);

    // 为每个基本块创建eval和kill集合
    b->gen = set_create();  // 这里gen实际是eval集合
    b->kill = set_create(); // 这里kill实际是kill集合

    // 遍历基本块中的每条指令，从后向前遍历
    for (int j = b->instructions->size - 1; j >= 0; --j) {
      Quad *q = (Quad *)vector_get(b->instructions, j);
      if (q->removed) {
        continue;
      }

      // 检查是否是计算指令
      if (quad_is_calculation(q)) {
        // 获取表达式字符串
        MyString *expr = get_expression(q);
        const char *expr_str = string_cstr(expr);

        // 添加到eval集合
        set_insert(b->gen, expr_str);
        string_free(expr);
      }

      // 处理变量赋值，kill相关表达式
      const char *result = string_cstr(q->result);
      const char *op = string_cstr(q->op);

      // For array write `a[i] = x`, the result is `a`
      if ((!str_equal(result, "null") && is_variable(result)) || str_equal(op, "=[]")) {
        // 遍历所有基本块，查找包含该变量的表达式
        for (int k = 0; k < blocks->size; ++k) {
          BasicBlock *other_b = (BasicBlock *)vector_get(blocks, k);
          for (int l = 0; l < other_b->instructions->size; ++l) {
            Quad *other_q = (Quad *)vector_get(other_b->instructions, l);
            if (other_q->removed) {
              continue;
            }

            if (quad_is_calculation(other_q)) {
              MyString *other_expr = get_expression(other_q);
              const char *other_expr_str = string_cstr(other_expr);

              // 检查表达式中是否包含result变量
              if (strstr(other_expr_str, result) != NULL) {
                // 添加到kill集合
                set_insert(b->kill, other_expr_str);
              }
              string_free(other_expr);
            }
          }
        }
      }
    }
  }
}

// 非常忙表达式分析（后向 ∩ 汇合）
void very_busy_expressions() {
  fprintf(outFile, "\n[分析] 1.4 预期执行的表达式分析...\n");

  compute_eval_kill_very_busy();

  /* 初始化: in[Exit]=空, 其余in=全集 */
  for (int i = 0; i < blocks->size; ++i) {
    BasicBlock *b = (BasicBlock *)vector_get(blocks, i);
    b->out = set_create();
    if (b->successors->size == 0) {
      b->in = set_create();
    } else {
      b->in = set_clone(g_expr_universe ? g_expr_universe : set_create());
    }
  }

  dataflow_solve_backward(1); /* 1 = intersection confluence */

  /* 打印结果 */
  for (int i = 0; i < blocks->size; ++i) {
    const BasicBlock *b = (BasicBlock *)vector_get(blocks, i);
    fprintf(outFile, "Block %d IN: { ", b->id);
    print_set(outFile, b->in, 10);
    fprintf(outFile, "}\n");
    fprintf(outFile, "Block %d OUT: { ", b->id);
    print_set(outFile, b->out, 10);
    fprintf(outFile, "}\n");
  }
}

// 执行所有数据流分析
void perform_all_analyses() {
  reaching_definitions();
  live_variables();
  available_expressions();
  very_busy_expressions();
}

// 基于 AE 的跨基本块 CSE（保守版：仅处理单前驱块，且表达式在 in[B] 可用且存在唯一定义时）
void cross_block_cse() {
  if (!g_expr_universe) return; // 需先跑 AE 构建全集
  int replaced = 0, removed = 0;
  for (int i = 0; i < blocks->size; ++i) {
    BasicBlock *b = (BasicBlock *)vector_get(blocks, i);
    if (b->predecessors->size != 1) continue; // 保守：只对单前驱

    int pred_id = *(int *)vector_get(b->predecessors, 0);
    BasicBlock *pred_b = NULL;
    for (int t = 0; t < blocks->size; ++t) {
      BasicBlock *cand = (BasicBlock *)vector_get(blocks, t);
      if (cand->id == pred_id) { pred_b = cand; break; }
    }
    if (!pred_b) continue;

    for (int j = 0; j < b->instructions->size; ++j) {
      Quad *q = (Quad *)vector_get(b->instructions, j);
      if (q->removed) continue;
      const char *op = string_cstr(q->op);
      if (!(quad_is_calculation(q) || str_equal(op, "[]="))) continue;

      MyString *expr = get_expression(q);
      const char *e = string_cstr(expr);
      if (e && e[0] != '\0' && set_contains(b->in, e)) {
        // 在前驱块 pred_b 中，从末尾往前找最后一次计算相同表达式的指令
        const char *srcVar = NULL;
        for (int k = pred_b->instructions->size - 1; k >= 0; --k) {
          Quad *pq = (Quad *)vector_get(pred_b->instructions, k);
          if (pq->removed) continue;
          const char *pop = string_cstr(pq->op);
          if (quad_is_calculation(pq) || str_equal(pop, "[]=")) {
            MyString *pexpr = get_expression(pq);
            const char *pe = string_cstr(pexpr);
            if (pe && strcmp(pe, e) == 0) {
              srcVar = string_cstr(pq->result);
              string_free(pexpr);
              break;
            }
            string_free(pexpr);
          }
        }
        if (srcVar && is_variable(srcVar)) {
          const char *dstVar = string_cstr(q->result);
          if (dstVar && strcmp(dstVar, srcVar) == 0) {
            // 相同变量，直接删除该条冗余计算
            q->removed = 1;
            removed++;
          } else {
            // 转换为拷贝 (=, srcVar, _, dstVar)
            string_clear(q->op); string_append(q->op, "=");
            string_clear(q->arg1); string_append(q->arg1, srcVar);
            string_clear(q->arg2); string_append(q->arg2, "null");
            string_clear(q->arg3); string_append(q->arg3, "null");
            replaced++;
          }
        }
      }
      string_free(expr);
    }
  }
  fprintf(outFile, "  [XBlockCSE] 替换了 %d 处跨块冗余计算为拷贝，删除 %d 条\n", replaced, removed);
}

// ==========================================
// 5.5 常量传播与折叠 + 无用分支删除
// ==========================================

/* 尝试计算常量表达式的结果，成功返回1 */
static int try_const_fold(const char *op, const char *a1, const char *a2, int *result) {
  if (!is_constant(a1)) return 0;
  int v1 = atoi(a1);

  if (str_equal(op, "=")) {
    *result = v1;
    return 1;
  }

  if (!is_constant(a2)) return 0;
  int v2 = atoi(a2);

  if (str_equal(op, "+"))  { *result = v1 + v2; return 1; }
  if (str_equal(op, "-"))  { *result = v1 - v2; return 1; }
  if (str_equal(op, "*"))  { *result = v1 * v2; return 1; }
  if (str_equal(op, "/"))  { if (v2 == 0) return 0; *result = v1 / v2; return 1; }
  if (str_equal(op, "<"))  { *result = (v1 < v2);  return 1; }
  if (str_equal(op, ">"))  { *result = (v1 > v2);  return 1; }
  if (str_equal(op, "<=")) { *result = (v1 <= v2); return 1; }
  if (str_equal(op, ">=")) { *result = (v1 >= v2); return 1; }
  if (str_equal(op, "==")) { *result = (v1 == v2); return 1; }
  if (str_equal(op, "!=")) { *result = (v1 != v2); return 1; }
  return 0;
}

/* 常量传播 + 折叠 + 无用分支删除 */
/* 获取复制链的最终源头（处理 a→b→c 链） */
static const char *resolve_copy_chain(const char *var, Map *copy_map, int depth) {
  if (depth > 10) return var; /* 防止循环引用 */
  if (!map_contains(copy_map, var)) return var;
  const char *src = (const char *)map_get(copy_map, var, (void*)0);
  if (!src || str_equal(src, var)) return var;
  return resolve_copy_chain(src, copy_map, depth + 1);
}

void constant_propagation() {
  fprintf(outFile, "\n>>>>>>>> 常量传播与折叠 <<<<<<<<\n");
  Map *const_map = map_create(); /* var_name -> (void*)(intptr_t)value */
  Map *copy_map = map_create();  /* var_name -> source_var_name */
  int changed = 1;
  int propagated = 0, folded = 0, branches_removed = 0, copied = 0;

  while (changed) {
    changed = 0;

    for (int i = 0; i < blocks->size; ++i) {
      BasicBlock *b = (BasicBlock *)vector_get(blocks, i);
      for (int j = 0; j < b->instructions->size; ++j) {
        Quad *q = (Quad *)vector_get(b->instructions, j);
        if (q->removed) continue;

        const char *op = string_cstr(q->op);
        const char *a1 = string_cstr(q->arg1);
        const char *a2 = string_cstr(q->arg2);
        const char *result = string_cstr(q->result);

        /* 替换操作数：先查常量表，再查复制表 */
        if (is_variable(a1) && !is_constant(a1)) {
          if (map_contains(const_map, a1)) {
            int val = (int)(intptr_t)map_get(const_map, a1, (void*)0);
            char buf[16]; sprintf(buf, "%d", val);
            string_clear(q->arg1); string_append(q->arg1, buf);
            propagated++; changed = 1;
            a1 = string_cstr(q->arg1);
          } else if (map_contains(copy_map, a1)) {
            const char *src = resolve_copy_chain(a1, copy_map, 0);
            if (!str_equal(src, a1)) {
              string_clear(q->arg1); string_append(q->arg1, src);
              copied++; changed = 1;
              a1 = string_cstr(q->arg1);
            }
          }
        }
        if (is_variable(a2) && !is_constant(a2)) {
          if (map_contains(const_map, a2)) {
            int val = (int)(intptr_t)map_get(const_map, a2, (void*)0);
            char buf[16]; sprintf(buf, "%d", val);
            string_clear(q->arg2); string_append(q->arg2, buf);
            propagated++; changed = 1;
            a2 = string_cstr(q->arg2);
          } else if (map_contains(copy_map, a2)) {
            const char *src = resolve_copy_chain(a2, copy_map, 0);
            if (!str_equal(src, a2)) {
              string_clear(q->arg2); string_append(q->arg2, src);
              copied++; changed = 1;
              a2 = string_cstr(q->arg2);
            }
          }
        }

        /* 尝试常量折叠（仅在非循环块中进行，避免循环内错误折叠） */
        int fold_result;
        if (b->id <= 2 && /* 仅入口块和初始块 */
            !str_equal(op, "=") && !quad_is_branch(q) &&
            !str_equal(op, "label") && !str_equal(op, "func") &&
            !str_equal(op, "[]=") && !str_equal(op, "=[]") &&
            try_const_fold(op, a1, a2, &fold_result)) {
          /* 将指令替换为 (=, const, _, result) */
          if (is_variable(result)) {
            string_clear(q->op); string_append(q->op, "=");
            char buf[16]; sprintf(buf, "%d", fold_result);
            string_clear(q->arg1); string_append(q->arg1, buf);
            string_clear(q->arg2); string_append(q->arg2, "null");
            map_insert(const_map, result, (void*)(intptr_t)fold_result);
            folded++; changed = 1;
            fprintf(outFile, "  [ConstFold] %s = %d\n", result, fold_result);
          }
        }

        /* 记录常量 */
        if (str_equal(op, "=") && is_constant(a1) && is_variable(result)) {
          int val = atoi(a1);
          if (!map_contains(const_map, result) && b->id <= 2) {
            map_insert(const_map, result, (void*)(intptr_t)val);
            /* 如果此变量之前在copy_map中，移除（现在有更精确的常量值） */
            changed = 1;
            fprintf(outFile, "  [ConstProp] %s = %d (常量)\n", result, val);
          }
        }

        /* 记录复制关系: (=, var, _, result) 且 var 是变量 */
        if (str_equal(op, "=") && is_variable(a1) && !is_constant(a1) &&
            is_variable(result) && b->id <= 2) {
          if (!map_contains(copy_map, result) && !map_contains(const_map, result)) {
            const char *src = resolve_copy_chain(a1, copy_map, 0);
            map_insert(copy_map, result, (void*)src);
            changed = 1;
            fprintf(outFile, "  [CopyProp] %s = %s (复制)\n", result, src);
          }
        }
      }
    }
  }

  /* 无用分支删除 */
  for (int i = 0; i < blocks->size; ++i) {
    BasicBlock *b = (BasicBlock *)vector_get(blocks, i);
    for (int j = 0; j < b->instructions->size; ++j) {
      Quad *q = (Quad *)vector_get(b->instructions, j);
      if (q->removed) continue;
      if (!str_equal(string_cstr(q->op), "if")) continue;

      const char *cond = string_cstr(q->arg1);
      if (!is_constant(cond)) continue;

      int cond_val = atoi(cond);
      const char *target = string_cstr(q->result);

      if (cond_val == 0) {
        /* if(0) → 永不跳转，删除 if */
        q->removed = 1;
        branches_removed++;
        fprintf(outFile, "  [BranchDel] 删除恒假分支 if(%d) → %s\n", cond_val, target);
      } else {
        /* if(非0) → 必定跳转，改为 goto */
        string_clear(q->op); string_append(q->op, "goto");
        q->removed = 0;
        branches_removed++;
        fprintf(outFile, "  [BranchFold] if(%d)→goto %s\n", cond_val, target);
      }
    }
  }

  fprintf(outFile, "  [ConstProp] 常量传播 %d 次, 折叠 %d 次, 复制传播 %d 次, 分支删除 %d 个\n",
          propagated, folded, copied, branches_removed);
  map_free(const_map);
  map_free(copy_map);
}

// ==========================================
// 6. 循环优化
// ==========================================

// --- 支配树（Cooper-Harvey-Kennedy 算法）---

#define UNDEFINED_IDOM -1

/* DFS 后序遍历 CFG，结果存入 postorder 数组 */
static void postorder_dfs(int bb_id, int *visited, int *postorder, int *po_idx) {
  visited[bb_id] = 1;
  BasicBlock *bb = find_block_by_id(bb_id + 1); /* bb_id 是 0-based */
  if (bb) {
    for (int i = 0; i < bb->successors->size; i++) {
      int succ = *(int *)vector_get(bb->successors, i);
      int succ_idx = succ - 1; /* 转换为 0-based */
      if (!visited[succ_idx]) {
        postorder_dfs(succ_idx, visited, postorder, po_idx);
      }
    }
  }
  postorder[(*po_idx)++] = bb_id;
}

/* 计算后序编号映射：postorder_map[bb] = 该结点在后序中的位置 */
static void compute_postorder_map(const int *postorder, int n, int *po_map) {
  for (int i = 0; i < n; i++) {
    po_map[postorder[i]] = i;
  }
}

/* intersect: 沿 idom 链向上找到 b1 和 b2 的最近公共支配者 */
static int intersect(int b1, int b2, const int *idoms, const int *po_map) {
  while (b1 != b2) {
    if (po_map[b1] < po_map[b2]) {
      b1 = idoms[b1];
    } else {
      b2 = idoms[b2];
    }
  }
  return b1;
}

/* 构建支配树：计算每个基本块的立即支配者（idom） */
static int *compute_dominators(int n_blocks) {
  /* 分配并初始化 */
  int *visited = (int *)calloc(n_blocks, sizeof(int));
  int *postorder = (int *)calloc(n_blocks, sizeof(int));
  int *po_map = (int *)calloc(n_blocks, sizeof(int));
  int *idoms = (int *)malloc(n_blocks * sizeof(int));
  int po_idx = 0;

  /* 从入口块（block 1, 0-based index 0）开始 DFS */
  postorder_dfs(0, visited, postorder, &po_idx);

  /* 验证所有块可达 */
  assert(po_idx == n_blocks && "CFG 中存在不可达的基本块");

  compute_postorder_map(postorder, n_blocks, po_map);

  /* 初始化：入口块的 idom 是自身，其余为 UNDEFINED */
  for (int i = 0; i < n_blocks; i++) {
    idoms[i] = UNDEFINED_IDOM;
  }
  idoms[0] = 0; /* 入口块支配自身 */

  /* 不动点迭代（仅处理可达块） */
  int change;
  do {
    change = 0;
    /* 逆后序遍历，排除入口块（最后一个）和不可达块 */
    for (int i = po_idx - 2; i >= 0; i--) {
      int bb = postorder[i];
      BasicBlock *b = find_block_by_id(bb + 1);
      if (!b || b->predecessors->size == 0) continue;

      /* new_idom = 第一个有已计算 idom 的前驱 */
      int new_idom = -1;
      for (int j = 0; j < b->predecessors->size; j++) {
        int pred = *(int *)vector_get(b->predecessors, j);
        int pred_idx = pred - 1;
        if (pred_idx >= 0 && pred_idx < n_blocks && idoms[pred_idx] != UNDEFINED_IDOM) {
          new_idom = pred_idx;
          break;
        }
      }

      /* 与其他前驱交运算 */
      if (new_idom >= 0) {
        for (int j = 0; j < b->predecessors->size; j++) {
          int pred = *(int *)vector_get(b->predecessors, j);
          int pred_idx = pred - 1;
          if (pred_idx >= 0 && pred_idx < n_blocks &&
              pred_idx != new_idom && idoms[pred_idx] != UNDEFINED_IDOM) {
            new_idom = intersect(new_idom, pred_idx, idoms, po_map);
          }
        }
      }

      if (new_idom >= 0 && idoms[bb] != new_idom) {
        idoms[bb] = new_idom;
        change = 1;
      }
    }
  } while (change);

  free(visited);
  free(postorder);
  free(po_map);
  return idoms;
}

/* 查询 a 是否支配 b（a 和 b 均为 0-based 索引） */
static int dominates(int a, int b, const int *idoms, int n_blocks) {
  if (a == 0) return 1; /* 入口块支配所有 */
  if (idoms[b] == UNDEFINED_IDOM) return 0; /* 不可达块 */
  int runner = idoms[b];
  while (runner != 0) {
    if (runner == a) return 1;
    if (runner == UNDEFINED_IDOM || runner >= n_blocks) return 0;
    runner = idoms[runner];
  }
  return 0;
}

/* 自然循环结构 */
typedef struct {
  int header;       /* 循环头块 ID */
  int latch;        /* 回边源块 ID */
  int *bbs;         /* 循环体内的块 ID 数组（0-based） */
  int bbs_count;    /* 循环体内块数 */
} NaturalLoop;

/* 基于支配树检测自然循环：回边 n→h 满足 h dominates n */
/* 循环体 = {h} ∪ {能到达 n 但不经过 h 的所有结点} */
static NaturalLoop *detect_natural_loops(int *idoms, int n_blocks, int *loop_count) {
  NaturalLoop *loops = NULL;
  int capacity = 0;
  *loop_count = 0;

  for (int n = 0; n < n_blocks; n++) {
    BasicBlock *b = find_block_by_id(n + 1);
    if (!b) continue;
    /* 跳过不可达块（idom 未定义） */
    if (idoms[n] == UNDEFINED_IDOM) continue;
    for (int s = 0; s < b->successors->size; s++) {
      int h = *(int *)vector_get(b->successors, s);
      int h_idx = h - 1;
      if (h_idx < 0 || h_idx >= n_blocks) continue;

      /* 回边：h dominates n */
      if (dominates(h_idx, n, idoms, n_blocks)) {
        /* 收集循环体：从 n 反向 BFS，不经过 h */
        int *in_loop = (int *)calloc(n_blocks, sizeof(int));
        int *queue = (int *)malloc(n_blocks * sizeof(int));
        int qh = 0, qt = 0;

        in_loop[h_idx] = 1; /* header 在循环中 */
        if (!in_loop[n]) {
          in_loop[n] = 1;
          queue[qt++] = n;
        }

        while (qh < qt) {
          int cur = queue[qh++];
          BasicBlock *cur_bb = find_block_by_id(cur + 1);
          if (!cur_bb) continue;
          for (int p = 0; p < cur_bb->predecessors->size; p++) {
            int pred = *(int *)vector_get(cur_bb->predecessors, p);
            int pred_idx = pred - 1;
            if (!in_loop[pred_idx]) {
              in_loop[pred_idx] = 1;
              queue[qt++] = pred_idx;
            }
          }
        }

        /* 压缩到数组 */
        int count = 0;
        for (int i = 0; i < n_blocks; i++) {
          if (in_loop[i]) count++;
        }

        /* 扩容并添加 */
        if (*loop_count >= capacity) {
          capacity = capacity == 0 ? 4 : capacity * 2;
          loops = (NaturalLoop *)realloc(loops, capacity * sizeof(NaturalLoop));
        }

        loops[*loop_count].header = h;
        loops[*loop_count].latch = n + 1;
        loops[*loop_count].bbs = (int *)malloc(count * sizeof(int));
        loops[*loop_count].bbs_count = count;
        int idx = 0;
        for (int i = 0; i < n_blocks; i++) {
          if (in_loop[i]) loops[*loop_count].bbs[idx++] = i + 1; /* 转回 1-based */
        }
        (*loop_count)++;

        free(in_loop);
        free(queue);
      }
    }
  }
  return loops;
}

static void free_natural_loops(NaturalLoop *loops, int count) {
  for (int i = 0; i < count; i++) free(loops[i].bbs);
  free(loops);
}

/* 基于支配树查找循环，返回兼容旧接口的 Map */
static Map *find_loops_with_dominators(int n_blocks) {
  Map *loops = map_create();
  int loop_count = 0;

  int *idoms = compute_dominators(n_blocks);
  int nat_count = 0;
  NaturalLoop *nat_loops = detect_natural_loops(idoms, n_blocks, &nat_count);

  for (int li = 0; li < nat_count; li++) {
    char loop_header_key[32], loop_back_key[32], loop_exists_key[32];
    sprintf(loop_exists_key, "loop_%d_exists", loop_count);
    sprintf(loop_header_key, "loop_%d_header", loop_count);
    sprintf(loop_back_key, "loop_%d_back", loop_count);

    map_insert(loops, loop_header_key, (void *)(intptr_t)nat_loops[li].header);
    map_insert(loops, loop_back_key, (void *)(intptr_t)nat_loops[li].latch);
    map_insert(loops, loop_exists_key, (void *)(intptr_t)1);

    fprintf(outFile, "检测到循环 %d: Header=%d Back=%d (支配树检测, 体=%d块)\n",
            loop_count, nat_loops[li].header, nat_loops[li].latch, nat_loops[li].bbs_count);
    loop_count++;
  }

  map_insert(loops, "loop_count", (void *)(intptr_t)loop_count);
  free_natural_loops(nat_loops, nat_count);
  free(idoms);
  return loops;
}

// 查找循环（新接口：使用支配树）
Map *find_loops() {
  return find_loops_with_dominators(blocks->size);
}

// 辅助函数：检查变量在循环内是否为不变量
// 核心规则：如果变量在循环范围内有任何定值，就不是不变量
// （因为回边会将该定值传播到下一次迭代，导致变量在不同迭代间可能变化）
static int is_var_invariant(const char *var, int loop_header, int loop_back, Map *invariant_map) {
    if (is_constant(var)) return 1;
    if (!is_variable(var)) return 1;

    char key[64];
    sprintf(key, "%s_invariant", var);
    if (map_contains(invariant_map, key)) {
      return (int)(intptr_t)map_get(invariant_map, key, (void*)0);
    }

    int invariant = 1;
    for (int k = 0; k < blocks->size; ++k) {
      BasicBlock *b = (BasicBlock *)vector_get(blocks, k);
      if (b->id < loop_header || b->id > loop_back) continue;
      for (int j = 0; j < b->instructions->size; ++j) {
        Quad *other_q = (Quad *)vector_get(b->instructions, j);
        if (other_q->removed) continue;
        if (str_equal(string_cstr(other_q->result), var)) {
          invariant = 0;
          break;
        }
      }
      if (!invariant) break;
    }

    map_insert(invariant_map, key, (void*)(intptr_t)invariant);
    return invariant;
}

// 检查指令是否为循环不变量
int is_loop_invariant(Quad *q, int loop_header, int loop_back,
                      Map *invariant_map) {
  const char *op = string_cstr(q->op);
  const char *result = string_cstr(q->result);

  if (str_equal(result, "null")) {
    return 0;
  }

  if (str_equal(op, "=[]")) { // Array write
    return 0;
  }
  // 比较运算不标记为不变量，避免外提循环条件
  if (str_equal(op, "<") || str_equal(op, ">") || str_equal(op, "<=") ||
      str_equal(op, ">=") || str_equal(op, "==") || str_equal(op, "!=")) {
    return 0;
  }

  const char *arg1 = string_cstr(q->arg1);
  const char *arg2 = string_cstr(q->arg2);
  const char *arg3 = string_cstr(q->arg3);

  if (str_equal(op, "[]=")) { // Array read
    return is_var_invariant(arg1, loop_header, loop_back, invariant_map) &&
           is_var_invariant(arg2, loop_header, loop_back, invariant_map) &&
           is_var_invariant(arg3, loop_header, loop_back, invariant_map);
        }

  if (str_equal(op, "=") || quad_is_calculation(q)) {
    return is_var_invariant(arg1, loop_header, loop_back, invariant_map) &&
           is_var_invariant(arg2, loop_header, loop_back, invariant_map);
  }

  return 0;
}

// 检查变量在循环内是否有多个定值
int has_multiple_defs_in_loop(const char *var, int loop_header, int loop_back) {
  int def_count = 0;

  for (int k = 0; k < blocks->size; k++) {
    BasicBlock *b = (BasicBlock *)vector_get(blocks, k);
    if (b->id >= loop_header && b->id <= loop_back) {
      for (int j = 0; j < b->instructions->size; j++) {
        Quad *q = (Quad *)vector_get(b->instructions, j);
        if (q->removed) {
          continue;
        }
        if (str_equal(string_cstr(q->result), var)) {
          def_count++;
          if (def_count > 1) {
            return 1;
          }
        }
      }
    }
  }

  return 0;
}

// 4.1 循环不变量外提
void optimize_invariant_code_motion() {
  fprintf(outFile, "\n>>>>>>>> 1.7.1 执行循环不变量外提 <<<<<<<<\n");
  Map *loops = find_loops();
  int loop_count = (int)(intptr_t)map_get(loops, "loop_count", (void *)0);

  // 遍历所有检测到的循环
  for (int loop_idx = 0; loop_idx < loop_count; loop_idx++) {
    char loop_header_key[32];
    char loop_back_key[32];
    sprintf(loop_header_key, "loop_%d_header", loop_idx);
    sprintf(loop_back_key, "loop_%d_back", loop_idx);

    int header = (int)(intptr_t)map_get(loops, loop_header_key, (void *)-1);
    int back = (int)(intptr_t)map_get(loops, loop_back_key, (void *)-1);

    if (header != -1 && back != -1) {
      fprintf(outFile, "  [LICM] 处理循环 %d: Header=%d Back=%d\n", loop_idx,
              header, back);

      // 1. 识别循环中的基本块（必须在创建preheader之前，否则ID会变）
      Vector *loop_blocks = vector_create();
      for (int k = 0; k < blocks->size; k++) {
        BasicBlock *b = (BasicBlock *)vector_get(blocks, k);
        if (b->id >= header && b->id <= back) {
          vector_push_back(loop_blocks, b);
        }
      }

      // 2. 标记循环不变量（单次扫描，不迭代，避免缓存污染）
      Map *invariant_map = map_create();

      for (int i = 0; i < loop_blocks->size; i++) {
        BasicBlock *b = (BasicBlock *)vector_get(loop_blocks, i);
        for (int j = 0; j < b->instructions->size; j++) {
          Quad *q = (Quad *)vector_get(b->instructions, j);
          if (q->removed) continue;

          const char *result = string_cstr(q->result);
          if (str_equal(result, "null")) continue;

          int invariant = is_loop_invariant(q, header, back, invariant_map);
          char key[64];
          sprintf(key, "%s_invariant", result);

          if (invariant) {
            // 只有操作数都是常量或循环外定义的变量时，才标记为不变量
            map_insert(invariant_map, key, (void *)(intptr_t)invariant);
          }
        }
      }

      // 3. 外提循环不变量（使用 preHeaderInstructions，避免 CFG 重建）
      Set *lifted_ids = set_create();

      for (int i = 0; i < loop_blocks->size; i++) {
        BasicBlock *b = (BasicBlock *)vector_get(loop_blocks, i);
        for (int j = 0; j < b->instructions->size; j++) {
          Quad *q = (Quad *)vector_get(b->instructions, j);
          if (q->removed) continue;

          const char *result = string_cstr(q->result);
          if (str_equal(result, "null")) continue;

          char idkey[32]; sprintf(idkey, "id:%d", q->id);
          if (set_contains(lifted_ids, idkey)) continue;

          char key[64];
          sprintf(key, "%s_invariant", result);
          if (map_contains(invariant_map, key)) {
            int invariant = (int)(intptr_t)map_get(invariant_map, key, (void *)0);
            if (invariant) {
              int can_lift = 1;

              // 不外提比较运算和分支指令
              const char *hoist_op = string_cstr(q->op);
              if (str_equal(hoist_op, "<") || str_equal(hoist_op, ">") ||
                  str_equal(hoist_op, "<=") || str_equal(hoist_op, ">=") ||
                  str_equal(hoist_op, "==") || str_equal(hoist_op, "!=") ||
                  str_equal(hoist_op, "if") || str_equal(hoist_op, "goto") ||
                  str_equal(hoist_op, "label") || str_equal(hoist_op, "=[]")) {
                can_lift = 0;
              }

              // 条件3: 循环内不能有其他定值
              if (can_lift && has_multiple_defs_in_loop(result, header, back)) {
                can_lift = 0;
              }

              // 条件2: 同块内在定值之前不能有对v的使用
              if (can_lift) {
                for (int p = 0; p < j; ++p) {
                  Quad *prev_q = (Quad *)vector_get(b->instructions, p);
                  if (prev_q->removed) continue;
                  const char *a1 = string_cstr(prev_q->arg1);
                  const char *a2 = string_cstr(prev_q->arg2);
                  const char *a3 = string_cstr(prev_q->arg3);
                  if (str_equal(a1, result) || str_equal(a2, result) || str_equal(a3, result)) {
                    can_lift = 0;
                    break;
                  }
                }
              }

              if (can_lift) {
                // 使用 preHeaderInstructions 避免 CFG 重建
                BasicBlock *header_block = find_block_by_id(header);
                if (header_block) {
                  q->removed = 1;
                  Quad *movedQ = quad_copy(q);
                  movedQ->removed = 0;
                  vector_push_back(header_block->preHeaderInstructions, movedQ);
                  set_insert(lifted_ids, idkey);
                  fprintf(outFile, "  [LICM] 外提指令 %s 到循环头前置\n",
                          string_cstr(quad_to_string(q)));
                }
              }
            }
          }
        }
      }
      set_free(lifted_ids);

      // 释放资源
      vector_free(loop_blocks, NULL);
      map_free(invariant_map);
    }
  }

  map_free(loops);
}

// 归纳变量三元组结构
typedef struct {
  const char *var;      // 归纳变量名
  const char *base_var; // 基本归纳变量名
  int coefficient;      // 系数C1
  int constant;         // 常数项C2
} InductionVar;

// 4.2 强度削弱：将 iv * const 替换为累加变量
// 算法：对于循环中的 (+, iv, stride, iv) 和 (*, iv, factor, result)，
// 在 preheader 中初始化 result，每次 iv 更新后累加 result
void optimize_strength_reduction() {
  fprintf(outFile, "\n>>>>>>>> 1.7.2 执行强度削弱 <<<<<<<<\n");
  Map *loops = find_loops();
  int loop_count = (int)(intptr_t)map_get(loops, "loop_count", (void *)0);

  for (int loop_idx = 0; loop_idx < loop_count; loop_idx++) {
    char loop_header_key[32], loop_back_key[32];
    sprintf(loop_header_key, "loop_%d_header", loop_idx);
    sprintf(loop_back_key, "loop_%d_back", loop_idx);

    int header = (int)(intptr_t)map_get(loops, loop_header_key, (void *)-1);
    int back = (int)(intptr_t)map_get(loops, loop_back_key, (void *)-1);

    if (header == -1 || back == -1) continue;

    fprintf(outFile, "  [SR] 处理循环 %d: Header=%d Back=%d\n", loop_idx, header, back);

    // 1. 找到基本归纳变量及其步长: iv = iv + stride 或 iv = iv - stride
    //    iv_stride: var_name -> stride_value (int)
    Map *iv_stride = map_create();

    for (int k = 0; k < blocks->size; ++k) {
      BasicBlock *b = (BasicBlock *)vector_get(blocks, k);
      if (b->id < header || b->id > back) continue;
      for (int i = 0; i < b->instructions->size; ++i) {
        Quad *q = (Quad *)vector_get(b->instructions, i);
        if (q->removed) continue;

        const char *op = string_cstr(q->op);
        const char *result = string_cstr(q->result);

        // 模式 A：简化后的形式 (+, iv, const, iv)
        if (str_equal(op, "+") && is_constant(string_cstr(q->arg2)) &&
            str_equal(string_cstr(q->arg1), result) && is_variable(result)) {
          int stride = atoi(string_cstr(q->arg2));
          if (!map_contains(iv_stride, result)) {
            map_insert(iv_stride, result, (void*)(intptr_t)stride);
            fprintf(outFile, "  [SR] 基本归纳变量: %s (步长 +%d)\n", result, stride);
          }
        }

        // 模式 B: (-, iv, const, iv)
        if (str_equal(op, "-") && is_constant(string_cstr(q->arg2)) &&
            str_equal(string_cstr(q->arg1), result) && is_variable(result)) {
          int stride = -atoi(string_cstr(q->arg2));
          if (!map_contains(iv_stride, result)) {
            map_insert(iv_stride, result, (void*)(intptr_t)stride);
            fprintf(outFile, "  [SR] 基本归纳变量: %s (步长 %d)\n", result, stride);
          }
        }

        // 模式 C: 未简化的形式 (+, iv, const, temp); (=, temp, _, iv)
        if ((str_equal(op, "+") || str_equal(op, "-")) &&
            is_constant(string_cstr(q->arg2)) &&
            is_variable(string_cstr(q->arg1)) &&
            string_cstr(q->result)[0] == 't') {  // temp variable
          // 看下一条指令
          if (i + 1 < b->instructions->size) {
            Quad *next_q = (Quad *)vector_get(b->instructions, i + 1);
            if (!next_q->removed && str_equal(string_cstr(next_q->op), "=") &&
                str_equal(string_cstr(next_q->arg1), string_cstr(q->result)) &&
                str_equal(string_cstr(next_q->result), string_cstr(q->arg1))) {
              const char *iv_var = string_cstr(q->arg1);
              int stride = atoi(string_cstr(q->arg2));
              if (str_equal(op, "-")) stride = -stride;
              if (!map_contains(iv_stride, iv_var)) {
                map_insert(iv_stride, iv_var, (void*)(intptr_t)stride);
                fprintf(outFile, "  [SR] 基本归纳变量: %s (步长 %d, 未简化)\n", iv_var, stride);
              }
            }
          }
        }
      }
    }

    if (iv_stride->size == 0) { map_free(iv_stride); continue; }

    // 2. 寻找 iv * factor，替换为累加变量
    for (int k = 0; k < blocks->size; ++k) {
      BasicBlock *b = (BasicBlock *)vector_get(blocks, k);
      if (b->id < header || b->id > back) continue;
      for (int i = 0; i < b->instructions->size; ++i) {
        Quad *q = (Quad *)vector_get(b->instructions, i);
        if (q->removed) continue;
        if (!str_equal(string_cstr(q->op), "*")) continue;

        const char *a1 = string_cstr(q->arg1);
        const char *a2 = string_cstr(q->arg2);
        const char *result = string_cstr(q->result);

        // 确定哪个操作数是归纳变量，哪个是常量因子
        const char *iv_var = NULL;
        int factor = 0;
        if (map_contains(iv_stride, a1) && is_constant(a2)) {
          iv_var = a1; factor = atoi(a2);
        } else if (map_contains(iv_stride, a2) && is_constant(a1)) {
          iv_var = a2; factor = atoi(a1);
        }
        if (!iv_var) continue;

        int stride = (int)(intptr_t)map_get(iv_stride, iv_var, (void*)0);
        int increment = factor * stride;  // 每次迭代 result 应该增加的量

        fprintf(outFile, "  [SR] 强度削弱: %s = %s * %d → 累加变量 (增量 %d)\n",
                result, iv_var, factor, increment);

        // 标记原乘法为删除
        q->removed = 1;

        // 在循环头的 preHeaderInstructions 中添加初始化
        // （后续由 move_preheader_to_start 统一移到程序开始处）
        BasicBlock *header_block = find_block_by_id(header);
        if (header_block) {
          Quad *init_q = quad_create();
          string_append(init_q->op, "=");
          string_append(init_q->arg1, "0");
          string_append(init_q->arg2, "null");
          string_append(init_q->result, result);
          vector_push_back(header_block->preHeaderInstructions, init_q);
          fprintf(outFile, "  [SR] preheader初始化: %s = 0\n", result);
        }

        // 在 IV 更新指令之后插入累加: (+, result, increment, result)
        // 只在循环范围内查找，且只插入一次
        int increment_inserted = 0;
        for (int h = 0; h < blocks->size && !increment_inserted; ++h) {
          BasicBlock *ib = (BasicBlock *)vector_get(blocks, h);
          if (ib->id < header || ib->id > back) continue;  // 只搜索循环内的块
          for (int j = 0; j < ib->instructions->size && !increment_inserted; ++j) {
            Quad *iq = (Quad *)vector_get(ib->instructions, j);
            if (iq->removed) continue;

            int found = 0;
            // 简化形式: (+, iv, C, iv) 或 (-, iv, C, iv)
            if ((str_equal(string_cstr(iq->op), "+") || str_equal(string_cstr(iq->op), "-")) &&
                str_equal(string_cstr(iq->arg1), iv_var) &&
                str_equal(string_cstr(iq->result), iv_var) &&
                is_constant(string_cstr(iq->arg2))) {
              found = 1;
            }
            // 未简化形式: (op, iv, C, temp); (=, temp, _, iv)
            if (!found && str_equal(string_cstr(iq->op), "=") &&
                str_equal(string_cstr(iq->result), iv_var) && j > 0) {
              Quad *prev = (Quad *)vector_get(ib->instructions, j - 1);
              if (!prev->removed && quad_is_calculation(prev) &&
                  str_equal(string_cstr(prev->arg1), iv_var)) {
                found = 1;
              }
            }

            if (found) {
              Quad *inc_q = quad_create();
              string_append(inc_q->op, "+");
              string_append(inc_q->arg1, result);
              char inc_str[16];
              sprintf(inc_str, "%d", increment);
              string_append(inc_q->arg2, inc_str);
              string_append(inc_q->result, result);
              vector_insert(ib->instructions, j + 1, inc_q);
              fprintf(outFile, "  [SR] 插入累加: (+, %s, %d, %s) 在块%d之后\n",
                      result, increment, result, ib->id);
              increment_inserted = 1;
            }
          }
        }
        if (!increment_inserted) {
          fprintf(outFile, "  [SR] 警告: 未找到 %s 在循环内的更新点\n", iv_var);
        }
      }
    }

    map_free(iv_stride);
  }

  map_free(loops);
}

// 4.3 循环展开
// 当前实现：检测循环并输出信息，但不执行展开
// 完整的循环展开需要同时复制循环体和调整IV，实现较复杂
// 强度削弱和归纳变量消除已提供主要的循环优化效果
void optimize_loop_unrolling() {
  fprintf(outFile, "\n>>>>>>>> 1.7.3 执行循环展开 <<<<<<<<\n");
  Map *loops = find_loops();
  int loop_count = (int)(intptr_t)map_get(loops, "loop_count", (void *)0);

  for (int loop_idx = 0; loop_idx < loop_count; loop_idx++) {
    char loop_header_key[32], loop_back_key[32];
    sprintf(loop_header_key, "loop_%d_header", loop_idx);
    sprintf(loop_back_key, "loop_%d_back", loop_idx);
    int header = (int)(intptr_t)map_get(loops, loop_header_key, (void *)-1);
    int back = (int)(intptr_t)map_get(loops, loop_back_key, (void *)-1);
    if (header != -1 && back != -1) {
      fprintf(outFile, "  [Unroll] 检测到循环 %d: Header=%d Back=%d (暂不展开)\n",
              loop_idx, header, back);
    }
  }

  map_free(loops);
}

// 指令简化：将 (op, a, b, t); (=, t, _, c) 简化为 (op, a, b, c)
void simplify_instructions() {
  fprintf(outFile, "\n>>>>>>>> 简化指令序列 <<<<<<<<\n");

  int simplified_count = 0;
    int changed = 1;

    // Keep iterating until no more simplifications can be made
    while (changed) {
        changed = 0;
  for (int i = 0; i < blocks->size; ++i) {
    BasicBlock *b = (BasicBlock *)vector_get(blocks, i);
            Vector *new_instructions = vector_create();

            for (int j = 0; j < b->instructions->size; ++j) {
      Quad *q1 = (Quad *)vector_get(b->instructions, j);

                if (q1->removed) {
        continue;
      }

                // Look ahead to the next instruction
                if (j + 1 < b->instructions->size) {
                    Quad *q2 = (Quad *)vector_get(b->instructions, j + 1);

                    // Pattern: (op, a, b, t) followed by (=, t, _, c)
                    if (!q2->removed &&
                        (quad_is_calculation(q1) || str_equal(string_cstr(q1->op), "[]=")) &&
                        str_equal(string_cstr(q2->op), "=") &&
                        str_equal(string_cstr(q1->result), string_cstr(q2->arg1)) &&
                        is_variable(string_cstr(q1->result))) {

                        // 安全检查：中间变量 t 不能在其他地方被使用
                        // （除了 q2 这个赋值指令本身）
                        const char *temp_var = string_cstr(q1->result);
                        int temp_used_elsewhere = 0;
                        for (int bi = 0; bi < blocks->size && !temp_used_elsewhere; ++bi) {
                          BasicBlock *bb = (BasicBlock *)vector_get(blocks, bi);
                          for (int bj = 0; bj < bb->instructions->size; ++bj) {
                            Quad *oq = (Quad *)vector_get(bb->instructions, bj);
                            if (oq->removed || oq == q1 || oq == q2) continue;
                            if (str_equal(string_cstr(oq->arg1), temp_var) ||
                                str_equal(string_cstr(oq->arg2), temp_var) ||
                                str_equal(string_cstr(oq->arg3), temp_var)) {
                              temp_used_elsewhere = 1;
                              break;
                            }
                          }
                        }
                        if (temp_used_elsewhere) {
                          fprintf(outFile, "  [Simplify] 跳过合并 %d+%d: '%s' 在其他指令中使用\n",
                                  q1->id, q2->id, temp_var);
                          // 不合并，但 q1 仍会通过循环末尾的 push 加入
                        } else {
                          // Perform the simplification
                        fprintf(outFile,
                                "  [Simplify] Merging instructions %d and %d. Replacing result '%s' with '%s'.\n",
                                q1->id, q2->id, temp_var, string_cstr(q2->result));

                        // Update q1's result
                        string_clear(q1->result);
                        string_append(q1->result, string_cstr(q2->result));

                        // Mark q2 for removal
                        q2->removed = 1;
                        simplified_count++;
                        changed = 1;
                        } /* end else (merge allowed) */
                    }
                }
                vector_push_back(new_instructions, q1);
              }

            // Filter out removed instructions from the block
            Vector *final_instructions = vector_create();
            for(int k=0; k < new_instructions->size; ++k) {
                Quad* q = (Quad*)vector_get(new_instructions, k);
                if(!q->removed) {
                    vector_push_back(final_instructions, q);
                } else {
                    quad_free(q); // Free the memory of the removed quad
            }
          }
            vector_free(b->instructions, NULL); // Free the old vector shell
            b->instructions = final_instructions;
            vector_free(new_instructions, NULL);
    }
  }

  fprintf(outFile, "  [Simplify] 共简化 %d 条指令\n", simplified_count);
}

// 辅助：查找变量在循环外的常量初始值
static int get_initial_value(const char *var, int loop_header) {
  for (int k = 0; k < blocks->size; ++k) {
    BasicBlock *b = (BasicBlock *)vector_get(blocks, k);
    if (b->id >= loop_header) break; // 只查循环之前的块
    for (int i = 0; i < b->instructions->size; ++i) {
      Quad *q = (Quad *)vector_get(b->instructions, i);
      if (q->removed) continue;
      if (str_equal(string_cstr(q->op), "=") &&
          str_equal(string_cstr(q->result), var) &&
          is_constant(string_cstr(q->arg1))) {
        return atoi(string_cstr(q->arg1));
      }
    }
  }
  return 0; // 默认 0
}

// 辅助：获取变量的常量值（直接检查或通过初始值查找）
static int resolve_constant(const char *s, int loop_header) {
  if (is_constant(s)) return atoi(s);
  if (is_variable(s)) return get_initial_value(s, loop_header);
  return 0;
}

// 4.4 归纳变量消除
void optimize_induction_variable_elimination() {
  fprintf(outFile, "\n>>>>>>>> 1.7.4 执行删除归纳变量 <<<<<<<<\n");
  Map *loops = find_loops();
  int loop_count = (int)(intptr_t)map_get(loops, "loop_count", (void *)0);

  for (int loop_idx = 0; loop_idx < loop_count; loop_idx++) {
    char loop_header_key[32], loop_back_key[32];
    sprintf(loop_header_key, "loop_%d_header", loop_idx);
    sprintf(loop_back_key, "loop_%d_back", loop_idx);
    int header = (int)(intptr_t)map_get(loops, loop_header_key, (void *)-1);
    int back = (int)(intptr_t)map_get(loops, loop_back_key, (void *)-1);

    if (header == -1 || back == -1) continue;

    fprintf(outFile, "  [IVE] 处理循环 %d: Header=%d Back=%d\n", loop_idx, header, back);

    // 1. 找到基本归纳变量及其步长
    const char *iv_var = NULL;
    int iv_stride = 0;
    for (int k = 0; k < blocks->size && !iv_var; ++k) {
      BasicBlock *b = (BasicBlock *)vector_get(blocks, k);
      if (b->id < header || b->id > back) continue;
      for (int i = 0; i < b->instructions->size; ++i) {
        Quad *q = (Quad *)vector_get(b->instructions, i);
        if (q->removed) continue;
        if ((str_equal(string_cstr(q->op), "+") || str_equal(string_cstr(q->op), "-")) &&
            is_constant(string_cstr(q->arg2)) &&
            str_equal(string_cstr(q->result), string_cstr(q->arg1)) &&
            is_variable(string_cstr(q->result))) {
          iv_var = string_cstr(q->result);
          iv_stride = atoi(string_cstr(q->arg2));
          if (str_equal(string_cstr(q->op), "-")) iv_stride = -iv_stride;
          fprintf(outFile, "  [IVE] 基本归纳变量: %s (步长 %d)\n", iv_var, iv_stride);
          break;
        }
      }
    }
    if (!iv_var) continue;

    // 2. 查找导出归纳变量
    // 方法A：显式乘法 (*, iv, factor, result)
    // 方法B：SR后的累加变量（初始值相同，步长成比例）
    const char *derived_iv = NULL;
    int factor_val = 0;

    // 方法A：查找显式乘法
    for (int k = 0; k < blocks->size && !derived_iv; ++k) {
      BasicBlock *b = (BasicBlock *)vector_get(blocks, k);
      if (b->id < header || b->id > back) continue;
      for (int i = 0; i < b->instructions->size && !derived_iv; ++i) {
        Quad *q = (Quad *)vector_get(b->instructions, i);
        if (q->removed) continue;
        if (!str_equal(string_cstr(q->op), "*")) continue;
        const char *a1 = string_cstr(q->arg1);
        const char *a2 = string_cstr(q->arg2);
        if (str_equal(a1, iv_var)) {
          int f = resolve_constant(a2, header);
          if (f > 0) { derived_iv = string_cstr(q->result); factor_val = f; }
        } else if (str_equal(a2, iv_var)) {
          int f = resolve_constant(a1, header);
          if (f > 0) { derived_iv = string_cstr(q->result); factor_val = f; }
        }
      }
    }

    // 方法B：SR后通过步长比例识别（变量初始化相同、步长成整数倍）
    if (!derived_iv) {
      int iv_init = get_initial_value(iv_var, header);
      for (int k = 0; k < blocks->size && !derived_iv; ++k) {
        BasicBlock *b = (BasicBlock *)vector_get(blocks, k);
        for (int i = 0; i < b->instructions->size && !derived_iv; ++i) {
          Quad *q = (Quad *)vector_get(b->instructions, i);
          if (q->removed) continue;
          // 查找累加变量: (+, var, const, var) 且 var != iv_var
          if (!str_equal(string_cstr(q->op), "+")) continue;
          if (!is_constant(string_cstr(q->arg2))) continue;
          const char *var = string_cstr(q->result);
          if (!is_variable(var) || str_equal(var, iv_var)) continue;
          if (!str_equal(string_cstr(q->arg1), var)) continue;

          int var_stride = atoi(string_cstr(q->arg2));
          int var_init = get_initial_value(var, header);
          // 检查：相同初始值，步长成整数比例
          if (var_init == iv_init && var_stride % iv_stride == 0 && var_stride / iv_stride > 0) {
            derived_iv = var;
            factor_val = var_stride / iv_stride;
            fprintf(outFile, "  [IVE] 导出归纳变量(步长比): %s (步长%d/%d=%d)\n",
                    derived_iv, var_stride, iv_stride, factor_val);
          }
        }
      }
    }

    if (derived_iv) {
      fprintf(outFile, "  [IVE] 导出归纳变量: %s = %s * %d\n", derived_iv, iv_var, factor_val);
    }
    if (!derived_iv) continue;

    // 3. 查找循环条件
    const char *loop_cond_var = NULL, *loop_cond_limit = NULL;
    const char *cond_op = NULL;
    int loop_cond_limit_val = 0;
    BasicBlock *header_block = find_block_by_id(header);
    if (header_block) {
      for (int i = 0; i < header_block->instructions->size; ++i) {
        Quad *q = (Quad *)vector_get(header_block->instructions, i);
        if (q->removed) continue;
        const char *op = string_cstr(q->op);
        if (str_equal(op, "<") || str_equal(op, ">") ||
            str_equal(op, "<=") || str_equal(op, ">=")) {
          loop_cond_var = string_cstr(q->arg1);
          loop_cond_limit = string_cstr(q->arg2);
          cond_op = op;
          loop_cond_limit_val = resolve_constant(loop_cond_limit, header);
          fprintf(outFile, "  [IVE] 循环条件: %s %s %s (%d)\n",
                  loop_cond_var, cond_op, loop_cond_limit, loop_cond_limit_val);
          break;
        }
      }
    }

    // 4. 替换循环条件中的 IV 为导出 IV，调整边界
    if (str_equal(loop_cond_var, iv_var) && loop_cond_limit_val > 0 && factor_val > 0) {
      int new_limit = loop_cond_limit_val * factor_val;
      char new_limit_str[16];
      sprintf(new_limit_str, "%d", new_limit);

      for (int i = 0; i < header_block->instructions->size; ++i) {
        Quad *q = (Quad *)vector_get(header_block->instructions, i);
        if (q->removed) continue;
        if (str_equal(string_cstr(q->arg1), iv_var) &&
            (str_equal(string_cstr(q->op), "<") || str_equal(string_cstr(q->op), ">") ||
             str_equal(string_cstr(q->op), "<=") || str_equal(string_cstr(q->op), ">="))) {
          string_clear(q->arg1);
          string_append(q->arg1, derived_iv);
          string_clear(q->arg2);
          string_append(q->arg2, new_limit_str);
          fprintf(outFile, "  [IVE] 替换条件: %s %s %s → %s %s %s\n",
                  iv_var, string_cstr(q->op), loop_cond_limit,
                  derived_iv, string_cstr(q->op), new_limit_str);
          break;
        }
      }

      // 5. 安全检查：IV 是否还有其他用途
      int iv_is_used = 0;
      for (int k = 0; k < blocks->size && !iv_is_used; ++k) {
        BasicBlock *b = (BasicBlock *)vector_get(blocks, k);
        for (int i = 0; i < b->instructions->size && !iv_is_used; ++i) {
          Quad *q = (Quad *)vector_get(b->instructions, i);
          if (q->removed) continue;
          // 检查 IV 是否作为操作数（排除 IV 自己的更新指令）
          int uses_iv = (str_equal(string_cstr(q->arg1), iv_var) ||
                         str_equal(string_cstr(q->arg2), iv_var) ||
                         str_equal(string_cstr(q->arg3), iv_var));
          int is_self_update = (str_equal(string_cstr(q->result), iv_var) &&
                                (str_equal(string_cstr(q->op), "+") ||
                                 str_equal(string_cstr(q->op), "-")));
          if (uses_iv && !is_self_update) {
            iv_is_used = 1;
            fprintf(outFile, "  [IVE] IV %s 仍被指令 %d 使用，无法删除\n", iv_var, q->id);
          }
        }
      }

      // 6. 删除 IV 更新指令
      if (!iv_is_used) {
        int deleted = 0;
        for (int k = 0; k < blocks->size; ++k) {
          BasicBlock *b = (BasicBlock *)vector_get(blocks, k);
          Vector *new_instrs = vector_create();
          for (int i = 0; i < b->instructions->size; ++i) {
            Quad *q = (Quad *)vector_get(b->instructions, i);
            if (!q->removed &&
                str_equal(string_cstr(q->result), iv_var) &&
                (str_equal(string_cstr(q->op), "+") ||
                 str_equal(string_cstr(q->op), "-") ||
                 str_equal(string_cstr(q->op), "="))) {
              q->removed = 1;
              deleted++;
            }
            if (!q->removed) vector_push_back(new_instrs, q);
            else quad_free(q);
          }
          vector_free(b->instructions, NULL);
          b->instructions = new_instrs;
        }
        fprintf(outFile, "  [IVE] 删除了 %d 条 IV 更新指令\n", deleted);
      }
    }
  }
  map_free(loops);
}

// ==========================================
// 6.x Preheader finalize (ensure no backedges to PREHDR_)
// ==========================================
void finalize_preheaders() {
  Map *loops = find_loops();
  int loop_count = (int)(intptr_t)map_get(loops, "loop_count", (void *)0);
  for (int loop_idx = 0; loop_idx < loop_count; ++loop_idx) {
    char loop_header_key[32];
    char loop_back_key[32];
    sprintf(loop_header_key, "loop_%d_header", loop_idx);
    sprintf(loop_back_key, "loop_%d_back", loop_idx);
    int header = (int)(intptr_t)map_get(loops, loop_header_key, (void *)-1);
    int back   = (int)(intptr_t)map_get(loops, loop_back_key,   (void *)-1);
    if (header == -1 || back == -1) continue;

    BasicBlock *hb = find_block_by_id(header);
    if (!hb) continue;
    const char *hdr_label = get_block_label(hb);
    if (!hdr_label) continue;

    // 预期 preheader 名称：PREHDR_<hdr_label>
    char pre_label[300];
    snprintf(pre_label, sizeof(pre_label), "PREHDR_%s", hdr_label);

    for (int id = header; id <= back; ++id) {
      BasicBlock *b = find_block_by_id(id);
      if (!b) continue;
      for (int j = 0; j < b->instructions->size; ++j) {
        Quad *q = (Quad *)vector_get(b->instructions, j);
        if (str_equal(string_cstr(q->op), "goto") || str_equal(string_cstr(q->op), "if")) {
          const char *tgt = string_cstr(q->result);
          if (!str_equal(tgt, "null") && strcmp(tgt, pre_label) == 0) {
            string_clear(q->result);
            string_append(q->result, hdr_label);
          }
        }
      }
    }
  }
  map_free(loops);
}

// ==========================================
// 7. 文件 IO
// ==========================================

// 清理行，去除括号和逗号
void clean_line(char *line, char *clean) {
  char *dst = clean;
  while (*line) {
    if (*line != '(' && *line != ')' && *line != ',') {
      *dst++ = *line;
    }
    line++;
  }
  *dst = '\0';
}

// 加载文件
void load_file(const char *path) {
  FILE *file = fopen(path, "r");
  if (!file) {
    fprintf(stderr, "无法读取 %s\n", path);
    exit(1);
  }

  char line[256];
  char clean[256];
  BasicBlock *currentBlock = basic_block_create();
  currentBlock->id = 1;

  while (fgets(line, sizeof(line), file)) {
    // 去除换行符
    line[strcspn(line, "\n")] = '\0';

    if (line[0] == '\0') {
      continue;
    }

    clean_line(line, clean);

    Quad *q = quad_create();

    char *token = strtok(clean, " ");
    if (!token) {
      continue;
    }
    q->id = atoi(token);

    // 使用一个数组来存储解析出的所有标记
    char *tokens[5];
    int token_count = 0;
    while (token_count < 5 && (token = strtok(NULL, " ")) != NULL) {
      tokens[token_count++] = token;
    }

    // 根据标记数量填充四元式字段
    string_append(q->op, (token_count > 0) ? tokens[0] : "null");
    string_append(q->arg1, (token_count > 1) ? tokens[1] : "null");
    string_append(q->arg2, (token_count > 2) ? tokens[2] : "null");

    if (token_count == 5) { // 五元组，用于二维数组
      string_append(q->arg3, tokens[3]);
      string_append(q->result, tokens[4]);
    } else { // 四元组或其他
      string_append(q->result, (token_count > 3) ? tokens[3] : "null");
      string_append(q->arg3, "null");
    }

    // 转换 _ 为 null
    if (str_equal(string_cstr(q->arg1), "_")) {
      string_clear(q->arg1);
      string_append(q->arg1, "null");
    }

    if (str_equal(string_cstr(q->arg2), "_")) {
      string_clear(q->arg2);
      string_append(q->arg2, "null");
    }

    if (str_equal(string_cstr(q->result), "_")) {
      string_clear(q->result);
      string_append(q->result, "null");
    }

    // 兼容非标准数组记号："[]" 读，"[]=" 写（而程序内部采用 []= 读、=[] 写）
    if (str_equal(string_cstr(q->op), "[]")) {
      // 读：([] , A, i, t) 或 ([] , A, i, j, t) -> 规范化为 []=
      string_clear(q->op); string_append(q->op, "[]=");
    } else if (str_equal(string_cstr(q->op), "[]=")) {
      // 尝试将 ([]=, A, i, v) 或 ([]=, A, i, j, v) 规范化为 (=[], v, i, j, A)
      // 仅当这是"写"形式时才转换；这里按 input_array2 的约定进行转换
      // 规则：把当前 result 当作写入值，把 arg1 当作数组名，把 arg2/arg3 当作下标
      MyString *orig_op = string_copy(q->op);
      (void)orig_op; // 无实际用途，仅保留注释语义
      // 构造为写操作
      MyString *old_arg1 = string_copy(q->arg1); // 数组名
      MyString *old_arg2 = string_copy(q->arg2); // i
      MyString *old_arg3 = string_copy(q->arg3); // 可能为第二下标或 null
      MyString *old_result = string_copy(q->result); // 值

      string_clear(q->op);     string_append(q->op, "=[]");
      string_clear(q->arg1);   string_append(q->arg1, string_cstr(old_result)); // 值
      string_clear(q->arg2);   string_append(q->arg2, string_cstr(old_arg2));  // i
      string_clear(q->arg3);   string_append(q->arg3, string_cstr(old_arg3));  // j 或 null
      string_clear(q->result); string_append(q->result, string_cstr(old_arg1)); // 数组名

      string_free(old_arg1); string_free(old_arg2); string_free(old_arg3); string_free(old_result);
    }

    if (str_equal(string_cstr(q->op), "label")) {
      if (currentBlock->instructions->size > 0) {
        vector_push_back(blocks, currentBlock);
        currentBlock = basic_block_create();
        currentBlock->id = blocks->size + 1;
      }
      if (q->result->length > 0 && !str_equal(string_cstr(q->result), "null")) {
        map_insert(labelToBlockId, string_cstr(q->result),
                   (void *)(intptr_t)currentBlock->id);
      }
    }

    vector_push_back(currentBlock->instructions, q);

    if (quad_is_branch(q)) {
      vector_push_back(blocks, currentBlock);
      currentBlock = basic_block_create();
      currentBlock->id = blocks->size + 1;
    }
  }

  if (currentBlock->instructions->size > 0) {
    vector_push_back(blocks, currentBlock);
  } else {
    basic_block_free(currentBlock);
  }

  fclose(file);

  // 构建 CFG
  for (int i = 0; i < blocks->size; i++) {
    BasicBlock *block = (BasicBlock *)vector_get(blocks, i);
    if (block->instructions->size == 0) {
      continue;
    }

    Quad *lastQ =
        (Quad *)vector_get(block->instructions, block->instructions->size - 1);
    if (!str_equal(string_cstr(lastQ->op), "goto") &&
        !str_equal(string_cstr(lastQ->op), "ret") && i < blocks->size - 1) {
      BasicBlock *nextBlock = (BasicBlock *)vector_get(blocks, i + 1);

      // 添加后继
      int *succ = (int *)malloc(sizeof(int));
      *succ = nextBlock->id;
      vector_push_back(block->successors, succ);

      // 添加前驱
      int *pred = (int *)malloc(sizeof(int));
      *pred = block->id;
      vector_push_back(nextBlock->predecessors, pred);
    }

    const char *target = "";
    if (str_equal(string_cstr(lastQ->op), "goto")) {
      target = string_cstr(lastQ->result);
    } else if (str_equal(string_cstr(lastQ->op), "if")) {
      target = string_cstr(lastQ->result);
    }

    if (target[0] != '\0' && !str_equal(target, "null") &&
        map_contains(labelToBlockId, target)) {
      int tid = (int)(intptr_t)map_get(labelToBlockId, target, (void *)-1);

      // 添加后继
      int *succ = (int *)malloc(sizeof(int));
      *succ = tid;
      vector_push_back(block->successors, succ);

      // 添加前驱
      for (int j = 0; j < blocks->size; j++) {
        BasicBlock *b = (BasicBlock *)vector_get(blocks, j);
        if (b->id == tid) {
          int *pred = (int *)malloc(sizeof(int));
          *pred = block->id;
          vector_push_back(b->predecessors, pred);
          break;
        }
      }
    }
  }
}

// 删除死代码（未使用的变量定义）
void eliminate_dead_code() {
  fprintf(outFile, "\n>>>>>>>> 删除死代码 <<<<<<<<\n");

  int total_deleted = 0;
  int change = 1;

  // 迭代删除直到收敛（删除一条死代码可能暴露新的死代码）
  while (change) {
    change = 0;
    int deleted_count = 0;

    for (int i = 0; i < blocks->size; ++i) {
      BasicBlock *b = (BasicBlock *)vector_get(blocks, i);
      for (int j = 0; j < b->instructions->size; ++j) {
        Quad *q = (Quad *)vector_get(b->instructions, j);
        if (q->removed) continue;

      const char *result = string_cstr(q->result);
      const char *op = string_cstr(q->op);

      // 跳过无结果的指令、标签、分支等
      if (str_equal(result, "null") || str_equal(result, "_") ||
          quad_is_branch(q) || str_equal(op, "label") ||
          str_equal(op, "func")) {
        continue;
      }

      // 只处理产生值的指令（赋值、计算、数组读），跳过数组写（有副作用）
      if (str_equal(op, "=[]") || str_equal(op, "label") ||
          str_equal(op, "func") || str_equal(op, "ret") ||
          str_equal(op, "if") || str_equal(op, "goto")) {
        continue;
      }

      // 检查result是否在后续指令中被使用（包括preHeaderInstructions）
      int is_used = 0;
      for (int k = 0; k < blocks->size; ++k) {
        BasicBlock *other_b = (BasicBlock *)vector_get(blocks, k);

        // 也检查preHeaderInstructions
        for (int l = 0; l < other_b->preHeaderInstructions->size; ++l) {
          Quad *other_q = (Quad *)vector_get(other_b->preHeaderInstructions, l);

          if (other_q->removed) {
            continue;
          }

          if (str_equal(string_cstr(other_q->arg1), result) ||
              str_equal(string_cstr(other_q->arg2), result) ||
              str_equal(string_cstr(other_q->arg3), result)) {
            is_used = 1;
            break;
          }
        }

        if (is_used) {
          break;
        }

        for (int l = 0; l < other_b->instructions->size; ++l) {
          Quad *other_q = (Quad *)vector_get(other_b->instructions, l);

          if (other_q->removed || (i == k && l == j)) {
            continue;
          }

          // 检查result是否作为操作数使用
          if (str_equal(string_cstr(other_q->arg1), result) ||
              str_equal(string_cstr(other_q->arg2), result) ||
              str_equal(string_cstr(other_q->arg3), result)) {
            is_used = 1;
            break;
          }
        }
        if (is_used) {
          break;
        }
      }

        // 如果未使用，标记为删除
        if (!is_used) {
          q->removed = 1;
          deleted_count++;
          change = 1;
          fprintf(outFile, "  [DCE] 删除未使用的变量定义：%s = %s\n", result,
                  string_cstr(q->arg1));
        }
      }
    }
    total_deleted += deleted_count;
  }

  fprintf(outFile, "  [DCE] 共删除 %d 条死代码指令\n", total_deleted);
}

// 删除无用基本块（没有前驱的块，不可达代码）
void dead_block_elimination() {
  fprintf(outFile, "\n>>>>>>>> 删除无用基本块 <<<<<<<<\n");
  int removed_blocks = 0, removed_instrs = 0;

  /* 迭代删除，因为删除一个块可能使其他块也失去前驱 */
  int changed = 1;
  while (changed) {
    changed = 0;
    for (int i = 1; i < blocks->size; ++i) { /* 跳过 entry 块(0) */
      BasicBlock *b = (BasicBlock *)vector_get(blocks, i);
      if (!b) continue;

      /* 检查是否有前驱（排除自己） */
      int has_pred = 0;
      for (int p = 0; p < b->predecessors->size; ++p) {
        int pred_id = *(int *)vector_get(b->predecessors, p);
        if (pred_id != b->id) { has_pred = 1; break; }
      }

      if (!has_pred) {
        fprintf(outFile, "  [DeadBlock] 删除块 %d (无前驱)\n", b->id);
        /* 标记所有指令为删除 */
        for (int j = 0; j < b->instructions->size; ++j) {
          Quad *q = (Quad *)vector_get(b->instructions, j);
          if (!q->removed) { q->removed = 1; removed_instrs++; }
        }
        /* 从后继的前驱列表中移除本块 */
        for (int s = 0; s < b->successors->size; ++s) {
          int succ_id = *(int *)vector_get(b->successors, s);
          BasicBlock *succ = find_block_by_id(succ_id);
          if (succ) {
            /* 清理后继的前驱列表 */
            Vector *new_preds = vector_create();
            for (int p = 0; p < succ->predecessors->size; ++p) {
              int pid = *(int *)vector_get(succ->predecessors, p);
              if (pid != b->id) {
                int *np = (int *)malloc(sizeof(int)); *np = pid;
                vector_push_back(new_preds, np);
              }
            }
            vector_free(succ->predecessors, free);
            succ->predecessors = new_preds;
          }
        }
        /* 清空本块的前驱和后继（标记为死块） */
        vector_free(b->predecessors, free);
        b->predecessors = vector_create();
        vector_free(b->successors, free);
        b->successors = vector_create();
        removed_blocks++;
        changed = 1;
      }
    }
  }

  fprintf(outFile, "  [DeadBlock] 删除 %d 个无用块, %d 条指令\n", removed_blocks, removed_instrs);
}

// 将所有preHeader指令移到第一个基本块
void move_preheader_to_start() {
  fprintf(outFile, "\n>>>>>>>> 将循环前置指令移到程序开始 <<<<<<<<\n");

  if (blocks->size == 0) {
    return;
  }

  BasicBlock *first_block = (BasicBlock *)vector_get(blocks, 0);
  int moved_count = 0;

  // 收集所有preHeader指令
  Vector *all_preheader = vector_create();

  for (int i = 0; i < blocks->size; ++i) {
    BasicBlock *b = (BasicBlock *)vector_get(blocks, i);
    for (int j = 0; j < b->preHeaderInstructions->size; ++j) {
      Quad *q = (Quad *)vector_get(b->preHeaderInstructions, j);
      vector_push_back(all_preheader, q);
      moved_count++;
    }
  }

  // 计算插入点：保证 func 在首行，插在 func 之后；
  // 若 func 后存在若干 "= 常量" 初始化，则插在这些初始化之后；若没有常量赋值，则紧随 func 之后；
  // 并且在第一个 label 之前停止。
  int func_index = -1;
  int first_label_index = first_block->instructions->size; // 若无 label 则为末尾
  for (int j = 0; j < first_block->instructions->size; ++j) {
    Quad *q = (Quad *)vector_get(first_block->instructions, j);
    const char *op = string_cstr(q->op);
    if (func_index == -1 && str_equal(op, "func")) {
      func_index = j;
    }
    if (str_equal(op, "label")) {
      first_label_index = j;
      break;
    }
  }

  // 设定基础插入位置为 func 之后（若未找到 func，则从 0 开始）
  int insert_pos = (func_index >= 0) ? func_index + 1 : 0;

  // 在 func 与首 label 之间，尽量放在最后一个 "= 常量" 初始化之后
  for (int j = insert_pos; j < first_label_index; ++j) {
    Quad *q = (Quad *)vector_get(first_block->instructions, j);
    const char *op = string_cstr(q->op);
    if (str_equal(op, "=") && is_constant(string_cstr(q->arg1))) {
      insert_pos = j + 1;
    }
  }

  // 将所有preHeader指令插入到计算好的位置
  for (int j = 0; j < all_preheader->size; ++j) {
    Quad *q = (Quad *)vector_get(all_preheader, j);
    vector_insert(first_block->instructions, insert_pos + j, q);
  }

  // 清空所有块的preHeaderInstructions
  for (int i = 0; i < blocks->size; ++i) {
    BasicBlock *b = (BasicBlock *)vector_get(blocks, i);
    b->preHeaderInstructions->size = 0;
  }

  vector_free(all_preheader, NULL);
  fprintf(outFile, "  [MovePreHeader] 移动了 %d 条前置指令到程序开始\n",
          moved_count);
}

// 重新编号指令
void renumber_instructions() {
  int new_id = 0;

  for (int i = 0; i < blocks->size; ++i) {
    BasicBlock *b = (BasicBlock *)vector_get(blocks, i);

    // 重新编号前置指令
    for (int j = 0; j < b->preHeaderInstructions->size; ++j) {
      Quad *q = (Quad *)vector_get(b->preHeaderInstructions, j);
      q->id = new_id++;
    }

    // 重新编号基本块指令
    for (int j = 0; j < b->instructions->size; ++j) {
      Quad *q = (Quad *)vector_get(b->instructions, j);
      if (!q->removed) {
        q->id = new_id++;
      }
    }
  }
}

// 打印最终结果
void print_final_result() {
  // 仅输出四元式结果，格式与输入一致，不输出额外注释/标题
  for (int i = 0; i < blocks->size; i++) {
    const BasicBlock *b = (BasicBlock *)vector_get(blocks, i);

    // 预置指令已在 move_preheader_to_start() 中移动至第一个基本块，无需单独打印

    for (int j = 0; j < b->instructions->size; j++) {
      const Quad *q = (Quad *)vector_get(b->instructions, j);
      if (!q->removed) {
        MyString *str = quad_to_string(q);
        fprintf(outFile, "%s\n", string_cstr(str));
        string_free(str);
      }
    }
  }
}
