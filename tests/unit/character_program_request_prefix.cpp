#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/character_program_directional_control.hpp"
#include "tests/unit/character_motion_directional_fixture.hpp"
namespace {unsigned checks=0;void check(bool b,const char* why){++checks;if(!b){std::fprintf(stderr,"request prefix: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;std::string error;character_motion_directional_fixture::Fixture fixture;check(fixture.load(&error),"owned motion fixture");
    auto asset=fixture.profile;asset.control_flow=asset.request_prefix_only=true;asset.programs={{5,0,{1,5,0,0,0,0,0,0,0,2,255,248}}};
    check(character_program_detail::valid_impl(asset,true,true,true)&&!character_program_detail::valid_control_flow(asset),"nonterminal wait is a bounded prefix, not a full program");
    auto bad=asset;bad.programs[0].bytes.insert(bad.programs[0].bytes.end(),{0,1,255,255});check(!character_program_detail::valid_impl(bad,true,true,true),"prefix cannot include trailing commands beyond first wait");
    bad=asset;bad.programs[0].bytes.resize(8);check(!character_program_detail::valid_impl(bad,true,true,true),"missing first wait refuses");
    CharacterProgramVm legacy;check(!legacy.load_fresh_actor_control_flow(asset,&error),"old VM entry cannot adopt prefix-only asset");
    CharacterMotionDriver legacy_driver;check(!legacy_driver.load_directional_transitions({fixture.model.data(),fixture.model.size()},fixture.rig,asset,fixture.clips,fixture.initial,{0,-1},&error),"old driver entry remains closed");
    CharacterMotionDriver driver;CharacterProgramVm vm;
    check(driver.load_request_prefix({fixture.model.data(),fixture.model.size()},fixture.rig,asset,fixture.clips,fixture.initial,{0,-1},&error),"explicit shared request-prefix driver");
    check(vm.load_fresh_actor_request_prefix(asset,{0,1,0},{10,20,30},&error),"explicit fresh VM retains placed root");
    CharacterPoseHeightProfile height{0,.5f,bits_to_f32(0xbd4cccCD),bits_to_f32(0x3f333333),5,true};CharacterPoseHeightFields fields;fields.scale_b4=1;fields.actor_y=20;
    CharacterMotionRootFields root;root.scale_b4=root.delta5f4=1;CharacterProgramDirectionalControl control;
    check(control.bind_with_root(vm,driver,height,fields,{},root,&error),"same VM/driver scope, no fabricated world positions");
    std::vector<CharacterProgramOperation> operations;check(vm.request_with_actor_controls(5,0,3,-1,1,driver,control,&operations,&error,{0,0}),error.c_str());
    const auto before=vm.state();check(vm.ready()&&before.lanes[0].wait==8&&before.lanes[0].cursor==12&&before.lanes[0].threshold==-8&&!before.lanes[0].done&&before.actor.root.position==std::array<f32,3>{10,20,30},"unchanged nonterminal wait, actual clip request and root position");
    check(vm.program_asset()->programs[0].bytes==asset.programs[0].bytes,"no synthetic terminal/fallback bytes");
    auto replay=vm;check(!replay.request_with_actor_controls(5,0,3,-1,1,driver,control,&operations,&error)&&operations.empty(),"prefix-only request cannot replay");
    check(!vm.update_with_actor_controls(3,1,driver,control,&operations,&error)&&operations.empty()&&!vm.ready()&&vm.state().lanes[0].cursor==before.lanes[0].cursor&&vm.state().lanes[0].motion.context==before.lanes[0].motion.context&&vm.state().actor.root.updates==before.actor.root.updates,"unqualified continuation refuses before time, motion or actor mutation");
    std::printf("character program request prefix: %u checks PASS\n",checks);
}
