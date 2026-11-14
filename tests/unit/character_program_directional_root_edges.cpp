#include <cstdio>
#include <cstdlib>
#include "tests/unit/character_program_directional_root_fixture.hpp"
namespace {
using namespace gf;using namespace character_program_directional_root_fixture;
unsigned checks=0;void check(bool x,const char* why){++checks;if(!x){std::fprintf(stderr,"directional VM root edge: %s\n",why);std::abort();}}
bool old_bind(CharacterProgramVm& vm,CharacterMotionDriver& d,CharacterProgramDirectionalControl& c,std::string& e){CharacterPoseHeightFields h;h.scale_b4=1;h.actor_y=vm.state().actor.root.position[1];CharacterMotionRootFields f;f.scale_b4=1;f.delta5f4=1;return c.bind_with_root(vm,d,height(),h,{},f,&e);}
struct OrdinaryOnly final:ICharacterProgramActorControls {
    const CharacterProgramDirectionalControl& c;mutable unsigned calls=0;
    explicit OrdinaryOnly(const CharacterProgramDirectionalControl& c):c(c){}
    std::weak_ptr<const CharacterProgramClassScope> scope()const override{return c.scope();}
    bool motion_driver_matches(const ICharacterProgramMotion& d)const override{return c.motion_driver_matches(d);}
    bool ordinary_root_controls()const override{return true;}
    bool pose_height_command(const CharacterProgramActor& a,u8 m,u8 o,CharacterPoseHeightState* r)const override{++calls;return c.pose_height_command(a,m,o,r);}
    bool pose_height_refresh(const CharacterProgramActor& a,CharacterPoseHeightState* r)const override{++calls;return c.pose_height_refresh(a,r);}
    bool motion_select(const CharacterProgramActor& a,unsigned l,const CharacterProgramDescriptor& d,bool mirror,const CharacterProgramMotion& p,f32 s,f32 dt,u8 m,u8 n,CharacterProgramMotionResult* r)const override{++calls;return c.motion_select(a,l,d,mirror,p,s,dt,m,n,r);}
    bool motion_advance(const CharacterProgramActor& a,unsigned l,const CharacterProgramMotion& p,f32 dt,CharacterProgramMotionResult* r)const override{++calls;return c.motion_advance(a,l,p,dt,r);}
};
}
int main(){using namespace gf;std::string e;std::vector<CharacterProgramOperation> ops;Fixture f;CharacterMotionDriver d;CharacterProgramVm vm;CharacterProgramDirectionalControl c;
    check(f.setup(0,e)&&f.driver(d,e)&&vm.load_fresh_actor_root_controls(f.p,{0,1,0},{0,10,0},&e),"old ordinary-root VM");
    check(!bind(vm,d,c,e)&&!c.directional_root_controls(),"new bind cannot silently enable old VM mode");
    check(old_bind(vm,d,c,e)&&vm.request_with_actor_controls(0,0,3,0,.5f,d,c,&ops,&e),"old root consumer remains usable");const auto old=vm.state();
    check(!vm.request_with_actor_controls(0,1,3,0,.5f,d,c,&ops,&e)&&ops.empty()&&vm.state().actor.root.updates==old.actor.root.updates,"old root entry still refuses root-lane13");
    check(vm.load_fresh_actor_directional_root_controls(f.p,{0,1,0},{0,10,0},&e)&&old_bind(vm,d,c,e)&&!vm.request_with_actor_controls(0,0,3,0,.5f,d,c,&ops,&e)&&ops.empty(),"new VM mode refuses ordinary-only root binding even on ordinary prefix");
    check(vm.load_fresh_actor_directional_root_controls(f.p,{0,1,0},{0,10,0},&e)&&bind(vm,d,c,e),"explicit reload before rebinding after refused candidate");OrdinaryOnly only(c);
    check(!vm.request_with_actor_controls(0,0,3,0,.5f,d,only,&ops,&e)&&ops.empty()&&only.calls==0,"request capability checked before any evaluator work");
    check(vm.request_with_actor_controls(0,0,3,0,.5f,d,c,&ops,&e)&&!vm.update_with_actor_controls(3,.5f,d,only,&ops,&e)&&ops.empty()&&only.calls==0,"update capability checked before any evaluator work");
    check(vm.request_with_actor_controls(0,0,3,0,.5f,d,c,&ops,&e)&&!vm.refresh_height_with_actor_controls(only,&ops,&e)&&ops.empty()&&only.calls==0,"height refresh cannot bypass new root capability");
    check(vm.load_fresh_actor_directional_root_controls(f.p,{0,1,0},{0,10,0},&e)&&bind(vm,d,c,e)&&!vm.request(0,0,3,0,.5f,d,&ops,&e)&&ops.empty(),"ordinary request entry cannot bypass scoped directional-root controls");
    check(vm.load_fresh_actor_directional_controls(f.p,{0,1,0},&e)&&!vm.actor_directional_root_control_mode()&&!c.directional_root_controls(),"old reload resets new-mode flag and retires scope");
    //Current lane1 global byte must override an older root-motion context.
    f.p.descriptors[0].bytes[19]=0;f.p.descriptors[1].bytes[19]=1;f.p.programs.push_back({0,7,{1,0,1,0,0,0,0,0,0,0,0,100,0,1,255,255}});
    check(f.driver(d,e)&&vm.load_fresh_actor_directional_root_controls(f.p,{0,1,0},{0,10,0},&e)&&bind(vm,d,c,e)&&vm.request_with_actor_controls(0,0,3,0,.5f,d,c,&ops,&e)&&steer(vm,8192,e)&&vm.request_with_actor_controls(0,1,1,0,.5f,d,c,&ops,&e),"root follow with source apply0");
    const auto no_apply=vm.state().actor.root.position;check(no_apply==std::array<f32,3>{0,10,0}&&vm.state().actor.lane1_descriptor[2]==0,"old global suppresses displacement");
    const auto root_context=vm.state().lanes[0].motion.context;check(vm.request_with_actor_controls(0,7,2,0,.5f,d,c,&ops,&e)&&vm.state().actor.lane1_descriptor[2]==1&&vm.state().lanes[0].motion.context==root_context,"other lane publishes new global without replacing root context");
    check(vm.update_with_actor_controls(1,.5f,d,c,&ops,&e)&&vm.state().actor.root.position!=no_apply&&c.directional_root_result(vm.state().lanes[0].motion)->callbacks[0].apply_position1d9b==1,"next root uses current candidate global, not stale root context");
    //Later lane failure rolls back the earlier root and primary model updates.
    f.p.descriptors[1].bytes[13]=99;check(f.driver(d,e)&&vm.load_fresh_actor_directional_root_controls(f.p,{0,1,0},{0,10,0},&e)&&bind(vm,d,c,e)&&vm.request_with_actor_controls(0,0,3,0,.5f,d,c,&ops,&e)&&steer(vm,8192,e),"missing later secondary clip fixture");const auto before=vm.state();
    check(!vm.request_with_actor_controls(0,1,3,0,.5f,d,c,&ops,&e)&&ops.empty()&&!vm.ready()&&vm.state().actor.root.position==before.actor.root.position&&vm.state().actor.root.updates==before.actor.root.updates&&vm.state().actor.primary_models==before.actor.primary_models&&vm.state().actor.models==before.actor.models&&vm.state().lanes[0].motion.context==before.lanes[0].motion.context&&vm.state().lanes[1].motion.context==before.lanes[1].motion.context,"whole two-lane/model/root transaction rolls back on later clip failure");
    //Null-root lane order is a clock provenance boundary, not a fake player0.
    Fixture n;check(n.setup(1,e),"null root fixture");n.rig.order={0,1};n.rig.cuts={0,0,2,2,2};for(auto& clip:n.clips){clip.clip.bones[0].has_aobj=false;clip.clip.bones[0].tracks.clear();}
    check(d.load_directional_transitions({n.model.data(),n.model.size()},n.rig,n.p,n.clips,n.initial,{-1,-1},&e)&&vm.load_fresh_actor_directional_root_controls(n.p,{0,1,0},{0,10,0},&e)&&bind(vm,d,c,e)&&!vm.request_with_actor_controls(0,1,3,0,.5f,d,c,&ops,&e)&&ops.empty(),"default AObj-only root remains strict in new VM mode");
    check(vm.load_fresh_actor_directional_root_controls(n.p,{0,1,0},{0,10,0},&e)&&bind(vm,d,c,e,true,{},CharacterMotionRootObservationMode::AObjOrPublicNull)&&steer(vm,8192,e)&&vm.request_with_actor_controls(0,1,3,0,.5f,d,c,&ops,&e),"explicit public-null mode joins new VM");
    const auto* trace=c.directional_root_result(vm.state().lanes[1].motion);check(trace&&trace->callbacks[0].after.clock_source==CharacterMotionRootClockSource::InheritedMotion&&trace->callbacks[0].after.frame>0&&vm.state().actor.root.position==std::array<f32,3>{0,10,0},"inherited positive clock retained without fabricated root displacement");
    check(vm.update_with_actor_controls(3,3.5f,d,c,&ops,&e)&&vm.update_with_actor_controls(3,.5f,d,c,&ops,&e)&&c.directional_root_result(vm.state().lanes[1].motion)->callbacks[0].after.frame!=vm.state().lanes[1].motion.frame,"root-first callback sees advanced clock before a stopped child restores its retained final lane frame");
    std::printf("directional-root capability/global/public-null edges: %u checks PASS\n",checks);return 0;
}
