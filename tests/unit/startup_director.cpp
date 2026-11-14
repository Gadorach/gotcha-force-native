#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "tests/unit/startup_director_fixture.hpp"
#include "tests/unit/character_program_fixture.hpp"
#include "engine/gameplay/startup_actor_dispatch.hpp"
namespace {
unsigned checks=0;void check(bool v,const char* why){++checks;if(!v){std::fprintf(stderr,"startup director: %s\n",why);std::abort();}}
std::vector<gf::u8> read(const std::string& path){std::ifstream f(path,std::ios::binary);return {(std::istreambuf_iterator<char>(f)),{}};}
}
int main(){
    using namespace startup_director_fixture;std::string error;gf::StartupDirectorProfile profile;gf::StartupDirector director;std::array<gf::u8,2> indices={7,29};std::vector<gf::StartupEffect> effects;auto input=observations();
    auto load=[&](const auto& bytes){return gf::startup_director_load({bytes.data(),bytes.size()},&profile,&error);};
    auto bytes=encode({5,0,2,13,2,1,1,4});check(load(bytes)&&director.load(profile,indices,&error),"owned GFDR strict profile and explicit stored render-index binding");
    for(unsigned step=0;step<2;++step)check(director.step(input,&effects,&error)&&director.state().cursor==0&&director.state().counter==2-int(step),"wait entry initializes without decrement, then one signed decrement per update");
    check(director.step(input,&effects,&error)&&director.state().terminal&&director.state().cursor==7,"wait completion runs following prefix in same update and terminal does not advance");
    bool group=false;for(const auto& e:effects)if(e.opcode==13&&e.kind==gf::StartupEffectKind::Command)group=e.actor==1&&e.render_group==94;
    check(group,"render group uses supplied actor[96]+41, not command-array slot+41");
    check(director.step(input,&effects,&error)&&effects.size()==1&&effects[0].kind==gf::StartupEffectKind::FrameEnd,"terminal handler side effect runs once, pointer not advanced/replayed");
    for(auto word:{gf::u16(0),gf::u16(0xffff),gf::u16(0x8000)}){bytes=encode({5,gf::u8(word>>8),gf::u8(word),4});check(load(bytes)&&director.load(profile,indices,&error)&&director.step(input,&effects,&error)&&!director.state().terminal,"signed wait entry always yields");
        check(director.step(input,&effects,&error)&&(word==0x8000?(!director.state().terminal&&director.state().counter==32767):director.state().terminal),"source signed16 wrap/CLZ-rotate completion means signed result<=0, not only zero");}
    bytes=encode({2,10,6,12,7,0,255,1,8,255,4});check(load(bytes)&&director.load(profile,indices,&error),"camera/gate fixture");input.fade_busy=true;
    check(director.step(input,&effects,&error)&&director.state().cursor==0,"fade observation blocks before all later commands");input.fade_busy=false;input.resources_busy=true;
    check(director.step(input,&effects,&error)&&director.state().cursor==1,"resource observation remains independent gate");input.resources_busy=false;input.camera_end[12]=2;
    check(director.step(input,&effects,&error)&&director.state().camera_frame==0&&director.state().camera_active==1,"camera request entry marks active but post-command update does not advance first time");
    check(director.step(input,&effects,&error)&&director.state().camera_frame==1&&!director.state().terminal,"camera advances after actor/update boundary, not before wait test");
    check(director.step(input,&effects,&error)&&director.state().camera_frame==2&&director.state().camera_active==0&&!director.state().terminal,"camera clamp/finish becomes visible to next prefix");
    check(director.step(input,&effects,&error)&&director.state().terminal,"camera wait resumes next update after completion");
    check(director.load(profile,indices,&error),"restore camera fixture");auto unknown=input;unknown.camera_end_valid[12]=false;
    check(!director.step(unknown,&effects,&error)&&effects.empty()&&!director.ready()&&director.state().cursor==0,"unresolved camera end refuses atomically; no partial external commands published");
    bytes=encode({14,0,14,1,4});check(load(bytes)&&director.load(profile,indices,&error)&&!director.step(input,&effects,&error)&&effects.empty()&&!director.ready(),"no-wait loop exhausts bounded prefix before any partial publication");
    bytes=encode({16,15,1,17,0,2,5,0,2,4});check(load(bytes)&&director.load(profile,indices,&error)&&director.step(input,&effects,&error)&&director.state().config_value==.25f,"configuration ramp starts after command/actor boundary");
    check(director.step(input,&effects,&error)&&director.state().config_value==0&&director.state().config_step==0,"configuration ramp clamps once and clears slope");
    bytes=encode({9,1,13,0,0,7,5,0,1,4});check(load(bytes)&&director.load(profile,indices,&error)&&director.step(input,&effects,&error),"actor order fixture");
    unsigned command=999,actor=999,finish=999;for(unsigned i=0;i<effects.size();++i){if(effects[i].kind==gf::StartupEffectKind::Command&&effects[i].opcode==13)command=i;if(effects[i].kind==gf::StartupEffectKind::ActorFrame)actor=i;if(effects[i].kind==gf::StartupEffectKind::FrameEnd)finish=i;}
    check(command<actor&&actor<finish,"motion commands precede actor update, with no actor-done gate");
    for(std::size_t size=0;size<bytes.size();++size)check(!gf::startup_director_load({bytes.data(),size},&profile,&error)&&!profile.loaded,"all truncated director profiles refuse atomically");
    for(unsigned at:{0u,4u,6u,7u,8u,12u,14u,16u,20u,24u,28u,88u}){auto bad=bytes;bad[at]=255;check(!load(bad)&&profile.script.empty(),"invalid headers/extents/opcode/counts refuse");}
    auto bad=encode({14,1,4});check(!load(bad),"unbound loop target refuses");bad=encode({17,0,0,4});check(!load(bad),"zero ramp denominator refuses instead of hiding nonfinite output");
    auto clip_bytes=character_program_fixture::bytes();gf::CharacterProgramAsset program;check(gf::character_program_load({clip_bytes.data(),clip_bytes.size()},0x12345678,&program,&error),"owned program for actor substate ordering");
    program.programs[0].bytes.insert(program.programs[0].bytes.end()-4,{2,255,1,0,0,0,0,0}); //owned control1 transition before terminal
    gf::CharacterProgramVm vm;gf::CharacterProgramActor initial;initial.default_group=5;check(vm.load(program,initial,{},&error),"owned post-binding VM");gf::StartupActorDispatch dispatch;character_program_fixture::Clock clock;std::vector<gf::CharacterProgramOperation> operations;std::vector<gf::StartupActorEffect> actor_effects;
    check(!dispatch.update(vm,1,clock,&operations,&actor_effects,&error),"unbound actor construction cannot be treated as implemented");dispatch.bind(7);check(dispatch.reset(&actor_effects)&&!actor_effects.empty(),"reset emits explicit world/fade work instead of invented transforms");
    check(dispatch.request(vm,0,-1,1,clock,&operations,&error)&&dispatch.state().render_group==72,"motion request resets actor start phase and uses stored render index");
    check(dispatch.update(vm,1,clock,&operations,&actor_effects,&error)&&operations.empty()&&vm.state().lanes[0].motion.frame==0&&vm.state().actor.control0[0]==2,"first actor update changes start substate only: no motion advancement or premature control consumption");
    check(dispatch.update(vm,1,clock,&operations,&actor_effects,&error)&&vm.state().lanes[0].motion.frame==1&&vm.state().actor.control0[0]==0,"motion substate updates VM then consumes control0 exactly once");
    bool required=false;for(auto e:actor_effects)required|=e.kind==gf::StartupActorEffectKind::Program7ERequired;check(required,"unported actor program7E is explicit required effect, not silently dropped");
    for(unsigned i=0;i<16&&dispatch.state().phase!=gf::StartupActorDispatchState::ScaleOut;++i)check(dispatch.update(vm,1,clock,&operations,&actor_effects,&error),"source actor motion dispatch progression");
    check(dispatch.state().phase==gf::StartupActorDispatchState::ScaleOut&&!dispatch.update(vm,1,clock,&operations,&actor_effects,&error)&&!dispatch.ready()&&operations.empty()&&actor_effects.empty(),"scale-out root-world branch refuses explicitly after source control1 transition");
    auto turn=program;turn.programs[0].bytes={1,5,0,0,0,0,0,0,2,255,4,0,0,0,0,0,0,1,255,255};
    check(vm.load(turn,initial,{},&error),"owned control4 program");dispatch.bind(7);
    check(dispatch.request(vm,0,-1,1,clock,&operations,&error)&&dispatch.update(vm,1,clock,&operations,&actor_effects,&error)&&dispatch.update(vm,1,clock,&operations,&actor_effects,&error)&&dispatch.state().facing_yaw==-32768,"source control4 writes signed angle8000 before requiring anchor effect0");
    bool anchor=false;for(auto e:actor_effects)anchor|=e.kind==gf::StartupActorEffectKind::AnchorEffect0Required;check(anchor,"world anchor effect remains explicit, never a fabricated zero-position effect");
    unsigned seed=0x77c011;bool atomic=true;
    for(unsigned n=0;n<512;++n){auto mutated=bytes;seed=seed*1664525u+1013904223u;const auto at=seed%mutated.size();seed=seed*1664525u+1013904223u;mutated[at]^=gf::u8((seed>>24)|1);const bool ok=load(mutated);atomic=atomic&&(ok||(!profile.loaded&&profile.script.empty()&&profile.command_offsets.empty()));}
    check(atomic,"512 deterministic malformed GFDR probes preserve bounded atomic refusal");
    unsigned real=0,frames=0,requests=0;
    auto actual=read("local/content/startup-director/startup.gfdr");if(!actual.empty()){
        check(load(actual)&&profile.script.size()==255&&profile.command_offsets.size()==91&&profile.actor_ids==std::vector<gf::s16>{0x615,0xa,-1,-1,-1,-1},"actual source director profile matches script/actor identity extent");
        std::array<gf::u8,6> supplied_indices={7,29,0,0,0,0};check(director.load(profile,supplied_indices,&error),"actual script with explicitly supplied render indices, not original runtime state trace");input=observations(96);
        for(unsigned tick=0;tick<8192;++tick){input.fade_busy=tick<3;input.resources_busy=tick<5;check(director.step(input,&effects,&error),"actual prefix/counter/gate step under declared external observations");++frames;
            for(const auto& e:effects)if(e.kind==gf::StartupEffectKind::Command&&e.opcode==13&&e.operands[0]==0)++requests;
            if(director.state().terminal)break;}
        check(director.state().terminal&&director.state().phase==12&&director.state().phase12&&requests==12,"director reaches terminal without waiting on any actor-motion-done signal");real=1;
    }
    std::printf("{\"startup_director_checks\":%u,\"source_profiles\":%u,\"observed_input_updates\":%u,\"motion_request_commands\":%u,\"actor_camera_effects_visual_startup_admitted\":false,\"all_passed\":true}\n",checks,real,frames,requests);
}
