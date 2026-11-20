#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include "engine/core/types.hpp"

namespace gf {
using SwitchingNinjaPosition=std::array<f32,3>;
namespace switching_ninja_detail {
inline bool valid(f32 x){return std::isfinite(x)&&std::abs(x)<=1e7f;}
inline bool valid(const SwitchingNinjaPosition& v){return valid(v[0])&&valid(v[1])&&valid(v[2]);}
}

struct SwitchingNinjaContactOwner {
    u8 slot=0,kind83=0,flags1d9=0,flags1da=0;
    u16 receiver_mask59c=0;
    SwitchingNinjaPosition position{};
    f32 height_reference6d0=0;
};
struct SwitchingNinjaContact {
    u16 caster_model=0;
    u8 caster_variant=0,carrier_type=0,hit_row=0;
    bool observed_resolved_contact=false;
    SwitchingNinjaContactOwner caster,recipient;
};
struct SwitchingNinjaLeg {
    bool queued=false;
    u8 slot=0;
    SwitchingNinjaPosition destination{};
    bool operator==(const SwitchingNinjaLeg&)const=default;
};
struct SwitchingNinjaStagedExchange {
    SwitchingNinjaLeg caster,recipient;
    u8 caster_flags1d9=0,recipient_flags1da=0;
    bool operator==(const SwitchingNinjaStagedExchange&)const=default;
};

// Qualified contact tail only. Stages two slot destinations; neither actor
// changes position here. No lock-on target, damage or collision discovery.
inline bool normal_switching_ninja_stage_exchange(const SwitchingNinjaContact& in,SwitchingNinjaStagedExchange* out){
    using switching_ninja_detail::valid;
    const auto& c=in.caster;const auto& t=in.recipient;
    if(!out||!in.observed_resolved_contact||in.caster_model!=9||in.caster_variant||
       in.carrier_type!=20||in.hit_row!=12||c.slot>=6||t.slot>=6||c.slot==t.slot||t.kind83||
       !valid(c.position)||!valid(t.position)||!valid(c.height_reference6d0)||!valid(t.height_reference6d0))return false;
    SwitchingNinjaStagedExchange next{{},{},c.flags1d9,t.flags1da};
    if(!c.kind83&&!(t.receiver_mask59c&0x10u)&&!(c.flags1d9&4u)&&!(t.flags1da&4u)){
        const f32 to_caster=f32(t.height_reference6d0-c.height_reference6d0);
        const f32 to_recipient=f32(c.height_reference6d0-t.height_reference6d0);
        next.caster={true,c.slot,{t.position[0],f32(t.position[1]+to_caster),t.position[2]}};
        next.recipient={true,t.slot,{c.position[0],f32(c.position[1]+to_recipient),c.position[2]}};
        if(!valid(next.caster.destination)||!valid(next.recipient.destination))return false;
        next.caster_flags1d9|=4;next.recipient_flags1da|=4;
    }
    *out=next;return true;
}

struct SwitchingNinjaTransferOwner {
    u8 slot=0,existing_mode7c9=0;
    s16 hp_after_hit=0;
    u32 flags5e0=0;
    bool observed_current_slot=false,post_hit_route_selected=false;
};
struct SwitchingNinjaTransferState {
    u8 slot=0,mode7c9=0,phase7ca=0,clock7cb=0;
    u32 flags5e0=0;
    u16 collision_mask272=0;
    SwitchingNinjaPosition queued{};
    bool operator==(const SwitchingNinjaTransferState&)const=default;
};
struct SwitchingNinjaTransferArm {
    bool request_selector35=false;
    SwitchingNinjaTransferState state;
};

// Per-actor 6AB04 prefix after the appropriate post-hit route. A busy/dead
// actor emits no pulse; there is deliberately no partner acceptance input.
inline bool normal_switching_ninja_arm_leg(const SwitchingNinjaLeg& leg,const SwitchingNinjaTransferOwner& owner,
                                          SwitchingNinjaTransferArm* out){
    if(!out||!owner.observed_current_slot||!leg.queued||leg.slot!=owner.slot||owner.slot>=6||
       owner.hp_after_hit<0||!switching_ninja_detail::valid(leg.destination))return false;
    SwitchingNinjaTransferArm next;
    if(owner.post_hit_route_selected&&owner.hp_after_hit&&!owner.existing_mode7c9)
        next={true,{owner.slot,2,0,0,owner.flags5e0|0x1000u,0xffffu,leg.destination}};
    *out=next;return true;
}

struct SwitchingNinjaTransferStep {
    SwitchingNinjaTransferState state;
    bool place_this_slot=false,finish_this_slot=false;
};
// Lower-43 mode-2 counter only. Placement and finish consumers remain
// separate; neither callback consults the other slot's progress.
inline bool normal_switching_ninja_transfer_step(const SwitchingNinjaTransferState& in,SwitchingNinjaTransferStep* out){
    if(!out||in.slot>=6||in.mode7c9!=2||in.phase7ca>2||!(in.flags5e0&0x1000u)||
       !switching_ninja_detail::valid(in.queued)||
       (in.phase7ca==0?in.clock7cb!=0:in.clock7cb<1||in.clock7cb>2))return false;
    SwitchingNinjaTransferStep next{in};
    if(!in.phase7ca){next.state.phase7ca=1;next.state.clock7cb=2;next.state.collision_mask272=0xffffu;}
    else if(!--next.state.clock7cb){
        ++next.state.phase7ca;
        if(in.phase7ca==1){next.state.clock7cb=2;next.place_this_slot=true;}
        else{
            next.state.mode7c9=0;next.state.flags5e0&=~0x1000u;next.state.collision_mask272=0;
            next.finish_this_slot=true;
        }
    }
    *out=next;return true;
}

struct SwitchingNinjaStageBounds {f32 min_x=0,max_x=0,min_z=0,max_z=0;bool observed=false;};
struct SwitchingNinjaPlacement {
    u8 slot=0;
    SwitchingNinjaPosition queued7bc{},position{},previous2c{},older5e8{};
    bool terrain_query_due=false;
};
// Bounded 3F7D4 mode-2 prefix before terrain query. A clamped root isn't a
// swept path, clearance proof, final contact response, or paired commit.
inline bool normal_switching_ninja_place_leg(const SwitchingNinjaTransferStep& step,const SwitchingNinjaStageBounds& bounds,
                                             SwitchingNinjaPlacement* out){
    using switching_ninja_detail::valid;
    const auto& s=step.state;
    if(!out||!bounds.observed||!step.place_this_slot||step.finish_this_slot||s.slot>=6||s.mode7c9!=2||
       s.phase7ca!=2||s.clock7cb!=2||!valid(s.queued)||!valid(bounds.min_x)||!valid(bounds.max_x)||
       !valid(bounds.min_z)||!valid(bounds.max_z)||bounds.min_x>bounds.max_x||bounds.min_z>bounds.max_z)return false;
    const SwitchingNinjaPosition p{std::clamp(s.queued[0],bounds.min_x,bounds.max_x),s.queued[1],
        std::clamp(s.queued[2],bounds.min_z,bounds.max_z)};
    *out={s.slot,p,p,p,p,true};return true;
}
}
