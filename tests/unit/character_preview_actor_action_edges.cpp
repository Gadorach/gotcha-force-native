#include <cstdio>
#include <cstdlib>
#include <limits>
#include "tests/unit/character_preview_actions_fixture.hpp"
namespace {using namespace gf;using namespace character_preview_actions_fixture;unsigned checks=0;void check(bool x,const char* why){++checks;if(!x){std::fprintf(stderr,"preview action receiver edge: %s\n",why);std::abort();}}
bool load(CharacterPreviewFrame& a,const Fixture& f,CharacterPreviewOutput& out,std::string& e){return character_preview_directional_fixture::load(a,f,out,e);}}
int main(){using namespace gf;std::string e;CharacterPreviewOutput out;CharacterPreviewActionOutput action;Fixture f;check(f.load(&e),"owned inputs");
    {CharacterPreviewFrame a;check(load(a,f,out,e),"before explicit binding");const auto* old=a.state();const auto frame=old->frames;check(!a.step_actor_actions(0,0x400,&action,&e)&&!a.ready()&&!old->published&&old->frames==frame&&action.preview.frame==0,"unbound owner cannot silently consume source actions");}
    {CharacterPreviewFrame a;check(load(a,f,out,e),"bad profile fixture");auto wrong=profile();wrong.bank_hash=124;check(!a.bind_actor_actions(wrong,&e)&&!a.ready(),"source action scene must match actual actor preview");
        check(load(a,f,out,e)&&a.bind_actor_actions(profile(),&e)&&!a.bind_actor_actions(profile(),&e)&&!a.ready(),"rebinding cannot reset cooldown/UI state or create fresh action authority");}
    {CharacterPreviewFrame a;check(load(a,f,out,e)&&a.bind_actor_actions(profile(),&e),"generic entry separation");check(!a.step({},&out,&e)&&!a.ready()&&out.frame==0,"bound action owner cannot mix a free orbit step entry");
        check(load(a,f,out,e)&&a.bind_actor_actions(profile(),&e),"generic request guard");CharacterPreviewMotionRequest q;q.choice=6;check(!a.request_controlled_motion(q,{},&out,&e)&&!a.ready()&&out.frame==0,"generic motion requests cannot bypass bound source action state");}
    {CharacterPreviewFrame a;check(load(a,f,out,e)&&a.bind_actor_actions(profile(),&e),"null output fixture");const auto* old=a.state();const auto frame=old->frames;check(!a.step_actor_actions(0,0x400,nullptr,&e)&&!a.ready()&&old->frames==frame&&!old->published,"missing action output refuses without accepted frame/UI mutation");}
    {CharacterPreviewFrame left,right;check(load(left,f,out,e)&&left.bind_actor_actions(profile(),&e)&&left.step_actor_actions(0,0x400,&action,&e),"left reset receipt");const auto receipt=action;const auto binding=left.model()->instance_binding();
        check(load(right,f,out,e)&&right.bind_actor_actions(profile(),&e)&&right.step_actor_actions(0,0x800,&action,&e)&&action.preview.frame==receipt.preview.frame&&!right.reset_action_current(receipt)&&left.reset_action_current(receipt),"same model/frame neighbor is not action receiver identity");
        check(load(left,f,out,e)&&left.model()->instance_binding()!=binding&&left.bind_actor_actions(profile(),&e)&&left.step_actor_actions(0,0x400,&action,&e)&&action.preview.frame==receipt.preview.frame&&!left.reset_action_current(receipt),"reload retires old receipts even with equal callback counters and retained binding");}
    {CharacterPreviewFrame a;check(load(a,f,out,e),"scoped position writer fixture");CharacterProgramRootFields expected;check(a.state()->vm.observe_root_fields(&expected),"native root view");auto candidate=a.state()->vm;const auto original=candidate.state().actor.root;
        check(candidate.restore_root_position(expected,{4,5,6},&e)&&candidate.state().actor.root.position==std::array<f32,3>{4,5,6}&&candidate.state().actor.root.delta1da0==original.delta1da0&&candidate.state().actor.root.updates==original.updates&&a.state()->vm.state().actor.root.position==original.position,"position restore writes only candidate20, not root clocks/delta/live owner");
        check(!candidate.restore_root_position(expected,{1,2,3},&e)&&!candidate.ready(),"stale position/state view rejected");
        candidate=a.state()->vm;auto nonfinite=expected;nonfinite.root.updates+=1;check(!candidate.restore_root_position(nonfinite,{0,0,0},&e),"updated root serial participates in expected-field guard");
        candidate=a.state()->vm;check(!candidate.restore_root_position(expected,{0,std::numeric_limits<f32>::quiet_NaN(),0},&e)&&candidate.state().actor.root.position==original.position,"consumed nonfinite position fails without partial store");
        CharacterPreviewFrame neighbor;check(load(neighbor,f,out,e),"neighbor VM scope");candidate=neighbor.state()->vm;check(!candidate.restore_root_position(expected,{0,0,0},&e),"foreign same-model root view rejected");}
    {Fixture bad;check(bad.load(&e),"owned late-prefix rollback fixture");auto assets=bad.assets();CharacterProgramAsset p;check(character_program_load_control_flow(assets.program,character_program_model_hash(assets.model),&p,&e),"decode owned program");
        //Owned budget boundary: constructor's13 negative start resolves from0;
        //after a legitimate8192 sample, reset's authored negative start exceeds
        //the existing sample budget. No original program/loop/end is changed.
        for(auto& q:p.programs)if(!q.group&&!q.choice)q.bytes={13,0,4,1,4,0,255,255,0,0,0,100,0,1,255,255};
        for(auto& d:p.descriptors)if(d.choice==4)d.bytes[19]=0; //Owned constructor placement invariant.
        bad.program=character_preview_controls_fixture::encode(p);
        CharacterPreviewFrame a;check(load(a,bad,out,e),"constructor owned boundary program");CharacterPreviewMotionRequest q;q.choice=1;q.start=8192;check(a.request_controlled_motion(q,{0x1900,0x3400},&out,&e)&&a.bind_actor_actions(profile(),&e),"actual accepted high-frame owner before reset");
        const auto* accepted=a.state();const auto* model=a.model();const auto frame=accepted->frames;const auto position=accepted->setup.position;const auto root=accepted->vm.state().actor.root;const auto ui=accepted->actor_actions;const auto context=accepted->vm.state().lanes[0].motion.context;
        check(!a.step_actor_actions(0x900000,0x400,&action,&e)&&!a.ready()&&!model->ready()&&!accepted->published&&accepted->frames==frame&&accepted->setup.position==position&&accepted->vm.state().actor.root.position==root.position&&accepted->vm.state().actor.root.updates==root.updates&&accepted->vm.state().lanes[0].motion.context==context&&accepted->actor_actions.angles.pitch==ui.angles.pitch&&accepted->actor_actions.angles.yaw==ui.angles.yaw&&accepted->actor_actions.cooldown6c==ui.cooldown6c&&action.performed==ForceSpecActorAction::None&&action.preview.frame==0,"failed prefix after position/UI candidate rolls back bookkeeping/VM/receipt and invalidates draws");}
    std::printf("source reset receiver scope/rollback edges: %u checks PASS\n",checks);return 0;
}
