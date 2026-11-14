#ifndef GF_LEAF_NATIVE_H
#define GF_LEAF_NATIVE_H
#include <stdint.h>

// Host equivalents of the integer bit operations used by native leaf bodies.
static inline uint32_t gf_leaf_rotl32(uint32_t value, unsigned shift) {
  shift &= 31u;
  return shift ? (value << shift) | (value >> (32u - shift)) : value;
}

// Inclusive PowerPC bit range, numbered from the most-significant bit. A range
// whose end precedes its start wraps around the word.
static inline uint32_t gf_leaf_mask32(unsigned mb, unsigned me) {
  const uint32_t start = 0xffffffffu >> mb;
  const uint32_t end = (me == 31u) ? 0u : (0xffffffffu >> (me + 1u));
  return (mb <= me) ? (start & ~end) : (start | ~end);
}

static inline uint32_t gf_leaf_cntlzw(uint32_t value) {
  unsigned n = 0;
  while (n < 32u && !(value & (0x80000000u >> n))) ++n;
  return n;
}
#endif
