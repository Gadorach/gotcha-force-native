#include <cstdio>
#include <cstdlib>
#include <limits>
#include "engine/gameplay/battle_split_projection.hpp"
#include "engine/graphics/projected_triangle.hpp"
namespace {unsigned checks=0;void check(bool b,const char* why){++checks;if(!b){std::fprintf(stderr,"split projection: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;std::string error;std::array<u8,128> bytes{};
    const std::array<u32,32> words{0x47465650,1,128,0,0x8008cbd4,0x8008c440,0x80005668,0x80009c4c,0x800058d0,0x80009cbc,0x8020b8ec,0x8002aaac,0x42240000,0x402aaaab,0x41200000,0x471c4000,0,0,0x44200000,0x43600000,0,0x3f800000,0,0x43600000,0x44200000,0x43600000,0,0x3f800000,1,1,4,7};
    for(unsigned i=0;i<words.size();++i)store_be32(bytes.data()+4*i,words[i]);
    BattleSplitProjectionProfile profile;check(profile.load({bytes.data(),bytes.size()},&error),"source profile");
    Camera top,bottom;check(battle_split_projection(profile,0,&top,&error)&&battle_split_projection(profile,1,&bottom,&error),"both source viewports");
    Camera::Basis basis;check(top.basis(&basis)&&basis.aspect==bits_to_f32(0x402aaaab)&&basis.aspect!=f32(640)/224,"explicit aspect differs from pixel ratio");
    const f32 depth=100,horizontal=depth*basis.tan_half_fov*basis.aspect,vertical=depth*basis.tan_half_fov;
    Vertex world[3];world[0].x=-horizontal;world[0].y=-vertical;world[1].x=horizontal;world[1].y=-vertical;world[2].y=vertical;
    for(auto& v:world)v.z=-depth;
    std::vector<Vertex> a,b;project_triangle(top,world,a);project_triangle(bottom,world,b);
    check(a.size()==3&&b.size()==3,"source frustum boundary triangle retained");
    for(unsigned i=0;i<3;++i){
        check(std::abs(a[i].x-b[i].x)<.001f&&std::abs(b[i].y-a[i].y-224)<.001f,"same projection in alternate half");
        const f32 point[3]{world[i].x,world[i].y,world[i].z};f32 x=0,y=0,scale=0;
        check(bottom.project(point,&x,&y,&scale)&&std::abs(x-b[i].x)<.001f&&std::abs(y-b[i].y)<.001f&&std::abs(scale-.1f)<.00001f,"point and clipped triangle share aspect/origin");
    }
    check(std::abs(a[0].x)<.001f&&std::abs(a[1].x-640)<.001f&&std::abs(a[2].y)<.001f&&std::abs(b[0].y-448)<.001f,"source viewport edges");
    for(auto& v:world)v.x=horizontal*1.02f;
    a.clear();project_triangle(top,world,a);check(a.empty(),"explicit aspect clips geometry that pixel-ratio frustum would admit");
    const auto key=top.cache_key();check(!battle_split_projection(profile,2,&top,&error)&&top.cache_key()==key,"unknown viewport refuses without mutation");
    for(float aspect:{-1.f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}){top.projection_aspect=aspect;check(!top.basis(nullptr),"invalid explicit aspect");}
    top=bottom;top.fov_y_degrees=170;top.projection_aspect=std::numeric_limits<float>::max();check(!top.basis(nullptr),"overflowing horizontal frustum refuses");
    top=bottom;top.projection_aspect=std::numeric_limits<float>::denorm_min();check(!top.basis(nullptr),"underflowing horizontal frustum refuses");
    top=bottom;top.viewport_y=-1;check(!top.basis(nullptr),"negative viewport origin");
    top=bottom;top.viewport_y=0x7fffffff;check(!top.basis(nullptr),"overflowing viewport extent");
    top=bottom;top.target[2]=top.eye[2];const auto invalid=top.cache_key();check(!battle_split_projection(profile,0,&top,&error)&&top.cache_key()==invalid,"projection does not invent camera orientation");
    bytes[55]^=1;check(!profile.load({bytes.data(),bytes.size()},&error),"changed source aspect refuses");
    std::printf("battle split projection: %u checks PASS\n",checks);
}
