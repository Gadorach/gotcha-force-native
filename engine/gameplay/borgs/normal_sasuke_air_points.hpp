#pragma once
#include "engine/gameplay/character_air_resource.hpp"
#include "engine/gameplay/character_movement_input.hpp"

namespace gf {
struct NormalSasukeAirPoints {
    static constexpr u16 model=0x000a;
    static constexpr u8 source_variant=0;
    static constexpr AirPointRules rules{3,true};
};

struct NormalSasukeRelaunchFields {
    u8 selector=0,air_ready588=0;
    u32 flags5e0=0;
    bool prior_handlers_declined=false;
};

// Only the count relaunch request leaves on an explicitly supplied normal
// Sasuke path. An unchanged selector makes no request. The input conditioner
// owns the air-jump pulse; a raw held button is not a substitute for it.
// This is not actor admission, full-frame arbitration or an entry receipt.
inline bool normal_sasuke_air_relaunch_request(const CharacterMovementInputState& in,
                                              const NormalSasukeRelaunchFields& fields,u8 used,u8* out){
    if(!out||!fields.prior_handlers_declined||!character_movement_input_state_valid(in)||(in.commands5d4&~3u)||
       !NormalSasukeAirPoints::rules.accepts(used))return false;
    switch(fields.selector){
    case 6:case 17:case 16:break;
    default:return false;
    }
    *out=fields.selector;
    if(!(fields.flags5e0&2u)&&(in.commands5d4&2u)&&
       (fields.selector==16||fields.air_ready588)&&NormalSasukeAirPoints::rules.has_point(used))*out=8;
    return true;
}
}
