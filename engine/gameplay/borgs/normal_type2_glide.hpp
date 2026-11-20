#pragma once
#include "engine/gameplay/character_air_resource.hpp"
#include "engine/gameplay/character_movement_input.hpp"

namespace gf {
// Only the reviewed normal Wing Soldier and Quick Valkrie paths. This
// source identity is not a package ID, asset qualification or actor receipt.
struct NormalType2GlideSource {
    u16 model=0;
    u8 variant=0;
    constexpr bool reviewed()const{return !variant&&(model==0x0a00||model==0x0b00);}
    static constexpr AirMeterRules meter{AirMeterRecoveryGate::Behavior80Clear,10000,0,2000,{150,50,450,80}};
    static constexpr u8 fall=6,landing=7,glide=18,steer=19,directional_dash=22;
};

struct Type2GlideRequest {
    u8 selector=0,direction58f=0;
    u16 remaining_directions5d8=0;
    bool consume_direction=false;
};

// Suffixes of the fall and near-floor glide selector routines only. Earlier
// class/relaunch/overrides must decline; later floor tails are separate.
inline bool normal_type2_glide_request(NormalType2GlideSource source,u8 selector,u32 flags5e0,bool earlier_declined,
                                     const CharacterMovementInputState& in,Type2GlideRequest* out){
    using S=NormalType2GlideSource;
    if(!out||!source.reviewed()||!earlier_declined||(selector!=S::fall&&selector!=S::glide)||
       !character_movement_input_state_valid(in)||(in.commands5d4&~3u))return false;
    Type2GlideRequest next{selector,0,in.directions5d8,false};
    const bool held=(in.previous_held&0x100u)!=0;
    if(selector==S::glide&&!held)next.selector=S::fall; // release leaf precedes the direction leaves
    else if(!(flags5e0&2u)){
        if(in.directions5d8&0xf0u){
            next.selector=S::directional_dash;
            next.direction58f=u8((in.directions5d8>>4)&15u);
            next.remaining_directions5d8&=0xff0fu;
            next.consume_direction=true;
        }else if(in.previous_held&0xf0000u)next.selector=S::steer;
        else if(selector==S::fall&&held)next.selector=S::glide;
    }
    *out=next;return true;
}

struct Type2GlideFrameScalars {
    f32 offset6d0=0;
    u8 cached87=0;
};
inline bool normal_type2_glide_frame_scalars(NormalType2GlideSource source,u32 flags5e0,Type2GlideFrameScalars* out){
    if(!out||!source.reviewed())return false;
    *out={0.f,u8((flags5e0>>6)&1u)};
    return true;
}

// The source refill pass happens before the selector and floor response.
// Inhibit 0x80 is not assumed to mean the same action as Blue Striker boost.
inline bool normal_type2_glide_refill(NormalType2GlideSource source,const CharacterAirResourceState& in,
                                    u32 prior_flags5e0,CharacterAirResourceState* out){
    const auto& meter=NormalType2GlideSource::meter;
    return source.reviewed()&&in.value736>=0&&in.value736<=meter.limit&&in.delay73a>=0&&
        character_air_resource_recover(in,meter,prior_flags5e0,0,out);
}

enum class Type2GlideContinuation:u8 {Unresolved,None,Landing};
enum class Type2GlideFloorTail:u8 {Unresolved,PlainReturn,QualifiedStaticSnap};
struct Type2GlideFloorInput {
    f32 root_y=0,floor_y=0,offset6d0=0;
    u32 flags5e0=0;
    u8 selector=0;
    Type2GlideContinuation continuation=Type2GlideContinuation::Unresolved;
    Type2GlideFloorTail tail=Type2GlideFloorTail::Unresolved;
};
struct Type2GlideFloorPlan {
    f32 root_y=0;
    u32 flags5e0=0;
    u8 selector=0;
    Type2GlideContinuation continuation=Type2GlideContinuation::Unresolved;
    bool snapped=false,clear_clock69c=false,clear_motion_and_turn=false;
    bool operator==(const Type2GlideFloorPlan&)const=default;
};

// Given an already qualified static snap or ordinary no-response return, apply
// only the snap/callback and 18/19 caller tails. No floor, normal, gap, overlap,
// attachment or live ownership is established by these scalar observations.
inline bool normal_type2_glide_floor_tail(NormalType2GlideSource source,const Type2GlideFloorInput& in,Type2GlideFloorPlan* out){
    using S=NormalType2GlideSource;
    using C=Type2GlideContinuation;
    using T=Type2GlideFloorTail;
    if(!out||!source.reviewed()||!std::isfinite(in.root_y)||f32_to_bits(in.offset6d0)!=0||
       (in.tail!=T::PlainReturn&&in.tail!=T::QualifiedStaticSnap)||
       !((in.selector==S::glide&&in.continuation==C::Landing)||(in.selector==S::steer&&in.continuation==C::None)))return false;
    Type2GlideFloorPlan next{in.root_y,in.flags5e0|0x840u,in.selector,in.continuation};
    if(in.tail==T::QualifiedStaticSnap){
        if(!std::isfinite(in.floor_y)||in.root_y>in.floor_y)return false;
        next.root_y=in.floor_y;
        next.snapped=true;
        if(in.continuation==C::Landing){
            next.selector=S::landing;
            next.continuation=C::None; // actual 6A750 clears 754
            next.clear_clock69c=true;
            next.clear_motion_and_turn=true; // glide caller zeros 44/48/4C/50 and 7C/7E/80 on return 1
        }
    }
    *out=next;return true;
}

struct Type2GlideLandingPlan {
    u32 flags5e0=0;
    u8 cached87=0;
};
// Flag/cache prefix of a separately accepted ordinary phase-zero landing.
// Landing does not clear 0x80 or invoke another resource update.
inline bool normal_type2_glide_landing_flags(NormalType2GlideSource source,u8 selector,u8 phase,u32 flags5e0,
                                           Type2GlideLandingPlan* out){
    if(!out||!source.reviewed()||selector!=NormalType2GlideSource::landing||phase)return false;
    *out={(flags5e0&~0x2040u)|0x200u,0};return true;
}
}
