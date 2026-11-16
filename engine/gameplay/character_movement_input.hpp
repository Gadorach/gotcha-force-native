#pragma once
#include "engine/assets/character_movement_input.hpp"
#include "engine/core/source_heading.hpp"
namespace gf {
struct CharacterMovementCommandState {u8 phase=0,remaining=0,pulse=0;bool operator==(const CharacterMovementCommandState&)const=default;};
struct CharacterMovementInputState {
    u32 previous_held=0,pressed5bc=0,released5c0=0,commands5d4=0;
    u16 directions5d8=0;u8 flick_flags=0,initial_window=0,release_window=0,sustain_window=0;
    std::array<CharacterMovementCommandState,10> commands{};
    bool operator==(const CharacterMovementInputState&)const=default;
};
struct CharacterMovementInputSample {s16 stick_x=0,stick_y=0;u32 held5b4=0,flags5e0=0;}; //source signed8 axes; positiveY up
inline bool character_movement_stick_nibble(const CharacterMovementInputProfile& p,s16 x,s16 y,u8 threshold,u8* out){
    if(!out||x< -128||x>127||y< -128||y>127||!threshold||threshold>127)return false;
    *out=0;
    if(s32(x)*x+s32(y)*y<s32(threshold)*threshold)return true;
    s16 angle=0;
    //8B73C loads Y into f1 and X into f2 before452A0. Reuse the shared
    //bounded host heading model; do not swap source X/Y or invent another atan.
    if(!source_heading_s16(f32(y),f32(x),&angle))return false;
    *out=p.octants[((u32(std::bit_cast<u16>(angle))+0x1000u)>>13)&7u];return true;
}
inline bool character_movement_input_state_valid(const CharacterMovementInputState& s){
    if((s.previous_held&~0x000f0100u)||(s.flick_flags&~11u)||s.initial_window>127||s.release_window>127||s.sustain_window>127)return false;
    for(const auto& c:s.commands)if((c.phase!=0&&c.phase!=2)||c.remaining>127||c.pulse>127)return false;
    return true;
}
//Movement-facing portion of685C0/68980/68B8C, not a complete actor input update.
//Weapon/charge inputs refuse.686F0 effects and6DE44/field749 are NOT fabricated.
//Callers must separately resolve those actor-owned boundaries before admission.
inline bool character_movement_input_step(const CharacterMovementInputProfile& p,const CharacterMovementInputState& input,
                                          const CharacterMovementInputSample& sample,CharacterMovementInputState* out,std::string* error){
    auto fail=[&](){if(error)*error="movement input: reviewed movement-only source fields/profile required";return false;};
    if(!out||!character_movement_input_profile_valid(p)||!character_movement_input_state_valid(input)||
       (sample.held5b4&~p.held_mask)||(sample.held5b4&~0x000f0100u)||sample.stick_x< -128||sample.stick_x>127||sample.stick_y< -128||sample.stick_y>127)return fail();
    auto s=input;s.pressed5bc=sample.held5b4&~input.previous_held;s.released5c0=input.previous_held&~sample.held5b4;s.commands5d4=0;
    u8 nibble=0;
    if(!(s.flick_flags&1u)){
        if(!character_movement_stick_nibble(p,sample.stick_x,sample.stick_y,p.low_threshold,&nibble))return fail();
        if(nibble){s.flick_flags|=1;s.directions5d8=u16((s.directions5d8&0xfff0u)|nibble);s.initial_window=(s.directions5d8&12u)?p.other_window:p.axis_window;}
    }
    if(!(s.flick_flags&2u)){
        if(!character_movement_stick_nibble(p,sample.stick_x,sample.stick_y,p.high_threshold,&nibble))return fail();
        if(nibble){s.flick_flags|=2;s.directions5d8=u16((s.directions5d8&0xff0fu)|((nibble&s.directions5d8&0xffu)<<4));s.initial_window=p.direction_window;}
    }
    const u32 held_directions=sample.held5b4&0xf0000u;
    if(!(s.flick_flags&8u)){
        if(held_directions){s.flick_flags|=8;s.directions5d8=u16((s.directions5d8&0xf0ffu)|((held_directions>>8)&(s.directions5d8>>4)));s.sustain_window=p.direction_window;}
    }else if(!held_directions){
        s.flick_flags=0;s.directions5d8=u16((s.directions5d8&0x0fffu)|((input.previous_held>>4)&0xf000u));s.release_window=p.release_window;
    }
    if(s.initial_window&&!--s.initial_window)s.directions5d8&=0xff00u;
    if(s.sustain_window&&!--s.sustain_window)s.directions5d8&=0xf0ffu;
    if(s.release_window&&!--s.release_window)s.directions5d8&=0x0fffu;
    constexpr std::array<u32,10> outputs={0x20,0x40,0x80,0x200,1,2,4,8,0x10,0x2000};
    for(unsigned i=0;i<s.commands.size();++i){
        auto& state=s.commands[i];const auto& command=p.commands[i];
        if(!state.phase){state.phase=2;state.remaining=127;}
        bool matched=false;
        if(state.remaining&&--state.remaining){
            const bool airborne=(sample.flags5e0&0x40u)!=0;
            const bool admitted=(!(command.flags&2u)||!airborne)&&(!(command.flags&4u)||airborne)&&(!(command.flags&8u)||held_directions);
            const u32 bits=command.flags&1u?sample.held5b4:s.pressed5bc,mask=command.masks[0];
            matched=admitted&&((command.flags&16u)?(bits&mask)!=0:(bits&mask)==mask);
        }else state.phase=0;
        if(matched){state.phase=0;state.pulse=u8(command.pulse);}
        if(state.pulse){--state.pulse;s.commands5d4|=outputs[i];}
    }
    s.previous_held=sample.held5b4;
    if(!character_movement_input_state_valid(s))return fail();
    *out=s;if(error)error->clear();return true;
}
}
