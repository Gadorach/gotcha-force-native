#include <cmath>
#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borgs/normal_gred_beam_transform.hpp"

namespace {
void check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"normal G-Red beam transform: %s\n",why);std::abort();}}
}
int main(){
    using namespace gf;
    using V=std::array<f32,3>;
    using R=GRedBeamBasisRoute;
    using source_basis_detail::column;
    constexpr NormalGRedNeutralBSource source{0x0615,0,3};
    GRedBeamScalePlan scale;
    check(normal_gred_beam_scale_seed(source,1.25f,81.f,&scale)&&
        scale.reference188==f32(f32(40.f/bits_to_f32(0x4213d70au))*1.25f)&&
        scale.physical_c0==scale.reference188&&scale.relative_c4==1&&scale.length184==81&&scale.scale58[2]==81.f/22.f,
        "parent scale ratio, physical scalar and length are independent");
    constexpr f32 reference=2;
    check(normal_gred_beam_scale_step(source,0,reference,1.5f,1200,&scale)&&
        scale.reference188==2&&scale.physical_c0==1.5f&&scale.relative_c4==.75f&&scale.length184==1000&&
        scale.scale58==V{1.5f,1.5f,1000.f/22.f},"active length cap retains physical scale and ratio");
    check(!normal_gred_beam_scale_step(source,1,reference,1.5f,1200,&scale)&&
        !normal_gred_beam_scale_seed(source,0,1,&scale)&&scale.length184==1000,"modifiers and invalid parent scale refused");

    GRedBeamTransformPlan plan;
    GRedBeamFlightSample flight{{1,2,3},{1,2,25},{80,0,0},80,22};
    check(normal_gred_beam_scale_step(source,0,2,2,22,&scale)&&
        normal_gred_beam_transform(source,flight,scale,{},&plan)&&plan.basis==R::UpReference&&
        column(plan.frame114,2)==V{0,0,1}&&column(plan.frame114,3)==flight.position20,
        "basis comes from origin-to-position, not stored velocity");
    flight.position20={1,24,3};
    check(normal_gred_beam_transform(source,flight,scale,{},&plan)&&plan.basis==R::SideReference&&
        column(plan.frame114,0)==V{2,0,0}&&column(plan.frame114,2)==V{0,1,0},"vertical direction selects +X fallback");
    flight.position20=flight.origin174;
    constexpr Mat4 prior{{0,0,3,80, 0,4,0,90, -5,0,0,100, 0,0,0,1}};
    check(normal_gred_beam_transform(source,flight,scale,prior,&plan)&&plan.basis==R::Retained&&
        column(plan.frame114,0)==V{0,0,-10}&&column(plan.frame114,1)==V{0,8,0}&&
        column(plan.frame114,2)==V{3,0,0}&&column(plan.frame114,3)==flight.position20,
        "two failed bases retain columns, post-scale and replace translation");
    check(normal_gred_beam_scale_step(source,0,2,2,0,&scale)&&
        normal_gred_beam_transform(source,flight,scale,prior,&plan)&&column(plan.frame114,2)==V{},
        "zero length is a real candidate, not a fabricated collision radius or identity");
    std::puts("normal G-Red beam transform: detached scales and matrix recipes PASS");
}
