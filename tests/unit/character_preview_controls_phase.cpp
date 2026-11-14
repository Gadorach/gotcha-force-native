#include <cstdio>
#include <cstdlib>
#include "tests/unit/character_preview_controls_fixture.hpp"
namespace {using namespace gf;unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"preview request phase: %s\n",why);std::abort();}}
bool load(CharacterPreviewFrame& actor,character_preview_controls_fixture::Fixture& f,CharacterPreviewOutput& out,std::string& error){return actor.load_with_actor_controls(f.assets(),character_preview_height_fixture::preview(),CharacterPreviewMode::SettledIdleWithAuxiliaryRequirements,0,character_preview_height_fixture::height(),f.event_view(),{},&out,&error);}}
int main(){using namespace gf;using namespace character_preview_controls_fixture;Fixture f;std::string error;check(f.load(&error),"fixture");CharacterPreviewFrame actor;CharacterPreviewOutput out;CharacterPreviewMotionRequest q;q.choice=1;
    check(load(actor,f,out,error)&&actor.state()->scalars.phase19==2&&actor.state()->scalars.counter6ba==0,"constructor keeps source idle phase2 and fresh counter6BA0");
    check(actor.request_ordinary_motion(q,{0,0x4000},&out,&error)&&actor.state()->scalars.phase19==0&&actor.state()->scalars.field5da==0&&actor.state()->root_yaw5ae==0,"ordinary request sets source phase19=0, does not fake full demo state60");
    check(actor.state()->setup.position==std::array<f32,3>{-59,17,3}&&out.sound_requirements.size()==2&&out.height_source_frame==1,"phase correction retains accepted root/world/event outputs");
    check(actor.step({},&out,&error)&&actor.state()->scalars.phase19==0&&actor.state()->scalars.counter6ba==0&&actor.state()->setup.position==std::array<f32,3>{-58,19,6},"ordinary callback phase0 stays coherent at following source step");
    check(load(actor,f,out,error),"branch rollback setup");const auto* before=actor.state();q.choice=5;check(!actor.request_ordinary_motion(q,{},&out,&error)&&out.frame==0&&before->scalars.phase19==2&&before->frames==1&&!before->published,"unconsumed branch pulse rolls back candidate request phase");
    check(load(actor,f,out,error),"event rollback setup");before=actor.state();q.choice=3;check(!actor.request_ordinary_motion(q,{},&out,&error)&&out.sound_requirements.empty()&&before->scalars.phase19==2&&before->frames==1,"late missing event rolls back request phase with actor state");
    check(load(actor,f,out,error),"demo gate setup");before=actor.state();q.choice=40;check(!actor.request_ordinary_motion(q,{},&out,&error)&&before->scalars.phase19==2&&before->scalars.field5da==0,"full demo refusal does not set actor state/phase as acknowledged");
    check(actor.load_with_actor_height(f.assets(),character_preview_height_fixture::preview(),CharacterPreviewMode::SettledIdleWithAuxiliaryRequirements,0,character_preview_height_fixture::height(),&out,&error),"legacy owner");before=actor.state();q.choice=1;check(!actor.request_ordinary_motion(q,{},&out,&error)&&before->scalars.phase19==2,"old entry cannot gain new request phase semantics silently");
    std::printf("native ordinary-request phase/rollback boundaries: %u checks PASS\n",checks);return 0;
}
