#pragma once
#include "engine/assets/challenge_difficulty.hpp"
namespace gf {
enum class ChallengeDifficultyPhase {Closed,Enter,Opening,Active,Closing,PlayersRequested,BackRequested};
struct ChallengeDifficultyEvent {
    std::array<u16,2> sounds{};unsigned sound_count=0;
    bool changed=false,players_requested=false,back_requested=false;
    s32 committed_energy=0;
};
//8018FA34/FA90/FACC/FC30: difficulty-page reducer at the existing normalized
// repeated/pressed input seam. Calls are UI updates, not FPS or audio ticks.
// Resources/fades/widgets, four-port input production and the next screens are
// deliberately not acknowledged by a page request. No inventory/save writes.
class ChallengeDifficulty {
public:
    bool open(const ChallengeDifficultyProfile& profile){
        *this={};if(!profile.valid())return false;
        profile_=profile;phase_=ChallengeDifficultyPhase::Enter;return true;
    }
    bool step(u16 repeated,u16 pressed,ChallengeDifficultyEvent* out){
        if(!out)return false;
        *out={};
        if(phase_==ChallengeDifficultyPhase::Closed||phase_==ChallengeDifficultyPhase::PlayersRequested||
           phase_==ChallengeDifficultyPhase::BackRequested||updates_==~u64(0))return false;
        auto sound=[&](u16 cue){out->sounds[out->sound_count++]=cue;};
        switch(phase_){
        case ChallengeDifficultyPhase::Enter:
            selection_=0;remaining_=profile_.open_updates;visible_=8;interactive_=closing_=0;
            phase_=ChallengeDifficultyPhase::Opening;break;
        case ChallengeDifficultyPhase::Opening:
            if(!--remaining_){phase_=ChallengeDifficultyPhase::Active;interactive_|=8;}
            break;
        case ChallengeDifficultyPhase::Active:{
            const unsigned previous=selection_;
            if(repeated&1){if(selection_)--selection_;}
            else if((repeated&2)&&selection_<2)++selection_;
            if(selection_!=previous){out->changed=true;sound(119);}
            if(pressed&0x200){phase_=ChallengeDifficultyPhase::BackRequested;out->back_requested=true;sound(123);}
            else if(pressed&0x180){remaining_=profile_.close_updates;interactive_&=u8(~8u);closing_|=8;phase_=ChallengeDifficultyPhase::Closing;sound(122);}
            break;}
        case ChallengeDifficultyPhase::Closing:
            if(!--remaining_){visible_&=u8(~8u);phase_=ChallengeDifficultyPhase::PlayersRequested;out->players_requested=true;out->committed_energy=profile_.energy[selection_];}
            break;
        default:return false;
        }
        ++updates_;return true;
    }
    ChallengeDifficultyPhase phase()const{return phase_;}
    unsigned selection()const{return selection_;}unsigned remaining()const{return remaining_;}
    u8 visible_mask()const{return visible_;}u8 interactive_mask()const{return interactive_;}u8 closing_mask()const{return closing_;}
    s32 selected_energy()const{return profile_.valid()?profile_.energy[selection_]:0;}
    u64 updates()const{return updates_;}
private:
    ChallengeDifficultyProfile profile_;ChallengeDifficultyPhase phase_=ChallengeDifficultyPhase::Closed;
    unsigned selection_=0,remaining_=0;u8 visible_=0,interactive_=0,closing_=0;u64 updates_=0;
};
} // namespace gf
