#include <cstdio>
#include <cstdlib>
#include "tests/unit/character_motion_driver_fixture.hpp"
namespace {
using namespace gf;unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"motion transitions: %s\n",why);std::abort();}}
bool near(float a,float b){return std::abs(a-b)<.00002f;}
CharacterLocalPose seed(const CharacterPoseRig& rig){CharacterLocalPose p;for(const auto& b:rig.binding.bones){CharacterBonePose v;v.joint_offset=b.joint_offset;std::copy_n(b.translation,3,v.translation);p.bones.push_back(v);}p.bones[0].translation[0]=90;return p;}
}
int main(){
    using namespace gf;using namespace character_motion_driver_fixture;Fixture f;std::string error;check(f.load(&error),"owned fixture");auto initial=seed(f.rig);
    // New group0/mode2 path is explicit; legacy APIs still refuse it.
    auto a=f.profile;for(auto& d:a.descriptors)d.group=0;a.programs={{0,0,{1,0,0,2,2,0,0,0,0,1,255,255}}};a.descriptors[0].bytes[6]=a.descriptors[0].bytes[14]=5;a.descriptors[0].bytes[7]=a.descriptors[0].bytes[15]=1;
    check(!character_program_detail::valid(a)&&character_program_detail::valid_transitions(a),"separate asset validation");CharacterProgramVm vm;check(!vm.load_fresh_actor(a,&error)&&vm.load_fresh_actor_transitions(a,&error),"separate VM admission");
    CharacterMotionDriver d;check(!d.load({f.model.data(),f.model.size()},f.rig,a,f.clips,&error)&&d.load_transitions({f.model.data(),f.model.size()},f.rig,a,f.clips,initial,&error),"explicit driver/initial pose");
    CharacterProgramMotion x,y,z;check(d.select(0,a.descriptors[0],false,{},0,1,&x)&&x.frame==0&&!x.rewound&&d.snapshot(x)->looping(),"instant loop request");
    check(d.advance(0,x,5.5f,&y)&&y.frame==2.5f&&y.elapsed==5.5f&&y.rewound&&!y.finished&&near(d.snapshot(y)->sample().bones[0].value[5],2.5f),"nonzero loop-start overshoot wraps and resamples");
    check(d.advance(0,y,.25f,&z)&&z.frame==2.75f&&!z.rewound&&near(d.snapshot(z)->previous_frame(),2.5f),"rewind pulse clears next callback");
    check(d.select(0,a.descriptors[0],false,{},8,1,&x)&&x.frame==2&&!x.rewound&&!x.finished,"FIRST_PLAY wrapping clears lane rewind after sample");
    auto degenerate=a;degenerate.descriptors[0].bytes[7]=4;CharacterMotionDriver clamped;check(clamped.load_transitions({f.model.data(),f.model.size()},f.rig,degenerate,f.clips,initial,&error)&&clamped.select(0,degenerate.descriptors[0],false,{},4,1,&x)&&x.frame==4&&x.finished&&!x.rewound,"loop-start>=end source clamp branch");check(clamped.advance(0,x,1,&y)&&y.frame==4&&y.rewound&&y.finished,"degenerate loop does not divide by zero");
    check(d.select_transition(0,a.descriptors[0],false,{},0,1,2,2,&x)&&d.snapshot(x)->blending()&&near(d.snapshot(x)->previous_weight(),2.f/3)&&near(d.snapshot(x)->local_pose()->bones[0].translation[0],60),"first blend samples new motion but freezes supplied displayed pose");
    check(d.advance(0,x,1,&y)&&y.frame==1&&d.snapshot(y)->blending()&&near(d.snapshot(y)->local_pose()->bones[0].translation[0],90.f/3+2.f/3),"blend timer grows and current clip advances");
    check(d.advance(0,y,1,&z)&&z.frame==1&&!d.snapshot(z)->blending()&&near(d.snapshot(z)->local_pose()->bones[0].translation[0],1),"completion re-request retains previous frame at zero overshoot");
    check(d.advance(0,z,1,&y)&&y.frame==2&&y.elapsed==3,"ordinary advance resumes after blend");
    check(d.select_transition(0,a.descriptors[1],false,x,0,1,2,2,&z)&&near(d.snapshot(z)->local_pose()->bones[0].translation[0],100.f/3+40),"interrupted blend freezes displayed result, not raw previous sample");
    check(near(d.snapshot(x)->local_pose()->bones[0].translation[0],60)&&x.frame==0,"immutable old snapshot unchanged");
    check(d.advance(0,z,3,&y)&&!d.snapshot(y)->blending()&&y.frame==1&&near(d.snapshot(y)->local_pose()->bones[0].translation[0],101),"fractional/large completion carries only timer overshoot");
    check(d.select_transition(0,a.descriptors[1],false,x,0,1,2,0,&z)&&!d.snapshot(z)->blending()&&near(d.snapshot(z)->local_pose()->bones[0].translation[0],100),"zero duration delegates source instant mode");
    check(!d.select_transition(0,a.descriptors[0],false,x,0,1,1,2,&z)&&!d.select_transition(0,a.descriptors[0],false,x,0,16,2,1,&z),"unsupported modes/extrapolation refuse");
    check(!d.advance(1,x,1,&z)&&!d.advance(0,x,NAN,&z)&&!d.advance(0,x,-1,&z),"lane and numeric guards");auto forged=x;forged.frame=1;check(!d.advance(0,forged,1,&z),"opaque state/scalar binding");
    std::vector<CharacterProgramOperation> operations;check(vm.request(0,0,3,0,1,d,&operations,&error)&&vm.state().lanes[0].motion.selected&&vm.state().lanes[1].motion.selected,"actual VM transition dispatch on both lanes");
    bool done=false;for(unsigned i=0;i<32&&!done;++i){check(vm.update(3,1,d,&operations,&error),"VM loop waiting");done=vm.state().lanes[0].done&&vm.state().lanes[1].done;}check(done,"terminal negative wait accepts actual rewind");
    auto legacy=f;legacy.profile.descriptors[0].bytes[6]|=4;CharacterMotionDriver old;check(legacy.driver(old,&error)&&!old.select(0,legacy.profile.descriptors[0],false,{},0,1,&z)&&!old.select_transition(0,legacy.profile.descriptors[0],false,{},0,1,2,2,&z),"legacy loop/blend refusals retained");
    auto bad=initial;bad.bones[0].quaternion[3]=0;check(!d.load_transitions({f.model.data(),f.model.size()},f.rig,a,f.clips,bad,&error)&&!d.snapshot(x),"invalid initial pose retires library");
    auto file=character_program_fixture::bytes();store_be16(file.data()+4,2);CharacterProgramAsset decoded;const auto hash=load_be32(file.data()+8);check(!character_program_load({file.data(),file.size()},hash,&decoded,&error)&&character_program_load_transitions({file.data(),file.size()},hash,&decoded,&error),"GFMP2 explicit loader and legacy version refusal");
    std::printf("character motion transitions: %u owned checks PASS\n",checks);
}
