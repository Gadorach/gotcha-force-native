#include <cstdio>
#include <cstdlib>
#include "tests/unit/character_preview_actions_fixture.hpp"
namespace {using namespace gf;using namespace character_preview_actions_fixture;std::string error;unsigned checks=0;
void check(bool x,const char* why){++checks;if(!x){std::fprintf(stderr,"demo receiver edges: %s (%s)\n",why,error.c_str());std::abort();}}
bool load(CharacterPreviewFrame& a,const Fixture& f,CharacterPreviewOutput& out){return a.load_with_demo_actor_controls(f.assets(),character_preview_height_fixture::preview(),0,character_preview_height_fixture::height(),f.event_view(),{},&out,&error);}
void modify(Fixture& f,unsigned kind){CharacterProgramAsset p;const auto assets=f.assets();check(character_program_load_control_flow(assets.program,character_program_model_hash(assets.model),&p,&error),"owned input library");
    for(auto& q:p.programs)if(!q.group&&q.choice==40){if(kind==1)q.bytes={1,0,1,0,0,0,0,0,2,255,0,0,2,0,0,0,0,0,0,100,0,1,255,255};if(kind==2)q.bytes={1,0,1,0,0,0,0,0,3,1,0,0,0,0,0,0,0,0,0,100,0,1,255,255};if(kind==3)q.bytes={1,0,1,0,0,0,0,0,3,0,0,0,0,0,0,32,0,0,0,100,0,1,255,255};if(kind==4)q.choice=41;}
    if(kind==5){for(auto& q:p.programs)if(!q.group&&!q.choice)q.bytes.insert(q.bytes.begin()+8,{2,255,0,0,1,0,0,0});}
    f.program=character_preview_controls_fixture::encode(p);
}
}
int main(){using namespace gf;using namespace character_preview_actions_fixture;Fixture f;CharacterPreviewFrame a;CharacterPreviewOutput out;CharacterPreviewActionOutput action;check(f.load(&error)&&load(a,f,out)&&a.bind_actor_actions(profile(),&error)&&a.step_actor_actions(0,0x100,&action,&error)&&a.action_start_current(action),"fresh actual owned start");const auto receipt=action;
    check(load(a,f,out)&&!a.action_start_current(receipt)&&a.bind_actor_actions(profile(),&error),"explicit reload invalidates start even on same model/profile");
    check(!a.bind_actor_actions(profile(),&error)&&!a.ready()&&!a.action_start_current(receipt),"duplicate binding refuses, cannot preserve receipt authority");
    check(load(a,f,out),"fresh demo-only load");CharacterPreviewMotionRequest q;q.choice=1;check(!a.request_controlled_motion(q,{0x800,0},&out,&error)&&!a.ready()&&out.frame==0,"generic controlled request cannot bypass source demo action caller");
    for(unsigned kind:{1u,2u,3u,4u}){Fixture bad;CharacterPreviewFrame actor;check(bad.load(&error),"fresh owned rejection fixture");modify(bad,kind);check(load(actor,bad,out)&&actor.bind_actor_actions(profile(),&error),"accepted prior actor before unsupported action");const auto* live=actor.state();const auto before=*live;check(!actor.step_actor_actions(0,0x100,&action,&error)&&!actor.ready()&&action.start.expired()&&action.preview.motion.empty(),"unimported child mode/control1/behavior/missing40 refuses without receipt");check(live->frames==before.frames&&live->scalars.field5db==before.scalars.field5db&&live->actor_actions.cooldown6c==before.actor_actions.cooldown6c&&live->setup.position==before.setup.position,"guard failure rolls back candidate field request");}
    Fixture child;CharacterPreviewFrame actor;check(child.load(&error),"fresh child mode fixture");modify(child,5);check(!character_preview_directional_fixture::load(actor,child,out,error),"old directional load still refuses nonzero source6E8");check(load(actor,child,out)&&actor.state()->vm.state().actor.control0[2]==1,"new preview entry carries6E8 without zeroing it");const auto first=actor.state()->children;
    check(actor.step({0x800,0},&out,&error)&&actor.state()->children[0].draw_scale>first[0].draw_scale&&actor.state()->children[1].draw_scale==0&&actor.state()->child_visible[0]&&!actor.state()->child_visible[1],"6E8 reaches both native child owners: mode1 grows first, not mode2 second");
    std::printf("explicit demo receiver edges: %u checks PASS; old guard domains retained\n",checks);return 0;
}
