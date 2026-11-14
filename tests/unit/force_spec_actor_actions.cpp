#include <cstdio>
#include <cstdlib>
#include <limits>
#include <vector>
#include "engine/gameplay/force_spec_actor_actions.hpp"
namespace {
using namespace gf;unsigned checks=0,cases=0;void check(bool x,const char* why){++checks;if(!x){std::fprintf(stderr,"spec actor action plan: %s\n",why);std::abort();}}
std::vector<u8> profile(){std::vector<u8> b(32);store_be32(b.data(),0x47464152);store_be16(b.data()+4,1);store_be16(b.data()+6,32);store_be32(b.data()+8,123);store_be16(b.data()+12,0x615);store_be16(b.data()+14,10);b[16]=b[17]=1;b[18]=60;b[19]=15;b[21]=40;store_be16(b.data()+24,0x800);b[28]=b[29]=255;store_be16(b.data()+30,0xffff);return b;}
}
int main(){using namespace gf;std::string e;auto b=profile();ForceSpecActorActionProfile p;check(force_spec_actor_action_load({b.data(),b.size()},123,&p,&e),"owned scene-bound action profile");for(unsigned i=0;i<b.size();++i){auto bad=b;bad[i]^=64;ForceSpecActorActionProfile rejected;check(!force_spec_actor_action_load({bad.data(),bad.size()},123,&rejected,&e)&&!rejected.loaded,"profile extent/scene/source operand byte guard");}
    for(unsigned available=0;available<2;++available)for(unsigned gate=0;gate<5;++gate)for(unsigned raw=0;raw<256;++raw)for(unsigned buttons=0;buttons<8;++buttons){auto declared=p;declared.demonstration[0]=u8(available);ForceSpecActorActionState s;s.cooldown6c=std::bit_cast<s8>(u8(raw));s.angles={0xffffff00u,0xffffff80u};ForceSpecActorActionInput in;in.model_id=0x615;in.actor_index96=u8(raw);in.actor_yaw72=-4096;in.base_position1c={-60,15,0};in.pressed=0x200u|((buttons&1)?0x100u:0)|((buttons&2)?0x400u:0)|((buttons&4)?0x800u:0);in.held=0x800000u|0x100000u;
        if(gate==1)in.hidden67=128;else if(gate==2)in.component17=1;else if(gate==3)in.entry6b=1;else if(gate==4)in.entry6b=2;
        ForceSpecActorActionPlan plan;check(force_spec_actor_action_plan(declared,s,in,&plan,&e),"numeric signed cooldown/gate/button case");const bool active=gate==0||gate==4;int cooldown=int(s.cooldown6c);if(active&&cooldown>0)--cooldown;unsigned action=0;if(active){if(cooldown==0&&(buttons&1)){if(available){action=1;cooldown=60;}}else if(buttons&6){action=2;cooldown=0;}}
        check(unsigned(plan.action)==action&&plan.state.cooldown6c==cooldown,"decrement-before-test, A-before-reset and unavailable-A no fallthrough");
        const ForceSpecAngles expected=action==2?ForceSpecAngles{0x800,0}:ForceSpecAngles{u32(s.angles.pitch+0x180u),u32(s.angles.yaw+0x180u)};
        check(plan.state.angles.pitch==expected.pitch&&plan.state.angles.yaw==expected.yaw,"held accumulators update before every gate; reset replaces them");
        check(plan.actor_callback_required==(gate!=1)&&plan.camera_callback_required==(gate!=1&&gate!=2)&&plan.actor_yaw_write==active,"source callback requirements and yaw-write gates remain distinct");
        check(!active||plan.actor_yaw72==std::bit_cast<s16>(u16(expected.yaw)),"actor current yaw write follows optional request");
        if(action){check(plan.position_write&&plan.mode_bytes_write&&plan.position20==in.base_position1c&&plan.request_group==0&&plan.request_choice==(action==1?40:0)&&plan.request_mask==15&&plan.start==-1&&plan.duration==-1&&plan.mode==-1&&plan.phase19==0&&plan.render_group82==u8(raw+65),"request and field plan exactly preserves57FF8 defaults and index wrap");check(plan.field5da==(action==1?60:0)&&plan.field5db==plan.field5da&&plan.root_yaw_write==(action==1),"60mode writes are not countdown; only demo copies5AE");if(action==1)check(plan.root_yaw5ae==in.actor_yaw72,"demo saves prior actor yaw, not newly accumulated camera yaw");}
        else check(!plan.position_write&&!plan.mode_bytes_write&&!plan.root_yaw_write&&!plan.request_mask,"no fabricated actor request/write when inactive or gated");
        ++cases;
    }
    ForceSpecActorActionInput in;in.model_id=10;in.pressed=0x100;in.base_position1c={-60,15,0};ForceSpecActorActionState state;ForceSpecActorActionPlan plan;
    check(force_spec_actor_action_plan(p,state,in,&plan,&e)&&plan.action==ForceSpecActorAction::Demonstrate&&plan.state.cooldown6c==60,"both explicitly audited normal models can request demo");state=plan.state;
    for(unsigned i=0;i<59;++i){check(force_spec_actor_action_plan(p,state,in,&plan,&e)&&plan.action==ForceSpecActorAction::None,"cooldown callback, not game FPS");state=plan.state;}
    check(force_spec_actor_action_plan(p,state,in,&plan,&e)&&plan.action==ForceSpecActorAction::Demonstrate&&plan.state.cooldown6c==60,"counter1 decrements to0 before same-callback A retry");
    state=plan.state;in.pressed=0xc00;check(force_spec_actor_action_plan(p,state,in,&plan,&e)&&plan.action==ForceSpecActorAction::Reset&&plan.state.cooldown6c==0&&!plan.root_yaw_write,"reset can interrupt a running demo immediately, no VM done/60-counter wait required");
    s16 saved_yaw=-12345;if(plan.root_yaw_write)saved_yaw=plan.root_yaw5ae;check(saved_yaw==-12345&&plan.actor_yaw72==0,"reset preserves5AE while writing current actor yaw0");
    in.model_id=0xffff;in.pressed=0x100;check(!force_spec_actor_action_plan(p,{},in,&plan,&e)&&plan.action==ForceSpecActorAction::None&&!plan.request_mask,"unknown model capability refuses on consumed A branch");
    in.pressed=0x400;check(force_spec_actor_action_plan(p,{},in,&plan,&e)&&plan.action==ForceSpecActorAction::Reset,"reset does not consume a model capability lookup");
    in.model_id=0x615;in.pressed=0;in.base_position1c[0]=std::numeric_limits<f32>::quiet_NaN();check(force_spec_actor_action_plan(p,{},in,&plan,&e),"unused position does not fabricate a request dependency");in.pressed=0x100;check(!force_spec_actor_action_plan(p,{},in,&plan,&e)&&!plan.position_write&&!plan.request_mask,"consumed nonfinite source base position refuses whole plan");
    check(!force_spec_actor_action_plan(p,{},in,nullptr,&e),"missing output refuses");
    std::printf("source spec demo/reset action planning: %u checks/%u numeric cases PASS; no actor request/delivery acknowledged\n",checks,cases);return 0;
}
