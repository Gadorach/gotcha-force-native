#pragma once
#include "engine/assets/challenge_setup_widgets.hpp"
namespace challenge_widgets_fixture {
inline gf::ChallengeDifficultyArt common(){
    using namespace gf;ChallengeDifficultyArt a;a.bank_hash=1;a.camera.eye[2]=10;a.aspect=4.f/3;a.limits={1,2,3};
    a.operands={0,3,1,2,-1,.1f,120,9,.15f,-.15f,-96,-297,-120,.4f,7,.5f,176,0,63,40,106,-40,-350,160,364,91,0,11};
    for(unsigned i=0;i<4;++i)a.frames[i]={u16(1u<<i),u16(10+i),{0,0,0}};
    for(auto& row:a.footers)row.records={-1,-1,-1,-1};
    return a;
}
inline gf::ChallengeSetupWidgetsArt art(){
    using namespace gf;ChallengeSetupWidgetsArt a;a.bank_hash=1;
    for(unsigned i=0;i<2;++i)for(unsigned j=0;j<4;++j)a.players[i][j]={s16(i*4+j),36,{f32(i)*20,0,0}};
    a.players[0][3].record=-1;a.focus={ChallengePlayerFocus{10,{0,0,-100}},ChallengePlayerFocus{11,{0,0,-100}}};
    a.box={ChallengeBoxPart{20,36,1,{0,0,0}},ChallengeBoxPart{21,1,.5f,{0,1,0}}};
    a.operands={-400,400,200,.8f,10,180,-10,-100,-300,200,-100,-500,700,-700,14};return a;
}
} // namespace challenge_widgets_fixture
