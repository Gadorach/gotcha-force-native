#include <cstdio>
#include <cstdlib>
#include "tests/unit/character_motion_driver_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"loop clock: %s\n",why);std::abort();}}}
int main(){using namespace gf;using namespace character_motion_driver_fixture;Fixture f;std::string error;check(f.load(&error),"fixture");auto a=f.profile;a.programs={{5,0,{1,5,0,0,0,0,0,0,0,0,255,255,1,5,1,0,0,0,0,0,0,1,255,255}}};a.descriptors[0].bytes[6]=a.descriptors[0].bytes[14]=5;
    CharacterLocalPose rest;for(const auto& b:f.rig.binding.bones){CharacterBonePose p;p.joint_offset=b.joint_offset;std::copy_n(b.translation,3,p.translation);rest.bones.push_back(p);}CharacterMotionDriver driver;CharacterProgramVm vm;std::vector<CharacterProgramOperation> ops;
    check(driver.load_transitions({f.model.data(),f.model.size()},f.rig,a,f.clips,rest,&error)&&vm.load_fresh_actor_transitions(a,&error)&&vm.request_with_transition(5,0,3,0,1,{0,0},driver,&ops,&error),"explicit looping VM");
    for(unsigned i=1;i<=10000;++i){check(vm.update(3,1,driver,&ops,&error),"long-lived loop callback");for(const auto& lane:vm.state().lanes)check(lane.motion.elapsed==float(i)&&lane.motion.frame==float(i%4)&&lane.clock==float(i)&&lane.offset==float(i-i%4)&&!lane.done&&!lane.motion.finished,"accumulated clock not clipped/moduloed with sample frame");}
    check(vm.request_with_transition(5,0,3,0,1,{0,0},driver,&ops,&error)&&vm.state().lanes[0].motion.elapsed==0,"re-request after long playback");
    CharacterProgramMotion m;m.selected=true;m.elapsed=10000;m.frame=3;m.end_frame=4;check(!character_program_vm_detail::motion(m)&&character_program_vm_detail::motion(m,true),"old elapsed8192 guard retained");m.frame=8193;check(!character_program_vm_detail::motion(m,true),"extended mode retains sample-frame budget");m.frame=0;m.elapsed=INFINITY;check(!character_program_vm_detail::motion(m,true),"nonfinite accumulated clock refuses");m.elapsed=-1;check(!character_program_vm_detail::motion(m,true),"negative accumulated clock refuses");
    CharacterMotionDriver legacy;check(f.driver(legacy,&error),"legacy fixture");CharacterProgramMotion x,y;check(legacy.select(0,f.profile.descriptors[0],false,{},0,1,&x),"legacy select");for(unsigned i=0;i<512;++i){check(legacy.advance(0,x,16,&y),"legacy within elapsed budget");x=y;}check(x.elapsed==8192&&!legacy.advance(0,x,1,&y),"legacy still refuses beyond8192");
    std::printf("loop lifetime clock: %u checks PASS (10000 numeric updates, not games)\n",checks);
}
