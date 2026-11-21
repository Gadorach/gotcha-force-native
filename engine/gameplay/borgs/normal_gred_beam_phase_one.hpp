#pragma once
#include "engine/gameplay/borgs/normal_gred_beam_flight.hpp"
#include "engine/gameplay/stage_projectile_segment.hpp"

namespace gf {
struct GRedBeamSlotSample {
    bool enabled60=false,present68=false;
    std::array<f32,3> position20{};
};
struct GRedBeamSlotSnapshot {
    std::array<GRedBeamSlotSample,6> slots{};
    u8 selection_eb=0;
    bool declared=false;
};
struct GRedBeamProximityPlan {
    std::array<f32,3> position20{};
    f32 weight29c=0;
    u8 examined_mask=0,stop_slot=255;
};
// 83714: six declared type-7 scene object slots, not actors or colliders. A nonzero
// scene mask filters slots. After the first far slot the same weight is
// used for every later contribution; all directions use the old position.
// A later near stop keeps that weight write but discards the nudge sum.
inline bool normal_gred_beam_proximity_plan(NormalGRedNeutralBSource source,const GRedBeamSlotSnapshot& snapshot,
                                            const std::array<f32,3>& position,f32 weight,GRedBeamProximityPlan* out){
    using namespace source_intersection_detail;
    using gred_beam_flight_detail::advance;
    if(!out||!source.reviewed()||!snapshot.declared||!valid(position)||!std::isfinite(weight)||weight<0||weight>100)return false;
    GRedBeamProximityPlan next;next.position20=position;next.weight29c=weight;
    Vec sum{};bool nudged=false;
    for(unsigned i=0;i<snapshot.slots.size();++i){
        const auto& slot=snapshot.slots[i];const u8 bit=u8(1u<<i);
        if(!slot.enabled60||(snapshot.selection_eb&&!(snapshot.selection_eb&bit))||!slot.present68)continue;
        if(!valid(slot.position20))return false;
        auto delta=sub(slot.position20,position);
        const f32 distance=source_basis_detail::length(delta);
        if(!std::isfinite(distance))return false;
        next.examined_mask|=bit;
        if(distance<=150.f){next.stop_slot=u8(i);*out=next;return true;}
        if(!nudged){next.weight29c=std::min(f32(weight+3.f),100.f);nudged=true;}
        if(!source_basis_detail::normalize(&delta))return false;
        sum=advance(sum,scale(delta,next.weight29c));
    }
    if(nudged)next.position20=advance(position,sum);
    if(!valid(next.position20))return false;
    *out=next;return true;
}

struct GRedBeamPhaseOneInputs {
    GRedBeamFlightSample flight;
    GRedBeamSlotSnapshot slots;
    std::array<f32,3> previous2c{};
    f32 weight29c=0;
    s16 lifetime1c=0;
    u8 flags1d9=0;
    bool null_target_c8=false,previous_declared=false;
};
struct GRedBeamPhaseOnePlan {
    GRedBeamFlightSample flight;
    GRedBeamProximityPlan proximity;
    StageProjectileSegment stage;
    GRedBeamActivePlan active;
    bool stage_visited=false,bounds_visited=false;
};
// Detached untargeted phase-1 proposal, AFTER the separate credit guard.
// The null-C8 aim visit does not change motion; skip modifier/scale tails
// and all callbacks. Previous +2C and slot fields must be declared.
inline bool normal_gred_beam_phase_one_plan(NormalGRedNeutralBSource source,const GRedBeamPhaseOneInputs& in,
                                            const std::array<const StihGrid*,3>& grids,const StageProjectileBounds& bounds,
                                            GRedBeamPhaseOnePlan* out,std::string* error){
    using S=GRedBeamStopSample;
    auto fail=[&](const char* why){if(error)*error=std::string("G-Red beam phase one: ")+why;return false;};
    if(!out||!in.previous_declared||!source_intersection_detail::valid(in.previous2c)||
       in.lifetime1c<0||in.lifetime1c>300)return fail("declared previous position and lifetime required");
    GRedBeamPhaseOnePlan next;
    if(!normal_gred_beam_flight_prefix(source,in.null_target_c8,in.flight,&next.flight)||
       !normal_gred_beam_proximity_plan(source,in.slots,next.flight.position20,in.weight29c,&next.proximity))
        return fail("untargeted motion or six-slot snapshot refused");
    next.flight.position20=next.proximity.position20;
    GRedBeamActiveInputs gates{in.lifetime1c,in.flags1d9};
    gates.proximity=next.proximity.stop_slot<6?S::Stop:S::Clear;
    if(gates.proximity==S::Clear&&!(in.flags1d9&0xa2u)){
        if(!stage_projectile_segment(grids,bounds.horizontal,next.flight.position20,in.previous2c,&next.stage,error))return false;
        next.stage_visited=true;
        gates.stage=next.stage.surface.grid_code?S::Stop:S::Clear;
        if(gates.stage==S::Stop){
            next.flight.position20=next.stage.position;
            next.flight.distance180=gred_beam_flight_detail::distance(next.flight.position20,next.flight.origin174);
        }else{
            bool outside=false;
            if(!stage_projectile_outside(bounds,next.flight.position20,&outside))return fail("stage bounds unavailable");
            next.bounds_visited=true;gates.bounds=outside?S::Stop:S::Clear;
        }
    }
    if(!gred_beam_flight_detail::valid(next.flight)||!normal_gred_beam_active_plan(source,gates,&next.active))
        return fail("numeric result or branch order refused");
    *out=next;if(error)error->clear();return true;
}
}
