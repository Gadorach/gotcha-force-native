#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/character_program_directional_control.hpp"
#include "tests/unit/character_motion_directional_fixture.hpp"
namespace {
using namespace gf;
unsigned checks=0;
void check(bool value,const char* why){++checks;if(!value){std::fprintf(stderr,"selection height: %s\n",why);std::abort();}}
CharacterPoseHeightProfile height(){CharacterPoseHeightProfile p;p.loaded=true;p.half=.5f;p.negative_factor=bits_to_f32(0xbd4ccccd);p.gain=bits_to_f32(0x3f333333);p.step=5;return p;}
CharacterPoseHeightFields fields(u8 family=6,bool world=true){CharacterPoseHeightFields f;f.model_group3e8=family;f.scale_b4=2;f.actor_y=10;f.height66c=8;f.radius670=2;f.middle_bone=2;f.world_positions_declared=world;return f;}
CharacterProgramAsset asset(character_motion_directional_fixture::Fixture& f){auto p=f.profile;for(auto& d:p.descriptors)d.group=0;p.control_flow=true;
    p.programs={{0,0,{3,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,100,0,1,255,255}},
                {0,1,{3,0,0,0,0,0,0,64,1,0,0,0,0,0,0,0,0,0,0,100,0,1,255,255}},
                {0,2,{3,0,0,0,0,0,0,64,13,0,1,0,2,0,0,0,0,0,0,100,0,1,255,255}},
                {0,3,{3,0,0,0,0,0,0,64,1,0,0,0,0,0,0,0,8,0,0,3,0,0,0,100,0,1,255,255}},
                {0,4,{3,0,0,0,0,0,0,64,1,0,0,0,0,0,0,0,13,0,99,0,2,0,0,0,0,0,0,100,0,1,255,255}}};return p;}
bool bind(CharacterProgramVm& vm,CharacterMotionDriver& driver,CharacterProgramDirectionalControl& control,const CharacterPoseHeightFields& f,std::string& error){const f32 y[]={0,30,20};return control.bind(vm,driver,height(),f,y,CharacterProgramDirectionalControlMode::DeclaredHeightFields,&error);}
unsigned count(const std::vector<CharacterProgramOperation>& ops,CharacterProgramOperationKind kind){unsigned n=0;for(const auto& op:ops)n+=op.kind==kind;return n;}
}
int main(int argc,char** argv){using namespace gf;character_motion_directional_fixture::Fixture fixture;std::string error;check(fixture.load(&error),"owned shared native motion fixture");const auto p=asset(fixture);CharacterMotionDriver driver;
    check(driver.load_directional_transitions({fixture.model.data(),fixture.model.size()},fixture.rig,p,fixture.clips,fixture.initial,{0,-1},&error),"shared driver/profile");
    CharacterProgramVm vm;CharacterProgramDirectionalControl control;std::vector<CharacterProgramOperation> ops;
    const auto enabled=CharacterProgramSelectionHeightMode::OrdinaryLane1;
    check(vm.load_fresh_actor_directional_controls(p,{3,.25f,99},&error,enabled)&&bind(vm,driver,control,fields(),error)&&vm.request_with_actor_controls(0,1,3,0,1,driver,control,&ops,&error),"explicit ordinary selection consumer");
    check(vm.state().actor.pose_height.mode71d==0&&vm.state().actor.pose_height.multiplier6d8==1&&vm.state().actor.pose_height.offset6d0==20,"evaluate OLD71D after multiplier reset, then outer clears71D");
    check(count(ops,CharacterProgramOperationKind::PoseHeightSelection)==1,"distinct selection-height journal");
    std::size_t selected=ops.size(),evaluated=ops.size(),reset=ops.size();for(std::size_t i=0;i<ops.size();++i){if(ops[i].kind==CharacterProgramOperationKind::Select&&ops[i].lane==1)selected=i;if(ops[i].kind==CharacterProgramOperationKind::PoseHeightSelection)evaluated=i;if(ops[i].kind==CharacterProgramOperationKind::ResetLane1)reset=i;}
    check(selected<evaluated&&evaluated<reset,"driver selection -> height -> handler reset journal order");
    check(vm.update_with_actor_controls(3,1,driver,control,&ops,&error)&&!count(ops,CharacterProgramOperationKind::PoseHeightSelection)&&vm.state().actor.pose_height.offset6d0==20,"ordinary advance does not repeat selection side effects");
    check(vm.refresh_height_with_actor_controls(control,&ops,&error)&&vm.state().actor.pose_height.offset6d0==3.5f,"next refresh observes cleared71D and reset multiplier");
    check(vm.load_fresh_actor_directional_controls(p,{3,.25f,99},&error)&&bind(vm,driver,control,fields(),error)&&vm.request_with_actor_controls(0,1,3,0,1,driver,control,&ops,&error)&&vm.state().actor.pose_height.multiplier6d8==.25f&&vm.state().actor.pose_height.offset6d0==99&&!count(ops,CharacterProgramOperationKind::PoseHeightSelection),"old directional entry remains handler-only");
    check(vm.load_fresh_actor_directional_controls(p,{3,.25f,99},&error,enabled)&&bind(vm,driver,control,fields(),error)&&vm.request_with_actor_controls(0,2,3,0,1,driver,control,&ops,&error)&&vm.state().actor.pose_height.multiplier6d8==.25f&&vm.state().actor.pose_height.offset6d0==99&&!count(ops,CharacterProgramOperationKind::PoseHeightSelection),"opcode13 bypasses ordinary48648 height effect");
    check(vm.load_fresh_actor_directional_controls(p,{3,.25f,99},&error,enabled)&&bind(vm,driver,control,fields(6,false),error)&&vm.request_with_actor_controls(0,1,3,0,1,driver,control,&ops,&error),"explicit nonzero mode does not invent world requirements");
    check(vm.load_fresh_actor_directional_controls(p,{0,.25f,99},&error,enabled)&&bind(vm,driver,control,fields(6,false),error),"missing-world dynamic setup");
    check(!vm.request_with_actor_controls(0,1,3,0,1,driver,control,&ops,&error)&&ops.empty()&&!vm.ready()&&!vm.state().lanes[0].motion.selected&&!vm.state().lanes[1].motion.selected&&vm.state().actor.models==std::array<u8,2>{}&&vm.state().actor.pose_height.multiplier6d8==.25f&&vm.state().actor.pose_height.offset6d0==99,"late height failure rolls back both selected lanes/models/height/journal");
    check(vm.load_fresh_actor_directional_controls(p,{3,.25f,99},&error,enabled)&&bind(vm,driver,control,fields(),error)&&!vm.request_with_actor_controls(0,4,3,0,1,driver,control,&ops,&error)&&ops.empty()&&vm.state().actor.pose_height.mode71d==3&&vm.state().actor.pose_height.offset6d0==99,"later directional failure rolls back successful selection height");
    check(vm.load_fresh_actor_directional_controls(p,{0,.25f,99},&error,enabled)&&bind(vm,driver,control,fields(),error)&&vm.request_with_actor_controls(0,3,3,0,1,driver,control,&ops,&error)&&vm.state().actor.pose_height.mode71d==3&&vm.state().actor.pose_height.offset6d0==20&&count(ops,CharacterProgramOperationKind::PoseHeightSelection)==1&&count(ops,CharacterProgramOperationKind::PoseHeight)==1,"later opcode8 observes selection state, not reordered before it");
    const auto stale=control;check(vm.load_fresh_actor_directional_controls(p,{3,.25f,99},&error,enabled)&&!vm.request_with_actor_controls(0,1,3,0,1,driver,stale,&ops,&error)&&ops.empty(),"reloaded scope refuses old consumer");
    check(!vm.load_fresh_actor_directional_controls(p,{0,1,0},&error,CharacterProgramSelectionHeightMode(99))&&!vm.ready()&&!vm.actor_directional_control_mode(),"unknown selection mode refuses binding");
    FILE* journal=argc==2?std::fopen(argv[1],"wb"):nullptr;check(argc==1||journal,"journal output");
    unsigned rows=0;for(u8 mask:{u8(1),u8(2),u8(3)})for(u8 mode:{u8(0),u8(1),u8(2)})for(u8 initial:{u8(0),u8(1),u8(3),u8(255)})for(u8 family:{u8(0),u8(5),u8(6)})for(u8 behaviour:{u8(0),u8(1)}){
        check(vm.load_fresh_actor_directional_controls(p,{initial,.25f,99},&error,enabled)&&bind(vm,driver,control,fields(family),error)&&vm.request_with_actor_controls(0,behaviour,mask,0,1,driver,control,&ops,&error,{s16(mode?2:0),s8(mode)}),"owned mask/mode/family/candidate-behaviour matrix");
        const auto& h=vm.state().actor.pose_height;const bool secondary=(mask&2)!=0;const f32 expected=!secondary?99:(family==5||!behaviour?0:(initial?f32(10*(s32(std::bit_cast<s8>(initial))-1)):3.5f));
        check(h.mode71d==(secondary?0:initial)&&h.multiplier6d8==(secondary?1:.25f)&&h.offset6d0==expected&&count(ops,CharacterProgramOperationKind::PoseHeightSelection)==unsigned(secondary),"branch-local selection arithmetic and mask");
        if(journal)std::fprintf(journal,"%u %u %u %u %u %u %08x %08x %u\n",mask,mode,initial,family,behaviour,h.mode71d,f32_to_bits(h.multiplier6d8),f32_to_bits(h.offset6d0),count(ops,CharacterProgramOperationKind::PoseHeightSelection));
        ++rows;
    }
    if(journal)check(std::fclose(journal)==0,"journal close");
    std::printf("ordinary lane1 selection height: %u checks PASS; %u owned rows; not full actor/demo admission\n",checks,rows);return 0;
}
