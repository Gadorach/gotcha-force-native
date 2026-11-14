#include "engine/core/leaf_native.h"
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <initializer_list>

static void check(bool ok) { if (!ok) std::abort(); }

static uint32_t reference_rotl(uint32_t v, unsigned s) {
  s &= 31u;
  if (!s) return v;
  return (v << s) | (v >> (32u - s));
}

int main() {
  unsigned cases = 0;
  for (unsigned shift = 0; shift < 32; ++shift) {
    for (uint32_t value : {0u, 1u, 0x80000000u, 0xffffffffu, 0x12345678u, 0xdeadbeefu}) {
      check(gf_leaf_rotl32(value, shift) == reference_rotl(value, shift));
      ++cases;
    }
  }
  // Bit-field masks: contiguous MSB-numbered ranges, including the wrap case.
  for (unsigned mb = 0; mb < 32; ++mb) {
    for (unsigned me = 0; me < 32; ++me) {
      uint32_t expected = 0;
      if (mb <= me) {
        for (unsigned b = mb; b <= me; ++b) expected |= 0x80000000u >> b;
      } else {
        for (unsigned b = mb; b < 32; ++b) expected |= 0x80000000u >> b;
        for (unsigned b = 0; b <= me; ++b) expected |= 0x80000000u >> b;
      }
      check(gf_leaf_mask32(mb, me) == expected);
      ++cases;
    }
  }
  check(gf_leaf_mask32(0, 26) == 0xFFFFFFE0u);
  check(gf_leaf_mask32(24, 31) == 0x000000FFu);
  check(gf_leaf_mask32(31, 31) == 0x00000001u);
  for (uint32_t value : {0u, 1u, 2u, 0x80000000u, 0xffffffffu, 0x0000ffffu, 0x00ff0000u}) {
    unsigned expected = 0;
    while (expected < 32 && !(value & (0x80000000u >> expected))) ++expected;
    check(gf_leaf_cntlzw(value) == expected);
    ++cases;
  }
  check(gf_leaf_cntlzw(0) == 32);
  std::printf("{\"leaf_kernel_property_cases\":%u,\"shift_mask_clz_checks\":true,\"all_passed\":true}\n", cases);
}
