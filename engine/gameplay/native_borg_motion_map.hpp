#pragma once
#include "engine/gameplay/native_battle_mvp.hpp"
namespace gf {
struct BattleMvpMotionChoice {u8 group=0,choice=0;bool operator==(const BattleMvpMotionChoice&)const=default;};
enum class NativeBorgMotion:u8 {
    Idle,Walk,WeaponNeutral,LegacyJump,DashForward,DashBackward,DashLeft,DashRight,
    Takeoff,AirHold,Apex,Fall,Land,AirDash,LabAirWindup,LabAirRecovery,LabGlideEntry,LabGlide,LabCruise,LabBoost,Count
};
struct NativeBorgMotionMap {
    std::array<BattleMvpMotionChoice,unsigned(NativeBorgMotion::Count)> clips{};
    bool supported=false,user_classified=false;
};
inline NativeBorgMotionMap native_borg_motion_map(u16 model){
    NativeBorgMotionMap map;if(model!=0x615&&model!=10)return map;
    map.supported=true;map.clips[unsigned(NativeBorgMotion::Walk)]={0,1};map.clips[unsigned(NativeBorgMotion::LegacyJump)]={3,0};
    if(model==0x615){
        //User-classified G-Red raw IDs. No universal map or Sasuke inference.
        //Unclassified14 (possible crouch/fall) and all melee entries stay unused.
        map.user_classified=true;
        map.clips[unsigned(NativeBorgMotion::LegacyJump)]={0,7}; //unused by the classified phase selector
        map.clips[unsigned(NativeBorgMotion::DashForward)]={0,2};
        map.clips[unsigned(NativeBorgMotion::DashBackward)]={0,3};
        map.clips[unsigned(NativeBorgMotion::DashLeft)]={0,4};
        map.clips[unsigned(NativeBorgMotion::DashRight)]={0,5};
        map.clips[unsigned(NativeBorgMotion::Takeoff)]={0,6};
        map.clips[unsigned(NativeBorgMotion::AirHold)]={0,7};
        map.clips[unsigned(NativeBorgMotion::Apex)]={0,8};
        map.clips[unsigned(NativeBorgMotion::Fall)]={0,24};
        map.clips[unsigned(NativeBorgMotion::Land)]={0,9};
        map.clips[unsigned(NativeBorgMotion::AirDash)]={0,23};
    }
    return map;
}
inline NativeBorgMotion native_borg_ground_dash_motion(BattleMvpDashDirection direction){
    return direction==BattleMvpDashDirection::Backward?NativeBorgMotion::DashBackward:
        direction==BattleMvpDashDirection::Left?NativeBorgMotion::DashLeft:
        direction==BattleMvpDashDirection::Right?NativeBorgMotion::DashRight:NativeBorgMotion::DashForward;
}
inline NativeBorgMotion native_borg_select_motion(const BattleMvpActor& actor,const NativeBorgMotionMap& map){
    if(map.user_classified){
        if(actor.dash_ticks)return actor.dash_airborne?NativeBorgMotion::AirDash:native_borg_ground_dash_motion(actor.dash_motion);
        if(!actor.grounded)return actor.takeoff_ticks?NativeBorgMotion::Takeoff:actor.vertical_speed>2?NativeBorgMotion::AirHold:actor.vertical_speed>=-2?NativeBorgMotion::Apex:NativeBorgMotion::Fall;
        if(actor.landing_ticks)return NativeBorgMotion::Land;
    }else{if(!actor.grounded)return NativeBorgMotion::LegacyJump;if(actor.landing_ticks)return NativeBorgMotion::Idle;}
    return actor.movement_speed>.05f?NativeBorgMotion::Walk:NativeBorgMotion::Idle;
}
inline bool native_borg_motion_loops(NativeBorgMotion motion){
    return motion==NativeBorgMotion::Idle||motion==NativeBorgMotion::Walk||motion==NativeBorgMotion::AirHold||motion==NativeBorgMotion::Fall||motion==NativeBorgMotion::LegacyJump||motion==NativeBorgMotion::LabGlide;
}
//Short classified phases must reach their last pose inside the gameplay window.
//These timings/blends are native presentation policy, not original frame rates.
inline f64 native_borg_phase_time(unsigned remaining,unsigned duration,f32 end_frame){
    const unsigned elapsed=duration-std::min(remaining,duration);
    return std::min(1.0,f64(elapsed)/std::max(1u,duration-1))*end_frame;
}
inline f32 native_borg_motion_blend_ticks(NativeBorgMotion motion,bool classified){
    return classified&&(motion==NativeBorgMotion::Takeoff||motion==NativeBorgMotion::Apex||motion==NativeBorgMotion::Land)?2.f:6.f;
}
}
