#pragma once
#include <array>
#include "engine/input/input_map.hpp"
namespace gf {
struct ChallengeSetupPad {u32 source_buttons=0;bool connected=false;};
struct ChallengeSetupButtons {u16 held=0,previous=0,repeated=0,pressed=0;std::array<u8,16> counters{};};
//8016E694 consumes the source SDK word, NOT MAIN's repeat word. High nibble
//aliases represent already-produced source direction fields, not new stick
//thresholds. Original analogue production/calibration remains a separate seam.
inline u16 challenge_setup_fold_buttons(u32 word){
    return u16((word&0xf7fu)|((word>>16)&15u)|((word>>8)&0xf000u)|((word>>5)&0x80u));
}
inline void challenge_setup_input_step(const ChallengeSetupPad& pad,ChallengeSetupButtons* state){
    if(!state)return;
    state->previous=state->held;state->held=pad.connected?challenge_setup_fold_buttons(pad.source_buttons):0;
    state->pressed=u16(state->held&u16(~state->previous));state->repeated=state->pressed;
    for(unsigned bit=0;bit<16;++bit){const u16 mask=u16(1u<<bit);
        state->counters[bit]=(state->held&mask)?u8(unsigned(state->counters[bit])+1):0;
        if(state->counters[bit]>=32){state->repeated|=mask;state->counters[bit]=27;}
    }
}
//Existing native GamePadState digital policy only. No analogue threshold,
//device polling, remapping or alternative input backend is introduced here.
inline ChallengeSetupPad challenge_setup_digital_pad(const GamePadState& pad,bool connected){
    return {(pad.dpad_left?1u:0u)|(pad.dpad_right?2u:0u)|(pad.dpad_down?4u:0u)|(pad.dpad_up?8u:0u)|
            (pad.z?0x10u:0u)|(pad.r?0x20u:0u)|(pad.l?0x40u:0u)|(pad.a?0x100u:0u)|
            (pad.b?0x200u:0u)|(pad.x?0x400u:0u)|(pad.y?0x800u:0u)|(pad.start?0x1000u:0u),connected};
}
} // namespace gf
