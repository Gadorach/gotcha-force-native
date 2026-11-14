#include <cstdio>
#include <cstdlib>
#include <vector>
#include "engine/gameplay/force_spec_actor_actions.hpp"
namespace {using namespace gf;unsigned checks=0;void check(bool x,const char* why){++checks;if(!x){std::fprintf(stderr,"spec action source sentinel: %s\n",why);std::abort();}}
std::vector<u8> profile(){std::vector<u8> b(32);store_be32(b.data(),0x47464152);store_be16(b.data()+4,1);store_be16(b.data()+6,32);store_be32(b.data()+8,456);store_be16(b.data()+12,0x615);store_be16(b.data()+14,10);b[16]=b[17]=1;b[18]=60;b[19]=15;b[21]=40;store_be16(b.data()+24,0x800);for(unsigned i=28;i<32;++i)b[i]=255;return b;}}
int main(){using namespace gf;auto b=profile();ForceSpecActorActionProfile p;ForceSpecActorActionPlan plan;ForceSpecActorActionState state;ForceSpecActorActionInput in;std::string e;
    check(force_spec_actor_action_load({b.data(),b.size()},456,&p,&e)&&p.start==-1&&p.duration==-1&&p.mode==-1,"source57FF8 start/duration/mode sentinels independently bound");
    auto bad=b;store_be16(bad.data()+30,0);check(!force_spec_actor_action_load({bad.data(),bad.size()},456,&p,&e)&&!p.loaded,"old guessed frame0 profile is now rejected");
    check(force_spec_actor_action_load({b.data(),b.size()},456,&p,&e),"restore correct source profile");in.model_id=0x615;in.base_position1c={-60,15,0};in.actor_yaw72=-9876;in.actor_index96=251;in.pressed=0x100;in.held=0x900000;
    check(force_spec_actor_action_plan(p,state,in,&plan,&e)&&plan.action==ForceSpecActorAction::Demonstrate&&plan.start==-1&&plan.duration==-1&&plan.mode==-1&&plan.request_mask==15&&plan.request_choice==40,"demo preserves authored start instead of forcing0");
    check(plan.root_yaw_write&&plan.root_yaw5ae==-9876&&plan.actor_yaw72==0x180&&plan.render_group82==60&&plan.state.cooldown6c==60,"save old yaw before new UI yaw and preserve byte render-index wrap");
    state=plan.state;in.pressed=0x500;check(force_spec_actor_action_plan(p,state,in,&plan,&e)&&plan.action==ForceSpecActorAction::Reset&&plan.start==-1&&plan.state.cooldown6c==0&&!plan.root_yaw_write&&plan.actor_yaw72==0,"reset during cooldown is independent of VM done and preserves5AE");
    state={};auto unavailable=p;unavailable.demonstration[0]=0;check(force_spec_actor_action_plan(unavailable,state,in,&plan,&e)&&plan.action==ForceSpecActorAction::None&&plan.state.cooldown6c==0,"eligible but unavailable A swallows simultaneous reset");
    state.cooldown6c=1;check(force_spec_actor_action_plan(p,state,in,&plan,&e)&&plan.action==ForceSpecActorAction::Demonstrate,"last cooldown callback decrements before A priority");
    in.entry6b=1;state.cooldown6c=12;check(force_spec_actor_action_plan(p,state,in,&plan,&e)&&plan.state.cooldown6c==12&&plan.action==ForceSpecActorAction::None&&plan.actor_callback_required&&plan.camera_callback_required&&!plan.actor_yaw_write,"entry lock defers actions without fabricating actor/camera omission");
    in.entry6b=2;check(force_spec_actor_action_plan(p,state,in,&plan,&e)&&plan.action==ForceSpecActorAction::Reset,"only source6B1 locks, not every nonzero value");
    in.entry6b=0;in.model_id=0xffff;in.pressed=0x100;check(!force_spec_actor_action_plan(p,{},in,&plan,&e)&&!plan.request_mask&&!plan.position_write,"unknown consumed capability refuses candidate");
    in.pressed=0x800;check(force_spec_actor_action_plan(p,{},in,&plan,&e)&&plan.request_choice==0&&plan.start==-1,"reset neither queries capability nor reloads actor resources");
    in.hidden67=1;check(force_spec_actor_action_plan(p,state,in,&plan,&e)&&!plan.actor_callback_required&&!plan.camera_callback_required&&!plan.actor_yaw_write&&plan.state.angles.pitch==state.angles.pitch+0x180,"held accumulation precedes hidden gate");
    in.hidden67=0;in.component17=1;check(force_spec_actor_action_plan(p,state,in,&plan,&e)&&plan.actor_callback_required&&!plan.camera_callback_required&&!plan.request_mask,"component branch still requires actor callback, not actor action");
    std::printf("source-bound spec action start/reset edges: %u checks PASS; requests remain unfulfilled plans\n",checks);return 0;
}
