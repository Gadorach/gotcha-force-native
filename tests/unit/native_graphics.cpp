#include "engine/graphics/command_packets.h"
#include "engine/animation/native_transform.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <limits>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"graphics property failure line %u\n",__LINE__); std::abort(); } } while(false)
static bool close(double a,double b) { return std::abs(a-b)<2e-5*(1+std::abs(b)); }
int main() {
  gf_gfx_packet p{};
  std::array<uint32_t,16> words;
  for (unsigned i=0;i<16;++i) words[i]=0x3f800000u+i;
  CHECK(gf_gfx_matrix(&p,words.data(),12,0,3));
  CHECK(p.count==8 && p.writes[0].width==1 && p.writes[0].value==0x10);
  CHECK(p.writes[1].value==0x000b000cu);
  for (unsigned i=0;i<6;++i) CHECK(p.writes[i+2].value==((uint64_t(words[i*2])<<32)|words[i*2+1]));
  CHECK(gf_gfx_matrix(&p,words.data(),0x409,1,3));
  CHECK(p.writes[1].value==0x00080409u && p.count==8);
  for (unsigned r=0;r<3;++r) {
    CHECK(p.writes[2+r*2].width==8 && p.writes[3+r*2].width==4);
    CHECK(p.writes[3+r*2].value==words[r*4+2]);
  }
  CHECK(gf_gfx_light(&p,words.data(),7));
  CHECK(p.writes[1].value==0x000f0670u && p.count==12 && p.writes[5].value==words[3]);
  CHECK(!gf_gfx_light(&p,words.data(),8));
  CHECK(!gf_gfx_xf_start(&p,0xffff,2));
  CHECK(gf_gfx_bp(&p,0x41001234) && p.count==2 && p.writes[0].value==0x61 && p.writes[1].value==0x41001234);
  CHECK(gf_gfx_field(0x12345678,1,3,1)==0x12345678);
  CHECK(gf_gfx_field(0x12345678,0,3,1)==0x12345670);
  CHECK(gf_gfx_depth(0x40000000,1,3,1)==0x40000017);
  CHECK(gf_gfx_blend(0x41000000,3,4,5,6)==0x41006ca1);
  unsigned args[4]={1,2,3,4};
  CHECK(gf_gfx_color_inputs(0xc0000000,args)==0xc0001234);
  CHECK(gf_gfx_alpha_inputs(0xc100000f,args)==(0xc100000f|1u<<13|2u<<10|3u<<7|4u<<4));
  CHECK(gf_gfx_combiner(0xc0000000,1,2,3,1,2)==0xc0be0000);
  for (unsigned i=0;i<1000;++i) {
    float s[3]={1.0f+i*0.001f,2,3}, r[3]={i*0.001f,0.2f,-0.3f}, t[3]={1,2,3};
    float m[12], inverse[12], normal[12], product[12];
    CHECK(gf_transform_srt(s,r,t,nullptr,0,m));
    CHECK(gf_transform_inverse(m,0,inverse)==1);
    CHECK(gf_math_evaluate(GF_MATH_CONCAT,m,inverse,0,product)==12);
    for (unsigned row=0;row<3;++row) for (unsigned c=0;c<4;++c) CHECK(close(product[row*4+c],row==c ? 1 : 0));
    CHECK(gf_transform_inverse(m,1,normal)==1);
    for (unsigned row=0;row<3;++row) {
      CHECK(normal[row*4+3]==0);
      for (unsigned c=0;c<3;++c) CHECK(close(normal[row*4+c],inverse[c*4+row]));
    }
    float q[4]={0,0,sinf(r[2]/2),cosf(r[2]/2)}, zrotation[3]={0,0,r[2]}, from_q[12];
    CHECK(gf_transform_srt(s,q,t,nullptr,1,from_q));
    CHECK(gf_transform_srt(s,zrotation,t,nullptr,0,m));
    for (unsigned k=0;k<12;++k) CHECK(close(m[k],from_q[k]));
  }
  float zeros[12]={0}, output[12];
  for (float& f:output) f=99;
  CHECK(gf_transform_inverse(zeros,0,output)==0);
  CHECK(!gf_transform_quaternion(zeros,output));
  zeros[0]=std::numeric_limits<float>::infinity();
  CHECK(gf_transform_inverse(zeros,0,output)==-1);
  for (float f:output) CHECK(f==99);
  std::puts("{\"command_packet_properties\":true,\"transform_property_cases\":1000,\"invalid_domains\":true,\"all_passed\":true}");
}
