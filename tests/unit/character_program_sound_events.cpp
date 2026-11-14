#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/character_program_sound_events.hpp"
#include "tests/unit/character_motion_directional_fixture.hpp"
namespace {
using namespace gf;unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"program sound events: %s\n",why);std::abort();}}
CharacterPoseHeightProfile height(){CharacterPoseHeightProfile p;p.loaded=true;p.half=.5f;p.negative_factor=bits_to_f32(0xbd4ccccd);p.gain=bits_to_f32(0x3f333333);p.step=5;return p;}
std::vector<u8> bank_bytes(u32 hash,bool second=true){std::vector<u8> raw(36);store_be32(raw.data(),0x47465345);store_be16(raw.data()+4,1);store_be16(raw.data()+6,second?2:1);store_be32(raw.data()+12,hash);store_be16(raw.data()+16,0x615);store_be32(raw.data()+24,0x3e4ccccd);store_be32(raw.data()+28,0x3f333333);store_be32(raw.data()+32,0x40000000);
    //OWNED scripts: elapsed at2 then frame3; second lane frame3/size branch.
    const u8 rows[]={24,2,0,0,0,2,0,0,0,36,0,0,0,3,1,0,0,37,255,0};raw.insert(raw.end(),std::begin(rows),std::end(rows));
    if(second){const u8 row[]={25,1,0,0,0,3,1,1,0,18,255,0};raw.insert(raw.end(),std::begin(row),std::end(row));}
    store_be32(raw.data()+8,u32(raw.size()));return raw;
}
bool bind(CharacterProgramVm& vm,CharacterMotionDriver& driver,CharacterProgramDirectionalControl& control,std::string& error){CharacterPoseHeightFields f;f.scale_b4=1;return control.bind(vm,driver,height(),f,{},CharacterProgramDirectionalControlMode::DeclaredHeightFields,&error);}
}
int main(){using namespace gf;character_motion_directional_fixture::Fixture f;std::string error;check(f.load(&error),"owned fixture");auto p=f.profile;p.control_flow=true;for(auto& d:p.descriptors){d.group=0;d.bytes[8]=24;d.bytes[16]=25;d.bytes[6]|=4;d.bytes[7]=1;d.bytes[14]|=4;d.bytes[15]=1;}
    p.programs={{0,0,{1,0,0,1,3,0,0,0,0,0,0,100,0,1,255,255}},{0,1,{13,0,1,0,2,0,0,0,0,0,0,100,0,1,255,255}}};
    CharacterMotionDriver driver;CharacterProgramVm vm;CharacterProgramDirectionalControl control;std::vector<CharacterProgramOperation> ops;
    check(driver.load_directional_transitions({f.model.data(),f.model.size()},f.rig,p,f.clips,f.initial,{0,-1},&error),"shared driver");
    check(vm.load_fresh_actor_directional_controls(p,{0,1,0},&error)&&bind(vm,driver,control,error),"same VM bound control");ActorSoundEventBank bank;auto raw=bank_bytes(p.model_hash);check(bank.load({raw.data(),raw.size()},p.model_hash,&error),"owned event bank");std::vector<StartupSoundPendingCall> calls;
    check(character_program_sound_events(vm,control,bank,1,77,&calls,&error)&&calls.empty(),"fresh sentinel lanes do not fabricate clocks");
    check(vm.request_with_actor_controls(0,0,3,0,1,driver,control,&ops,&error)&&character_program_sound_events(vm,control,bank,1,77,&calls,&error)&&calls.empty(),"frozen target initial request has no events");
    check(vm.update_with_actor_controls(3,1,driver,control,&ops,&error)&&character_program_sound_events(vm,control,bank,1,77,&calls,&error)&&calls.empty(),"elapsed1/frozen frame0");
    check(vm.update_with_actor_controls(3,1,driver,control,&ops,&error)&&character_program_sound_events(vm,control,bank,1,77,&calls,&error)&&calls.size()==1&&calls[0].cue==36&&calls[0].kind==StartupSoundCallKind::StartDirect&&calls[0].owner==77,"elapsed event fires despite frozen animation frame");
    for(unsigned tick=0;tick<4;++tick)check(vm.update_with_actor_controls(3,1,driver,control,&ops,&error),"advance after elapsed event");
    const auto* snapshot=control.snapshot(vm.state().lanes[0].motion);check(snapshot&&snapshot->previous_frame()==2&&vm.state().lanes[0].motion.frame==3,"actual native previous/current clocks");
    check(character_program_sound_events(vm,control,bank,.5f,77,&calls,&error)&&calls.size()==2&&calls[0].cue==37&&calls[1].cue==17,"primary lane order and source actorB4 size branch");
    const auto before=vm.state();check(character_program_sound_events(vm,control,bank,3,77,&calls,&error)&&calls.size()==2&&calls[1].cue==19&&vm.state().lanes[0].motion.context==before.lanes[0].motion.context,"pure event calculation does not advance/acknowledge VM");
    ActorSoundEventBank missing;raw=bank_bytes(p.model_hash,false);check(missing.load({raw.data(),raw.size()},p.model_hash,&error)&&!character_program_sound_events(vm,control,missing,1,77,&calls,&error)&&calls.empty()&&vm.ready(),"later missing script discards earlier lane events without altering accepted VM");
    check(!character_program_sound_events(vm,control,bank,0,77,&calls,&error)&&calls.empty(),"invalid consumed actorB4 refuses");
    check(!character_program_sound_events(vm,control,bank,1,0,&calls,&error)&&calls.empty(),"owner required, no invented global actor");
    CharacterProgramVm neighbor;CharacterProgramDirectionalControl neighbor_control;check(neighbor.load_fresh_actor_directional_controls(p,{0,1,0},&error)&&bind(neighbor,driver,neighbor_control,error)&&!character_program_sound_events(vm,neighbor_control,bank,1,77,&calls,&error)&&calls.empty(),"same-model neighbor VM scope refused");
    ActorSoundEventBank foreign;raw=bank_bytes(p.model_hash^1);check(foreign.load({raw.data(),raw.size()},p.model_hash^1,&error)&&!character_program_sound_events(vm,control,foreign,1,77,&calls,&error)&&calls.empty(),"foreign model bank refused");
    check(vm.update_with_actor_controls(3,1,driver,control,&ops,&error)&&character_program_sound_events(vm,control,bank,1,77,&calls,&error)&&calls.empty(),"loop rewind uses native prior frame plus end, not elapsed cue replay");
    check(vm.request_with_actor_controls(0,1,3,0,1,driver,control,&ops,&error)&&character_program_sound_events(vm,control,bank,1,77,&calls,&error)&&calls.empty(),"directional request reads current primary, not alternate events");
    check(vm.load_fresh_actor_directional_controls(p,{0,1,0},&error)&&!character_program_sound_events(vm,control,bank,1,77,&calls,&error)&&calls.empty(),"reload retires old event-control scope");
    std::printf("native primary motion -> shared sound-event rule: %u checks PASS; no sound delivery/actor completion acknowledgement\n",checks);return 0;
}
