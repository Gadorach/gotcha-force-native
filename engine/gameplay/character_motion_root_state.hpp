#pragma once
#include <algorithm>
#include <array>
#include <span>
#include <string>
#include "engine/graphics/transform.hpp"
#include "engine/core/source_heading.hpp"
namespace gf {
enum class CharacterMotionEvaluation {Request,Advance,FrozenBlend,ReRequest,Directional};
enum class CharacterMotionRootObservationMode {AObjOnly,AObjOrPublicNull};
enum class CharacterMotionRootClockSource {RootAObj,InheritedMotion};
struct CharacterMotionRootObservation {std::array<f32,3> translation{};f32 frame=0;s8 scale_selector=-1;CharacterMotionRootClockSource clock_source=CharacterMotionRootClockSource::RootAObj;};
struct CharacterMotionRootFields {
    f32 bias1d9c=0,delta5f4=0,scale_b4=0;s16 yaw5ae=0;
    u8 suppress_selector1db2=0,vertical_only598=0,apply_position1d9b=0;
    std::span<const f32> multipliers1dd4;
};
struct CharacterMotionRootState {
    std::array<f32,3> position{},delta1da0{};f32 speed1dac=0;s16 heading1db0=0;u64 updates=0;
};
inline bool character_motion_root_number(f32 value){return std::isfinite(value)&&std::abs(value)<=1e7f;}
inline bool character_motion_root_state_valid(const CharacterMotionRootState& state){
    for(auto values:{state.position,state.delta1da0})for(auto value:values)if(!character_motion_root_number(value))return false;
    return character_motion_root_number(state.speed1dac)&&state.speed1dac>=0;
}
// Shared source4AAE0 numeric step. Native owners supply qualified observations
// and operation provenance; this pure kernel alone grants no actor admission.
inline bool character_motion_root_step(const CharacterMotionRootObservation& before,const CharacterMotionRootObservation& after,
                                      CharacterMotionEvaluation evaluation,const CharacterMotionRootFields& fields,
                                      const CharacterMotionRootState& state,CharacterMotionRootState* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("character motion root: ")+why;return false;};
    auto number=character_motion_root_number;
    if(!out||!character_motion_root_state_valid(state)||state.updates==~u64(0)||!std::isfinite(after.frame)||after.frame<0||after.frame>8192||evaluation==CharacterMotionEvaluation::Directional||unsigned(evaluation)>unsigned(CharacterMotionEvaluation::Directional))return fail("ordinary candidate root state/evaluation");
    auto candidate=state;candidate.delta1da0={};candidate.speed1dac=0;candidate.heading1db0=0;
    //Completion re-requests have already cleared final2F, but still do not move.
    if(evaluation==CharacterMotionEvaluation::Advance&&after.frame>0){
        if(!number(fields.bias1d9c))return fail("root bias domain");
        f32 factor=f32(1+fields.bias1d9c);
        if(!fields.suppress_selector1db2&&after.scale_selector>=0){const auto index=unsigned(after.scale_selector);
            if(index>=fields.multipliers1dd4.size()||!number(fields.multipliers1dd4[index]))return fail("selected multiplier owner/extent");
            factor=f32(factor*fields.multipliers1dd4[index]);
        }
        if(!number(factor)||!number(fields.delta5f4)||fields.delta5f4<=0)return fail("root factor/positive source delta5F4 required");
        f64 square=0;for(unsigned i=0;i<3;++i){if(!number(before.translation[i])||!number(after.translation[i]))return fail("native local root domain");candidate.delta1da0[i]=f32(f32(after.translation[i]-before.translation[i])*factor);if(!number(candidate.delta1da0[i]))return fail("derived local root delta");square+=f64(candidate.delta1da0[i])*candidate.delta1da0[i];}
        //Bounded host sqrt/atan2/libm model, not paired-single/GX/FPSCR parity.
        const f32 length=f32(std::sqrt(square));candidate.speed1dac=f32(length/fields.delta5f4);
        if(!source_heading_s16(candidate.delta1da0[0],candidate.delta1da0[2],&candidate.heading1db0)||!number(candidate.speed1dac))return fail("derived speed/heading domain");
        if(fields.apply_position1d9b){
            if(!number(fields.scale_b4))return fail("actorB4 required for position integration");
            const f32 yaw=f32(f32(fields.yaw5ae)*bits_to_f32(0x38c90fdbu));const auto rotation=mat4_rotation_y(yaw);
            for(auto& value:candidate.delta1da0)value=f32(value*fields.scale_b4);
            const auto local=candidate.delta1da0;mat4_transform_direction(rotation,local.data(),candidate.delta1da0.data());
            if(fields.vertical_only598)candidate.delta1da0[0]=candidate.delta1da0[2]=0;
            for(unsigned i=0;i<3;++i){candidate.position[i]=f32(candidate.position[i]+candidate.delta1da0[i]);if(!number(candidate.position[i])||!number(candidate.delta1da0[i]))return fail("derived actor position");}
        }
    }
    ++candidate.updates;*out=candidate;if(error)error->clear();return true;
}
} //namespace gf
