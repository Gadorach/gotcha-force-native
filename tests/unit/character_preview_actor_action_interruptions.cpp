#include <cstdio>
#include <cstdlib>
#include "tests/unit/character_preview_actions_fixture.hpp"
namespace {using namespace gf;using namespace character_preview_actions_fixture;unsigned checks=0;void check(bool x,const char* why){++checks;if(!x){std::fprintf(stderr,"source reset interruption: %s\n",why);std::abort();}}}
int main(){using namespace gf;Fixture f;CharacterPreviewFrame actor;CharacterPreviewOutput out;CharacterPreviewActionOutput action;std::string e;check(f.load(&e)&&character_preview_directional_fixture::load(actor,f,out,e)&&actor.step({0x1100,0x2800},&out,&e),"actual prior-yaw/world fixture");CharacterPreviewMotionRequest q;q.choice=6;
    check(actor.request_controlled_motion(q,{0x1a00,0x5300},&out,&e)&&actor.state()->directional_root_known&&actor.state()->setup.position!=character_preview_height_fixture::preview().position,"existing native directional pose/root before action binding");
    const auto binding=actor.model()->instance_binding();const auto saved=actor.state()->root_yaw5ae;const auto frames=actor.state()->frames;auto p=profile();check(actor.bind_actor_actions(p,&e),"adopt this directional actor without recreation");p.pitch=1;p.demonstration={0,0};
    check(actor.step_actor_actions(0,0x400,&action,&e)&&actor.reset_action_current(action)&&actor.model()->instance_binding()==binding&&actor.state()->frames==frames+1,"source reset interrupts actual dual snapshot while preserving owner/frame continuity");
    check(!actor.state()->vm.state().lanes[0].following&&!actor.state()->directional_root_known&&actor.state()->directional_root_frame==0&&actor.state()->setup.position==character_preview_height_fixture::preview().position&&actor.state()->root_yaw5ae==saved,"displayed directional pose seed changes to ordinary authored reset, not raw-centre replay or yaw overwrite");
    check(actor.state()->action_transitions[0].blending&&actor.state()->action_transitions[0].mode==2&&actor.state()->action_transitions[0].duration==11&&actor.state()->controls.pitch==0x800,"bound profile is copied and actual reset keeps authored transition");
    check(actor.step_actor_actions(0,0x100,&action,&e)&&action.requested==ForceSpecActorAction::Demonstrate&&action.performed==ForceSpecActorAction::None&&action.unfulfilled_actions==0x100&&actor.state()->actor_actions.cooldown6c==0,"caller capability edits cannot change bound action policy or fabricate demo acknowledgement");
    std::printf("same-owner directional reset/profile-copy edges: %u checks PASS\n",checks);return 0;
}
