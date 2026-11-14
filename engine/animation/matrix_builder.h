#ifndef GF_ANIMATION_MATRIX_BUILDER_H
#define GF_ANIMATION_MATRIX_BUILDER_H
#include "native_transform.h"

// Native quaternion transform construction. Parent compensation is explicit
// policy data so the boundary can retain a reviewed legacy convention without
// embedding game object layouts, addresses or guessed constants here.
static inline int gf_matrix_quaternion_srt(const float* scale,const float* rotation,
                                           const float* translation,const float* parent_scale,
                                           float regularizer,const unsigned* numerator,float* out) {
  float m[12];
  if(!isfinite(regularizer) || regularizer<0 ||
     !gf_transform_srt(scale,rotation,translation,0,1,m)) return 0;
  if(parent_scale) {
    for(unsigned i=0;i<3;++i) if(!isfinite(parent_scale[i])) return 0;
    const unsigned positions[6]={1,2,4,6,8,9};
    for(unsigned i=0;i<6;++i) {
      if(!numerator || numerator[i]>=3) return 0;
      unsigned row=positions[i]/4;
      float denominator=parent_scale[row]+(parent_scale[row]>=0 ? regularizer : -regularizer);
      if(denominator==0 || !isfinite(denominator)) return 0;
      float reciprocal=1.0f/denominator;
      m[positions[i]]*=parent_scale[numerator[i]]*reciprocal;
    }
  }
  for(unsigned i=0;i<12;++i) if(!isfinite(m[i])) return 0;
  for(unsigned i=0;i<12;++i) out[i]=m[i];
  return 1;
}

struct gf_matrix_result { float world[12],accumulated_scale[3]; };
static inline int gf_matrix_joint_quaternion(const float* scale,const float* rotation,
                                             const float* translation,const float* parent_scale,
                                             const float* parent_matrix,float regularizer,
                                             const unsigned* numerator,struct gf_matrix_result* out) {
  struct gf_matrix_result result;
  float local[12];
  if(!gf_matrix_quaternion_srt(scale,rotation,translation,parent_scale,regularizer,numerator,local)) return 0;
  for(unsigned i=0;i<3;++i) {
    result.accumulated_scale[i]=scale[i]*(parent_scale ? parent_scale[i] : 1.0f);
    if(!isfinite(result.accumulated_scale[i])) return 0;
  }
  if(parent_matrix) {
    if(gf_math_evaluate(GF_MATH_CONCAT,parent_matrix,local,0,result.world)!=12) return 0;
  } else for(unsigned i=0;i<12;++i) result.world[i]=local[i];
  *out=result;return 1;
}
#endif
