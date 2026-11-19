#pragma once
#include "engine/assets/character_movement.hpp"
#include "engine/assets/character_movement_input.hpp"
#include "engine/core/source_heading.hpp"
namespace gf {
enum class CharacterMovementHeadingMode {Held,CardinalDash,OctantDash};
struct CharacterMovementHeadingFrame {
    s16 body72=0,camera5b0=0,player_heading=0;bool use_player_reference=false,player_reference_declared=false;
};
struct CharacterMovementHeading {
    s16 relative734=0,desired5ac=0,delta5aa=0;bool has_direction=false;
};
//66AC0's direction selection does NOT write body72. Body turn and travel
//heading remain separately owned; changing the camera cannot rotate idle body.
inline bool character_movement_heading(const CharacterMovementInputProfile& input,const CharacterMovementProfile& motion,
                                      u8 nibble,CharacterMovementHeadingMode mode,const CharacterMovementHeadingFrame& frame,
                                      CharacterMovementHeading* out,std::string* error){
    auto fail=[&](){if(error)*error="movement heading: same normal G-Red source profiles/reference required";return false;};
    if(!out||!character_movement_input_profile_valid(input)||!character_movement_profile_valid(motion)||input.model!=0x615||motion.model!=0x615||
       input.model_hash!=motion.model_hash||nibble>=16||unsigned(mode)>2)return fail();
    CharacterMovementHeading result;const int index=input.nibble_indices[nibble];
    if(index<1){result.relative734=source_heading_wrap(s32(frame.body72)-frame.camera5b0+0x8000);}
    else{
        if(mode==CharacterMovementHeadingMode::Held)result.relative734=source_heading_wrap((index-1)*0x2000);
        else result.relative734=source_heading_wrap(s32((mode==CharacterMovementHeadingMode::CardinalDash?motion.cardinal_headings:motion.octant_headings)[unsigned(index-1)])<<8);
        if(mode==CharacterMovementHeadingMode::Held&&frame.use_player_reference){
            if(!frame.player_reference_declared)return fail();
            result.desired5ac=source_heading_wrap(s32(result.relative734)+frame.player_heading);
        }else result.desired5ac=source_heading_wrap(s32(frame.camera5b0)+result.relative734-0x8000);
        result.delta5aa=source_heading_wrap(s32(result.desired5ac)-frame.body72);result.has_direction=true;
    }
    *out=result;if(error)error->clear();return true;
}
struct CharacterMovementTurn {s16 body72=0,delta5aa=0,rate7e=0;};
//66E40 shared source turn operation; gain0 is ground walk, gain1 ascent.
inline bool character_movement_turn_steer(const CharacterMovementProfile& p,const CharacterMovementTurn& input,s16 delta5aa,
                                         f32 rate664,f32 delta1dc8,unsigned gain,CharacterMovementTurn* out){
    if(!out||!character_movement_profile_valid(p)||gain>=p.turn_gains.size()||
       !std::isfinite(rate664)||rate664<0||rate664>16||!std::isfinite(delta1dc8)||delta1dc8<0||delta1dc8>16)return false;
    const f32 step=f32(f32(f32(rate664*delta1dc8)*f32(delta5aa))*p.turn_gains[gain]);
    if(!std::isfinite(step)||std::abs(step)>1e7f)return false;
    auto result=input;result.rate7e=source_heading_wrap(s32(step));result.body72=source_heading_wrap(s32(result.body72)+result.rate7e);
    result.delta5aa=source_heading_wrap(s32(delta5aa)-result.rate7e);*out=result;return true;
}
inline bool character_movement_turn_begin(const CharacterMovementTurn& input,s32 duration,CharacterMovementTurn* out){
    if(!out||duration<0||duration>32767)return false;
    auto result=input;if(duration)result.rate7e=source_heading_wrap(s32(result.delta5aa)/duration);*out=result;return true;
}
//66F1C preserves signed16 stores and its asymmetric zero-step completion test.
inline bool character_movement_turn_step(const CharacterMovementTurn& input,f32 delta,CharacterMovementTurn* out,bool* complete){
    if(!out||!complete||!std::isfinite(delta)||delta<0||delta>16)return false;
    auto result=input;bool done=false;
    if(!result.rate7e){result.delta5aa=0;done=true;}
    else{
        const s16 step=source_heading_wrap(s32(f32(f32(result.rate7e)*delta)));
        result.body72=source_heading_wrap(s32(result.body72)+step);result.delta5aa=source_heading_wrap(s32(result.delta5aa)-step);
        if((step<1&&result.delta5aa>=0)||(step>=1&&result.delta5aa<1)){result.body72=source_heading_wrap(s32(result.body72)+result.delta5aa);result.rate7e=result.delta5aa=0;done=true;}
    }
    *out=result;*complete=done;return true;
}
}
