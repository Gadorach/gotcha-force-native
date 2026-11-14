#ifndef GF_NATIVE_TRANSFORM_H
#define GF_NATIVE_TRANSFORM_H
#include "native_math.h"

// Semantic transform kernels. All results are staged before output is committed.
// The caller decides how guest singular/error returns should be represented.
static inline int gf_transform_inverse(const float* m, int transpose, float* out) {
  for (unsigned i = 0; i < 12; ++i) if (!isfinite(m[i])) return -1;
  double a[3][6];
  for (unsigned r = 0; r < 3; ++r) for (unsigned c = 0; c < 6; ++c)
    a[r][c] = c < 3 ? m[r*4+c] : (c-3 == r ? 1.0 : 0.0);
  for (unsigned c = 0; c < 3; ++c) {
    unsigned pivot = c;
    for (unsigned r = c+1; r < 3; ++r) if (fabs(a[r][c]) > fabs(a[pivot][c])) pivot = r;
    if (a[pivot][c] == 0) return 0;
    for (unsigned k = 0; k < 6; ++k) { double t = a[c][k]; a[c][k] = a[pivot][k]; a[pivot][k] = t; }
    double divisor = a[c][c];
    for (unsigned k = 0; k < 6; ++k) a[c][k] /= divisor;
    for (unsigned r = 0; r < 3; ++r) if (r != c) {
      double factor = a[r][c];
      for (unsigned k = 0; k < 6; ++k) a[r][k] -= factor*a[c][k];
    }
  }
  float result[12];
  for (unsigned r = 0; r < 3; ++r) {
    for (unsigned c = 0; c < 3; ++c) result[r*4+c] = (float)(transpose ? a[c][r+3] : a[r][c+3]);
    result[r*4+3] = transpose ? 0 : (float)(-a[r][3]*m[3]-a[r][4]*m[7]-a[r][5]*m[11]);
  }
  for (unsigned i = 0; i < 12; ++i) if (!isfinite(result[i])) return -1;
  for (unsigned i = 0; i < 12; ++i) out[i] = result[i];
  return 1;
}

static inline int gf_transform_quaternion(const float* q, float* out) {
  double norm = 0;
  for (unsigned i = 0; i < 4; ++i) { if (!isfinite(q[i])) return 0; norm += (double)q[i]*q[i]; }
  if (!(norm > 0) || !isfinite(norm)) return 0;
  double x=q[0], y=q[1], z=q[2], w=q[3], factor=2.0/norm;
  float result[12] = {
    (float)(1-factor*(y*y+z*z)), (float)(factor*(x*y-z*w)), (float)(factor*(x*z+y*w)), 0,
    (float)(factor*(x*y+z*w)), (float)(1-factor*(x*x+z*z)), (float)(factor*(y*z-x*w)), 0,
    (float)(factor*(x*z-y*w)), (float)(factor*(y*z+x*w)), (float)(1-factor*(x*x+y*y)), 0};
  for (unsigned i=0; i<12; ++i) out[i]=result[i];
  return 1;
}

static inline int gf_transform_srt(const float* scale, const float* rotation,
                                   const float* translation, const float* parent_scale,
                                   int quaternion, float* out) {
  float m[12];
  for (unsigned i=0; i<3; ++i)
    if (!isfinite(scale[i]) || !isfinite(translation[i]) ||
        (parent_scale && (!isfinite(parent_scale[i]) || parent_scale[i]==0))) return 0;
  if (quaternion) { if (!gf_transform_quaternion(rotation,m)) return 0; }
  else {
    for (unsigned i=0; i<3; ++i) if (!isfinite(rotation[i])) return 0;
    float sx=sinf(rotation[0]), cx=cosf(rotation[0]);
    float sy=sinf(rotation[1]), cy=cosf(rotation[1]);
    float sz=sinf(rotation[2]), cz=cosf(rotation[2]);
    const float r[12] = {cz*cy, cz*sx*sy-cx*sz, cz*cx*sy+sx*sz, 0,
                         sz*cy, sz*sx*sy+cx*cz, sz*cx*sy-sx*cz, 0,
                         -sy, cy*sx, cy*cx, 0};
    for (unsigned i=0; i<12; ++i) m[i]=r[i];
  }
  for (unsigned r=0; r<3; ++r) {
    for (unsigned c=0; c<3; ++c) {
      m[r*4+c] *= scale[c];
      if (parent_scale && r!=c) m[r*4+c] *= parent_scale[c]/parent_scale[r];
    }
    m[r*4+3]=translation[r];
  }
  for (unsigned i=0; i<12; ++i) if (!isfinite(m[i])) return 0;
  for (unsigned i=0; i<12; ++i) out[i]=m[i];
  return 1;
}
#endif
