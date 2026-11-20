#pragma once
#include "engine/gameplay/borgs/normal_gred_beam_muzzle.hpp"

namespace gf {
struct GRedBeamFlightSample {
    std::array<f32,3> origin174{},position20{},motion38{};
    f32 speed44=0,distance180=0;
};
namespace gred_beam_flight_detail {
using namespace source_intersection_detail;
inline Vec advance(Vec position,const Vec& delta){
    for(unsigned i=0;i<3;++i)position[i]=f32(position[i]+delta[i]);
    return position;
}
inline f32 distance(const Vec& a,const Vec& b){return source_basis_detail::length(sub(a,b));}
inline bool valid(const GRedBeamFlightSample& in){
    return source_intersection_detail::valid(in.origin174)&&source_intersection_detail::valid(in.position20)&&
        source_intersection_detail::valid(in.motion38)&&std::isfinite(in.speed44)&&in.speed44>0&&in.speed44<=160&&
        std::isfinite(in.distance180)&&in.distance180>=0&&in.distance180<=4e7f;
}
}
// Numeric subset of the initializer after the untargeted muzzle branch.
// Advance by the unit bearing BEFORE multiplying the vector by seed speed.
// No allocation, collider, shape scale, art or scene state is supplied.
inline bool normal_gred_beam_flight_seed(NormalGRedNeutralBSource source,const GRedBeamMuzzlePlan& muzzle,
                                         GRedBeamFlightSample* out){
    using namespace source_intersection_detail;
    if(!out||!source.reviewed()||!valid(muzzle.origin174)||!valid(muzzle.unit_bearing)||
       std::abs(dot(muzzle.unit_bearing,muzzle.unit_bearing)-1.f)>.0001f)return false;
    GRedBeamFlightSample next;
    next.origin174=muzzle.origin174;next.position20=gred_beam_flight_detail::advance(next.origin174,muzzle.unit_bearing);
    next.speed44=80;next.motion38=scale(muzzle.unit_bearing,next.speed44);
    next.distance180=gred_beam_flight_detail::distance(next.position20,next.origin174);
    if(!gred_beam_flight_detail::valid(next))return false;
    *out=next;return true;
}

// C4540 null-C8 motion then C40BC..C40DC's speed change. The movement
// uses the old speed, not the increased one. Stops before the six-slot
// proximity/steering helper, stage queries and modifier-specific tail.
inline bool normal_gred_beam_flight_prefix(NormalGRedNeutralBSource source,bool null_target_c8,
                                           const GRedBeamFlightSample& in,GRedBeamFlightSample* out){
    using namespace source_intersection_detail;
    if(!out||!source.reviewed()||!null_target_c8||!gred_beam_flight_detail::valid(in))return false;
    auto next=in;
    if(!source_basis_detail::normalize(&next.motion38))return false;
    next.motion38=scale(next.motion38,in.speed44);
    next.position20=gred_beam_flight_detail::advance(in.position20,next.motion38);
    next.distance180=gred_beam_flight_detail::distance(next.position20,in.origin174);
    next.speed44=std::min(f32(in.speed44+.5f),160.f);
    if(!gred_beam_flight_detail::valid(next))return false;
    *out=next;return true;
}

enum class GRedBeamStopSample:u8 {Unobserved,Clear,Stop};
enum class GRedBeamActiveRoute:u8 {AwaitProximity,ProximityStop,LatchStop,AwaitStage,StageStop,AwaitBounds,BoundsStop,Continue,Expired};
struct GRedBeamActiveInputs {
    s16 lifetime1c=0;
    u8 flags1d9=0;
    GRedBeamStopSample proximity=GRedBeamStopSample::Unobserved,stage=GRedBeamStopSample::Unobserved,bounds=GRedBeamStopSample::Unobserved;
};
struct GRedBeamActivePlan {
    GRedBeamActiveRoute route=GRedBeamActiveRoute::AwaitProximity;
    s16 lifetime1c=0;
    u8 phase18=1,marker19=0; // zero: no write to +19
    bool request_remove_shapes=false,refresh_stage_distance=false;
};
// Phase-1 branch order after the motion prefix. Supplied query outcomes
// are not live contact receipts. Do not accept later samples from a
// skipped branch; never treat an unobserved query as clear.
inline bool normal_gred_beam_active_plan(NormalGRedNeutralBSource source,const GRedBeamActiveInputs& in,GRedBeamActivePlan* out){
    using S=GRedBeamStopSample;using R=GRedBeamActiveRoute;
    if(!out||!source.reviewed()||in.lifetime1c<0||in.lifetime1c>300||
       in.proximity>S::Stop||in.stage>S::Stop||in.bounds>S::Stop)return false;
    GRedBeamActivePlan next;next.lifetime1c=in.lifetime1c;
    auto retire=[&](R route,u8 marker=1){next.route=route;next.phase18=2;next.marker19=marker;next.request_remove_shapes=true;};
    if(in.proximity!=S::Clear||(in.flags1d9&0xa2u)){
        if(in.stage!=S::Unobserved||in.bounds!=S::Unobserved)return false;
        if(in.proximity==S::Stop)retire(R::ProximityStop);
        else if(in.proximity==S::Clear)retire(R::LatchStop);
    }else if(in.stage!=S::Clear){
        if(in.bounds!=S::Unobserved)return false;
        if(in.stage==S::Stop){retire(R::StageStop);next.refresh_stage_distance=true;}
        else next.route=R::AwaitStage;
    }else if(in.bounds==S::Stop)retire(R::BoundsStop,2);
    else if(in.bounds==S::Unobserved)next.route=R::AwaitBounds;
    else{
        --next.lifetime1c;
        if(next.lifetime1c<=0)retire(R::Expired,2);else next.route=R::Continue;
    }
    *out=next;return true;
}
}
