#ifndef GF_COMMAND_PACKETS_H
#define GF_COMMAND_PACKETS_H

// Independently implemented GX command construction. This is a portable CPU
// submission layer, not a rasterizer or a copy of recovered guest functions.
#include <stdint.h>

enum { GF_GFX_PACKET_CAPACITY = 24 };
struct gf_gfx_write { uint64_t value; unsigned width; };
struct gf_gfx_packet { struct gf_gfx_write writes[GF_GFX_PACKET_CAPACITY]; unsigned count; };

static inline int gf_gfx_append(struct gf_gfx_packet* p, unsigned width, uint64_t value) {
  if (p->count >= GF_GFX_PACKET_CAPACITY || (width != 1 && width != 4 && width != 8)) return 0;
  p->writes[p->count].value = value;
  p->writes[p->count++].width = width;
  return 1;
}

static inline uint32_t gf_gfx_field(uint32_t old, uint32_t value, unsigned shift, unsigned bits) {
  uint32_t mask = ((UINT32_C(1)<<bits)-1u)<<shift;
  return (old & ~mask) | ((value<<shift)&mask);
}

static inline int gf_gfx_bp(struct gf_gfx_packet* p, uint32_t reg) {
  p->count = 0;
  return gf_gfx_append(p, 1, 0x61) && gf_gfx_append(p, 4, reg);
}

static inline int gf_gfx_xf_start(struct gf_gfx_packet* p, unsigned address, unsigned count) {
  p->count = 0;
  if (!count || count > 16 || address > 65535u || count > 65536u-address) return 0;
  return gf_gfx_append(p, 1, 0x10) && gf_gfx_append(p, 4, ((count-1u)<<16)|address);
}

// Words are host integers representing the original float bit patterns. No
// unaligned host float pointers or unnecessary float conversions are involved.
static inline int gf_gfx_matrix(struct gf_gfx_packet* p, const uint32_t* words,
                                 unsigned address, int normal, unsigned rows) {
  if (rows != 2 && rows != 3) return 0;
  if (!gf_gfx_xf_start(p, address, rows*(normal ? 3u : 4u))) return 0;
  for (unsigned r = 0; r < rows; ++r) {
    if (!gf_gfx_append(p, 8, (uint64_t)words[r*4]<<32 | words[r*4+1])) return 0;
    if (normal) {
      if (!gf_gfx_append(p, 4, words[r*4+2])) return 0;
    } else if (!gf_gfx_append(p, 8, (uint64_t)words[r*4+2]<<32 | words[r*4+3])) return 0;
  }
  return 1;
}

static inline int gf_gfx_light(struct gf_gfx_packet* p, const uint32_t* words, unsigned index) {
  if (index >= 8 || !gf_gfx_xf_start(p, 0x600u+index*16u, 16)) return 0;
  for (unsigned i = 0; i < 3; ++i) if (!gf_gfx_append(p, 4, 0)) return 0;
  if (!gf_gfx_append(p, 4, words[3])) return 0;
  for (unsigned i = 4; i < 16; i += 2)
    if (!gf_gfx_append(p, 8, (uint64_t)words[i]<<32 | words[i+1])) return 0;
  return 1;
}

static inline uint32_t gf_gfx_blend(uint32_t old, unsigned mode, unsigned source,
                                    unsigned destination, unsigned logic) {
  old = gf_gfx_field(old, mode == 1 || mode == 3, 0, 1);
  old = gf_gfx_field(old, mode == 2, 1, 1);
  old = gf_gfx_field(old, mode == 3, 11, 1);
  old = gf_gfx_field(old, logic, 12, 4);
  old = gf_gfx_field(old, source, 8, 3);
  return gf_gfx_field(old, destination, 5, 3);
}

static inline uint32_t gf_gfx_depth(uint32_t old, unsigned enable, unsigned compare, unsigned update) {
  old = gf_gfx_field(old, enable, 0, 1);
  old = gf_gfx_field(old, compare, 1, 3);
  return gf_gfx_field(old, update, 4, 1);
}

static inline uint32_t gf_gfx_color_inputs(uint32_t old, const unsigned* args) {
  for (unsigned i = 0; i < 4; ++i) old = gf_gfx_field(old, args[i], 12u-i*4u, 4);
  return old;
}

static inline uint32_t gf_gfx_alpha_inputs(uint32_t old, const unsigned* args) {
  for (unsigned i = 0; i < 4; ++i) old = gf_gfx_field(old, args[i], 13u-i*3u, 3);
  return old;
}

static inline uint32_t gf_gfx_combiner(uint32_t old, unsigned op, unsigned bias,
                                       unsigned scale, unsigned clamp, unsigned output) {
  old = gf_gfx_field(old, op, 18, 1);
  old = gf_gfx_field(old, op <= 1 ? scale : op>>1, 20, 2);
  old = gf_gfx_field(old, op <= 1 ? bias : 3, 16, 2);
  old = gf_gfx_field(old, clamp, 19, 1);
  return gf_gfx_field(old, output, 22, 2);
}
#endif
