#ifndef GF_ANIMATION_EULER_BUILDER_H
#define GF_ANIMATION_EULER_BUILDER_H
#include "matrix_builder.h"
#include <stdint.h>
#include <string.h>

// A reviewed direct-trigonometry domain and compatibility cost model. Limits and
// debit values are supplied by the private binding; they do not clamp angles.
struct gf_euler_domain { uint32_t maximum,tiny,low,high;unsigned pair_debit[4]; };
static inline int gf_euler_class(float angle,const struct gf_euler_domain* domain) {
  if(!isfinite(angle))return -1;
  double value=angle;uint64_t bits;memcpy(&bits,&value,8);
  uint32_t high=(uint32_t)(bits>>32)&0x7fffffffu;
  if(high>domain->maximum)return -1;
  if(high<domain->tiny)return 0;
  if(high<domain->low)return 1;
  return high>domain->high ? 3 : 2;
}

static inline int gf_matrix_euler_srt(const float* scale,const float* rotation,const float* translation,
                                     const float* parent_scale,float regularizer,float* out) {
  float m[12];
  if(!isfinite(regularizer) || regularizer<0 || !gf_transform_srt(scale,rotation,translation,0,0,m))return 0;
  if(parent_scale){
    for(unsigned i=0;i<3;++i)if(!isfinite(parent_scale[i]))return 0;
    for(unsigned row=0;row<3;++row)for(unsigned col=0;col<3;++col)if(row!=col){
      float denominator=parent_scale[row]+(parent_scale[row]>=0 ? regularizer : -regularizer);
      if(denominator==0 || !isfinite(denominator))return 0;
      m[row*4+col]*=parent_scale[col]*(1.0f/denominator);
    }
  }
  for(unsigned i=0;i<12;++i)if(!isfinite(m[i]))return 0;
  for(unsigned i=0;i<12;++i)out[i]=m[i];
  return 1;
}
static inline int gf_matrix_joint_euler(const float* scale,const float* rotation,const float* translation,
                                       const float* parent_scale,const float* parent_matrix,float regularizer,
                                       struct gf_matrix_result* out) {
  struct gf_matrix_result result;float local[12];
  if(!gf_matrix_euler_srt(scale,rotation,translation,parent_scale,regularizer,local))return 0;
  for(unsigned i=0;i<3;++i){result.accumulated_scale[i]=scale[i]*(parent_scale ? parent_scale[i] : 1.0f);if(!isfinite(result.accumulated_scale[i]))return 0;}
  if(parent_matrix){if(gf_math_evaluate(GF_MATH_CONCAT,parent_matrix,local,0,result.world)!=12)return 0;}
  else for(unsigned i=0;i<12;++i)result.world[i]=local[i];
  *out=result;return 1;
}
#endif
