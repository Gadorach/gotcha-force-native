#pragma once
// Recovered reaction direction stage of `800300BC`.
//
// The reviewed source branches initialize a direction for modes 0 through 4,
// falls back to (0,0,-1) when |v|^2 < 0.01, then writes the
// receiver link object's absolute yaw (+0x284) and pitch (+0x282) in 16-bit
// angle units. Native keeps the direction/angle math and stores the result
// relative to the receiver's current facing; presentation owns the duration.
#include <array>
#include <cmath>
#include <optional>
#include "engine/gameplay/battle_swept_circle.hpp"
namespace gf {
// The native API admits mode 4 only with a scoped body root at contact.
// 1: attacker/receiver object origins
// 3: attacker/receiver pair geometry centres
enum class NativeBattleReactionMode:u8 {
    ObjectMotion=0, AttackerToReceiver=1, PairMotion=2, CentreToCentre=3, BodyRoot=4};
struct NativeBattleReaction {f32 yaw=0,pitch=0;};
struct NativeBattleReactionMotion {
    std::array<f32,3> object_delta{},pair_delta{},object_vector{};
};
struct NativeBattleReactionCentres {
    std::array<f32,3> attacker{},receiver{};
};
struct NativeBattleReactionContact {
    std::optional<BattlePoint> body_forward{};
    std::optional<NativeBattleReactionMotion> motion{};
    std::optional<std::array<f32,3>> object_origin{};
    std::optional<NativeBattleReactionCentres> centres{};
};
constexpr f32 kNativeBattleReactionEpsilon=0.01f; // lbl_80436FC0

// Modes 0/2 consume the attacking object or pair displacement, with the
// live object vector as the zero-length fallback. Unsampled contacts still
// use the authored planar attacker-velocity approximation.
// Mode 1 receives the selected object origins; mode 3 reads the two proxy
// centres when available, otherwise it uses the same origin approximation.
// Mode 4 takes the direct melee proxy or linked body's world-root +Z column.
inline bool native_battle_reaction_mode_valid(unsigned mode){
    return mode<=unsigned(NativeBattleReactionMode::BodyRoot);
}
inline bool native_battle_reaction_direction(unsigned mode,const f32 source[3],
        BattlePoint velocity,const f32 receiver[3],f32 out[3],const NativeBattleReactionContact& contact={}){
    if(!native_battle_reaction_mode_valid(mode))return false;
    if(mode==unsigned(NativeBattleReactionMode::BodyRoot)){
        if(!contact.body_forward)return false;
        out[0]=contact.body_forward->x;out[1]=0;out[2]=contact.body_forward->z;return true;
    }
    if(mode==unsigned(NativeBattleReactionMode::ObjectMotion)||mode==unsigned(NativeBattleReactionMode::PairMotion)){
        if(contact.motion){
            const auto& motion=*contact.motion;
            const auto& delta=mode==unsigned(NativeBattleReactionMode::ObjectMotion)?motion.object_delta:motion.pair_delta;
            const auto& v=delta[0]*delta[0]+delta[1]*delta[1]+delta[2]*delta[2]==0.f?motion.object_vector:delta;
            for(unsigned k=0;k<3;++k)out[k]=v[k];
            return true;
        }
        out[0]=velocity.x;out[1]=0;out[2]=velocity.z;return true;
    }
    if(mode==unsigned(NativeBattleReactionMode::CentreToCentre)&&contact.centres){
        const auto& centres=*contact.centres;
        for(unsigned k=0;k<3;++k)out[k]=centres.receiver[k]-centres.attacker[k];
    }else for(unsigned k=0;k<3;++k)out[k]=receiver[k]-source[k];
    return true;
}
constexpr f32 kNativeBattleReactionPi=3.14159265358979323846f;
// The source subtracts the attack-table offsets (+0x14/+0x15) in 0x100
// 16-bit angle units, i.e. one turn / 256 per signed byte.
constexpr f32 kNativeBattleReactionUnit=kNativeBattleReactionPi/128.f;

inline f32 native_battle_reaction_wrap(f32 angle){
    while(angle>kNativeBattleReactionPi)angle-=2*kNativeBattleReactionPi;
    while(angle<-kNativeBattleReactionPi)angle+=2*kNativeBattleReactionPi;
    return angle;
}

// `direction` supplies the selected vector. `facing` is the receiver's current
// unit heading. A degenerate vector takes the source (0,0,-1) fallback, so the
// receiver's absolute yaw becomes pi and the relative yaw is pi minus its
// heading.
inline NativeBattleReaction native_battle_reaction_angles(const f32 direction[3],BattlePoint facing){
    f32 x=direction[0],y=direction[1],z=direction[2];
    if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(z))return {};
    const f32 squared=x*x+y*y+z*z;
    if(squared<kNativeBattleReactionEpsilon){x=0;y=0;z=-1;}
    const f32 xz=x*x+z*z;
    f32 yaw=std::atan2(x,z);
    const f32 pitch=-std::atan2(y,xz);
    if(std::isfinite(facing.x)&&std::isfinite(facing.z)&&(facing.x||facing.z)){
        yaw-=std::atan2(facing.x,facing.z);
        yaw=native_battle_reaction_wrap(yaw);
    }
    return {yaw,pitch};
}
} // namespace gf
