#pragma once
#include "engine/gameplay/challenge_difficulty_flow.hpp"
#include "engine/gameplay/challenge_players.hpp"
#include "engine/gameplay/challenge_load_box.hpp"
#include "engine/gameplay/challenge_setup_visual.hpp"
#include "engine/gameplay/challenge_exit_dialog.hpp"
#include "engine/gameplay/challenge_force_editor_bridge.hpp"
namespace gf {
enum class ChallengeSetupPhase {Closed,Entry,Players,DifficultyReturn,Box,ExitDispatch,ExitStart,ExitWait,ExitCleanup,ExitConfirmationRequested,SelectionRequested,MainRequested,Failed,Force,ForceEditorRequested,SecondBoxRequested,ForceFinished};
struct ChallengeSetupEvent {
    ChallengeEntryEvents entry;ChallengeBoxEvent box;
    std::array<u16,2> sounds{};unsigned sound_count=0;
    bool selection_requested=false,exit_confirmation_requested=false,team_up=false;u8 port=0,dialog_port=0;s32 energy=0;
    bool exit_dialog_created=false,exit_cancelled=false,exit_accepted=false;
    ChallengeForceEvent force;bool force_opened=false;
    bool second_box_opened=false,selections_ready=false;
};
//Controller join through FIRST box. SelectionRequested is a handoff, not a drawn
//Force Selection/roster/battle ACK. Later context2 is not entered before that
//screen accepts player1. Exit popup is a scoped external request, not faked here.
class ChallengeSetupFlow {
public:
    bool open(const ChallengeEntryProfile& entry,const ChallengeDifficultyProfile& difficulty,const ChallengeSetupPagesProfile& pages,u8 participants){
        *this={};if(!pages.valid()||!entry_.open(entry,difficulty,participants))return false;
        entry_profile_=entry;difficulty_profile_=difficulty;pages_profile_=pages;phase_=ChallengeSetupPhase::Entry;return true;
    }
    //Explicit opt-in: legacy request-only owner remains unchanged by default.
    bool enable_exit_dialog(const ChallengeExitDialogArt& art){if(phase_!=ChallengeSetupPhase::Entry||updates_||!art.valid())return false;exit_art_=art;dialogs_enabled_=true;return true;}
    bool enable_force_selection(const ChallengeForceSelectionProfile& profile){if(phase_!=ChallengeSetupPhase::Entry||updates_||force_enabled_||!dialogs_enabled_||!profile.valid())return false;force_profile_=profile;force_enabled_=true;return true;}
    bool enable_second_box(){if(phase_!=ChallengeSetupPhase::Entry||updates_||!force_enabled_||second_enabled_)return false;second_enabled_=true;return true;}
    bool admit_second_box(){
        if(!second_enabled_||phase_!=ChallengeSetupPhase::SecondBoxRequested||!team_up_||context_!=1||first_force_held_||!force_.terminal())return false;
        first_port_=port_;first_force_=std::move(force_);force_={};first_force_held_=true;context_=2;
        if(!box_.open(pages_profile_,context_,first_port_,participants_,inputs_))return fail();
        phase_=ChallengeSetupPhase::Box;return true;
    }
    //Only the retained presentation's successfully published inventory can cross
    //this opt-in seam; ordinary request-only callers still stop at the old phase.
    bool accept_selection_inventory(const ForceInventory& owner,unsigned requested_force=0){
        if(!force_enabled_||phase_!=ChallengeSetupPhase::SelectionRequested||!owner.ready()||!owner.state()->force_count||!force_.open(force_profile_,owner,requested_force,port_,context_,team_up_,energy_,inputs_[port_]))return false;
        phase_=ChallengeSetupPhase::Force;return true;
    }
    bool step(bool entry_resources_ready,bool box_resources_ready,const std::array<ChallengeSetupPad,4>& pads,const ChallengeBoxLoadReceipt* receipt,ChallengeSetupEvent* out,const ForceInventory* force_owner=nullptr){
        if(!out)return false;
        *out={};if(phase_==ChallengeSetupPhase::Closed||phase_==ChallengeSetupPhase::Failed||terminal())return false;
        if(updates_>=8192)return fail();
        auto* destination=out;ChallengeSetupEvent emitted;out=&emitted;
        switch(phase_){
        case ChallengeSetupPhase::Entry:{
            if(!entry_.step(entry_resources_ready,pads,&out->entry))return fail();
            const auto in=entry_.visual_input();visible_=in.visible;interactive_=in.interactive;closing_=in.closing;difficulty_=in.selection;
            out->sounds=out->entry.sounds;out->sound_count=out->entry.sound_count;port_=entry_.port();participants_=entry_.participants();
            if(out->entry.players_requested){inputs_[port_]=entry_.input();energy_=out->entry.committed_energy;if(!players_.open(pages_profile_,port_,inputs_[port_]))return fail();phase_=ChallengeSetupPhase::Players;}
            if(out->entry.main_requested)phase_=ChallengeSetupPhase::MainRequested;
            break;}
        case ChallengeSetupPhase::Players:{
            ChallengePlayersEvent event;if(!players_.step(pads,&event))return fail();merge(2,players_.visible_mask(),players_.interactive_mask(),players_.closing_mask());players_choice_=u8(players_.selection());inputs_[port_]=players_.input();out->sounds=event.sounds;out->sound_count=event.sound_count;
            if(event.difficulty_requested){if(!return_page_.open(difficulty_profile_))return fail();phase_=ChallengeSetupPhase::DifficultyReturn;}
            if(event.box_requested){context_=1;team_up_=event.team_up;if(!box_.open(pages_profile_,context_,port_,participants_,inputs_))return fail();phase_=ChallengeSetupPhase::Box;}
            break;}
        case ChallengeSetupPhase::DifficultyReturn:{
            auto& input=inputs_[port_];u16 repeated=0,pressed=0;
            if(return_page_.phase()==ChallengeDifficultyPhase::Active){challenge_setup_input_step(pads[port_],&input);repeated=input.repeated;pressed=input.pressed;}
            ChallengeDifficultyEvent event;if(!return_page_.step(repeated,pressed,&event))return fail();merge(8,return_page_.visible_mask(),return_page_.interactive_mask(),return_page_.closing_mask());difficulty_=u8(return_page_.selection());out->sounds=event.sounds;out->sound_count=event.sound_count;
            //Player Back has not changed setup context0; another difficulty B
            //uses the same B34-skip/D44-cover/DA8-wait/E90-cleanup policy.
            if(event.back_requested)phase_=ChallengeSetupPhase::ExitDispatch;
            if(event.players_requested){energy_=event.committed_energy;if(!players_.open(pages_profile_,port_,input))return fail();phase_=ChallengeSetupPhase::Players;}
            break;}
        case ChallengeSetupPhase::Box:
            if(!box_.step(box_resources_ready,pads,receipt,&out->box))return fail();
            merge(1,box_.visible_mask(),box_.interactive_mask(),box_.closing_mask());inputs_=box_.inputs();
            if(context_==2){port_=box_.port();participants_=box_.participants();}
            if(out->box.exit_confirmation_requested){phase_=ChallengeSetupPhase::ExitConfirmationRequested;out->exit_confirmation_requested=true;out->dialog_port=out->box.dialog_port;
                if(dialogs_enabled_&&!exit_dialog_.open(exit_art_,out->dialog_port,inputs_[out->dialog_port]))return fail();
            }
            if(out->box.selection_requested){phase_=ChallengeSetupPhase::SelectionRequested;out->selection_requested=true;}
            out->sounds={out->box.sounds[0],0};out->sound_count=out->box.sound_count;break;
        case ChallengeSetupPhase::ExitConfirmationRequested:{
            if(!dialogs_enabled_)return fail();
            ChallengeExitEvent event;if(!exit_dialog_.step(pads,&event))return fail();out->exit_dialog_created=event.create;out->sounds=event.sounds;out->sound_count=event.sound_count;
            const auto port=exit_dialog_.port();inputs_[port]=exit_dialog_.input();
            if(event.cancelled){
                if(exit_from_force_){if(!force_owner||!force_.resume_exit_cancel(*force_owner,inputs_[port]))return fail();phase_=ChallengeSetupPhase::Force;exit_from_force_=false;}
                else{if(!box_.resume_after_exit_cancel())return fail();box_.inputs_[port]=inputs_[port];phase_=ChallengeSetupPhase::Box;}
                out->exit_cancelled=true;
            }
            if(event.accepted){phase_=ChallengeSetupPhase::ExitStart;out->exit_accepted=true;}
            break;}
        case ChallengeSetupPhase::Force:{
            if(!force_owner||!force_.step(*force_owner,pads,&out->force))return fail();
            merge(4,force_.visible_mask(),force_.interactive_mask(),force_.closing_mask());inputs_[port_]=force_.input();out->sounds=out->force.sounds;out->sound_count=out->force.sound_count;
            if(out->force.exit_requested){exit_from_force_=true;if(!exit_dialog_.open(exit_art_,port_,inputs_[port_]))return fail();phase_=ChallengeSetupPhase::ExitConfirmationRequested;out->exit_confirmation_requested=true;out->dialog_port=port_;}
            else if(out->force.editor_requested)phase_=ChallengeSetupPhase::ForceEditorRequested;
            else if(out->force.second_box_requested)phase_=ChallengeSetupPhase::SecondBoxRequested;
            else if(out->force.finish_requested)phase_=ChallengeSetupPhase::ForceFinished;
            break;}
        case ChallengeSetupPhase::ExitDispatch:phase_=ChallengeSetupPhase::ExitStart;break;
        case ChallengeSetupPhase::ExitStart:
            out->entry.request_bgm_fade=true;out->entry.bgm_fade_updates=entry_profile_.exit_updates;
            if(!exit_fade_.request(0,entry_profile_.exit_updates,ScreenFadeDirection::Cover))return fail();
            exit_started_=true;phase_=ChallengeSetupPhase::ExitWait;break;
        case ChallengeSetupPhase::ExitWait:if(!exit_fade_.remaining())phase_=ChallengeSetupPhase::ExitCleanup;break;
        case ChallengeSetupPhase::ExitCleanup:exit_fade_.clear();out->entry.retire_widgets=true;out->entry.main_requested=true;phase_=ChallengeSetupPhase::MainRequested;break;
        default:return fail();
        }
        exit_fade_.advance();out->energy=energy_;out->team_up=team_up_;out->port=port_;++updates_;*destination=*out;return true;
    }
    bool cancel_exit_request(){if(dialogs_enabled_||phase_!=ChallengeSetupPhase::ExitConfirmationRequested||!box_.resume_after_exit_cancel())return false;phase_=ChallengeSetupPhase::Box;return true;}
    ChallengeSetupPhase phase()const{return phase_;}bool terminal()const{return phase_==ChallengeSetupPhase::SelectionRequested||phase_==ChallengeSetupPhase::MainRequested||phase_==ChallengeSetupPhase::ForceEditorRequested||phase_==ChallengeSetupPhase::SecondBoxRequested||phase_==ChallengeSetupPhase::ForceFinished||(phase_==ChallengeSetupPhase::ExitConfirmationRequested&&!dialogs_enabled_);}
    const ChallengeForceSelection& force_selection()const{return first_force_held_&&force_.phase()==ChallengeForcePhase::Closed?first_force_:force_;}
    bool complete_force_editor(ChallengeForceEditorBridge& bridge,ForceInventory& owner,const ChallengeSetupButtons& history){
        if(!force_enabled_||phase_!=ChallengeSetupPhase::ForceEditorRequested||!bridge.complete(force_,owner,history))return false;
        inputs_[port_]=history;phase_=ChallengeSetupPhase::Force;return true;
    }
    bool take_force_roster(const ForceInventory& owner,ForceBattleRoster* out){return force_enabled_&&(phase_==ChallengeSetupPhase::SecondBoxRequested||phase_==ChallengeSetupPhase::ForceFinished)&&force_.take_roster(owner,out);}
    bool take_random_request(const ForceInventory& owner,ChallengeForceRequest* out){return force_enabled_&&(phase_==ChallengeSetupPhase::SecondBoxRequested||phase_==ChallengeSetupPhase::ForceFinished)&&force_.take_random_request(owner,out);}
    s32 energy()const{return energy_;}bool team_up()const{return team_up_;}
    std::array<s8,2> box_ports()const{return context_==2?std::array<s8,2>{s8(first_port_),port_==first_port_?s8(-1):s8(port_)}:std::array<s8,2>{s8(port_),-1};}
    const ChallengeExitDialog& exit_dialog()const{return exit_dialog_;}
    const ChallengeDifficultyFlow& entry()const{return entry_;}const ChallengePlayersPage& players()const{return players_;}const ChallengeDifficulty& return_page()const{return return_page_;}const ChallengeLoadBox& box()const{return box_;}ChallengeLoadBox& box(){return box_;}
    ChallengeSetupVisualInput visual_input()const{return {visible_,interactive_,closing_,difficulty_,players_choice_,context_};}
    u8 port()const{return port_;}u8 participants()const{return participants_;}u32 updates()const{return updates_;}
    const ScreenFade& fade()const{return exit_started_?exit_fade_:entry_.fade();}
private:
    friend class ChallengeSetupPresentation;
    bool retained_final_callback(bool clear_input,bool retire){if(phase_!=ChallengeSetupPhase::ForceFinished||!second_enabled_||updates_>=8192)return fail();if(clear_input)inputs_={};if(retire)visible_=interactive_=closing_=0;++updates_;return true;}
    bool fail(){phase_=ChallengeSetupPhase::Failed;return false;}
    void merge(u8 mask,u8 visible,u8 interactive,u8 closing){visible_=u8((visible_&~mask)|visible);interactive_=u8((interactive_&~mask)|interactive);closing_=u8((closing_&~mask)|closing);}
    ChallengeDifficultyFlow entry_;ChallengeEntryProfile entry_profile_;ScreenFade exit_fade_;ChallengeDifficulty return_page_;ChallengeDifficultyProfile difficulty_profile_;ChallengeSetupPagesProfile pages_profile_;ChallengePlayersPage players_;ChallengeLoadBox box_;
    std::array<ChallengeSetupButtons,4> inputs_{};ChallengeSetupPhase phase_=ChallengeSetupPhase::Closed;u32 updates_=0;s32 energy_=0;u8 port_=0,participants_=0,visible_=0,interactive_=0,closing_=0,difficulty_=0,players_choice_=0,context_=0;bool team_up_=false,exit_started_=false;
    ChallengeExitDialogArt exit_art_;ChallengeExitDialog exit_dialog_;bool dialogs_enabled_=false;
    ChallengeForceSelectionProfile force_profile_;ChallengeForceSelection force_;bool force_enabled_=false,exit_from_force_=false;
    ChallengeForceSelection first_force_;u8 first_port_=0;bool second_enabled_=false,first_force_held_=false;
};
} // namespace gf
