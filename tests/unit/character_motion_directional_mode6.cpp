#include <cstdio>
#include <cstdlib>
#include "tests/unit/character_motion_directional_fixture.hpp"
namespace {
using namespace gf;unsigned checks=0;void check(bool value,const char* why){++checks;if(!value){std::fprintf(stderr,"mode6: %s\n",why);std::abort();}}
using Snapshot=std::shared_ptr<const CharacterDirectionalMotionSnapshot>;
bool load(character_motion_directional_fixture::Fixture& f,CharacterMotionDriver& driver,std::string& error){return driver.load_directional_transitions({f.model.data(),f.model.size()},f.rig,f.profile,f.clips,f.initial,{0,-1},&error);}
f32 x(const Snapshot& s){return s->local_pose().bones[0].translation[0];}
}
int main(){using namespace gf;using namespace character_motion_directional_fixture;Fixture f;CharacterMotionDriver driver;std::string error;Snapshot state;check(f.load(&error),"fixture");
    check(f.directional(driver,&error)&&!driver.request_directional_transition(0,controls,8192,5,0,1,3,{}, {22,23},{20,21},&state)&&!state,"instant-only driver cannot enable mode6");
    check(load(f,driver,error),"explicit transition load");CharacterProgramMotion ordinary;check(driver.select(0,f.profile.descriptors[2],false,{},0,1,&ordinary),"prior displayed ordinary pose");
    check(!driver.select_transition(0,f.profile.descriptors[0],false,ordinary,0,1,6,3,&ordinary),"generic motion interface still refuses mode6");
    check(driver.request_directional_transition(0,controls,8192,5,0,1,3,ordinary,{22,23},{20,21},&state)&&state->blending()&&state->blend_duration()==4&&state->blend_time()==1&&state->previous_weight()==.75f&&x(state)==162.5f,"duration+1 and initial delta; freeze displayed old pose");
    check(state->directional_pose().bones[0].translation[0]==50&&state->frozen_pose().bones[0].translation[0]==200&&state->models()==std::array<u8,2>{6,5},"frozen pose is separate; side D244 overwrites alternate model bytes before temporal selection");
    check(driver.snapshot(state->primary())->transition_mode()==6&&driver.snapshot(state->primary())->blending()&&state->primary().frame==0&&state->primary().elapsed==0,"primary mode6 suppresses completion without activation advance");
    const auto first=state;check(driver.advance_directional_transition(state,controls,8192,5,1,state->models(),&state)&&state->blend_time()==2&&state->previous_weight()==.5f&&x(state)==125&&state->models()==std::array<u8,2>{4,5},"flags0 freeze both target players; exact temporal tie uses scratch model");
    check(driver.advance_directional_transition(state,controls,0,5,1,state->models(),&state)&&x(state)==50&&state->models()==std::array<u8,2>{6,5},"scratch centreWeight>.5 takes frozen side model before temporal blend");
    check(driver.advance_directional_transition(state,controls,0,5,1.5f,state->models(),&state)&&!state->blending()&&state->blend_time()==4.5f&&state->primary().frame==.5f&&state->side().frame==.5f&&x(state)==.5f&&state->models()==std::array<u8,2>{4,5},"completion re-requests HELD prior frame+overshoot, resets descriptor models then clears mode");
    check(driver.advance_directional_transition(state,controls,8192,5,.5f,state->models(),&state)&&state->primary().frame==1&&state->side().frame==1&&!state->blending(),"next callback resumes ordinary follow, not another overshoot request");
    check(driver.interrupt_directional_transition(first,controls,-16384,5,0,1,3,first->models(),&state)&&x(state)==171.875f&&state->frozen_pose().bones[0].translation[0]==162.5f&&state->models()==std::array<u8,2>{8,5},"interrupt freezes displayed dual pose; new side descriptor replaces alternate model bytes");
    check(!driver.request_directional_transition(0,controls,0,5,0,1,3,state->primary(),{4,5},state->models(),&state)&&!state,"raw primary snapshot cannot substitute for displayed dual pose on interruption");
    check(driver.request_directional_transition(0,controls,0,5,5,1,1,{}, {2,3},{2,3},&state)&&state->primary().finished==0&&state->side().finished==1,"raw side may finish while primary mode6 suppresses completion");
    check(driver.advance_directional_transition(state,controls,0,5,1,state->models(),&state)&&!state->blending()&&state->primary().finished==0&&driver.snapshot(state->primary())->transition_mode()==0,"completion samples while mode6 is still set; no same-callback finished flag");
    check(driver.advance_directional_transition(state,controls,0,5,1,state->models(),&state)&&state->primary().finished==1,"finished becomes visible on subsequent ordinary callback");
    check(driver.request_directional_transition(0,controls,8192,5,0,0,1,{}, {2,3},{2,3},&state)&&state->previous_weight()==1&&state->blend_time()==0,"zero delta preserves full frozen weight");
    auto changed=controls;changed[4]=2;check(driver.advance_directional_transition(state,changed,-8192,5,.5f,state->models(),&state)&&state->primary_target().index==0&&state->side_target().index==2&&state->side().frame==0&&state->primary().frame==0,"unfinished mode6 side rebind uses HELD prior primary frame, not frame+delta; centre control ignored until completion");
    check(driver.advance_directional_transition(state,changed,0,5,2,state->models(),&state)&&!state->blending()&&state->primary_target().index==2&&state->primary().frame==.5f,"completion selects latest centre and carries timer overshoot only");
    check(driver.request_directional_transition(0,controls,-32768,5,0,.5f,2,{}, {2,3},{2,3},&state)&&state->directional_pose().bones[0].translation[0]==400,"scratch quaternion centreWeight=-1 admits explicit directional extrapolation");
    const auto saved=state;auto missing=controls;missing[5]=99;check(!driver.advance_directional_transition(state,missing,-1,5,.25f,state->models(),&state)&&!state&&saved->blend_time()==.5f&&saved->primary().elapsed==0,"late side refusal rolls back clocks/players/pose/models and clears output");
    CharacterMotionDriver foreign;check(load(f,foreign,error)&&!foreign.advance_directional_transition(saved,controls,0,5,1,{2,3},&state),"foreign library rejects mode6 state");
    check(load(f,driver,error)&&!driver.advance_directional_transition(saved,controls,0,5,1,{2,3},&state),"same-model reload rejects prior scope");
    check(!driver.request_directional_transition(0,controls,0,5,0,3,1,{}, {2,3},{2,3},&state)&&!state,"initial delta beyond duration+1 refuses; no extrapolated temporal blend");
    check(driver.request_directional_transition(0,controls,8192,5,0,1,0,{}, {2,3},{2,3},&state)&&!state->blending()&&x(state)==50,"duration0 uses instant follow, not mode6");
    check(driver.interrupt_directional_transition(state,controls,8192,5,0,.25f,2,state->models(),&state)&&state->blending(),"sole-owner/in-place interrupt keeps input pose/model provenance alive");
    for(unsigned i=0;i<12;++i)check(driver.interrupt_directional_transition(state,controls,s16(i%2?-8192:8192),5,.25f,.5f,2,state->models(),&state),"repeated interruption freezes values without snapshot history chains");
    std::printf("directional mode6 transition component: %u checks PASS; no VM/actor admission\n",checks);return 0;}
