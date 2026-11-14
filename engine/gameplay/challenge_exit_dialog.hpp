#pragma once
#include "engine/assets/challenge_exit_dialog.hpp"
#include "engine/gameplay/challenge_setup_input.hpp"
namespace gf {
enum class ChallengeExitPhase {Closed,Enter,Opening,Active,Closing,Cancelled,Accepted,Failed};
struct ChallengeExitEvent {std::array<u16,2> sounds{};unsigned sound_count=0;bool create=false,cancelled=false,accepted=false;};
//190B34/BB0/BDC/CF4. Timers are tested BEFORE decrement, unlike page gates.
//Navigation is PRESSED low3 (either/both toggles ONCE), not held/repeated.
class ChallengeExitDialog {
public:
    bool open(const ChallengeExitDialogArt& art,u8 port,const ChallengeSetupButtons& input){*this={};if(!art.valid()||port>=4)return false;art_=art;port_=port;input_=input;phase_=ChallengeExitPhase::Enter;return true;}
    bool step(const std::array<ChallengeSetupPad,4>& pads,ChallengeExitEvent* out){
        if(!out)return false;
        *out={};if(phase_==ChallengeExitPhase::Closed||phase_==ChallengeExitPhase::Failed||terminal())return false;
        if(updates_>=8192){phase_=ChallengeExitPhase::Failed;return false;}
        auto sound=[&](u16 cue){out->sounds[out->sound_count++]=cue;};
        switch(phase_){
        case ChallengeExitPhase::Enter:selection_=1;state_=1;remaining_=art_.open_wait;phase_=ChallengeExitPhase::Opening;out->create=true;break;
        case ChallengeExitPhase::Opening:if(remaining_)--remaining_;else phase_=ChallengeExitPhase::Active;break;
        case ChallengeExitPhase::Active:
            challenge_setup_input_step(pads[port_],&input_);
            if(input_.pressed&3){selection_^=1;sound(119);}
            if(input_.pressed&0x200){remaining_=art_.cancel_wait;state_=0;phase_=ChallengeExitPhase::Closing;sound(123);}
            else if(input_.pressed&0x180){state_=selection_?0:2;remaining_=selection_?art_.cancel_wait:art_.accept_wait;phase_=ChallengeExitPhase::Closing;sound(122);}
            break;
        case ChallengeExitPhase::Closing:
            if(remaining_)--remaining_;
            else if(state_){phase_=ChallengeExitPhase::Accepted;out->accepted=true;}
            else{phase_=ChallengeExitPhase::Cancelled;out->cancelled=true;}
            break;
        default:return false;
        }
        ++updates_;return true;
    }
    bool terminal()const{return phase_==ChallengeExitPhase::Cancelled||phase_==ChallengeExitPhase::Accepted;}
    ChallengeExitPhase phase()const{return phase_;}u8 state()const{return state_;}u8 selection()const{return selection_;}u8 port()const{return port_;}unsigned remaining()const{return remaining_;}
    const ChallengeSetupButtons& input()const{return input_;}
private:
    ChallengeExitDialogArt art_;ChallengeSetupButtons input_;ChallengeExitPhase phase_=ChallengeExitPhase::Closed;u32 updates_=0;u16 remaining_=0;u8 selection_=1,state_=0,port_=0;
};
} // namespace gf
