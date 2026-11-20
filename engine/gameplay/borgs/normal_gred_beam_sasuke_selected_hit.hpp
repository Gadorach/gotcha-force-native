#pragma once
#include <span>
#include "engine/gameplay/borgs/normal_gred_beam_sasuke_swept_pair.hpp"

namespace gf {
struct GRedSasukeMovingReceiver:GRedSasukeOrderedPairSample {GRedSasukeFreshSphere body;};
struct GRedSasukeMovingScanInput {
    bool complete_reviewed_list_declared=false;
    GRedBeamSweepSample beam;
    GRedBeamFlightSample carrier;
    GRedBeamScalePlan stored_scale;
    u32 contact_update100=0;
};
struct GRedSasukeMovingSelection {
    bool selected=false,moved_to_contact=false,refresh114=false;
    u8 slot=255,order=255;
    f32 source_distance_sq=0;
    std::array<f32,3> point{},end98{},center64{},offset{};
    GRedBeamFlightSample carrier;
    Mat4 frame114;
    GRedBeamSasukeContactPrefix contact;
};

// 2DB58 moving branch, for a complete DECLARED list of reviewed opposing
// normal body-9 samples, at most one per physical slot. This does not
// enumerate a scene, call a callback, write an object, or admit a live hit.
inline bool normal_gred_beam_sasuke_select_moving(NormalGRedNeutralBSource source,const GRedBeamShapeOperands& attack,
    const GRedSasukeMovingScanInput& in,std::span<const GRedSasukeMovingReceiver> receivers,GRedSasukeMovingSelection* out){
    using namespace source_intersection_detail;
    using gred_beam_flight_detail::advance;
    if(!out||!in.complete_reviewed_list_declared||receivers.size()>5||in.contact_update100!=0x800c44a8||
       !gred_beam_flight_detail::valid(in.carrier))return false;
    GRedBeamSweptShape beam;
    if(!normal_gred_beam_swept_shape(source,attack,in.beam,&beam))return false;
    GRedSasukeMovingSelection next;
    next.carrier=in.carrier;next.frame114=in.beam.beam114;next.end98=beam.end98;next.center64=beam.center64;
    constexpr f32 first_distance=bits_to_f32(0x4dbebc20u); // 2DBE4 / 4e8
    f32 nearest=first_distance;
    std::array<bool,6> seen{};
    for(unsigned i=0;i<receivers.size();++i){
        const auto& receiver=receivers[i];
        const auto& fields=receiver.contact_fields;const auto& shared=receivers.front().contact_fields;
        const unsigned slot=fields.receiver_slot;
        if(slot>=6||seen[slot])return false;
        if(fields.beam_token26c!=shared.beam_token26c||fields.beam_latch1d9!=shared.beam_latch1d9||
           fields.credit_latch1d9!=shared.credit_latch1d9||fields.beam_flags2ac!=shared.beam_flags2ac||
           receiver.attack_list.lists!=receivers.front().attack_list.lists)return false;
        seen[slot]=true;
        GRedSasukeSweptPairInput sample;
        static_cast<GRedSasukeOrderedPairSample&>(sample)=receiver;
        sample.beam=in.beam;sample.body=receiver.body;
        GRedSasukeSweptPairPlan overlap;
        if(!normal_gred_beam_sasuke_swept_pair(source,attack,sample,&overlap))return false;
        if(overlap.route!=GRedSasukeSweptRoute::RayCrossing&&overlap.route!=GRedSasukeSweptRoute::RadiusFallback)continue;
        const auto delta=sub(receiver.body.center64,beam.start8c);
        const f32 distance=dot(delta,delta);
        if(!std::isfinite(distance))return false;
        // 2DDBC compares the receiver center to start, with <= replacement.
        if(distance<=nearest){
            nearest=distance;next.selected=true;next.slot=u8(slot);next.order=u8(i);
            next.point=overlap.point;next.source_distance_sq=distance;
        }
    }
    if(next.selected){
        // Exact receiving row 0 has none of the +0x12 displacement skip bits.
        next.offset=sub(next.point,beam.end98);
        const f32 squared=dot(next.offset,next.offset);
        if(!std::isfinite(squared))return false;
        if(squared>0){
            next.end98=advance(beam.end98,next.offset);
            next.carrier.position20=advance(in.carrier.position20,next.offset);
            next.center64=advance(beam.center64,scale(next.offset,.5f));
            if(!valid(next.end98)||!valid(next.center64))return false;
            // C44A8 uses the moved carrier. The stored axial scale and
            // distance180 are not recalculated by this continuation.
            GRedBeamTransformPlan frame;
            if(!normal_gred_beam_transform(source,next.carrier,in.stored_scale,in.beam.beam114,&frame))return false;
            next.frame114=frame.frame114;next.moved_to_contact=next.refresh114=true;
        }
        auto contact=receivers[next.order].contact_fields;
        contact.admitted_pair_declared=true;contact.contact_vector=next.point;
        if(!normal_gred_beam_sasuke_contact_prefix(source,attack,contact,&next.contact))return false;
    }
    *out=next;return true;
}
}
