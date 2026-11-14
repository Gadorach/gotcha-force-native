#pragma once
#include "engine/assets/challenge_force_notices.hpp"
#include "engine/gameplay/challenge_setup_chrome.hpp"
namespace gf {
struct ChallengeForceNoticesInput {
    u8 visible=0,interactive=0,closing=0;
    bool team_up=false;u8 box_index=0;std::array<s8,2> ports{-1,-1};
};
//Type13 prompt and type14 team/controller indicators. Source byte22 is TEAM UP;
//byte23 is setup box index0/1/2, NOT the player-choice value or controller port.
class ChallengeForceNotices {
public:
    bool open(const ChallengeForceNoticesArt& art){*this={};if(!art.valid())return false;art_=art;prompt_tick_=art.initial_tick;ready_=true;return true;}
    bool step(const ChallengeForceNoticesInput& input){
        if(!ready_||updates_>=8192||((input.visible|input.interactive|input.closing)&~15u)||input.box_index>2||input.ports[0]<-1||input.ports[0]>3||input.ports[1]<-1||input.ports[1]>3)return fail();
        auto next=*this;next.draws_.clear();u16 mask=0;
        if(((input.visible&input.interactive)|input.closing)&4){mask=input.closing&4?art_.closing_blink:input.interactive&4?art_.active_blink:0;++next.prompt_tick_;}
        else next.prompt_tick_=art_.initial_tick;
        if(next.prompt_tick_&mask){ChallengeSetupDraw d;d.record=art_.prompt_record;d.world=mat4_translation(art_.prompt_position.data());next.draws_.push_back(d);}
        if(input.box_index!=next.box_index_){next.box_index_=input.box_index;next.team_tick_=0;}else ++next.team_tick_;
        if(!input.team_up||!input.box_index)next.level_=0;
        else if(input.box_index==1){if(next.level_<1){next.level_=std::min(1.f,f32(next.level_+art_.alpha_step));next.team_tick_=0;}else next.level_=1;}
        else next.level_=2;
        if(input.team_up&&input.box_index){
            for(unsigned i=0;i<3;++i){const auto& p=art_.team[i];ChallengeSetupDraw d;d.world=mat4_translation(p.position.data());
                if(!i){d.record=u16(p.record);d.material_time=f32(next.level_+((next.team_tick_&32)?0.f:1.f));d.authored_alpha=true;}
                else{if(input.ports[i-1]<0)continue;d.record=u16(art_.port_record_base+input.ports[i-1]);d.material_time=i==input.box_index&&((next.team_tick_&32)!=0)?1.f:0.f;}
                next.draws_.push_back(d);
            }
        }
        ++next.updates_;*this=std::move(next);return true;
    }
    bool ready()const{return ready_;}const std::vector<ChallengeSetupDraw>& draws()const{return draws_;}
    u16 prompt_tick()const{return prompt_tick_;}u16 team_tick()const{return team_tick_;}f32 level()const{return level_;}u32 updates()const{return updates_;}
private:
    bool fail(){ready_=false;draws_.clear();return false;}
    ChallengeForceNoticesArt art_;std::vector<ChallengeSetupDraw> draws_;u32 updates_=0;u16 prompt_tick_=0,team_tick_=0;u8 box_index_=255;f32 level_=0;bool ready_=false;
};
} // namespace gf
