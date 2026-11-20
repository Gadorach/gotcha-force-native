#pragma once
#include "engine/gameplay/borgs/normal_gred_beam_custody.hpp"

namespace gf {
enum class GRedBeamWorldRoute:u8 {SceneHeld,SlotInactive,GroupMasked,CarrierInactive,Dispatch};
struct GRedBeamWorldVisitInput {
    bool source_pass_declared=false;
    u8 scene_hold66=0,scene_mask_d1=0,actor_active00=0,carrier_active00=0;
    std::array<u32,3> position20_bits{};
    std::array<u16,3> angle70_bits{};
};
struct GRedBeamWorldVisitPlan {
    GRedBeamWorldRoute route=GRedBeamWorldRoute::SceneHeld;
    u8 credit_slot=255,carrier_group=255;
    // This only visits the bucket, not an individual collider or registry entry.
    bool visit_shape_group=false;
    std::array<u32,3> previous2c_bits{};
    std::array<u16,3> previous76_bits{};
};
// Ordinary 29434 -> 89A94(8) -> 8997C: a declaration of this part of
// the object update, not proof of a scene, allocation, or callback.
// Registry traversal 89AEC uses actor activity, not the object D1 mask.
inline bool normal_gred_beam_world_visit(NormalGRedNeutralBSource source,const GRedBeamFactoryPlan& beam,
    const GRedBeamWorldVisitInput& in,GRedBeamWorldVisitPlan* out){
    if(!out||!source.reviewed()||!in.source_pass_declared||!beam.propose_header||beam.active00!=1||
       beam.call!=gred_beam_custody_detail::beam(source.slot)||beam.type10!=2||beam.argument11!=43||
       beam.parent_slot!=source.slot||beam.credit_slot!=source.slot||beam.copied_model94!=source.model||
       beam.update0c!=0x800c3cdc||beam.draw10c!=0x800c4468)return false;
    GRedBeamWorldVisitPlan next;
    next.credit_slot=source.slot;next.carrier_group=beam.call.resolved_group;
    if(in.scene_hold66)next.route=GRedBeamWorldRoute::SceneHeld;
    else if(!in.actor_active00)next.route=GRedBeamWorldRoute::SlotInactive;
    else{
        next.visit_shape_group=true;
        if(in.scene_mask_d1&(1u<<source.slot))next.route=GRedBeamWorldRoute::GroupMasked;
        else if(!in.carrier_active00)next.route=GRedBeamWorldRoute::CarrierInactive;
        else{
            next.route=GRedBeamWorldRoute::Dispatch;
            next.previous2c_bits=in.position20_bits;next.previous76_bits=in.angle70_bits;
        }
    }
    *out=next;return true;
}
// 89A54 reads 86 AFTER C3CDC returns. No callback replay or claim that
// activation is once for the lifetime of the carrier.
inline bool normal_gred_beam_world_clears_activation(const GRedBeamWorldVisitPlan& visit,u8 post_callback86){
    return visit.route==GRedBeamWorldRoute::Dispatch&&post_callback86!=0;
}
}
