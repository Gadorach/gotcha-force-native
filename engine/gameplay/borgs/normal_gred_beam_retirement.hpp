#pragma once
#include "engine/gameplay/borgs/normal_gred_beam_transform.hpp"
#include "engine/gameplay/borgs/normal_gred_beam_world_visit.hpp"

namespace gf {
enum class GRedBeamRetirementRoute:u8 {Hide,Retract,DriftFade,RequestCarrierRelease};
struct GRedBeamRetirementFields {
    u8 phase18=0,marker19=0,render82=0,retire1d8=0;
    GRedBeamFlightSample flight;
    f32 length184=0,radial_b4=0,acceleration4c=0;
    std::array<f32,3> scale58{};
};
struct GRedBeamRetirementPlan {
    GRedBeamRetirementFields fields;
    GRedBeamRetirementRoute route=GRedBeamRetirementRoute::Hide;
    bool hide_render=false;
};
// C426C marker dispatch or C4448, in a declared carrier callback. Transition
// to phase 3 is not the phase-3 visit, and 1D8 is not a collider release ACK.
// B4 and 58/5C may change without any write to the physical C0/C4.
inline bool normal_gred_beam_retirement_visit(NormalGRedNeutralBSource source,const GRedBeamWorldVisitPlan& visit,
    const GRedBeamRetirementFields& in,GRedBeamRetirementPlan* out){
    using R=GRedBeamRetirementRoute;
    using gred_beam_flight_detail::advance;
    if(!out||!source.reviewed()||visit.route!=GRedBeamWorldRoute::Dispatch||!visit.visit_shape_group||
       visit.credit_slot!=source.slot||visit.carrier_group!=9+source.slot||in.phase18<2||in.phase18>3||in.marker19>2||
       (in.marker19&&!in.retire1d8)||(in.phase18==3&&in.render82))return false;
    GRedBeamRetirementPlan next;next.fields=in;
    auto& f=next.fields;
    auto hide=[&](){f.phase18=3;f.render82=0;next.hide_render=true;};
    if(in.phase18==3)next.route=R::RequestCarrierRelease;
    else if(!in.marker19)hide();
    else if(in.marker19==1){
        if(!std::isfinite(in.length184)||in.length184<0||in.length184>1000||
           !std::isfinite(in.flight.speed44)||in.flight.speed44<=0||in.flight.speed44>160)return false;
        next.route=R::Retract;
        f.length184=f32(in.length184-in.flight.speed44);
        f.scale58[2]=f32(f.length184/22.f);
        if(f.scale58[2]<=0)hide();
    }else{
        using source_basis_detail::number;
        if(!gred_beam_flight_detail::valid(in.flight)||in.acceleration4c!=.5f||
           !number(in.radial_b4)||in.radial_b4<=0)return false;
        next.route=R::DriftFade;
        f.flight.position20=advance(in.flight.position20,in.flight.motion38);
        f.flight.distance180=gred_beam_flight_detail::distance(f.flight.position20,in.flight.origin174);
        f.flight.speed44=std::min(f32(in.flight.speed44+in.acceleration4c),160.f);
        f.radial_b4=f32(in.radial_b4*bits_to_f32(0x3f666666u));
        f.length184=std::min(f.flight.distance180,1000.f);
        f.scale58={f.radial_b4,f.radial_b4,f32(f.length184/22.f)};
        if(!gred_beam_flight_detail::valid(f.flight))return false;
        if(f.radial_b4<=bits_to_f32(0x3d4ccccdu))hide();
    }
    *out=next;return true;
}
// The late callback list can still visit a hidden carrier on the update
// that entered phase 3. Do not route it through active collider admission.
inline bool normal_gred_beam_retirement_frame(NormalGRedNeutralBSource source,const GRedBeamRetirementPlan& visit,
    u32 declared_late_callback100,const Mat4& prior114,GRedBeamTransformPlan* out){
    const auto& f=visit.fields;
    if(!source.reviewed()||declared_late_callback100!=0x800c44a8||
       visit.route==GRedBeamRetirementRoute::RequestCarrierRelease||
       (f.phase18!=2&&f.phase18!=3)||(f.scale58[2]<0&&(visit.route!=GRedBeamRetirementRoute::Retract||!visit.hide_render)))return false;
    return gred_beam_transform_detail::build_frame(f.flight,f.scale58,prior114,out);
}
}
