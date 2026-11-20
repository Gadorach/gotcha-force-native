#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/solid_pair_displacement.hpp"
#include "engine/gameplay/solid_sphere_box_face.hpp"

namespace {
unsigned checks=0;
void check(bool ok,const char* why){
    ++checks;if(!ok){std::fprintf(stderr,"solid pair displacement: %s\n",why);std::abort();}
}
}

int main(){
    using namespace gf;
    using S=SolidPairSide;
    constexpr f32 minimum=bits_to_f32(0x3b03126fu);
    SolidPairDisplacementInput in{{2,0,0},-8,{{{2,0,0},{6,0,0}}},0,true};
    SolidPairDisplacementPlan plan;
    std::string error;
    auto solve=[&](){return solid_pair_displacement_plan(in,&plan,&error);};
    auto step=[&](unsigned i,S target,f32 dx){
        check(i<plan.count&&plan.steps[i]==SolidPairDisplacementStep{target,{dx,0,0}},"ordered target and normalized displacement");
    };
    check(solve()&&plan.count==2&&plan.weights==std::array<f32,2>{2,6},"weight-based split");
    step(0,S::Second,-2);step(1,S::First,6);
    in.owners[0].weight204=-1;in.owners[1].weight204=0;
    check(solve()&&plan.weights==std::array<f32,2>{1,1},"weights have a lower bound of one");
    step(0,S::Second,-4);step(1,S::First,4);
    in.owners[1].weight204=50000;
    check(solve()&&plan.count==1,"exact heavy threshold selects one-sided displacement");
    step(0,S::First,8);
    in.owners[0].weight204=50000;in.owners[1].weight204=1;
    check(solve()&&plan.count==1,"reversed heavy pair keeps sign and side");
    step(0,S::Second,-8);
    in.owners[1].weight204=50000;
    check(solve()&&plan.count==2,"two heavy owners still use the split, not a fabricated no-op");
    step(0,S::Second,-4);step(1,S::First,4);
    in.owners={{{2,0,0x80},{6,0,1}}};in.common_mask_eb=0x80;
    check(solve()&&plan.count==1&&plan.weights==std::array<f32,2>{2,50000},"common EB mask keeps only the matching team movable");
    step(0,S::First,8);
    in.owners[1].kind83=0x16;in.common_mask_eb=1;
    in.owners[0].team_mask3e5=in.owners[1].team_mask3e5=1;
    check(solve()&&plan.count==1&&plan.weights==std::array<f32,2>{2,50000},"nonzero kind is protected only with the active mask");
    step(0,S::First,8);
    in.common_mask_eb=0;in.separation=.0001f;
    check(solve()&&plan.count==2&&plan.separation==minimum&&plan.weights==std::array<f32,2>{2,6},
          "inactive mask does not force weight; tiny positive separation retains its sign");
    step(0,S::Second,f32(minimum*.25f));step(1,S::First,-f32(minimum-f32(minimum*.25f)));
    in.separation=-.0001f;in.owners[1].weight204=50000;
    check(solve()&&plan.count==1&&plan.separation==-minimum,"tiny negative separation gets the negative minimum");
    step(0,S::First,minimum);
    in.normal={};in.separation=-8;
    check(solve()&&plan==SolidPairDisplacementPlan{},"zero normal produces no writes");
    in.normal={2,0,0};in.separation=0;
    check(solve()&&plan.count==1&&plan.separation==-minimum,"zero separation takes the source negative branch");
    step(0,S::First,minimum);

    const std::array<SolidBoxFacePlane,6> box{{
        {{0,1,0},{2,2,0}},{{0,0,1},{2,2,2}},{{-1,0,0},{0,2,0}},
        {{0,-1,0},{2,0,0}},{{0,0,-1},{0,0,0}},{{1,0,0},{2,0,2}},
    }};
    SolidSphereBoxFaceResult proposal;
    check(solid_sphere_box_face({{3,1,1},{4,1,1},1,box,box,true},&proposal,&error)&&proposal.contact&&
          proposal.signed_separation==0.f,"sphere tangent reaches the face proposal");
    in.normal=proposal.normal;in.separation=proposal.signed_separation;
    check(solve()&&plan.count==1,"that zero proposal is not a no-op in the separate numeric split");
    step(0,S::First,minimum);
    const auto retained=plan;
    in.owners[1].weight204=1e8f;
    check(!solve()&&plan==retained,"bad weight leaves the previous plan untouched");
    in.owners[1].weight204=1;in.declared=false;
    check(!solve()&&plan==retained,"unresolved owner observations refuse even with a known proposal");
    std::printf("solid pair clamping, weight splits and ordered displacement plans: %u checks PASS\n",checks);
}
