#include <cstdio>
#include <cstdlib>
#include "tests/unit/character_program_fixture.hpp"
namespace {
unsigned checks=0;std::string error;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"dash recovery control: %s (%s)\n",why,error.c_str());std::abort();}}
}
int main(){
    using namespace gf;const auto bytes=character_program_fixture::bytes();CharacterProgramAsset asset;
    check(character_program_load({bytes.data(),bytes.size()},0x12345678,&asset,&error),"owned program fixture");
    asset.control_flow=true;asset.programs={{5,0,{1,5,0,0,0,0,0,0,3,3,7,0,0,0,0,0,0,1,0,100,255,255}}};
    CharacterProgramVm vm,other;character_program_fixture::Clock clock;std::vector<CharacterProgramOperation> operations;
    CharacterProgramClassFields observed;
    auto load=[&](){
        check(vm.load_fresh_actor_height_controls(asset,{},&error)&&
            vm.request(5,0,3,0,1,clock,&operations,&error)&&vm.observe_class_fields(&observed),"fresh class-scoped control3");
    };
    load();const auto before=vm.state();
    check(vm.clear_ground_dash_recovery_control(observed,3,&error)&&vm.state().actor.control1==std::array<u8,2>{0,7}&&
        vm.state().lanes[0].motion.frame==before.lanes[0].motion.frame&&vm.state().lanes[1].clock==before.lanes[1].clock&&
        vm.state().actor.root.updates==0&&vm.state().actor.behaviour==before.actor.behaviour,
        "61160 clears only1D0F, not secondary control, clocks, flags, motion or root");
    check(!vm.clear_ground_dash_recovery_control(observed,3,&error)&&vm.state().actor.control1[0]==0&&!vm.ready(),
        "repeated acknowledgement refuses without inventing control3");
    load();check(!vm.clear_ground_dash_recovery_control(observed,2,&error)&&vm.state().actor.control1[0]==3,
        "only exact control3 can be cleared");
    load();const auto stale=observed;
    check(vm.write_ground_walk_flag(observed,true,&error)&&!vm.clear_ground_dash_recovery_control(stale,3,&error)&&
        vm.state().actor.control1[0]==3,"stale represented fields refuse");
    load();check(other.load_fresh_actor_height_controls(asset,{},&error)&&
        !other.clear_ground_dash_recovery_control(observed,3,&error),"foreign class scope refuses");
    const auto old=observed;load();
    check(!vm.clear_ground_dash_recovery_control(old,3,&error),"reload retires old write authority");
    load();auto edited=observed;++edited.steering1dfc;
    check(!vm.clear_ground_dash_recovery_control(edited,3,&error),"steering field must also match exactly");
    check(vm.load_fresh_actor_control_flow(asset,&error)&&vm.request(5,0,3,0,1,clock,&operations,&error)&&
        vm.observe_class_fields(&observed)&&!vm.clear_ground_dash_recovery_control(observed,3,&error),
        "older VM mode cannot silently acquire recovery control authority");
    std::printf("scoped dash recovery control acknowledgement: %u checks PASS; no callback/frame authority\n",checks);
}
