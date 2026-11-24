#pragma once
#include "engine/assets/battle_initial_shape_world.hpp"
#include "engine/gameplay/stage_ground_surface.hpp"
#include "engine/gameplay/attachment_transform.hpp"
#include "engine/graphics/source_basis.hpp"
namespace gf {
struct BattleInitialShapeWorld {
    Mat4 world114;std::array<f32,3> bone_point{},position20{},normal38{};
    f32 visibility144=0,base_scale=0,range_squared=0,distance_squared=0;
    StageGroundSurfaceResult surface;bool operator==(const BattleInitialShapeWorld&)const=default;
};
inline bool battle_initial_shape_world(const BattleInitialShapeWorldProfile& profile,const std::array<const StihGrid*,3>& grids,const StageGroundParameters& ground,const StageGroundActor& actor,u8 projection87,const Mat4& bone,const std::array<f32,3>& light,f32 height668,f32 size7fc,f32 parent_b4,f32 child_b4,BattleInitialShapeWorld* out,std::string* error){
    auto fail=[&](){if(error)*error="shape world: exact static/type0/bounded nondegenerate source inputs required";return false;};
    if(!out||!profile.ready()||projection87||actor.attached1db||!attachment_transform_detail::affine(bone))return fail();
    for(f32 value:{height668,size7fc,parent_b4,child_b4})if(!std::isfinite(value)||value<=0||value>1e4f)return fail();
    BattleInitialShapeWorld next;for(unsigned k=0;k<3;++k)next.bone_point[k]=bone.at(k,3);
    if(!stage_ground_surface(grids,ground,actor,next.bone_point,height668,&next.surface,error))return false;
    next.position20=next.bone_point;next.position20[1]=next.surface.height;next.normal38=next.surface.normal;
    Mat4 basis;bool built=false;
    if(!source_basis_detail::basis(4,next.normal38,light,bits_to_f32(0x2edbe6fe),&basis,&built)||!built)return fail(); //retained/unwritten basis branch stays unsupported
    next.base_scale=f32(child_b4*f32(size7fc*parent_b4));const f32 scale[3]{f32(next.base_scale*2.f),next.base_scale,1.f};
    next.world114=mat4_multiply(basis,mat4_scale(scale));for(unsigned k=0;k<3;++k)next.world114.at(k,3)=next.position20[k];
    const f32 dx=f32(next.position20[0]-next.bone_point[0]),dy=f32(next.position20[1]-next.bone_point[1]),dz=f32(next.position20[2]-next.bone_point[2]);
    next.distance_squared=f32(std::fma(dx,dx,f32(dy*dy))+f32(dz*dz));
    const f32 range=f32(f32(100.f*size7fc)*parent_b4);next.range_squared=f32(range*range);
    if(!std::isfinite(next.range_squared)||next.range_squared<=0||!std::isfinite(next.distance_squared))return fail();
    next.visibility144=next.distance_squared>next.range_squared?0.f:std::clamp(f32(1.f-f32(next.distance_squared/next.range_squared)),0.f,1.f);
    if(!attachment_transform_detail::affine(next.world114))return fail();
    *out=next;if(error){error->clear();}return true;
}
}
