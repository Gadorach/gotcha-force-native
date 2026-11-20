#pragma once
#include <algorithm>
#include <string>
#include "engine/graphics/source_intersection.hpp"

namespace gf {
// These are observations of the two already-selected owners, not the collider
// pointers themselves. The caller must resolve the source 2AC redirect and
// capture the current common EB byte before requesting this plan.
struct SolidPairOwnerFields {
    f32 weight204=0;
    u8 kind83=0,team_mask3e5=0;
};
struct SolidPairDisplacementInput {
    std::array<f32,3> normal{};
    f32 separation=0;
    std::array<SolidPairOwnerFields,2> owners{};
    u8 common_mask_eb=0;
    bool declared=false;
};
enum class SolidPairSide:u8 {First,Second};
struct SolidPairDisplacementStep {
    SolidPairSide target=SolidPairSide::First;
    std::array<f32,3> delta{};
    bool operator==(const SolidPairDisplacementStep&)const=default;
};
struct SolidPairDisplacementPlan {
    std::array<SolidPairDisplacementStep,2> steps{};
    std::array<f32,2> weights{};
    f32 separation=0;
    unsigned count=0;
    bool operator==(const SolidPairDisplacementPlan&)const=default;
};

// 358F4's numeric split after owner selection, before the 35CA4 actor writes.
// A zero normal is a no-op; zero separation with a nonzero normal is not.
// Host normalization models the direction, not the source PS/FPSCR rounding.
inline bool solid_pair_displacement_plan(const SolidPairDisplacementInput& in,SolidPairDisplacementPlan* out,std::string* error){
    using namespace source_intersection_detail;
    auto fail=[&](const char* why){if(error)*error=std::string("solid pair displacement: ")+why;return false;};
    const auto number=[](f32 v){return std::isfinite(v)&&std::abs(v)<=1e7f;};
    if(!out||!in.declared||!valid(in.normal)||!number(in.separation)||
       !number(in.owners[0].weight204)||!number(in.owners[1].weight204))
        return fail("resolved owner observations and numeric contact required");
    SolidPairDisplacementPlan next;
    const f32 squared=dot(in.normal,in.normal);
    if(squared>0.f){
        constexpr f32 minimum=bits_to_f32(0x3b03126fu),heavy=50000.f;
        next.separation=in.separation>0.f?std::max(in.separation,minimum):std::min(in.separation,-minimum);
        const auto normal=scale(in.normal,f32(1.0/std::sqrt(f64(squared))));
        for(unsigned i=0;i<2;++i){
            const auto& owner=in.owners[i];
            const bool protected_owner=in.common_mask_eb&&(owner.kind83||!(owner.team_mask3e5&in.common_mask_eb));
            next.weights[i]=protected_owner?heavy:std::max(owner.weight204,1.f);
        }
        const f32 a=next.weights[0],b=next.weights[1];
        const auto emit=[&](SolidPairSide side,f32 distance){next.steps[next.count++]={side,scale(normal,distance)};};
        if(b>=heavy&&a<heavy)emit(SolidPairSide::First,-next.separation);
        else if(a>=heavy&&b<heavy)emit(SolidPairSide::Second,next.separation);
        else{
            // Preserve the separate division, multiplication, and remainder,
            // as well as second-before-first order (including two heavy owners).
            const f32 for_second=f32(next.separation*f32(a/f32(a+b)));
            emit(SolidPairSide::Second,for_second);
            emit(SolidPairSide::First,-f32(next.separation-for_second));
        }
        for(unsigned i=0;i<next.count;++i)if(!valid(next.steps[i].delta))return fail("displacement outside numeric domain");
    }
    *out=next;if(error)error->clear();return true;
}
} // namespace gf
