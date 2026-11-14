#include <cstdio>
#include <cstdlib>
#include <limits>
#include "engine/gameplay/character_program_directional_control.hpp"
#include "tests/unit/character_motion_directional_fixture.hpp"
namespace {
using namespace gf;unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"program root: %s\n",why);std::abort();}}
bool near(f32 a,f32 b){return std::abs(a-b)<.00002f;}
CharacterPoseHeightProfile height(){CharacterPoseHeightProfile p;p.loaded=true;p.half=.5f;p.negative_factor=bits_to_f32(0xbd4ccccd);p.gain=bits_to_f32(0x3f333333);p.step=5;return p;}
struct Fixture:character_motion_directional_fixture::Fixture {
    CharacterProgramAsset p;unsigned root_lane=0;
    bool setup(unsigned lane,std::string& error){if(!load(&error))return false;root_lane=lane;if(lane)rig.order={1,0};p=profile;p.control_flow=true;for(auto& d:p.descriptors){d.group=0;d.bytes[19]=1;}
        for(auto& c:clips){for(unsigned k=1;k<3;++k){auto t=character_motion_driver_fixture::linear(0,c.clip.end_frame*f32(k+1),unsigned(c.clip.end_frame));t.channel=u8(5+k);c.clip.bones[0].tracks.push_back(t);}}
        p.programs={{0,0,{1,0,0,0,0,0,0,0,0,0,0,100,0,1,255,255}},
                    {0,1,{3,0,0,0,0,0,0,64,1,0,0,0,0,0,0,0,0,0,0,0,8,0,0,0,0,0,0,100,0,1,255,255}},
                    {0,2,{1,0,0,0,0,0,0,0,13,0,1,0,2,0,0,0,0,0,0,100,0,1,255,255}},
                    {0,3,{13,0,1,0,2,0,0,0,0,0,0,100,0,1,255,255}}};return true;
    }
    bool driver(CharacterMotionDriver& d,std::string& error)const{return d.load_directional_transitions({model.data(),model.size()},rig,p,clips,initial,{s8(root_lane?1:0),-1},&error);}
};
bool bind(CharacterProgramVm& vm,CharacterMotionDriver& driver,CharacterProgramDirectionalControl& control,bool world,std::string& error,std::span<const f32> multipliers={}){CharacterPoseHeightFields h;h.scale_b4=2;h.actor_y=vm.state().actor.root.position[1];h.height66c=8;h.radius670=2;h.middle_bone=2;h.world_positions_declared=world;h.behaviour5e0=vm.state().actor.behaviour;const f32 y[]={0,50,30};CharacterMotionRootFields r;r.scale_b4=2;r.delta5f4=1;r.multipliers1dd4=multipliers;return control.bind_with_root(vm,driver,height(),h,y,r,&error);}
unsigned count(const std::vector<CharacterProgramOperation>& ops,CharacterProgramOperationKind kind){unsigned n=0;for(const auto& op:ops)n+=op.kind==kind;return n;}
}
int main(){using namespace gf;std::string error;std::vector<CharacterProgramOperation> ops;
    for(unsigned root_lane:{0u,1u}){Fixture f;CharacterMotionDriver driver;CharacterProgramVm vm;CharacterProgramDirectionalControl control;check(f.setup(root_lane,error)&&f.driver(driver,error),"owned library and either root-owning lane");
        check(vm.load_fresh_actor_root_controls(f.p,{0,1,0},{-60,15,0},&error)&&bind(vm,driver,control,true,error),"explicit same-VM root/height binding");
        check(vm.request_with_actor_controls(0,0,1,0,1,driver,control,&ops,&error)&&vm.request_with_actor_controls(0,1,2,0,1,driver,control,&ops,&error),"independent lane programs and ordinary metadata setup");
        check(vm.state().actor.lane1_descriptor[2]==1&&vm.state().actor.root.updates==1&&vm.state().actor.root.position==std::array<f32,3>{-60,15,0},"selection clears root delta and stages source1D9B");
        check(vm.update_with_actor_controls(3,1,driver,control,&ops,&error)&&vm.state().actor.root.position==std::array<f32,3>{-58,19,6}&&count(ops,CharacterProgramOperationKind::RootMotion)==1,"ordinary root update commits with both native lanes");
        check(near(vm.state().actor.pose_height.offset6d0,root_lane?5.25f:3.85f),"lane-order opcode8 observes candidate position, never stale bound actorY");
        check(vm.refresh_height_with_actor_controls(control,&ops,&error)&&near(vm.state().actor.pose_height.offset6d0,3.85f),"later height refresh uses accepted root position");
        CharacterLocalPose pose;check(control.compose_local_pose(vm.state(),&pose,&error),"joined local pose remains native");CharacterPoseFrame frame;CharacterWorldPose world;check(character_pose_initial_frame(f.rig,mat4_translation(vm.state().actor.root.position.data()),&frame,&error)&&character_pose_direct_world(f.rig,pose,frame,&world,&error)&&world.matrices[0].at(1,3)==19,"candidate actor root feeds same direct-world kernel");
        const auto saved=vm.state();check(!vm.request_with_actor_controls(0,2,3,0,1,driver,control,&ops,&error)&&ops.empty()&&!vm.ready()&&vm.state().actor.root.updates==saved.actor.root.updates&&vm.state().actor.root.position==saved.actor.root.position&&vm.state().lanes[0].motion.context==saved.lanes[0].motion.context,"later directional root refusal rolls back prefix/root/models/both lanes");
        check(vm.load_fresh_actor_root_controls(f.p,{0,1,0},{-60,15,0},&error)&&bind(vm,driver,control,true,error)&&vm.request_with_actor_controls(0,0,3,0,1,driver,control,&ops,&error),"restore valid ordinary scope");
        const auto original=vm.state();CharacterMotionDriver neighbor;check(f.driver(neighbor,error)&&!vm.update_with_actor_controls(3,1,neighbor,control,&ops,&error)&&ops.empty()&&vm.state().actor.root.position==original.actor.root.position,"neighbor driver cannot update root state");
    }
    Fixture f;CharacterMotionDriver driver;CharacterProgramVm vm;CharacterProgramDirectionalControl control;check(f.setup(0,error)&&f.driver(driver,error)&&vm.load_fresh_actor_root_controls(f.p,{0,1,0},{-60,15,0},&error)&&bind(vm,driver,control,true,error)&&vm.request_with_actor_controls(0,0,1,0,1,driver,control,&ops,&error)&&vm.request_with_actor_controls(0,1,2,0,1,driver,control,&ops,&error),"late height failure setup");
    check(bind(vm,driver,control,false,error),"bind declared missing-world context without fake refresh");const auto before=vm.state();check(!vm.update_with_actor_controls(3,1,driver,control,&ops,&error)&&ops.empty()&&!vm.ready()&&vm.state().actor.root.position==before.actor.root.position&&vm.state().actor.root.updates==before.actor.root.updates&&vm.state().lanes[0].motion.context==before.lanes[0].motion.context,"secondary opcode8 failure discards earlier candidate root displacement");
    check(vm.load_fresh_actor_directional_controls(f.p,{0,1,0},&error)&&!bind(vm,driver,control,true,error),"old directional VM cannot silently acquire root binding");
    check(vm.load_fresh_actor_root_controls(f.p,{0,1,0},{-60,15,0},&error),"missing root consumer setup");CharacterPoseHeightFields h;h.scale_b4=2;check(control.bind(vm,driver,height(),h,{},CharacterProgramDirectionalControlMode::DeclaredHeightFields,&error)&&!vm.request_with_actor_controls(0,0,3,0,1,driver,control,&ops,&error)&&ops.empty(),"root VM rejects ordinary adapter without root result capability");
    check(!vm.load_fresh_actor_root_controls(f.p,{0,1,0},{0,std::numeric_limits<f32>::infinity(),0},&error)&&!vm.ready(),"nonfinite initial position refuses");
    f.p.descriptors[0].bytes[0]=1;check(f.driver(driver,error)&&vm.load_fresh_actor_root_controls(f.p,{0,1,0},{-60,15,0},&error),"owned selector table setup");std::vector<f32> multipliers={7,.5f};check(bind(vm,driver,control,true,error,multipliers)&&vm.request_with_actor_controls(0,0,3,0,1,driver,control,&ops,&error),"root fields copy selector multipliers");multipliers[1]=99;
    check(vm.update_with_actor_controls(3,1,driver,control,&ops,&error)&&vm.state().actor.root.position==std::array<f32,3>{-59,17,3},"caller multiplier edits cannot mutate bound candidate controls");
    const auto stale=control;check(vm.load_fresh_actor_root_controls(f.p,{0,1,0},{-60,15,0},&error)&&!vm.request_with_actor_controls(0,0,3,0,1,driver,stale,&ops,&error)&&ops.empty(),"reload retires prior root consumer scope");
    std::printf("scope-bound ordinary root/VM/height candidate join: %u checks PASS; full actor/directional root still refused\n",checks);return 0;
}
