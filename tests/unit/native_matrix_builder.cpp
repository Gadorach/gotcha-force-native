#include "engine/animation/euler_builder.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <limits>

#define CHECK(x) do { if(!(x)){std::fprintf(stderr,"matrix builder property failure line %u\n",__LINE__);std::abort();} }while(false)
static bool close(double a,double b){return std::abs(a-b)<3e-5*(1+std::abs(b));}
int main(){
  const unsigned policy[6]={1,2,0,2,0,1};
  for(unsigned trial=0;trial<1000;++trial){
    float scale[3]={1+trial*0.001f,2,3},rotation[4]={0.2f,0.1f,0.3f,1},translation[3]={4,5,6};
    float parent_scale[3]={1.2f,-2.3f,3.4f};
    float parent[12]={1,0,0,10,0,1,0,20,0,0,1,30};
    float raw[12],local[12];gf_matrix_result result{};
    CHECK(gf_transform_srt(scale,rotation,translation,nullptr,1,raw));
    float epsilon=std::numeric_limits<float>::epsilon();
    CHECK(gf_matrix_quaternion_srt(scale,rotation,translation,parent_scale,epsilon,policy,local));
    unsigned at[6]={1,2,4,6,8,9};
    for(unsigned i=0;i<6;++i){unsigned row=at[i]/4;double d=parent_scale[row]+(parent_scale[row]>=0 ? double(epsilon) : -double(epsilon));
      CHECK(close(local[at[i]],double(raw[at[i]])*parent_scale[policy[i]]/d));}
    CHECK(gf_matrix_joint_quaternion(scale,rotation,translation,parent_scale,parent,epsilon,policy,&result));
    for(unsigned r=0;r<3;++r){
      CHECK(close(result.accumulated_scale[r],double(scale[r])*parent_scale[r]));
      for(unsigned c=0;c<4;++c)CHECK(close(result.world[r*4+c],local[r*4+c]+(c==3 ? parent[r*4+3] : 0)));
    }
    CHECK(gf_matrix_joint_quaternion(scale,rotation,translation,nullptr,nullptr,epsilon,policy,&result));
    for(unsigned i=0;i<12;++i)CHECK(result.world[i]==raw[i]);
    float euler[3]={float(trial)*0.0007f,0.4f,-0.2f};
    CHECK(gf_transform_srt(scale,euler,translation,nullptr,0,raw));
    CHECK(gf_matrix_euler_srt(scale,euler,translation,parent_scale,epsilon,local));
    for(unsigned row=0;row<3;++row)for(unsigned col=0;col<3;++col){
      double expected=raw[row*4+col];
      if(row!=col)expected*=parent_scale[col]/(parent_scale[row]+(parent_scale[row]>=0 ? double(epsilon) : -double(epsilon)));
      CHECK(close(local[row*4+col],expected));
    }
  }
  float s[3]={1,2,3},q[4]={0,0,0,0},t[3]={0,0,0},out[12];for(float& f:out)f=99;
  CHECK(!gf_matrix_quaternion_srt(s,q,t,nullptr,0,policy,out));
  q[3]=1;s[0]=std::numeric_limits<float>::infinity();
  CHECK(!gf_matrix_quaternion_srt(s,q,t,nullptr,0,policy,out));
  for(float f:out)CHECK(f==99);
  gf_euler_domain domain{0x3fe921fb,0x3e400000,0x3fd33333,0x3fe90000,{68,98,108,107}};
  CHECK(gf_euler_class(0,&domain)==0 && gf_euler_class(.1f,&domain)==1 && gf_euler_class(.3f,&domain)==2);
  CHECK(gf_euler_class(.785f,&domain)==3 && gf_euler_class(1,&domain)==-1);
  std::puts("{\"matrix_builder_property_cases\":1000,\"parent_compensation_and_composition\":true,\"invalid_domains\":true,\"all_passed\":true}");
}
