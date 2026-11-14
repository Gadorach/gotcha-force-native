#pragma once
#include "engine/assets/challenge_setup_pages.hpp"
#include "engine/gameplay/challenge_setup_input.hpp"
namespace gf {
enum class ChallengePlayersPhase {Closed,Enter,Opening,Active,Closing,DifficultyRequested,BoxRequested};
struct ChallengePlayersEvent {
    std::array<u16,2> sounds{};unsigned sound_count=0;
    bool changed=false,difficulty_requested=false,box_requested=false,team_up=false;
};
//8018FCD4/FD30/FD6C/FEC4. Only this page's mask2 is represented here.
//Input history is CARRIED from difficulty, not reset at every page boundary.
class ChallengePlayersPage {
public:
    bool open(const ChallengeSetupPagesProfile& profile,u8 port,const ChallengeSetupButtons& input){
        *this={};if(!profile.valid()||port>=4)return false;
        profile_=profile;port_=port;input_=input;phase_=ChallengePlayersPhase::Enter;return true;
    }
    bool step(const std::array<ChallengeSetupPad,4>& pads,ChallengePlayersEvent* out){
        if(!out)return false;
        *out={};if(phase_==ChallengePlayersPhase::Closed||terminal())return false;
        auto sound=[&](u16 cue){out->sounds[out->sound_count++]=cue;};
        switch(phase_){
        case ChallengePlayersPhase::Enter:selection_=0;remaining_=profile_.players_open;visible_=2;interactive_=closing_=0;phase_=ChallengePlayersPhase::Opening;break;
        case ChallengePlayersPhase::Opening:if(!--remaining_){interactive_|=2;phase_=ChallengePlayersPhase::Active;}break;
        case ChallengePlayersPhase::Active:{
            challenge_setup_input_step(pads[port_],&input_);const unsigned prior=selection_;
            if(input_.repeated&1){if(selection_)--selection_;}
            else if((input_.repeated&2)&&selection_<1)++selection_;
            if(prior!=selection_){out->changed=true;sound(119);}
            if(input_.pressed&0x200){visible_&=u8(~2u);interactive_&=u8(~2u);out->difficulty_requested=true;phase_=ChallengePlayersPhase::DifficultyRequested;sound(123);}
            else if(input_.pressed&0x180){remaining_=profile_.players_close;interactive_&=u8(~2u);closing_|=2;phase_=ChallengePlayersPhase::Closing;sound(122);}
            break;}
        case ChallengePlayersPhase::Closing:
            if(!--remaining_){visible_&=u8(~2u);phase_=ChallengePlayersPhase::BoxRequested;out->box_requested=true;out->team_up=selection_!=0;}break;
        default:return false;
        }
        return true;
    }
    bool terminal()const{return phase_==ChallengePlayersPhase::DifficultyRequested||phase_==ChallengePlayersPhase::BoxRequested;}
    ChallengePlayersPhase phase()const{return phase_;}unsigned selection()const{return selection_;}unsigned remaining()const{return remaining_;}
    u8 visible_mask()const{return visible_;}u8 interactive_mask()const{return interactive_;}u8 closing_mask()const{return closing_;}u8 port()const{return port_;}
    const ChallengeSetupButtons& input()const{return input_;}
private:
    ChallengeSetupPagesProfile profile_;ChallengeSetupButtons input_;ChallengePlayersPhase phase_=ChallengePlayersPhase::Closed;
    unsigned selection_=0,remaining_=0;u8 port_=0,visible_=0,interactive_=0,closing_=0;
};
} // namespace gf
