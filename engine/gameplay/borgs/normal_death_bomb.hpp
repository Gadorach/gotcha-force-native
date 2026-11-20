#pragma once
#include "engine/gameplay/game_contact_terminal.hpp"

namespace gf {
struct NormalDeathBombSource {
    u16 model=0;
    u8 variant=0,slot=0;
    constexpr bool reviewed()const{return model==0x0d07&&!variant&&slot<6;}
};
struct DeathBombEntryPlan {
    u8 selector=0,upper_state=0,lower_state=0,program=0;
};
// Both selector lanes receive the entry code's request. The program number
// remains a source operand; it does not bind a missing motion bank.
inline bool normal_death_bomb_entry(NormalDeathBombSource source,u8 code742,DeathBombEntryPlan* out){
    if(!out||!source.reviewed())return false;
    *out=code742==2?DeathBombEntryPlan{6,0,7,13}:DeathBombEntryPlan{0,1,1,u8(code742==1?0:8)};
    return true;
}

struct DeathBombDropRequest {
    bool requested=false;
    u8 action_group=0,selector=0,class_state580=0,substate581=0;
};
// Reviewed neutral conditioned X only. It spends no stock and has no
// terminal request; whichever impact happens is assessed separately.
inline bool normal_death_bomb_drop_request(NormalDeathBombSource source,u32 commands5d4,u8 flags58e,
                                            bool reviewed_neutral_route,DeathBombDropRequest* out){
    if(!out||!source.reviewed()||!reviewed_neutral_route)return false;
    *out=(commands5d4&0x80u)&&!(flags58e&4u)?DeathBombDropRequest{true,2,45,0,0}:DeathBombDropRequest{};
    return true;
}

struct DeathBombDropMotion {
    f32 speed44=0,vertical_change50=0;
    u8 phase540=0;
    bool request_entry_presentation=false,turn_motion_due=false;
};
inline bool normal_death_bomb_drop_motion(NormalDeathBombSource source,u8 phase540,f32 speed44,
                                           bool direction_helper_found,DeathBombDropMotion* out){
    if(!out||!source.reviewed()||phase540>1||!std::isfinite(speed44)||std::abs(speed44)>1e5f)return false;
    *out={f32(0.9f*speed44),-0.2f,1,!phase540,direction_helper_found};return true;
}

struct DeathBombFallSteering {
    bool write_motion=false;
    f32 speed44=0,vertical48=0;
    s16 heading_history5ae=0;
};
// Exact ordinary pre-class steering tail, not a lower fall integrator.
// The caller supplies the completed direction/heading helper result.
inline bool normal_death_bomb_fall_steering(NormalDeathBombSource source,u8 lifecycle18,u8 phase19,u32 flags5e0,
                                            f32 speed44,f32 vertical48,s16 heading72,bool steering_helpers_completed,
                                            DeathBombFallSteering* out){
    if(!out||!source.reviewed()||!std::isfinite(speed44)||!std::isfinite(vertical48)||
       std::abs(speed44)>1e5f||std::abs(vertical48)>1e5f)return false;
    DeathBombFallSteering next;
    if(lifecycle18==1&&std::bit_cast<s8>(phase19)>1&&!(flags5e0&3u)){
        if(!steering_helpers_completed)return false;
        next={true,std::fma(0.9f,speed44,f32(0.1f*20.f)),vertical48<-2.f?-2.f:vertical48,heading72};
    }
    *out=next;return true;
}

struct DeathBombHistoryPlan {
    bool restore_history=false;
    std::array<f32,3> root{};
    f32 vertical48=0;
};
inline bool normal_death_bomb_early_history(NormalDeathBombSource source,u8 lifecycle18,u8 phase19,
                                            const std::array<f32,3>& history,bool observed_history,DeathBombHistoryPlan* out){
    if(!out||!source.reviewed())return false;
    if(lifecycle18==1&&std::bit_cast<s8>(phase19)>1){*out={};return true;}
    if(!observed_history)return false;
    for(f32 x:history)if(!std::isfinite(x)||std::abs(x)>1e7f)return false;
    *out={true,history,0};return true;
}

inline bool normal_death_bomb_game_impact(NormalDeathBombSource source,const GameContactImpactInput& in,GameContactImpactPlan* out){
    return source.reviewed()&&game_contact_terminal_detail::impact(game_contact_terminal_detail::Actor::DeathBomb,source.slot,in,out);
}
inline bool normal_death_bomb_consume_terminal(NormalDeathBombSource source,u8 lifecycle18,u8 pending663,
                                                u8 received1da,bool observed_active_slot_pass,GameContactDefeatPlan* out){
    return source.reviewed()&&game_contact_terminal_detail::consume_terminal(source.slot,lifecycle18,pending663,
                                                                            received1da,observed_active_slot_pass,out);
}
}
