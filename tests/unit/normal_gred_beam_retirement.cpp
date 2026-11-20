#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borgs/normal_gred_beam_retirement.hpp"

namespace {
void check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"normal G-Red beam retirement: %s\n",why);std::abort();}}
}
int main(){
    using namespace gf;
    using R=GRedBeamRetirementRoute;
    constexpr NormalGRedNeutralBSource red{0x0615,0,2};
    GRedNeutralBReleasePlan paid;paid.request_beam=true;paid.owner_slot=2;paid.object_type=2;paid.argument=43;
    GRedBeamFactoryPlan beam;
    GRedBeamWorldVisitPlan visit;
    check(normal_gred_beam_factory_plan(red,paid,true,GRedBeamPoolSample::Allocated,&beam)&&
          normal_gred_beam_world_visit(red,beam,{true,0,0,1,1},&visit),"declared carrier visit");
    GRedBeamRetirementFields in;
    in.phase18=2;
    GRedBeamRetirementPlan p;
    check(normal_gred_beam_retirement_visit(red,visit,in,&p)&&p.route==R::Hide&&p.hide_render&&
          p.fields.phase18==3&&!p.fields.render82&&!p.fields.retire1d8,"marker zero hides even before successful initialization");
    check(normal_gred_beam_retirement_visit(red,visit,p.fields,&p)&&p.route==R::RequestCarrierRelease&&!p.hide_render,
          "phase three requests a distinct later carrier release");
    in.marker19=1;in.render82=1;in.length184=90;in.flight={{0,0,0},{0,0,90},{0,0,80},80.5f,90};
    in.scale58={1,1,90.f/22};in.radial_b4=1;
    check(!normal_gred_beam_retirement_visit(red,visit,in,&p)&&p.route==R::RequestCarrierRelease,
          "a retract marker without the preceding stop flag is unsupported");
    in.retire1d8=1;
    check(normal_gred_beam_retirement_visit(red,visit,in,&p)&&p.route==R::Retract&&!p.hide_render&&
          p.fields.phase18==2&&p.fields.length184==9.5f&&p.fields.scale58[2]==9.5f/22&&p.fields.flight.position20==in.flight.position20,
          "length retracts while position and displayed thickness hold");
    check(normal_gred_beam_retirement_visit(red,visit,p.fields,&p)&&p.route==R::Retract&&p.hide_render&&
          p.fields.phase18==3&&!p.fields.render82&&p.fields.length184==-71&&p.fields.scale58[2]<0,
          "signed L/Z is not clamped when the beam hides");
    GRedBeamTransformPlan frame;
    GRedBeamScalePlan active_scale;active_scale.scale58=p.fields.scale58;
    check(!normal_gred_beam_transform(red,p.fields.flight,active_scale,Mat4{},&frame)&&
          !normal_gred_beam_retirement_frame(red,p,0,Mat4{},&frame)&&
          normal_gred_beam_retirement_frame(red,p,0x800c44a8,Mat4{},&frame)&&
          frame.frame114.at(2,2)==p.fields.scale58[2]&&frame.frame114.at(2,3)==90,
          "only the declared late retirement callback admits the hidden signed axis");
    in.marker19=2;in.flight.position20[2]=990;in.flight.distance180=990;in.flight.speed44=160;in.acceleration4c=.5f;
    check(normal_gred_beam_retirement_visit(red,visit,in,&p)&&p.route==R::DriftFade&&p.fields.flight.position20[2]==1070&&
          p.fields.flight.distance180==1070&&p.fields.flight.speed44==160&&p.fields.flight.motion38==in.flight.motion38&&
          p.fields.length184==1000&&p.fields.radial_b4==.9f&&p.fields.scale58[0]==.9f,
          "fade drifts using the stored vector, not the current or advanced scalar speed");
    in.radial_b4=.055f;
    check(normal_gred_beam_retirement_visit(red,visit,in,&p)&&p.hide_render&&p.fields.phase18==3&&p.fields.radial_b4<.05f,
          "fade hides only after writing scaled B4 and the new visual dimensions");
    visit.route=GRedBeamWorldRoute::GroupMasked;
    check(!normal_gred_beam_retirement_visit(red,visit,p.fields,&p),"masked carrier cannot advance or request pool cleanup");
    std::puts("normal G-Red beam retirement: marker branches, late frame and release boundary PASS");
}
