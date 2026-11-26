#pragma once
// Recovered reaction direction stage of `800300BC`.
//
// The source computes a direction from one of seven attacker/receiver modes,
// falls back to (0,0,-1) when |v|^2 < 0.01, then writes the
// receiver link object's absolute yaw (+0x284) and pitch (+0x282) in 16-bit
// angle units. Native keeps the direction/angle math and stores the result
// relative to the receiver's current facing; presentation owns the duration.
#include <cmath>
#include "engine/core/types.hpp"
namespace gf {
// Source modes. Only the modes whose native inputs exist are admitted.
// 1: attacker/receiver link positions (base positions)
// 3: attacker/receiver side positions (+0x64; modelled as actor centre)
enum class NativeBattleReactionMode:u8 {
    AttackerVelocity=0, AttackerToReceiver=1, PairVelocity=2, CentreToCentre=3};
struct NativeBattleReaction {f32 yaw=0,pitch=0;};
constexpr f32 kNativeBattleReactionEpsilon=0.01f; // lbl_80436FC0

// Source modes 0/2 read a pair-position delta; the native analogue is the
// attacker's per-tick velocity. Modes 1/3 use receiver - attacker.
inline void native_battle_reaction_direction(unsigned mode,const f32 source[3],
        BattlePoint velocity,const f32 receiver[3],f32 out[3]){
    if(mode==unsigned(NativeBattleReactionMode::AttackerVelocity)||
       mode==unsigned(NativeBattleReactionMode::PairVelocity)){
        out[0]=velocity.x;out[1]=0;out[2]=velocity.z;return;
    }
    for(unsigned k=0;k<3;++k)out[k]=receiver[k]-source[k];
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

inline bool native_battle_reaction_mode_valid(unsigned mode){
    return mode<=unsigned(NativeBattleReactionMode::CentreToCentre);
}
// `direction` is receiver minus attacker. `facing` is the receiver's current
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
