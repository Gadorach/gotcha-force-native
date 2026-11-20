#pragma once
#include <cmath>
#include "engine/core/types.hpp"

namespace gf {
struct NormalAccelerationSource {
    u16 model=0;
    u8 variant=0,slot=0;
    constexpr bool reviewed()const{return model==0x0004&&!variant&&slot<6;}
};
namespace acceleration_ninja_detail {
inline bool clock(f32 value,f32 limit){return std::isfinite(value)&&value>=0&&value<=limit;}
inline bool step(f32 value){return clock(value,16);}
}
struct AccelerationStanceEntry {
    f32 sustained558=0,lockout55c=0,slow560=0,stored_speed38=0,vertical_change50=0;
    u8 phase540=0,presentation6ea=0,mask=0,group=0,program=0;
};
// Already selected ordinary X on the existing actor. Describes setup
// operands only; no form replacement, stock debit, pose or motion execution.
inline bool normal_acceleration_start(NormalAccelerationSource source,bool reached_stance_entry,AccelerationStanceEntry* out){
    if(!out||!source.reviewed()||!reached_stance_entry)return false;
    *out={3600,20,0,39,-1.2f,1,1,15,4,0};return true;
}
struct AccelerationSlowStep {
    f32 slow560=0;
    bool request_recovery=false;
};
inline bool normal_acceleration_slow_step(NormalAccelerationSource source,f32 slow560,f32 rate1dc8,
                                          bool low_progress,AccelerationSlowStep* out){
    using namespace acceleration_ninja_detail;
    if(!out||!source.reviewed()||!clock(slow560,10)||!step(rate1dc8))return false;
    const f32 next=low_progress?f32(slow560+rate1dc8):slow560;
    *out={next,next>10};return true;
}

enum class AccelerationEdgeExit:u8 {None,Recovery,JumpOrFallRequest};
struct AccelerationLockoutStep {
    f32 lockout55c=0;
    AccelerationEdgeExit exit=AccelerationEdgeExit::None;
};
// Raw actor fresh-edge word, not held input. Jump/fall is still an
// external admission path; this cannot turn A into a guaranteed jump.
inline bool normal_acceleration_lockout(NormalAccelerationSource source,f32 lockout55c,f32 rate1dc8,u32 pressed5bc,
                                        AccelerationLockoutStep* out){
    using namespace acceleration_ninja_detail;
    if(!out||!source.reviewed()||!clock(lockout55c,20)||!step(rate1dc8))return false;
    const f32 left=f32(lockout55c-rate1dc8);
    AccelerationLockoutStep next{left>0?left:0.f};
    if(!next.lockout55c){
        if(pressed5bc&0x400u)next.exit=AccelerationEdgeExit::Recovery;
        else if(pressed5bc&0x100u)next.exit=AccelerationEdgeExit::JumpOrFallRequest;
    }
    *out=next;return true;
}
struct AccelerationSustainStep {
    f32 sustained558=0;
    bool request_recovery=false,request_trail_update=false;
};
// Called only at the sustained tail after earlier exit and contact work.
// The speed threshold belongs to presentation, never to a hit shape.
inline bool normal_acceleration_sustain(NormalAccelerationSource source,f32 sustained558,f32 rate1dc8,f32 speed44,
                                        AccelerationSustainStep* out){
    using namespace acceleration_ninja_detail;
    if(!out||!source.reviewed()||!clock(sustained558,3600)||!step(rate1dc8)||
       !std::isfinite(speed44)||std::abs(speed44)>1e5f)return false;
    const f32 left=f32(sustained558-rate1dc8);
    *out={left,left<=0,left>0&&speed44>3};return true;
}

enum class AccelerationProgramPath:u8 {StanceStartup,OrdinaryAirDash,StanceRecovery};
struct AccelerationHitShapeRequest {
    bool requested=false;
    u8 actor_slot=0,credited_slot=0,shape_group=0,hit_row=0,program_group=0,program_slot=0;
    s16 start=0,end=0;
};
// Only the inspected program site with lane bit 0. Neither a trail nor an
// aim target supplies the carrier, credit, or a hit.
inline bool normal_acceleration_hit_shape(NormalAccelerationSource source,AccelerationProgramPath path,
                                          u8 lane_mask,bool reached_reviewed_program,AccelerationHitShapeRequest* out){
    if(!out||!source.reviewed()||!reached_reviewed_program)return false;
    AccelerationHitShapeRequest next;
    switch(path){
    case AccelerationProgramPath::StanceStartup:
        if(lane_mask&1u)next={true,source.slot,source.slot,10,9,4,0,4,-2};
        break;
    case AccelerationProgramPath::OrdinaryAirDash:
        if(lane_mask&1u)next={true,source.slot,source.slot,11,11,0,11,5,-2};
        break;
    case AccelerationProgramPath::StanceRecovery:break;
    default:return false;
    }
    *out=next;return true;
}
enum class AccelerationHitGate:u8 {Waiting,EligibleToActivate,Retire};
struct AccelerationHitObservation {
    u8 actor_slot=0,program_group=0,program_slot=0;
    bool carrier_active=false,retire1d8=false,observed_shape_binding=false;
    f32 lane0_position1cdc=0;
};
// Only the reviewed pre-activation/update guards. This does not turn
// progress into a collision receipt or assert a rendered hit window.
inline bool normal_acceleration_hit_gate(NormalAccelerationSource source,const AccelerationHitShapeRequest& binding,
                                         const AccelerationHitObservation& in,AccelerationHitGate* out){
    if(!out||!source.reviewed()||!binding.requested||binding.actor_slot!=source.slot||binding.credited_slot!=source.slot||
       in.actor_slot!=source.slot||!in.observed_shape_binding||binding.end!=-2||
       !acceleration_ninja_detail::clock(in.lane0_position1cdc,8192))return false;
    const bool stance=binding.shape_group==10&&binding.hit_row==9&&binding.program_group==4&&!binding.program_slot&&binding.start==4;
    const bool dash=binding.shape_group==11&&binding.hit_row==11&&!binding.program_group&&binding.program_slot==11&&binding.start==5;
    if(!stance&&!dash)return false;
    if(!in.carrier_active||in.retire1d8||in.program_group!=binding.program_group||in.program_slot!=binding.program_slot)
        *out=AccelerationHitGate::Retire;
    else *out=s16(in.lane0_position1cdc)>=binding.start?AccelerationHitGate::EligibleToActivate:AccelerationHitGate::Waiting;
    return true;
}
}
