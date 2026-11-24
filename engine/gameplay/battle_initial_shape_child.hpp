#pragma once
#include "engine/assets/battle_initial_shape_child.hpp"
#include "engine/assets/hsd_light_constructor.hpp"
namespace gf {
struct BattleInitialShapeNumbers {
    std::array<f32,3> direction9c{0,0,1};f32 visibility144=1;
    std::optional<u16> selected_light;bool light_direction_read=false,normalization_fallback=false;
    bool operator==(const BattleInitialShapeNumbers&)const=default;
};
//2442DC then1C468, reviewed initial type0 shapes. Constructor values are not
//general live lights; the bootstrap separately qualifies their archive/lifetime.
inline bool battle_initial_shape_numbers(const BattleInitialShapeChildProfile& profile,const HsdInitialLightConstructor& lights,BattleInitialShapeNumbers* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("initial shape numbers: ")+why;return false;};
    if(!out||!profile.ready()||lights.generation!=1||lights.animation_requested||lights.nodes.size()>256)return fail("source profile and initial unanimated constructor required");
    BattleInitialShapeNumbers next;next.selected_light=lights.primary_infinite;
    if(lights.primary_infinite){
        if(*lights.primary_infinite>=lights.nodes.size())return fail("current primary alias index");
        const auto& light=lights.nodes[*lights.primary_infinite].light;if((light.flags&3u)!=1)return fail("selected source infinite light required");
        std::array<f32,3> vector{};bool nonzero=false;
        const f64 epsilon=std::bit_cast<f64>(u64(0x3810000000000000));
        for(unsigned k=0;k<3;++k){
            if(!std::isfinite(light.position[k])||!std::isfinite(light.interest[k])||std::abs(light.position[k])>1e7f||std::abs(light.interest[k])>1e7f)return fail("bounded initial WObj values");
            vector[k]=f32(light.interest[k]-light.position[k]);nonzero=nonzero||std::abs(f64(vector[k]))>epsilon;
        }
        if(nonzero){
            //Tiny nonzero normalization is not silently repaired into fallback.
            if(!lighting_detail::normalize(vector.data()))return fail("nonzero tiny/underflow normalization unsupported");
        }else{vector={0,0,1};next.normalization_fallback=true;}
        f32 length2=0;for(float value:vector)length2=f32(length2+f32(value*value));
        if(std::sqrt(length2)>bits_to_f32(0x3a83126f))next.direction9c=vector;
        next.light_direction_read=true;
    }
    *out=next;if(error)error->clear();return true;
}
}
