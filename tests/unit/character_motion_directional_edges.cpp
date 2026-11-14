#include <cstdio>
#include <cstdlib>
#include "tests/unit/character_motion_directional_fixture.hpp"
namespace {using namespace gf;unsigned checks=0;void check(bool value,const char* why){++checks;if(!value){std::fprintf(stderr,"directional edge: %s\n",why);std::abort();}}}
int main(){using namespace gf;using namespace character_motion_directional_fixture;Fixture f;std::string error;check(f.load(&error),"fixture");CharacterMotionDriver driver;std::shared_ptr<const CharacterDirectionalMotionSnapshot> state;
    for(auto& d:f.profile.descriptors){d.bytes[6]|=4;d.bytes[14]|=4;}
    check(f.directional(driver,&error)&&driver.request_directional_follow(0,controls,8192,5,3.5f,.75f,{},f.profile.default_models,&state),"fractional looped request");
    check(state->primary().frame==3.5f&&state->side().frame==1.5f,"initial side loop wraps independently of centre frame");
    check(driver.advance_directional_follow(state,controls,8192,5,.75f,state->models(),&state)&&state->primary().frame==.25f&&state->side().frame==.25f&&state->primary().elapsed==.75f&&state->side().elapsed==.75f,"retained loop players rewind and metadata elapsed remains primary copied");
    check(driver.advance_directional_follow(state,controls,8192,5,1.5f,state->models(),&state)&&state->primary().frame==1.75f&&state->side().frame==1.75f,"post-rewind player state advances without re-request");
    const auto keep=state;check(driver.request_directional_follow(0,controls,16384,5,.75f,1,{state->primary(),state->side()},state->models(),&state)&&state->side().frame==.75f,"request output may replace the last owner of input model/snapshot references");
    state=keep;check(driver.request_directional_follow(0,controls,8192,5,0,1,{},state->models(),&state)&&state,"sole owned snapshot models copied before output reset");
    check(driver.request_directional_follow(0,controls,8192,5,0,1,{},state->models(),&state)&&state,"repeat request with sole-owner model alias remains safe");
    Fixture mirrored;check(mirrored.load(&error),"mirror fixture");mirrored.profile.descriptors[0].bytes[6]=2;mirrored.profile.descriptors[1].bytes[6]=3;mirrored.profile.descriptors[2].bytes[6]=3;
    check(mirrored.directional(driver,&error)&&driver.request_directional_follow(0,controls,8192,5,0,1,{},mirrored.profile.default_models,&state)&&driver.snapshot(state->primary())->mirrored()&&driver.snapshot(state->side())->mirrored()&&state->local_pose().bones[0].translation[0]==-50,"primary mirror and independent side toggle");
    check(driver.advance_directional_follow(state,controls,8192,5,1,state->models(),&state)&&!state->side_rebound()&&driver.snapshot(state->side())->mirrored(),"unchanged descriptor must not re-toggle mirror");
    check(driver.advance_directional_follow(state,controls,-8192,5,1,state->models(),&state)&&state->side_rebound()&&!driver.snapshot(state->side())->mirrored()&&driver.snapshot(state->primary())->mirrored(),"changed descriptor toggles side only using retained mirror history");
    for(unsigned tick=0;tick<16;++tick){const auto steering=s16(tick%2?8192:-8192);check(driver.advance_directional_follow(state,controls,steering,5,.125f,state->models(),&state),"repeated mirror interruptions have bounded snapshot ownership");}
    CharacterProgramMotion invalid;invalid.frame=1;check(!driver.request_directional_follow(0,controls,1,5,0,1,{invalid,{}},mirrored.profile.default_models,&state)&&!state,"unselected nonzero previous frame refuses");
    check(mirrored.directional(driver,&error)&&driver.request_directional_follow(0,controls,0,5,8191.75f,0,{},mirrored.profile.default_models,&state),"explicit out-of-clip but bounded request");
    check(!driver.advance_directional_follow(state,controls,0,5,1,state->models(),&state)&&!state,"beyond8192 primary request budget refuses before publication");
    std::printf("dual-target loop/mirror/alias edges: %u checks PASS\n",checks);return 0;}
