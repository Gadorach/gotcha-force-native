#pragma once
#include "engine/assets/battle_contact_tail.hpp"
#include "engine/gameplay/collision_contact_reset.hpp"
namespace gf {
struct BattleContactTailActorInput {
    CollisionContactState contact;std::array<f32,3> position{};s16 health=0;u8 forced663=0,peer_mask71e=0,peer_mask71f=0,disabled4a1=0;
};
//Only the source non-response initial gates. Ground/attachment/contact effects
//must be implemented when reached, never replaced by a guessed collision miss.
inline bool battle_contact_tail_actor_gates(const BattleContactTailProfile& profile,const BattleContactTailActorInput& in,const std::array<f32,4>& bounds,std::string* error){
    auto fail=[&](const char* why){if(error)*error=std::string("initial contact tail actor: ")+why;return false;};
    if(!profile.ready()||!source_intersection_detail::valid(in.position)||!source_intersection_detail::valid(in.contact.retained.displacement1cc))return fail("source profile and bounded current actor fields required");
    for(f32 value:bounds)if(!std::isfinite(value)||std::abs(value)>1e7f)return fail("stage bounds numeric domain");
    if(bounds[1]>=bounds[0]||bounds[3]>=bounds[2])return fail("stage bounds ordering");
    if(in.contact.moved1dd||in.contact.attached1db||in.contact.current274||in.contact.field1da||in.contact.field1d9||in.contact.retained.timer1e8||in.contact.retained.displacement1cc!=std::array<f32,3>{})return fail("ground/contact/attachment response continuation required");
    if(in.health<1||in.forced663||in.peer_mask71e||in.peer_mask71f)return fail("damage/peer/recovery response continuation required");
    if(!in.disabled4a1&&(in.position[0]<f32(bounds[1]+20)||in.position[0]>f32(bounds[0]-20)||in.position[2]<f32(bounds[3]+20)||in.position[2]>f32(bounds[2]-20)))return fail("bound/clamp/effect response continuation required");
    if(error){error->clear();}return true;
}
inline bool battle_contact_static_pair_gate(u16 descriptor_flags,u8 freeze281,u8 kind83,bool* skip){
    if(!skip)return false;
    *skip=(descriptor_flags&0x800)||freeze281||kind83==0x17;return true;
}
}
