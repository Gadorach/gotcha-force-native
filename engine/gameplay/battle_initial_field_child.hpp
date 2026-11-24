#pragma once
#include "engine/assets/battle_initial_field_child.hpp"
#include "engine/gameplay/attachment_transform.hpp"
#include "engine/graphics/source_angles.hpp"
namespace gf {
struct BattleInitialFieldChildNumbers {
    std::array<f32,3> initial20{},smoothed148{},position20{},scale58{};
    std::array<s16,3> angles70{};s16 angle154=0;
    bool operator==(const BattleInitialFieldChildNumbers&)const=default;
};
//C63E8's first row3 initialization and immediate C6544. The original row
//has no auxiliary/effect, follow0, bob0 and step0; general rows still refuse.
//This numeric adapter does not select a bone, parent or current world owner.
inline bool battle_initial_field_child_numbers(const BattleInitialFieldChildProfile& profile,const Mat4& bone12,const std::array<s16,3>& parent_angles,f32 parent_scale,u8 flag168,BattleInitialFieldChildNumbers* out,std::string* error){
    if(out)*out={};
    auto fail=[&](){if(error)*error="initial field child numbers: fresh flag1/bounded bone12/scale required";return false;};
    if(!out||!profile.ready()||flag168!=1||!attachment_transform_detail::affine(bone12)||!std::isfinite(parent_scale)||parent_scale<=0||parent_scale>1e4f)return fail();
    BattleInitialFieldChildNumbers next;const f32 local[3]{0,30,0};
    mat4_transform_point(bone12,local,next.initial20.data());
    std::array<f32,3> target{};mat4_transform_point(bone12,local,target.data());
    for(unsigned k=0;k<3;++k){
        const f32 old=f32(next.initial20[k]*0.0f),follow=f32(target[k]*1.0f);
        next.smoothed148[k]=f32(old+follow);next.position20[k]=next.smoothed148[k];
        next.scale58[k]=parent_scale;next.angles70[k]=parent_angles[k];
        if(!std::isfinite(next.position20[k])||std::abs(next.position20[k])>1e7f)return fail();
    }
    const f32 bob=f32(0.0f*parent_scale);
    next.position20[1]=std::fma(source_angle_sin(next.angle154),bob,next.smoothed148[1]);
    *out=next;if(error)error->clear();return true;
}
}
