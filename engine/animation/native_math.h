#ifndef GF_NATIVE_MATH_H
#define GF_NATIVE_MATH_H

// Independently implemented host math. No guest addresses, CPU state or game
// source here. Matrices are row-major affine 3x4; vectors contain THREE floats.
// Callers snapshot inputs before writing output so in-place operations are safe.
#include <math.h>

enum gf_math_operation {
  GF_MATH_COPY, GF_MATH_CONCAT, GF_MATH_POINT, GF_MATH_DIRECTION,
  GF_MATH_ADD, GF_MATH_SUBTRACT, GF_MATH_SCALE, GF_MATH_DOT,
  GF_MATH_CROSS, GF_MATH_NORMALIZE, GF_MATH_OPERATION_COUNT
};

static inline unsigned gf_math_a_size(unsigned op) {
  return op <= GF_MATH_DIRECTION ? 12u : 3u;
}

static inline unsigned gf_math_b_size(unsigned op) {
  if (op == GF_MATH_CONCAT) return 12;
  if (op == GF_MATH_POINT || op == GF_MATH_DIRECTION || op == GF_MATH_ADD ||
      op == GF_MATH_SUBTRACT || op == GF_MATH_DOT || op == GF_MATH_CROSS) return 3;
  return 0;
}

// Returns the number of result floats, or zero for an unsupported domain.
// Ordinary host arithmetic intentionally does not reproduce PPC FPSCR, paired
// scratch lanes, approximate reciprocal instructions or intermediate rounding.
static inline unsigned gf_math_evaluate(unsigned op, const float* a,
                                        const float* b, float scale, float* out) {
  if (op >= GF_MATH_OPERATION_COUNT) return 0;
  for (unsigned i = 0; i < gf_math_a_size(op); ++i) if (!isfinite(a[i])) return 0;
  for (unsigned i = 0; i < gf_math_b_size(op); ++i) if (!isfinite(b[i])) return 0;
  float result[12] = {0};
  unsigned n = 3;
  switch (op) {
  case GF_MATH_COPY:
    n = 12;
    for (unsigned i = 0; i < n; ++i) result[i] = a[i];
    break;
  case GF_MATH_CONCAT:
    n = 12;
    for (unsigned r = 0; r < 3; ++r)
      for (unsigned c = 0; c < 4; ++c) {
        float value = c == 3 ? a[r*4+3] : 0.0f;
        for (unsigned k = 0; k < 3; ++k) value += a[r*4+k] * b[k*4+c];
        result[r*4+c] = value;
      }
    break;
  case GF_MATH_POINT:
  case GF_MATH_DIRECTION:
    for (unsigned r = 0; r < 3; ++r)
      result[r] = a[r*4]*b[0] + a[r*4+1]*b[1] + a[r*4+2]*b[2] +
                  (op == GF_MATH_POINT ? a[r*4+3] : 0.0f);
    break;
  case GF_MATH_ADD:
  case GF_MATH_SUBTRACT:
    for (unsigned i = 0; i < 3; ++i) result[i] = op == GF_MATH_ADD ? a[i]+b[i] : a[i]-b[i];
    break;
  case GF_MATH_SCALE:
    if (!isfinite(scale)) return 0;
    for (unsigned i = 0; i < 3; ++i) result[i] = a[i]*scale;
    break;
  case GF_MATH_DOT:
    n = 1;
    result[0] = a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
    break;
  case GF_MATH_CROSS:
    result[0] = a[1]*b[2] - a[2]*b[1];
    result[1] = a[2]*b[0] - a[0]*b[2];
    result[2] = a[0]*b[1] - a[1]*b[0];
    break;
  case GF_MATH_NORMALIZE: {
    // Double length avoids overflow/underflow for finite float components.
    double length = sqrt((double)a[0]*a[0] + (double)a[1]*a[1] + (double)a[2]*a[2]);
    if (!(length > 0.0) || !isfinite(length)) return 0;
    for (unsigned i = 0; i < 3; ++i) result[i] = (float)(a[i]/length);
    break;
  }
  default: return 0;
  }
  for (unsigned i = 0; i < n; ++i) if (!isfinite(result[i])) return 0;
  for (unsigned i = 0; i < n; ++i) out[i] = result[i];
  return n;
}
#endif
