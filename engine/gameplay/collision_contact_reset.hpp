#pragma once
#include <array>
#include <string>
#include "engine/graphics/source_intersection.hpp"
namespace gf {
struct CollisionContactRetained {
    std::array<f32,3> displacement1cc{};
    u8 previous275=0,timer1df=0,timer2ae=0,timer2af=0,timer1e8=0;
    bool operator==(const CollisionContactRetained&)const=default;
};
struct CollisionContactState {
    CollisionContactRetained retained;
    u8 field1d9=0,field1da=0,attached1db=0,previous1de=0,field1dc=0,moved1dd=0,field280=0,current274=0;
    u16 hidden270=0,shadow286=0;
    bool operator==(const CollisionContactState&)const=default;
};
//2CDA4's explicit reset kernel. This does not run shape tests or actor responses.
inline bool collision_contact_reset(const CollisionContactState& before,CollisionContactState* out,std::string* error){
    if(!out||!source_intersection_detail::valid(before.retained.displacement1cc)){if(error)*error="contact reset: finite retained displacement and output required";return false;}
    auto next=before;next.retained.displacement1cc={};next.field1d9=next.field1da=next.field1dc=next.moved1dd=next.field280=0;
    next.previous1de=before.attached1db;next.attached1db=0;next.retained.previous275=before.current274;next.current274=0;
    for(auto* timer:{&next.retained.timer1df,&next.retained.timer2ae,&next.retained.timer2af,&next.retained.timer1e8})if(*timer)--*timer;
    next.hidden270=next.shadow286=0;*out=next;if(error)error->clear();return true;
}
struct CollisionBroadphaseResult {f32 distance=0,radius_sum=0;bool radii_used=false,distance_used=false,overlap=false;};
//39F6C after3A05C's explicit displaced centers. Strict radius threshold, inclusive
//distance boundary; no narrow-phase or miss fabricated from missing shape data.
inline bool collision_contact_broadphase(const std::array<f32,3>& a,f32 ar,const std::array<f32,3>& b,f32 br,f32 threshold,CollisionBroadphaseResult* out,std::string* error){
    if(out)*out={};
    using namespace source_intersection_detail;
    if(!out||!valid(a)||!valid(b)||!std::isfinite(ar)||!std::isfinite(br)||!std::isfinite(threshold)||ar<0||br<0||std::abs(ar)>1e7f||std::abs(br)>1e7f||f32_to_bits(threshold)!=0x3dcccccdu){if(error)*error="contact broadphase: source centers/radii/threshold domain";return false;}
    CollisionBroadphaseResult p;p.radii_used=true;
    if(ar>threshold&&br>threshold){const auto delta=sub(a,b);const f32 squared=dot(delta,delta);if(!std::isfinite(squared)){if(error)*error="contact broadphase: derived distance domain";return false;}
        p.distance=f32(std::sqrt(f64(squared)));p.radius_sum=f32(ar+br);p.distance_used=true;p.overlap=p.distance<=p.radius_sum;}
    *out=p;if(error)error->clear();return true;
}
}
