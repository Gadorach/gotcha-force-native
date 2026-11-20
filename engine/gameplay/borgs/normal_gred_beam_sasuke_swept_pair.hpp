#pragma once
#include "engine/gameplay/borgs/normal_gred_beam_sasuke_fresh_pair.hpp"
#include "engine/gameplay/borgs/normal_gred_beam_transform.hpp"

namespace gf {
struct GRedBeamSweepSample {
    bool prior_phase_one_declared=false;
    std::array<f32,3> prior8c{},prior98{};
    Mat4 beam114;
    f32 physical_c0=0;
};
struct GRedBeamSweptShape {
    std::array<f32,3> previous_d4{},previous_e0{},start8c{},end98{},center64{};
    std::array<f32,3> forward_a4{},reverse_b0{},unit_bc{},reverse_unit_c8{};
    f32 radius88=0,broad_radius7c=0,travel=0;
};
// Moving portion of 3A5E4 for the exact flag-1 beam. The local point
// is the origin, so +114's translation gives the new center. Small
// travel takes a different sphere update and is not admitted here.
inline bool normal_gred_beam_swept_shape(NormalGRedNeutralBSource source,const GRedBeamShapeOperands& attack,
                                        const GRedBeamSweepSample& in,GRedBeamSweptShape* out){
    using namespace source_intersection_detail;
    using gred_beam_flight_detail::advance;
    if(!out||!normal_gred_beam_shape_scope(source,attack)||!in.prior_phase_one_declared||
       !valid(in.prior8c)||!valid(in.prior98)||!gred_beam_transform_detail::matrix(in.beam114)||
       !std::isfinite(in.physical_c0)||in.physical_c0<=0||in.physical_c0>1e4f)return false;
    GRedBeamSweptShape next;
    next.previous_d4=in.prior8c;next.previous_e0=next.start8c=in.prior98;
    next.end98=source_basis_detail::column(in.beam114,3);
    next.forward_a4=sub(next.end98,next.start8c);next.reverse_b0=sub(next.start8c,next.end98);
    next.travel=source_basis_detail::length(next.forward_a4);
    if(!std::isfinite(next.travel)||next.travel<=.5f)return false;
    next.unit_bc=next.forward_a4;next.reverse_unit_c8=next.reverse_b0;
    if(!source_basis_detail::normalize(&next.unit_bc)||!source_basis_detail::normalize(&next.reverse_unit_c8))return false;
    next.center64=scale(advance(next.start8c,next.end98),.5f);
    next.radius88=f32(attack.local_radius*in.physical_c0);
    next.broad_radius7c=f32(f32(next.travel*.5f)+next.radius88);
    if(!valid(next.center64)||!source_basis_detail::number(next.broad_radius7c))return false;
    *out=next;return true;
}

enum class GRedSasukeSweptRoute:u8 {NotQueried,BroadMiss,NarrowMiss,RayCrossing,RadiusFallback};
struct GRedSasukeSweptPairInput:GRedSasukeOrderedPairSample {GRedBeamSweepSample beam;GRedSasukeFreshSphere body;};
struct GRedSasukeSweptPairPlan {
    GRedSasukePairGate gate=GRedSasukePairGate::NotListed;
    GRedSasukeSweptRoute route=GRedSasukeSweptRoute::NotQueried;
    GRedBeamSweptShape beam;
    std::array<f32,3> point{};
    u8 ray_mask=255; // 255: no real roots in 39564
};

// One moving arg-43/body-9 point candidate, not 2DB58's later
// nearest-overlap selection, move-to-contact, callback or HP write.
inline bool normal_gred_beam_sasuke_swept_pair(NormalGRedNeutralBSource source,const GRedBeamShapeOperands& attack,
                                               const GRedSasukeSweptPairInput& in,GRedSasukeSweptPairPlan* out){
    using namespace source_intersection_detail;
    using gred_beam_flight_detail::advance;
    using Route=GRedSasukeSweptRoute;
    GRedSasukeSweptPairPlan next;
    if(!out||!normal_gred_beam_sasuke_pair_gate(source,attack,in,&next.gate))return false;
    auto finish=[&](Route route){next.route=route;*out=next;return true;};
    if(next.gate!=GRedSasukePairGate::Geometry)return finish(Route::NotQueried);
    const auto& body=in.body;
    if(!body.initialized_declared||body.phase10!=1||body.effective_kind58||!valid(body.center64)||
       !source_basis_detail::number(body.radius88)||body.radius88<0||body.radius88!=body.broad_radius7c||
       !normal_gred_beam_swept_shape(source,attack,in.beam,&next.beam))return false;
    const auto& beam=next.beam;
    constexpr f32 min_radius=bits_to_f32(0x3dcccccdu);
    const f32 broad=source_basis_detail::length(sub(beam.center64,body.center64));
    if(!std::isfinite(broad))return false;
    if(beam.broad_radius7c<=min_radius||body.broad_radius7c<=min_radius||
       broad>f32(beam.broad_radius7c+body.broad_radius7c))return finish(Route::BroadMiss);

    // 39564 first solves the centerline against ONLY the receiver radius.
    // 30A3C takes return 1 (one root on segment) immediately, not return 2.
    const auto m=sub(beam.start8c,body.center64);
    const f32 a=dot(beam.forward_a4,beam.forward_a4),b=f32(2.f*dot(beam.forward_a4,m));
    const f32 c=f32(dot(m,m)-f32(body.radius88*body.radius88));
    const f32 discriminant=f32(f32(b*b)-f32(f32(4.f*a)*c));
    if(!std::isfinite(discriminant)||a<=0)return false;
    if(discriminant>=0){
        const f32 root=f32(std::sqrt(f64(discriminant))),denom=f32(2.f*a);
        const auto plus=scale(beam.forward_a4,f32(f32(-b+root)/denom));
        const auto minus=scale(beam.forward_a4,f32(f32(-b-root)/denom));
        const auto p=advance(beam.start8c,plus),q=advance(beam.start8c,minus);
        if(!valid(p)||!valid(q))return false;
        next.ray_mask=u8((dot(plus,beam.forward_a4)<0?1:0)|(dot(sub(p,beam.end98),beam.reverse_b0)<0?2:0)|
            (dot(minus,beam.forward_a4)<0?4:0)|(dot(sub(q,beam.end98),beam.reverse_b0)<0?8:0));
        if(next.ray_mask==1||next.ray_mask==2){next.point=q;return finish(Route::RayCrossing);}
        if(next.ray_mask==4||next.ray_mask==8){next.point=p;return finish(Route::RayCrossing);}
    }

    // Fallback uses its endpoint projection followed by a strict sum of
    // radii test, unlike the fresh sphere query's inclusive tangency.
    const f32 from_start=dot(sub(body.center64,beam.start8c),beam.unit_bc);
    const f32 from_end=dot(sub(body.center64,beam.end98),beam.reverse_unit_c8);
    if(from_start<0&&from_end<0)return finish(Route::NarrowMiss);
    const auto at=from_start<0?beam.start8c:from_end<0?beam.end98:
        advance(beam.end98,scale(beam.reverse_unit_c8,from_end));
    auto direction=sub(at,body.center64);
    const f32 distance=source_basis_detail::length(direction);
    if(!std::isfinite(distance))return false;
    if(distance>=f32(body.radius88+beam.radius88))return finish(Route::NarrowMiss);
    next.point=at;
    if(distance>0){
        if(!source_basis_detail::normalize(&direction))return false;
        next.point=advance(body.center64,scale(direction,body.radius88));
    }
    if(!valid(next.point))return false;
    return finish(Route::RadiusFallback);
}
}
