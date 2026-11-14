#pragma once
#include <memory>
#include "engine/assets/challenge_setup_pages.hpp"
#include "engine/gameplay/challenge_setup_input.hpp"
namespace gf {
namespace challenge_box_detail {
struct Scope {u64 sequence=0;bool loading=false,claimed=false;};
struct ReadScope {bool acknowledged=false;};
}
class ChallengeLoadBox;class ChallengeBoxLoadDriver;class ForceCatalog;class ForceInventory;
bool challenge_box_fallback(ChallengeLoadBox&,const ForceCatalog&,const ForceInventory*,ForceInventory&);
class ChallengeBoxLoadRequest {
public:
    bool valid()const{const auto s=scope_.lock();return s&&s->loading&&s->sequence==sequence_&&sequence_;}
    u8 slot()const{return slot_;}u8 port()const{return port_;}u64 sequence()const{return sequence_;}
private:
    friend class ChallengeLoadBox;friend class ChallengeBoxLoadDriver;
    std::weak_ptr<challenge_box_detail::Scope> scope_;u64 sequence_=0;u8 slot_=0,port_=0;
};
class ChallengeBoxLoadReceipt {
public:
    bool valid()const{const auto d=driver_.lock();return request_.valid()&&d&&d->acknowledged;}
private:
    friend class ChallengeLoadBox;friend class ChallengeBoxLoadDriver;
    ChallengeBoxLoadRequest request_;std::weak_ptr<const challenge_box_detail::ReadScope> driver_;bool loaded_=false;
};
enum class ChallengeBoxPhase {Closed,Enter,Opening,Active,PreLoad,Loading,Closing,ExitConfirmationRequested,SelectionRequested,Failed};
struct ChallengeBoxEvent {
    std::array<u16,1> sounds{};unsigned sound_count=0;
    bool request_resources=false,request_load=false,exit_confirmation_requested=false,selection_requested=false,skipped=false,loaded=false;
    u8 slot=0,port=0,dialog_port=0,participants=0;ChallengeBoxLoadRequest load;
};
//8018FF50/FFC0/190038/19028C/190370. Context1 is first player's box;
//context2 excludes that controller and selects the first other A/Start presser.
//Receipt accepts only a genuine validated native read + explicit native notice
//dismissal. Neither a boolean nor the source's card-status numbers can mint it.
class ChallengeLoadBox {
public:
    ChallengeLoadBox()=default;ChallengeLoadBox(const ChallengeLoadBox&)=delete;ChallengeLoadBox& operator=(const ChallengeLoadBox&)=delete;
    ChallengeLoadBox(ChallengeLoadBox&&)=default;ChallengeLoadBox& operator=(ChallengeLoadBox&&)=default;
    bool open(const ChallengeSetupPagesProfile& profile,u8 context,u8 port,u8 participants,const std::array<ChallengeSetupButtons,4>& inputs){
        *this={};if(!profile.valid()||context<1||context>2||port>=4||!participants||(participants&~15u)||!(participants&(1u<<port)))return false;
        profile_=profile;context_=context;port_=port;participants_=participants;inputs_=inputs;scope_=std::make_shared<challenge_box_detail::Scope>();phase_=ChallengeBoxPhase::Enter;return true;
    }
    bool step(bool resources_ready,const std::array<ChallengeSetupPad,4>& pads,const ChallengeBoxLoadReceipt* receipt,ChallengeBoxEvent* out){
        if(!out)return false;
        *out={};if(!scope_||phase_==ChallengeBoxPhase::Closed||phase_==ChallengeBoxPhase::Failed||terminal())return false;
        if(updates_>=8192){phase_=ChallengeBoxPhase::Failed;scope_->loading=false;return false;}
        auto sound=[&](u16 cue){out->sounds[out->sound_count++]=cue;};
        switch(phase_){
        case ChallengeBoxPhase::Enter:
            remaining_=profile_.box_open;visible_=1;interactive_=closing_=0;out->request_resources=true;phase_=ChallengeBoxPhase::Opening;break;
        case ChallengeBoxPhase::Opening:
            if(remaining_)--remaining_;
            if(resources_ready&&!remaining_){loaded_=false;interactive_|=1;phase_=ChallengeBoxPhase::Active;}break;
        case ChallengeBoxPhase::Active:{
            u16 pressed=0;
            if(context_==1){challenge_setup_input_step(pads[port_],&inputs_[port_]);pressed=inputs_[port_].pressed;}
            else for(unsigned i=0;i<4;++i){if(i==port_)inputs_[i].pressed=0;else challenge_setup_input_step(pads[i],&inputs_[i]);pressed|=inputs_[i].pressed;}
            if(pressed&0x200){
                phase_=ChallengeBoxPhase::ExitConfirmationRequested;out->exit_confirmation_requested=true;out->dialog_port=port_;
                if(context_==2)for(unsigned i=0;i<4;++i)if(inputs_[i].pressed&0x200){out->dialog_port=u8(i);break;}
                sound(123);
            }else if(pressed&0x180){
                const bool load=(pressed&0x100)!=0;
                if(context_==2){for(unsigned i=0;i<4;++i)if(i!=port_&&(inputs_[i].pressed&(load?0x100:0x80))){port_=u8(i);participants_|=u8(1u<<i);break;}}
                interactive_&=u8(~1u);remaining_=load?profile_.load_delay:profile_.box_close;out->skipped=!load;
                if(load)phase_=ChallengeBoxPhase::PreLoad;
                else{closing_|=1;phase_=ChallengeBoxPhase::Closing;}
                sound(122);
            }
            break;}
        case ChallengeBoxPhase::PreLoad:
            if(!--remaining_){
                if(scope_->sequence==~u64(0)){phase_=ChallengeBoxPhase::Failed;return false;}
                ++scope_->sequence;scope_->loading=true;scope_->claimed=false;phase_=ChallengeBoxPhase::Loading;
                out->request_load=true;out->load.scope_=scope_;out->load.sequence_=scope_->sequence;out->load.slot_=u8(context_-1);out->load.port_=port_;
            }break;
        case ChallengeBoxPhase::Loading:
            if(receipt){
                if(!receipt->valid()||receipt->request_.scope_.lock()!=scope_||receipt->request_.sequence_!=scope_->sequence||receipt->request_.slot_!=context_-1||receipt->request_.port_!=port_)return false;
                loaded_=receipt->loaded_;scope_->loading=false;remaining_=profile_.box_close;closing_|=1;phase_=ChallengeBoxPhase::Closing;
            }break;
        case ChallengeBoxPhase::Closing:
            if(remaining_)--remaining_;
            if(resources_ready&&!remaining_){visible_&=u8(~1u);phase_=ChallengeBoxPhase::SelectionRequested;out->selection_requested=true;out->loaded=loaded_;out->slot=u8(context_-1);out->port=port_;out->participants=participants_;}break;
        default:return false;
        }
        ++updates_;return true;
    }
    bool resume_after_exit_cancel(){if(!scope_||phase_!=ChallengeBoxPhase::ExitConfirmationRequested)return false;phase_=ChallengeBoxPhase::Active;return true;}
    ChallengeBoxPhase phase()const{return phase_;}bool terminal()const{return phase_==ChallengeBoxPhase::ExitConfirmationRequested||phase_==ChallengeBoxPhase::SelectionRequested;}
    u8 context()const{return context_;}u8 port()const{return port_;}u8 participants()const{return participants_;}unsigned remaining()const{return remaining_;}
    u8 visible_mask()const{return visible_;}u8 interactive_mask()const{return interactive_;}u8 closing_mask()const{return closing_;}bool loaded()const{return loaded_;}
    const std::array<ChallengeSetupButtons,4>& inputs()const{return inputs_;}
private:
    friend class ChallengeBoxLoadDriver;
    friend bool challenge_box_fallback(ChallengeLoadBox&,const ForceCatalog&,const ForceInventory*,ForceInventory&);
    friend class ChallengeSetupFlow;
    ChallengeSetupPagesProfile profile_;std::array<ChallengeSetupButtons,4> inputs_{};std::shared_ptr<challenge_box_detail::Scope> scope_;
    ChallengeBoxPhase phase_=ChallengeBoxPhase::Closed;u32 updates_=0;unsigned remaining_=0;u8 context_=0,port_=0,participants_=0,visible_=0,interactive_=0,closing_=0;bool loaded_=false,published_=false;
};
} // namespace gf
