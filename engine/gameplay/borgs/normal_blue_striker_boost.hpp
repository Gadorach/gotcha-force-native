#pragma once
#include "engine/gameplay/type3_cruise_boost.hpp"

namespace gf {
struct NormalBlueStrikerBoost:Type3CruiseBoostRules {
    static constexpr u16 model=0x0d00;
    static constexpr u8 source_variant=0;
};
using BlueStrikerBoostRequestFields=Type3BoostRequestFields;
using BlueStrikerBoostDebit=Type3BoostDebit;
using BlueStrikerCruiseEntry=Type3CruiseEntry;
using BlueStrikerExitClockStep=Type3ExitClockStep;

inline bool normal_blue_striker_preselect_refill(const CharacterAirResourceState& in,u32 prior_flags5e0,
                                                 CharacterAirResourceState* out){
    return type3_preselect_refill(in,prior_flags5e0,out);
}
inline bool normal_blue_striker_boost_request(const CharacterMovementInputState& in,const BlueStrikerBoostRequestFields& fields,
                                               const CharacterAirResourceState& after_refill,u8* out){
    return type3_boost_request(in,fields,after_refill,out);
}
inline bool normal_blue_striker_boost_debit(const CharacterAirResourceState& in,u32 flags5e0,f32 rate5f4,BlueStrikerBoostDebit* out){
    return type3_boost_debit(in,flags5e0,rate5f4,out);
}
constexpr BlueStrikerCruiseEntry normal_blue_striker_cruise_entry(u32 prior_flags5e0){return type3_cruise_entry(prior_flags5e0);}
inline bool normal_blue_striker_exit_clock(f32 before568,f32 rate5f4,BlueStrikerExitClockStep* out){
    return type3_exit_clock(before568,rate5f4,out);
}
}
