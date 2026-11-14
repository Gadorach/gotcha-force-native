#include <cstdio>
#include <cstdlib>
#include "tests/unit/character_motion_directional_fixture.hpp"
namespace {
using namespace gf;unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"directional follow: %s\n",why);std::abort();}}
using Snapshot=std::shared_ptr<const CharacterDirectionalMotionSnapshot>;
f32 x(const Snapshot& state,unsigned bone=0){return state->local_pose().bones[bone].translation[0];}
bool near(f32 a,f32 b){return std::abs(a-b)<2e-5f;}
}
int main(){using namespace gf;using namespace character_motion_directional_fixture;Fixture f;std::string error;check(f.load(&error),"owned fixture");CharacterMotionDriver driver;Snapshot state;const std::array<CharacterProgramMotion,2> empty{};const std::array<u8,2> models={20,21};
    check(driver.load_frozen_target_transitions({f.model.data(),f.model.size()},f.rig,f.profile,f.clips,f.initial,&error)&&!driver.request_directional_follow(0,controls,8192,5,0,1,empty,models,&state)&&!state,"old driver entry retains explicit refusal");
    check(f.directional(driver,&error)&&driver.request_directional_follow(0,controls,8192,5,0,1,empty,models,&state),"explicit fresh dual target request");
    check(state->primary_target().index==0&&state->side_target().index==1&&state->side_rebound()&&state->side_weight()==.5f&&x(state)==50&&state->models()==std::array<u8,2>{4,5},"centre/side identities, translation and exact tie chooses centre models");
    check(state->primary().elapsed==0&&state->side().elapsed==0&&state->primary().frame==0&&state->side().frame==0,"request does not advance target players");
    auto first=state;check(driver.advance_directional_follow(state,controls,8193,5,1,state->models(),&state)&&!state->side_rebound()&&state->primary().elapsed==1&&state->side().elapsed==1&&state->primary().frame==1&&state->side().frame==1&&state->models()==std::array<u8,2>{6,5},"same target retains both players; strict weight>.5 selects alternate on configured bone only");
    check(driver.snapshot(state->primary())->trace()[0].tracks[0].callbacks>driver.snapshot(first->primary())->trace()[0].tracks[0].callbacks,"retained primary callback state advances");
    auto before=state;check(driver.advance_directional_follow(state,controls,-8192,5,1,state->models(),&state)&&state->side_rebound()&&state->side_target().index==2&&state->primary().frame==2&&state->side().frame==2&&state->primary().elapsed==2&&state->side().elapsed==2,"sign change rebinds only alternate at incremented primary frame");
    check(driver.snapshot(state->side())->trace()[0].tracks[0].callbacks<driver.snapshot(before->side())->trace()[0].tracks[0].callbacks+2,"new side is requested rather than advancing previous side");
    check(near(x(state),102)&&near(state->local_pose().bones[0].quaternion[2],std::sin(.11875f)),"new side requested values and spherical quaternion join");
    auto changed=controls;changed[4]=2;check(driver.advance_directional_follow(state,changed,0,5,.5f,state->models(),&state)&&state->primary_target().index==0&&state->side_target().index==1&&state->side_rebound(),"follow ignores changed centre control, per49F10 side-only identity dispatch");
    check(driver.request_directional_follow(0,controls,-32768,5,0,0,empty,models,&state)&&x(state)==400&&near(state->local_pose().bones[0].quaternion[2],std::sin(.4f))&&state->models()==std::array<u8,2>{8,5},"explicit directional extrapolation weight2 without clamp");
    f32 out[4];const f32 identity[4]={0,0,0,1};check(!character_pose_detail::blend(identity,identity,2,out),"old quaternion entry still refuses extrapolation");
    check(driver.request_directional_follow(1,controls,16384,5,0,1,empty,models,&state)&&state->models()==models&&x(state,1)==110,"secondary lane without matching model bones retains external models");
    check(f.directional(driver,&error,{1,-1})&&!driver.request_directional_follow(1,controls,1,5,0,1,empty,models,&state)&&!state,"secondary model-bone match requires actual shared actor buffer owner");
    check(f.directional(driver,&error,{0,0})&&driver.request_directional_follow(0,controls,16384,5,0,1,empty,models,&state)&&state->models()==std::array<u8,2>{6,5},"duplicate configured bones update first match only");
    auto fallback=controls;fallback[0]=fallback[1]=fallback[2]=0;check(driver.request_directional_follow(0,fallback,16384,5,0,1,empty,models,&state)&&state->primary_target().group==0&&state->side_target().group==0,"raw group0 identities retained while descriptor lookup resolves explicit default group");
    auto alias=f;alias.profile.descriptors[2].bytes[5]=1;alias.profile.descriptors[2].bytes[13]=1;check(alias.directional(driver,&error)&&driver.request_directional_follow(0,controls,8192,5,0,1,empty,models,&state)&&driver.advance_directional_follow(state,controls,-8192,5,1,state->models(),&state)&&state->side_rebound()&&driver.snapshot(state->side())->index()==1,"distinct descriptor identity rebinds even when clip identity aliases");
    auto defaults=f;defaults.profile.descriptors[1].bytes[9]=128;defaults.profile.descriptors[1].bytes[10]=254;check(defaults.directional(driver,&error)&&driver.request_directional_follow(0,controls,16384,5,0,1,empty,models,&state)&&state->models()==std::array<u8,2>{2,5},"negative model byte uses profile default, not justFF; unmatched model keeps centre");
    check(f.directional(driver,&error)&&driver.request_directional_follow(0,controls,8192,5,0,1,empty,models,&state),"reload original fixture");
    for(unsigned tick=0;tick<6;++tick)check(driver.advance_directional_follow(state,controls,8192,5,1,state->models(),&state),"different-length targets advance safely to stop");
    check(state->primary().frame==4&&state->side().frame==2&&state->primary().elapsed==6&&state->side().elapsed==6,"alternate metadata clocks copied from primary while stopped AObj frames remain distinct");
    const auto saved=state;auto missing=controls;missing[3]=99;check(!driver.advance_directional_follow(state,missing,1,5,1,models,&state)&&!state&&saved->primary().elapsed==6&&saved->side().elapsed==6,"later alternate target failure discards whole candidate and clears output");
    CharacterMotionDriver neighbor;check(f.directional(neighbor,&error)&&!neighbor.advance_directional_follow(saved,controls,1,5,1,models,&state)&&!state,"same-model foreign library refuses");
    check(f.directional(driver,&error)&&!driver.advance_directional_follow(saved,controls,1,5,1,models,&state),"old directional snapshot refuses after reload");
    for(f32 delta:std::array<f32,3>{-1,NAN,17})check(!driver.request_directional_follow(0,controls,0,5,0,delta,empty,models,&state)&&!state,"invalid rate clears output");
    auto common=controls;common[1]=128;check(!driver.request_directional_follow(0,common,0,5,0,1,empty,models,&state)&&!state,"common-bank descriptor requires separate qualification");
    check(!driver.request_directional_follow(2,controls,0,5,0,1,empty,models,&state)&&!driver.request_directional_follow(0,controls,0,5,0,1,empty,{128,0},&state)&&!driver.request_directional_follow(0,controls,0,5,0,1,empty,models,nullptr),"lane/model/output guards");
    auto large=f;for(unsigned index=0;index<2;++index)for(auto& bone:large.clips[index].clip.bones){bone.tracks[0]=character_motion_driver_fixture::linear(index?-1e7f:1e7f,index?-1e7f:1e7f,4);}
    check(large.directional(driver,&error)&&!driver.request_directional_follow(0,controls,32767,5,0,1,empty,models,&state)&&!state,"late derived pose out of budget rolls back both successful sample requests");
    check(!f.directional(driver,&error,{-2,0})&&!driver.advance_directional_follow(saved,controls,0,5,1,models,&state),"failed model-bone load retires prior library");
    std::printf("explicit dual-target follow sampler: %u checks PASS; not mode6/VM/actor admission\n",checks);return 0;
}
