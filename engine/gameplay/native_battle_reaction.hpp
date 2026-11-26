#pragma once
// Recovered reaction direction stage of `800300BC`.
//
// The source computes a direction from one of seven attacker/receiver modes,
// falls back to (0,0,1) when the vector is degenerate, then writes the
// receiver link object's absolute yaw (+0x284) and pitch (+0x282) in 16-bit
// angle units. Native keeps the direction/angle math and stores the result
// relative to the receiver's current facing; presentation owns the duration.
#include <cmath>
#include "engine/core/types.hpp"
namespace gf {
// Source modes. Only the modes whose native inputs exist are admitted.
// 1: attacker/receiver link positions (base positions)
// 3: attacker/receiver side positions (+0x64; modelled as actor centre)
enum class NativeBattleReactionMode:u8 {AttackerToReceiver=1,CentreToCentre=3};
struct NativeBattleReaction {f32 yaw=0,pitch=0;};
constexpr f32 kNativeBattleReactionEpsilon=1e-6f;
constexpr f32 kNativeBattleReactionPi=3.14159265358979323846f;

inline bool native_battle_reaction_mode_valid(unsigned mode){
    return mode==unsigned(NativeBattleReactionMode::AttackerToReceiver)||
           mode==unsigned(NativeBattleReactionMode::CentreToCentre);
}
// `direction` is receiver minus attacker. `facing` is the receiver's current
// unit heading. A degenerate vector takes the source (0,0,1) fallback, so the
// receiver's absolute yaw becomes +z and the relative yaw is minus its heading.
inline NativeBattleReaction native_battle_reaction_angles(const f32 direction[3],BattlePoint facing){
    f32 x=direction[0],y=direction[1],z=direction[2];
    if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(z))return {};
    const f32 squared=x*x+y*y+z*z;
    if(squared<kNativeBattleReactionEpsilon){x=0;y=0;z=1;}
    const f32 xz=x*x+z*z;
    f32 yaw=std::atan2(x,z);
    const f32 pitch=-std::atan2(y,xz);
    if(std::isfinite(facing.x)&&std::isfinite(facing.z)&&(facing.x||facing.z)){
        yaw-=std::atan2(facing.x,facing.z);
        while(yaw>kNativeBattleReactionPi)yaw-=2*kNativeBattleReactionPi;
        while(yaw<-kNativeBattleReactionPi)yaw+=2*kNativeBattleReactionPi;
    }
    return {yaw,pitch};
}
} // namespace gf
