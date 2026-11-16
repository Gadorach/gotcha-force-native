#include <cstdio>
#include <cstdlib>
#include "tests/unit/character_program_fixture.hpp"
namespace {
unsigned checks=0;std::string error;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"ground-loss VM flags: %s (%s)\n",why,error.c_str());std::abort();}}
}
int main(){
    using namespace gf;const auto bytes=character_program_fixture::bytes();CharacterProgramAsset asset;
    check(character_program_load({bytes.data(),bytes.size()},0x12345678,&asset,&error),"owned synthetic programs");
    asset.control_flow=true;CharacterProgramVm vm,foreign;CharacterProgramClassFields observed;
    auto load=[&](){check(vm.load_fresh_actor_height_controls(asset,{},&error)&&vm.observe_class_fields(&observed),"fresh explicit height/class scope");};
    load();check(vm.write_ground_walk_flag(observed,true,&error)&&vm.observe_class_fields(&observed)&&observed.behaviour==0x10,
        "existing walk flag entry");
    check(vm.write_ground_loss_flags(observed,CharacterProgramGroundLossFlagPhase::Correction,&error)&&
        vm.observe_class_fields(&observed)&&observed.behaviour==0,"6A5A4 masks walk flag before falling dispatch");
    check(vm.write_ground_loss_flags(observed,CharacterProgramGroundLossFlagPhase::FallingEntry,&error)&&
        vm.observe_class_fields(&observed)&&observed.behaviour==0x40,"5FECC explicitly sets airborne flag");
    check(vm.write_ground_loss_flags(observed,CharacterProgramGroundLossFlagPhase::LandingEntry,&error)&&
        vm.observe_class_fields(&observed)&&observed.behaviour==0x200,"60228 clears air/motion bits and sets landing bit");
    check(vm.write_ground_loss_flags(observed,CharacterProgramGroundLossFlagPhase::Idle,&error)&&
        vm.observe_class_fields(&observed)&&observed.behaviour==0,"6A474 numeric idle mask");
    check(vm.state().lanes[0].program==0xffff&&vm.state().lanes[1].program==0xffff&&vm.state().actor.root.updates==0,
        "flag writes do not execute programs, motion, callbacks or root updates");
    const auto stale=observed;
    check(vm.write_ground_walk_flag(observed,true,&error),"mutate represented flag");
    check(!vm.write_ground_loss_flags(stale,CharacterProgramGroundLossFlagPhase::Correction,&error)&&
        vm.state().actor.behaviour==0x10&&!vm.ready(),"stale numeric snapshot refuses without mutation and retires VM readiness");
    load();check(foreign.load_fresh_actor_height_controls(asset,{},&error),"foreign VM scope");
    check(!foreign.write_ground_loss_flags(observed,CharacterProgramGroundLossFlagPhase::FallingEntry,&error)&&
        foreign.state().actor.behaviour==0,"foreign scope refuses");
    const auto old=observed;load();
    check(!vm.write_ground_loss_flags(old,CharacterProgramGroundLossFlagPhase::Correction,&error),"reload retires prior scope");
    load();auto edited=observed;++edited.steering1dfc;
    check(!vm.write_ground_loss_flags(edited,CharacterProgramGroundLossFlagPhase::Correction,&error),"all represented class fields match exactly");
    load();check(!vm.write_ground_loss_flags(observed,CharacterProgramGroundLossFlagPhase(255),&error),"unknown phase refuses");
    load();check(vm.write_ground_loss_flags(observed,CharacterProgramGroundLossFlagPhase::FallingEntry,&error)&&vm.observe_class_fields(&observed),
        "source air flag before invalid ground correction");
    check(!vm.write_ground_loss_flags(observed,CharacterProgramGroundLossFlagPhase::Correction,&error)&&vm.state().actor.behaviour==0x40,
        "ground correction cannot reuse an airborne class state");
    check(vm.load_fresh_actor_control_flow(asset,&error)&&vm.observe_class_fields(&observed)&&
        !vm.write_ground_loss_flags(observed,CharacterProgramGroundLossFlagPhase::Correction,&error),
        "older non-height VM mode cannot acquire movement flag write authority");
    std::printf("scoped ground-loss flag portions: %u checks PASS; no actor/frame/callback authority\n",checks);
}
