#pragma once
#include "engine/assets/challenge_entry.hpp"
#include "engine/gameplay/challenge_setup_input.hpp"
#include "engine/gameplay/challenge_difficulty.hpp"
#include "engine/gameplay/screen_fade.hpp"
#include "engine/gameplay/challenge_difficulty_visual.hpp"
namespace gf {
enum class ChallengeEntryPhase {Closed,ResourceStart,ResourceWait,WidgetStart,RevealWait,Page,BackDispatch,ExitStart,ExitFadeWait,ReturnCleanup,MainRequested,PlayersRequested,Failed};
struct ChallengeEntryEvents {
    bool request_resources=false,build_widgets=false,retire_widgets=false,main_requested=false,players_requested=false;
    //Ordered requirements, not acknowledgements from a music or SFX consumer.
    bool request_bgm_start=false,request_bgm_fade=false;u32 bgm_member=0;u16 bgm_fade_updates=0;
    std::array<u16,2> sounds{};unsigned sound_count=0;s32 committed_energy=0;u8 difficulty=0,port=0;
};
//Shared setup's initial Challenge context (setup+17=0). Resource readiness is
//explicit native observation, not a fabricated source DMA/global-busy result.
//F6C4/F8BC/F954 -> difficulty ->B34/D44/DA8/E90. Next-screen events are requests,
//not completed main-menu/battle/editor entry. ScreenFade advances AFTER callback.
class ChallengeDifficultyFlow {
public:
    bool open(const ChallengeEntryProfile& entry,const ChallengeDifficultyProfile& difficulty,u8 participants){
        *this={};if(!entry.valid()||!difficulty.valid()||!participants||(participants&~15u))return false;
        entry_=entry;difficulty_=difficulty;participants_=participants;phase_=ChallengeEntryPhase::ResourceStart;return true;
    }
    bool step(bool resources_ready,const std::array<ChallengeSetupPad,4>& pads,ChallengeEntryEvents* out){
        if(!out)return false;
        *out={};
        if(phase_==ChallengeEntryPhase::Closed||phase_==ChallengeEntryPhase::Failed||terminal())return false;
        if(updates_>=8192){phase_=ChallengeEntryPhase::Failed;return false;}
        switch(phase_){
        case ChallengeEntryPhase::ResourceStart:out->request_resources=true;phase_=ChallengeEntryPhase::ResourceWait;break;
        case ChallengeEntryPhase::ResourceWait:
            if(resources_ready){
                port_=0;while(!(participants_&(1u<<port_)))++port_;
                participants_=u8(1u<<port_);out->request_bgm_start=true;out->bgm_member=entry_.bgm_member;
                if(!fade_.request(entry_.reveal_rgba,entry_.reveal_updates,ScreenFadeDirection::Reveal))return fail();
                phase_=ChallengeEntryPhase::WidgetStart;
            }break;
        case ChallengeEntryPhase::WidgetStart:
            out->build_widgets=true;visible_=8;interactive_=closing_=0;phase_=ChallengeEntryPhase::RevealWait;break;
        case ChallengeEntryPhase::RevealWait:
            if(!fade_.remaining()){fade_.clear();if(!page_.open(difficulty_))return fail();phase_=ChallengeEntryPhase::Page;}
            break;
        case ChallengeEntryPhase::Page:{
            u16 repeated=0,pressed=0;
            if(page_.phase()==ChallengeDifficultyPhase::Active){challenge_setup_input_step(pads[port_],&input_);repeated=input_.repeated;pressed=input_.pressed;}
            ChallengeDifficultyEvent event;if(!page_.step(repeated,pressed,&event))return fail();
            visible_=page_.visible_mask();interactive_=page_.interactive_mask();closing_=page_.closing_mask();
            out->sounds=event.sounds;out->sound_count=event.sound_count;
            if(event.back_requested)phase_=ChallengeEntryPhase::BackDispatch;
            if(event.players_requested){phase_=ChallengeEntryPhase::PlayersRequested;out->players_requested=true;out->committed_energy=event.committed_energy;out->difficulty=u8(page_.selection());out->port=port_;}
            break;}
        case ChallengeEntryPhase::BackDispatch:
            //Initial-context B34 skips popup construction and selects subphase4.
            phase_=ChallengeEntryPhase::ExitStart;break;
        case ChallengeEntryPhase::ExitStart:
            out->request_bgm_fade=true;out->bgm_fade_updates=entry_.exit_updates;
            if(!fade_.request(0,entry_.exit_updates,ScreenFadeDirection::Cover))return fail();
            phase_=ChallengeEntryPhase::ExitFadeWait;break;
        case ChallengeEntryPhase::ExitFadeWait:
            if(!fade_.remaining())phase_=ChallengeEntryPhase::ReturnCleanup;
            break;
        case ChallengeEntryPhase::ReturnCleanup:
            fade_.clear();out->retire_widgets=true;out->main_requested=true;phase_=ChallengeEntryPhase::MainRequested;break;
        default:return fail();
        }
        fade_.advance();++updates_;return true;
    }
    ChallengeEntryPhase phase()const{return phase_;}const ScreenFade& fade()const{return fade_;}
    const ChallengeDifficulty& page()const{return page_;}const ChallengeSetupButtons& input()const{return input_;}
    u8 port()const{return port_;}u8 participants()const{return participants_;}u32 updates()const{return updates_;}
    bool terminal()const{return phase_==ChallengeEntryPhase::MainRequested||phase_==ChallengeEntryPhase::PlayersRequested;}
    ChallengeDifficultyVisualInput visual_input()const{return {visible_,interactive_,closing_,u8(page_.selection())};}
private:
    bool fail(){phase_=ChallengeEntryPhase::Failed;return false;}
    ChallengeEntryProfile entry_;ChallengeDifficultyProfile difficulty_;ChallengeDifficulty page_;ScreenFade fade_;ChallengeSetupButtons input_;
    ChallengeEntryPhase phase_=ChallengeEntryPhase::Closed;u8 participants_=0,port_=0,visible_=0,interactive_=0,closing_=0;u32 updates_=0;
};
} // namespace gf
