#include <cstdio>
#include <cstdlib>
#include <cmath>
#include "engine/assets/hsd_static_identity_colour.hpp"
namespace {unsigned checks=0;void check(bool b,const char* why){++checks;if(!b){std::fprintf(stderr,"identity colour reduction: %s\n",why);std::abort();}}}
int main(){
 using namespace gf;std::array<u8,256> bytes{};constexpr unsigned m=0x20,t=0x40,tev=0xa0;
 store_be32(bytes.data()+m+4,0x60004012);store_be32(bytes.data()+m+8,t);store_be32(bytes.data()+t+12,4);
 for(unsigned k=0;k<3;++k)store_be_f32(bytes.data()+t+28+4*k,1);
 store_be32(bytes.data()+t+64,0x340010);store_be32(bytes.data()+t+68,0x3f800000);store_be32(bytes.data()+t+88,tev);
 const std::array<u8,16> expression{1,0,0,0,0,0,1,1,12,0x87,8,0x85,7,7,7,7};std::copy(expression.begin(),expression.end(),bytes.begin()+tev);
 store_be32(bytes.data()+tev+20,0xffffff00);store_be32(bytes.data()+tev+28,0x40000770);
 hsd_detail::Reader reader;reader.block={bytes.data(),bytes.size()};reader.fields={m+8,t+88};reader.targets={t,tev};
 HsdMeshProperties original;original.material_offset=m;original.texture=0;original.material_supported=false;HsdMeshProperties reduced;std::string error;
 check(hsd_static_identity_colour(reader,original,&reduced,&error)&&reduced.material_supported&&!original.material_supported,"exact immutable source subtract expression reduces without mutating source metadata");
 check(reduced.texture==original.texture&&reduced.texture_colour_mode==4&&reduced.texture_alpha_mode==3&&reduced.material_offset==m,"ordinary texture and alpha behavior preserved");
 for(unsigned i=0;i<32;++i){auto bad=bytes;bad[tev+i]^=1;auto r=reader;r.block={bad.data(),bad.size()};check(!hsd_static_identity_colour(r,original,&reduced,&error),"changed TEV operation, input, constant or activation refuses");}
 for(unsigned offset:{m,m+16,t,t+4,t+12,t+16,t+28,t+40,t+64,t+68}){auto bad=bytes;bad[offset]^=1;auto r=reader;r.block={bad.data(),bad.size()};check(!hsd_static_identity_colour(r,original,&reduced,&error),"custom class/global descriptor, next stage, UV transform or weight refuses");}
 auto bad=original;bad.texture_uv1=true;check(!hsd_static_identity_colour(reader,bad,&reduced,&error),"changed coordinate source refuses");
 bad=original;bad.material_supported=true;check(!hsd_static_identity_colour(reader,bad,&reduced,&error),"reduction is not an arbitrary supported-material bypass");
 auto blend=bytes;store_be32(blend.data()+t+64,0x230010);auto blend_reader=reader;blend_reader.block={blend.data(),blend.size()};auto blend_material=original;blend_material.texture_colour_mode=3;blend_material.texture_alpha_mode=2;
 check(hsd_static_identity_colour(blend_reader,blend_material,&reduced,&error)&&reduced.texture_colour_mode==3&&reduced.texture_alpha_mode==2&&reduced.texture_weight==1,"side single-stage identity proof preserves its ordinary colour/alpha interpolation modes");
 blend_material.texture_weight=.5f;check(!hsd_static_identity_colour(blend_reader,blend_material,&reduced,&error),"side partial interpolation weight remains unsupported");
 for(unsigned value=0;value<256;++value){const double texture=double(value)/255;const double source=1-((1-texture)*1+texture*0);check(std::abs(source-texture)<1e-15,"source normalized RGB expression is identity for every byte value");}
 std::printf("static identity colour: %u checks PASS; bounded expression proof, not hardware TEV timing\n",checks);
}
