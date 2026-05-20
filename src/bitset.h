/**
 * @file bitset.h
 * @brief 64位字对齐位集，用于数据流分析中的快速集合运算
 *
 * 参照 compiler-ref-optimization-passes/common/bitset.h 实现。
 * 并/交/差运算是逐字的，可以被编译器向量化。
 */

#ifndef BITSET_H
#define BITSET_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define BITSET_WORD_SIZE 64
typedef uint64_t BitSetWord;

/* 计算存储 max_elems 个元素所需的字数 */
static inline uint32_t bitset_num_words(uint32_t max_elems) {
  return (max_elems + BITSET_WORD_SIZE - 1) / BITSET_WORD_SIZE;
}

/* BitSet 结构体 */
typedef struct {
  uint32_t max_elems;      /* 最大元素编号 */
  BitSetWord *data;         /* 位数组 */
} BitSet;

/* 创建位集 */
static BitSet bitset_create(uint32_t max_elems) {
  BitSet bs;
  bs.max_elems = max_elems;
  uint32_t nwords = bitset_num_words(max_elems);
  bs.data = (BitSetWord *)calloc(nwords, sizeof(BitSetWord));
  if (!bs.data) {
    /* 分配失败时返回空数据位集，max_elems 置 0 标记无效 */
    bs.max_elems = 0;
  }
  return bs;
}

/* 在已分配的内存上创建位集（批量分配用） */
static BitSet bitset_from_mem(uint32_t max_elems, void *mem) {
  BitSet bs;
  bs.max_elems = max_elems;
  bs.data = (BitSetWord *)mem;
  return bs;
}

/* 释放位集 */
static void bitset_free(BitSet *bs) {
  free(bs->data);
  bs->data = NULL;
  bs->max_elems = 0;
}

/* 添加元素 */
static void bitset_add(BitSet *bs, uint32_t elem) {
  assert(elem < bs->max_elems);
  uint32_t word_idx = elem / BITSET_WORD_SIZE;
  uint32_t bit_idx  = elem % BITSET_WORD_SIZE;
  bs->data[word_idx] |= (1ULL << bit_idx);
}

/* 检查元素是否在集合中 */
static int bitset_contains(const BitSet *bs, uint32_t elem) {
  if (elem >= bs->max_elems) return 0;
  uint32_t word_idx = elem / BITSET_WORD_SIZE;
  uint32_t bit_idx  = elem % BITSET_WORD_SIZE;
  return (bs->data[word_idx] >> bit_idx) & 1ULL;
}

/* 复制位集（大小必须相同） */
static void bitset_copy(BitSet *dst, const BitSet *src) {
  assert(dst->max_elems == src->max_elems);
  uint32_t nwords = bitset_num_words(dst->max_elems);
  memcpy(dst->data, src->data, nwords * sizeof(BitSetWord));
}

/* 清空位集 */
static void bitset_clear(BitSet *bs) {
  uint32_t nwords = bitset_num_words(bs->max_elems);
  memset(bs->data, 0, nwords * sizeof(BitSetWord));
}

/* 全置1 */
static void bitset_fill(BitSet *bs) {
  uint32_t nwords = bitset_num_words(bs->max_elems);
  memset(bs->data, 0xFF, nwords * sizeof(BitSetWord));
}

/* 判断相等 */
static int bitset_equal(const BitSet *a, const BitSet *b) {
  if (a->max_elems != b->max_elems) return 0;
  uint32_t nwords = bitset_num_words(a->max_elems);
  for (uint32_t i = 0; i < nwords; i++) {
    if (a->data[i] != b->data[i]) return 0;
  }
  return 1;
}

/* a = a ∪ b  (并集，原地) */
static void bitset_union_into(BitSet *a, const BitSet *b) {
  assert(a->max_elems == b->max_elems);
  uint32_t nwords = bitset_num_words(a->max_elems);
  for (uint32_t i = 0; i < nwords; i++) {
    a->data[i] |= b->data[i];
  }
}

/* a = a ∩ b  (交集，原地) */
static void bitset_intersect_into(BitSet *a, const BitSet *b) {
  assert(a->max_elems == b->max_elems);
  uint32_t nwords = bitset_num_words(a->max_elems);
  for (uint32_t i = 0; i < nwords; i++) {
    a->data[i] &= b->data[i];
  }
}

/* a = a - b  (差集，原地) */
static void bitset_diff_into(BitSet *a, const BitSet *b) {
  assert(a->max_elems == b->max_elems);
  uint32_t nwords = bitset_num_words(a->max_elems);
  for (uint32_t i = 0; i < nwords; i++) {
    a->data[i] &= ~(b->data[i]);
  }
}

/* dest = a ∪ b */
static void bitset_union(BitSet *dest, const BitSet *a, const BitSet *b) {
  assert(a->max_elems == b->max_elems && dest->max_elems == a->max_elems);
  uint32_t nwords = bitset_num_words(a->max_elems);
  for (uint32_t i = 0; i < nwords; i++) {
    dest->data[i] = a->data[i] | b->data[i];
  }
}

/* dest = a ∩ b */
static void bitset_intersection(BitSet *dest, const BitSet *a, const BitSet *b) {
  assert(a->max_elems == b->max_elems && dest->max_elems == a->max_elems);
  uint32_t nwords = bitset_num_words(a->max_elems);
  for (uint32_t i = 0; i < nwords; i++) {
    dest->data[i] = a->data[i] & b->data[i];
  }
}

/* 获取大小（用于调试） */
static uint32_t bitset_count(const BitSet *bs) {
  uint32_t count = 0;
  uint32_t nwords = bitset_num_words(bs->max_elems);
  for (uint32_t i = 0; i < nwords; i++) {
    BitSetWord w = bs->data[i];
    while (w) { count += (uint32_t)(w & 1); w >>= 1; }
  }
  return count;
}

#endif /* BITSET_H */
