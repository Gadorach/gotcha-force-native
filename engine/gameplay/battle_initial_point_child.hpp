#pragma once
#include <cmath>
#include "engine/assets/battle_initial_point_child.hpp"
namespace gf {
struct BattleInitialPointChildNumbers {
    std::array<f32,3> smoothed14c{},position20{},scale58{1,1,1};
    f32 scale_b4=1,scale_b8=1;std::array<s16,3> angles70{};
    bool operator==(const BattleInitialPointChildNumbers&)const=default;
};
//46588 first cleared14C, thenFCEFC's vector add, NOT a matrix transform.
//No field-reference ownership, root binding, callback or live update authority.
inline bool battle_initial_point_child_numbers(const BattleInitialPointChildProfile& profile,const std::array<f32,3>& point150,const std::array<f32,3>& retained1dbc,f32 parent_scale,s16 steering1dfc,u8 flag169,BattleInitialPointChildNumbers* out,std::string* error){
    if(out)*out={};
    auto fail=[&](){if(error)*error="initial point child numbers: fresh flag1/bounded source fields required";return false;};
    if(!out||!profile.ready()||flag169!=1||!std::isfinite(parent_scale)||parent_scale<=0||parent_scale>1e4f)return fail();
    BattleInitialPointChildNumbers next;next.scale_b4=next.scale_b8=parent_scale;next.angles70[1]=steering1dfc;
    for(unsigned k=0;k<3;++k){
        if(!std::isfinite(point150[k])||!std::isfinite(retained1dbc[k])||std::abs(point150[k])>1e4f||std::abs(retained1dbc[k])>1e4f)return fail();
        //Two source vector scales then add. Do not reassociate into an FMA.
        const f32 target=f32(point150[k]*0.25f),old=f32(0.0f*0.75f);
        next.smoothed14c[k]=f32(target+old);next.position20[k]=f32(next.smoothed14c[k]+retained1dbc[k]);
    }
    *out=next;if(error)error->clear();return true;
}
}
