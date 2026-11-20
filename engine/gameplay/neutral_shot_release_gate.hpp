#pragma once
#include <cmath>
#include "engine/core/types.hpp"

namespace gf {
// Keep the detached result model-specific even when the scalar math agrees.
template<class Source>
struct NeutralShotReleaseGate {
    u8 slot=0,phase540=1;
    f32 aim_clock560=0;
    bool release_due=false;
};
// Scalar tail shared by the exact reviewed G-Red/Sasuke neutral shots.
// This is after the appropriate VM and aim visits; those are not supplied
// by this helper. Nonzero cue is necessary even on aim pass or timeout.
template<class Source>
inline bool neutral_shot_release_gate(Source source,u8 post_vm_event1cef,bool aim_pass,
                                      f32 aim_clock560,f32 actor_step,NeutralShotReleaseGate<Source>* out){
    if(!out||!source.reviewed()||!std::isfinite(aim_clock560)||aim_clock560< -8192.f||aim_clock560>30.f||
       !std::isfinite(actor_step)||actor_step<0||actor_step>16)return false;
    const f32 left=aim_pass?aim_clock560:f32(aim_clock560-actor_step);
    const bool due=post_vm_event1cef&&(aim_pass||left<=0);
    *out={source.slot,u8(due?2:1),left,due};return true;
}
}
