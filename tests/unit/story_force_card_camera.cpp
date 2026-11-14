#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <vector>
#include "engine/graphics/camera.hpp"
namespace {unsigned checks=0;void check(bool ok){++checks;if(!ok)std::abort();}}
int main(int argc,char** argv){using namespace gf;if(argc!=3)return 2;
    auto read=[](const char* p){std::ifstream f(p,std::ios::binary);return std::vector<u8>{std::istreambuf_iterator<char>(f),{}};};
    const auto a=read(argv[1]),b=read(argv[2]);check(a.size()==84&&b.size()==12);Camera canonical,constructed;canonical.fov_y_degrees=constructed.fov_y_degrees=41;canonical.near_plane=constructed.near_plane=10;canonical.far_plane=constructed.far_plane=40000;
    for(unsigned i=0;i<3;++i){canonical.eye[i]=load_be_f32(a.data()+12+i*4);canonical.target[i]=load_be_f32(a.data()+24+i*4);canonical.up[i]=load_be_f32(a.data()+36+i*4);}
    const auto zero=load_be_f32(b.data()),minus100=load_be_f32(b.data()+4),plus100=load_be_f32(b.data()+8);check(zero==0&&minus100==-100&&plus100==100);
    for(unsigned i=0;i<3;++i){constructed.eye[i]=zero;constructed.target[i]=i==2?minus100:zero;constructed.up[i]=i==1?plus100:zero;}
    Camera::Basis x,y;check(canonical.basis(&x)&&constructed.basis(&y));
    auto exact=[](f32 p,f32 q){return f32_to_bits(p)==f32_to_bits(q);};
    for(unsigned i=0;i<3;++i){check(exact(x.right[i],y.right[i]));check(exact(x.up[i],y.up[i]));check(exact(x.forward[i],y.forward[i]));check(exact(x.eye[i],y.eye[i]));}
    check(exact(x.tan_half_fov,y.tan_half_fov)&&exact(x.aspect,y.aspect));
    for(f32 px:{-200.f,-80.f,0.f,100.f,175.f})for(f32 py:{-60.f,0.f,90.f})for(f32 pz:{-20.f,-500.f,-1000.f,-40001.f,5.f}){
        const f32 p[3]={px,py,pz};f32 ax=0,ay=0,as=0,bx=0,by=0,bs=0;const bool ac=canonical.project(p,&ax,&ay,&as),bc=constructed.project(p,&bx,&by,&bs);check(ac==bc);
        if(ac){check(exact(ax,bx)&&exact(ay,by)&&exact(as,bs));check(exact(canonical.size_scale(20,as),constructed.size_scale(20,bs)));}
    }
    std::printf("Story canonical camera equivalence: %u checks PASS; source target-100/up100 versus normalized asset directions, not byte-identical fields\n",checks);
}
