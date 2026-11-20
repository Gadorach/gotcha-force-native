#pragma once
#include "engine/gameplay/game_contact_terminal.hpp"
#include "engine/gameplay/type3_cruise_boost.hpp"

namespace gf {
struct NormalDeathIcbmSource {
    u16 model=0;
    u8 variant=0,slot=0;
    constexpr bool reviewed()const{return model==0x0d06&&!variant&&slot<6;}
};
inline bool normal_death_icbm_preselect_refill(NormalDeathIcbmSource source,const CharacterAirResourceState& in,
                                              u32 prior_flags5e0,CharacterAirResourceState* out){
    return source.reviewed()&&type3_preselect_refill(in,prior_flags5e0,out);
}
inline bool normal_death_icbm_boost_request(NormalDeathIcbmSource source,const CharacterMovementInputState& input,
                                           const Type3BoostRequestFields& fields,const CharacterAirResourceState& after_refill,u8* out){
    return source.reviewed()&&type3_boost_request(input,fields,after_refill,out);
}
inline bool normal_death_icbm_boost_debit(NormalDeathIcbmSource source,const CharacterAirResourceState& in,
                                        u32 flags5e0,f32 rate5f4,Type3BoostDebit* out){
    return source.reviewed()&&type3_boost_debit(in,flags5e0,rate5f4,out);
}
using IcbmImpactProbeResult=GameImpactProbeResult;
using IcbmImpactQuery=GameImpactQuery;
using IcbmGameImpactInput=GameContactImpactInput;
using IcbmGameImpactPlan=GameContactImpactPlan;
using IcbmGameDefeatPlan=GameContactDefeatPlan;
inline bool normal_death_icbm_game_impact(NormalDeathIcbmSource source,const IcbmGameImpactInput& in,IcbmGameImpactPlan* out){
    return source.reviewed()&&game_contact_terminal_detail::impact(game_contact_terminal_detail::Actor::DeathIcbm,source.slot,in,out);
}
inline bool normal_death_icbm_consume_terminal(NormalDeathIcbmSource source,u8 lifecycle18,u8 pending663,
                                               u8 received1da,bool observed_active_slot_pass,IcbmGameDefeatPlan* out){
    return source.reviewed()&&game_contact_terminal_detail::consume_terminal(source.slot,lifecycle18,pending663,
                                                                            received1da,observed_active_slot_pass,out);
}
}
