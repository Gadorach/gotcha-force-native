#include <cstdio>
#include <cstdlib>
#include "tests/unit/character_preview_actions_fixture.hpp"
namespace {
using namespace gf;using namespace character_preview_actions_fixture;
unsigned checks=0;std::string error;
void check(bool x,const char* why){++checks;if(!x){std::fprintf(stderr,"demo receiver: %s (%s)\n",why,error.c_str());std::abort();}}
struct Demo:Fixture {
    bool setup(unsigned kind=0){if(!load(&error))return false;CharacterProgramAsset p;const auto a=assets();if(!character_program_load_control_flow(a.program,character_program_model_hash(a.model),&p,&error))return false;
        for(auto& q:p.programs)if(!q.group){if(q.choice==0)q.bytes={1,0,0,2,10,0,0,0,2,255,0,0,0,0,0,0,3,0,0,0,0,0,0,16,0,0,0,100,0,1,255,255};
            if(q.choice==40){q.bytes={1,0,1,1,5,0,0,0,2,255,255,0,1,0,0,0,3,0,0,0,0,0,0,0,0,0,0,100,0,1,255,255};
                if(kind==1)q.bytes={13,0,4,1,4,0,0,0,2,255,1,0,1,0,0,0,0,9,0,2}; //owned timer, not original wait
                if(kind==2)q.bytes[11]=1; //unsupported1CF0
                if(kind==3)q.bytes[13]=1; //unconsumed6E9 class effect
                if(kind==4)q.bytes[2]=3; //missing downstream event26
                if(kind==5)q.bytes={1,0,1,0,0,0,0,0,0,0,0,1,6,0,255,254,0,9,0,2}; //owned branch pulse
                if(kind==6)q.bytes={1,0,1,0,0,0,0,0,11,1,1,0,0,0,0,100,0,1,255,255};
            }}
        program=character_preview_controls_fixture::encode(p);return character_program_detail::valid_control_flow(p);
    }
};
bool load_demo(CharacterPreviewFrame& actor,const Demo& f,CharacterPreviewOutput& out){return actor.load_with_demo_actor_controls(f.assets(),character_preview_height_fixture::preview(),0,character_preview_height_fixture::height(),f.event_view(),{},&out,&error);}
}
int main(){
    Demo f;CharacterPreviewFrame actor;CharacterPreviewOutput out;CharacterPreviewActionOutput action;
    check(f.setup()&&load_demo(actor,f,out),"explicit normal preview constructor with real idle control10");
    check(actor.state()->frames==1&&actor.state()->vm.completed_directional_follow_tail_mode()&&actor.state()->vm.state().actor.behaviour==0x10,"constructor once and explicit follow-tail capability, behavior not erased");
    check(actor.step({0x1800,0x4000},&out,&error)&&actor.bind_actor_actions(profile(),&error),"adopt existing model/VM/current orientation");
    const auto* model=actor.model();const auto binding=model->instance_binding();CharacterProgramRootFields fields;check(actor.state()->vm.observe_root_fields(&fields),"capture same VM scope");const auto scope=fields.scope.lock();const auto prior=*actor.state();
    check(actor.step_actor_actions(0x100000,0xd00,&action,&error)&&actor.action_start_current(action)&&action.performed==ForceSpecActorAction::Demonstrate&&!action.unfulfilled_actions,"A beats simultaneous X/Y, actual scoped start not completion");
    check(actor.model()==model&&actor.model()->instance_binding()==binding&&actor.state()->vm.observe_root_fields(&fields)&&fields.scope.lock()==scope&&actor.state()->frames==prior.frames+1,"same actor and VM, no idle reconstruction");
    check(actor.state()->root_yaw5ae==0x4000&&actor.state()->controls.yaw!=0x4000&&actor.state()->actor_actions.cooldown6c==60&&actor.state()->scalars.field5da==60&&actor.state()->scalars.field5db==60,"save PRIOR yaw before current rotation; commit60 fields/cooldown");
    check(count(action.preview,CharacterProgramOperationKind::RootPositionRestore)==1&&action.preview.motion.front().kind==CharacterProgramOperationKind::RootPositionRestore&&count(action.preview,CharacterProgramOperationKind::Select)==2,"restore journal precedes actual authored0/40 selections");
    check(actor.state()->vm.state().actor.control0==std::array<u8,4>{255,0,1,0}&&actor.state()->vm.state().actor.behaviour==0&&actor.state()->scalars.field708==0,"VM markers/child-mode preserved; no fake clear or auxiliary ACK");
    check(actor.state()->action_transitions[0].blending&&actor.state()->action_transitions[0].mode==1&&actor.state()->action_transitions[0].duration==6,"authored mode1 duration5, never constructor override");
    const auto receipt=action;auto forged=action;forged.start.reset();check(!actor.action_start_current(forged),"public copied fields without owner receipt grant no start");forged=action;forged.performed=forged.requested=ForceSpecActorAction::Reset;check(!actor.action_start_current(forged),"receipt cannot change action kind");
    check(actor.step_actor_actions(0,0x100,&action,&error)&&action.performed==ForceSpecActorAction::None&&actor.state()->actor_actions.cooldown6c==59&&!actor.action_start_current(receipt)&&!actor.action_start_current(action),"cooldown ignores early A, next frame expires start receipt");
    const auto saved_yaw=actor.state()->root_yaw5ae;check(actor.step_actor_actions(0,0x500,&action,&error)&&action.performed==ForceSpecActorAction::Reset&&actor.action_start_current(action)&&actor.reset_action_current(action),"positive cooldown A ineligible, simultaneous X reset still interrupts");
    check(actor.state()->root_yaw5ae==saved_yaw&&actor.state()->controls.yaw==0&&actor.state()->actor_actions.cooldown6c==0&&actor.state()->scalars.field5da==0&&actor.state()->scalars.field5db==0&&actor.state()->setup.position==character_preview_height_fixture::preview().position,"reset keeps5AE/restores position and0 fields without wait completion");
    check(actor.state()->action_transitions[0].mode==2&&actor.state()->action_transitions[0].duration==11&&actor.state()->vm.state().actor.behaviour==0x10&&actor.model()==model,"authored reset2/10 on same actor and preview behavior10 retained");
    check(actor.step_actor_actions(0,0x100,&action,&error)&&actor.action_start_current(action),"new request immediately after reset");
    for(unsigned i=1;i<60;++i){check(actor.step_actor_actions(0,0x100,&action,&error)&&action.performed==ForceSpecActorAction::None&&actor.state()->actor_actions.cooldown6c==s8(60-i),"60-callback cooldown decrements before eligibility, not program-done gate");}
    check(actor.step_actor_actions(0,0x100,&action,&error)&&actor.action_start_current(action)&&actor.state()->actor_actions.cooldown6c==60,"A restarts at exact60 boundary although held program unfinished");
    const auto current=action;CharacterPreviewFrame moved=std::move(actor);check(!actor.action_start_current(current)&&moved.action_start_current(current),"move retains owner identity only in destination");
    CharacterPreviewFrame neighbor;check(load_demo(neighbor,f,out)&&neighbor.bind_actor_actions(profile(),&error)&&!neighbor.action_start_current(current),"same-model neighbor cannot consume receipt");moved.retire();check(!moved.action_start_current(current),"retired owner rejects receipt");
    for(unsigned kind:{1u,5u}){Demo timed;CharacterPreviewFrame a;check(timed.setup(kind)&&load_demo(a,timed,out)&&a.bind_actor_actions(profile(),&error)&&a.step_actor_actions(0,0x100,&action,&error),"owned follow/branch demo start");bool branch=count(action.preview,CharacterProgramOperationKind::ProgramBranch)!=0;bool tail=false;
        for(unsigned tick=0;tick<8;++tick){const auto before=*a.state();check(a.step_actor_actions(0,0,&action,&error),"owned subsequent motion/timer callback");branch=branch||count(action.preview,CharacterProgramOperationKind::ProgramBranch);if(before.vm.state().lanes[0].done&&before.vm.state().lanes[0].following){tail=true;check(a.state()->vm.state().lanes[0].clock==before.vm.state().lanes[0].clock&&a.state()->vm.state().lanes[0].motion.elapsed>before.vm.state().lanes[0].motion.elapsed,"completed-follow motion advances with program clocks held");}}
        check(kind==1?tail:branch,"owned coverage actually reaches tail or branch, not just starts");check(a.step_actor_actions(0,0x800,&action,&error)&&a.action_start_current(action),"Y reset independent of done/tail/pulse");
    }
    for(unsigned kind:{2u,3u,4u,6u}){Demo bad;CharacterPreviewFrame a;check(bad.setup(kind)&&load_demo(a,bad,out)&&a.bind_actor_actions(profile(),&error),"owned failing action prerequisites");const auto* live=a.state();const auto before=*live;check(!a.step_actor_actions(0,0x100,&action,&error)&&!a.ready()&&action.preview.frame==0&&action.performed==ForceSpecActorAction::None&&action.start.expired(),"unsupported control/effect/sound/auxiliary refuses, no start receipt");check(live->frames==before.frames&&live->actor_actions.cooldown6c==0&&live->scalars.field5da==0&&live->root_yaw5ae==before.root_yaw5ae&&live->vm.state().lanes[0].motion.context==before.vm.state().lanes[0].motion.context,"late failure preserves accepted UI/VM/scalars/root fields; draw invalidated");}
    Demo unavailable;CharacterPreviewFrame a;check(unavailable.setup()&&load_demo(a,unavailable,out),"unavailable demo fixture");auto p=profile();p.demonstration[0]=0;check(a.bind_actor_actions(p,&error)&&a.step_actor_actions(0,0xd00,&action,&error)&&action.requested==ForceSpecActorAction::None&&action.performed==ForceSpecActorAction::None&&a.state()->actor_actions.cooldown6c==0,"unavailable A does not fall through to simultaneous reset");
    std::printf("explicit same-actor demo receiver: %u checks PASS; owned waits/tails, request-start NOT completion\n",checks);return 0;
}
