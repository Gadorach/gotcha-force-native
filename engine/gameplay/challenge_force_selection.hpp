#pragma once
#include "engine/assets/challenge_force_selection.hpp"
#include "engine/gameplay/challenge_setup_input.hpp"
#include "engine/gameplay/force_selection.hpp"
namespace gf {
class ChallengeForceEditorBridge;class ChallengeRandomAssembly;
enum class ChallengeForceRequestKind {None,Editor,Random};
namespace challenge_force_detail {struct RequestScope {ChallengeForceRequestKind kind=ChallengeForceRequestKind::None;bool live=true,claimed=false;};}
class ChallengeForceRequest {
public:
    bool valid(const ForceInventory& owner)const{const auto s=scope_.lock();return owner.ready()&&s&&s->live&&s->kind==kind_&&owner_scope_.lock()==owner.instance_binding().lock()&&(kind_==ChallengeForceRequestKind::Editor||revision_==owner.revision());}
    bool current(const ForceInventory& owner)const{return valid(owner)&&revision_==owner.revision();}
    ChallengeForceRequestKind kind()const{return kind_;}u8 force()const{return kind_==ChallengeForceRequestKind::Editor?force_:u8(kForceCount);}u8 port()const{return port_;}s32 energy()const{return energy_;}
private:
    friend class ChallengeForceSelection;friend class ChallengeForceEditorBridge;friend class ChallengeRandomAssembly;
    std::weak_ptr<challenge_force_detail::RequestScope> scope_;std::weak_ptr<const ForceInventoryScope> owner_scope_;u64 revision_=0;ChallengeForceRequestKind kind_=ChallengeForceRequestKind::None;u8 force_=0,port_=0;s32 energy_=0;
};
enum class ChallengeForcePhase {Closed,Enter,Opening,Active,Closing,ExitRequested,EditorRequested,SecondBoxRequested,FinishRequested,Failed};
struct ChallengeForceEvent {
    std::array<u16,2> sounds{};unsigned sound_count=0;bool changed=false,rejected=false,exit_requested=false,editor_requested=false,second_box_requested=false,finish_requested=false,random=false;
    u8 port=0,context=0,force=0;ChallengeForceRequest edit;
};
//1905E0/1906C0/190710/19074C/190980. Ring rows are -1(Random),0..n-1;
//actual force slots are a separate increasing-order nonempty list. This page
//does not generate random Borgs, draw a screen or acknowledge editor/battle entry.
class ChallengeForceSelection {
public:
    ChallengeForceSelection()=default;ChallengeForceSelection(const ChallengeForceSelection&)=delete;ChallengeForceSelection& operator=(const ChallengeForceSelection&)=delete;
    ChallengeForceSelection(ChallengeForceSelection&&)=default;ChallengeForceSelection& operator=(ChallengeForceSelection&&)=default;
    bool open(const ChallengeForceSelectionProfile& profile,const ForceInventory& owner,unsigned requested_force,u8 port,u8 context,bool team_up,s32 energy,const ChallengeSetupButtons& input){
        *this={};if(!profile.valid()||!owner.ready()||requested_force>=kForceCount||port>=4||context<1||context>2||energy<1)return false;
        profile_=profile;owner_=owner.instance_binding();revision_=owner.revision();port_=port;context_=context;team_up_=team_up;energy_=energy;input_=input;rebuild(owner,requested_force);
        if(!selection_.open(owner,count_?slots_[unsigned(cursor_)]:0,energy,ForceCostMode::Full))return false;
        phase_=ChallengeForcePhase::Enter;return true;
    }
    bool step(const ForceInventory& owner,const std::array<ChallengeSetupPad,4>& pads,ChallengeForceEvent* out){
        if(!out)return false;
        *out={};if(phase_==ChallengeForcePhase::Closed||phase_==ChallengeForcePhase::Failed||suspended()||terminal())return false;
        if(!current(owner)||updates_>=8192)return fail();
        auto sound=[&](u16 cue){out->sounds[out->sound_count++]=cue;};
        switch(phase_){
        case ChallengeForcePhase::Enter:remaining_=profile_.opening;visible_=4;interactive_=closing_=0;phase_=ChallengeForcePhase::Opening;break;
        case ChallengeForcePhase::Opening:if(!--remaining_){interactive_|=4;phase_=ChallengeForcePhase::Active;}break;
        case ChallengeForcePhase::Active:{
            challenge_setup_input_step(pads[port_],&input_);const s16 prior=cursor_;
            if(input_.repeated&1){--cursor_;while(cursor_<-1)cursor_=s16(cursor_+s16(count_)+1);}
            else if(input_.repeated&2){++cursor_;while(cursor_>=s16(count_))cursor_=s16(cursor_-s16(count_)-1);}
            if(cursor_!=prior){out->changed=true;sound(119);}
            if(input_.pressed&0x200){phase_=ChallengeForcePhase::ExitRequested;out->exit_requested=true;sound(123);}
            else if(input_.pressed&0x400){
                const unsigned force=cursor_==-1||!count_?(count_?slots_[0]:0):selected_force();
                if(force>=kForceCount||!selection_.choose(owner,force))return fail();
                unsigned accepted=0;if(!selection_.begin_edit(owner,&accepted)||accepted!=force)return fail();
                request_scope_=std::make_shared<challenge_force_detail::RequestScope>();request_scope_->kind=ChallengeForceRequestKind::Editor;edit_force_=u8(force);phase_=ChallengeForcePhase::EditorRequested;out->editor_requested=true;out->force=edit_force_;out->edit=request(owner,ChallengeForceRequestKind::Editor,edit_force_);sound(122);
            }else if(input_.pressed&0x180){
                const unsigned force=selected_force();const bool accepted=cursor_==-1||(force<kForceCount&&selection_.choose(owner,force)&&selection_.confirm(owner));
                if(!accepted){out->rejected=true;sound(125);}
                else{random_=cursor_==-1;remaining_=profile_.closing;interactive_&=u8(~4u);closing_|=4;phase_=ChallengeForcePhase::Closing;sound(122);}
            }
            break;}
        case ChallengeForcePhase::Closing:
            if(!--remaining_){
                if(context_==1&&team_up_){visible_&=u8(~4u);closing_&=u8(~4u);phase_=ChallengeForcePhase::SecondBoxRequested;out->second_box_requested=true;}
                else{phase_=ChallengeForcePhase::FinishRequested;out->finish_requested=true;}
            }break;
        default:return fail();
        }
        out->random=random_;out->port=port_;out->context=context_;++updates_;return true;
    }
    bool resume_exit_cancel(const ForceInventory& owner,const ChallengeSetupButtons& input){if(phase_!=ChallengeForcePhase::ExitRequested||!current(owner))return false;input_=input;phase_=ChallengeForcePhase::Active;return true;}
    bool take_roster(const ForceInventory& owner,ForceBattleRoster* out){if(out)*out={};return out&&terminal()&&!random_&&current(owner)&&selection_.take(owner,out);}
    bool take_random_request(const ForceInventory& owner,ChallengeForceRequest* out){
        if(!out)return false;
        *out={};if(!terminal()||!random_||random_taken_||!current(owner))return false;
        request_scope_=std::make_shared<challenge_force_detail::RequestScope>();request_scope_->kind=ChallengeForceRequestKind::Random;*out=request(owner,ChallengeForceRequestKind::Random,0);random_taken_=true;return true;
    }
    bool terminal()const{return phase_==ChallengeForcePhase::SecondBoxRequested||phase_==ChallengeForcePhase::FinishRequested;}
    bool suspended()const{return phase_==ChallengeForcePhase::EditorRequested||phase_==ChallengeForcePhase::ExitRequested;}
    ChallengeForcePhase phase()const{return phase_;}s16 cursor()const{return cursor_;}unsigned count()const{return count_;}unsigned remaining()const{return remaining_;}
    unsigned selected_force()const{return cursor_>=0&&unsigned(cursor_)<count_?slots_[unsigned(cursor_)]:kForceCount;}
    std::span<const u8> slots()const{return {slots_.data(),count_};}const ChallengeSetupButtons& input()const{return input_;}
    u8 visible_mask()const{return visible_;}u8 interactive_mask()const{return interactive_;}u8 closing_mask()const{return closing_;}
private:
    friend class ChallengeForceEditorBridge;
    bool bound(const ForceInventory& owner)const{return owner.ready()&&owner_.lock()&&owner_.lock()==owner.instance_binding().lock();}
    bool current(const ForceInventory& owner)const{return bound(owner)&&owner.revision()==revision_;}
    bool fail(){phase_=ChallengeForcePhase::Failed;if(request_scope_)request_scope_->live=false;return false;}
    void rebuild(const ForceInventory& owner,unsigned requested){count_=0;cursor_=0;for(unsigned i=0;i<kForceCount;++i)if(owner.state()->forces[i].count){if(i==requested)cursor_=s16(count_);slots_[count_++]=u8(i);}}
    ChallengeForceRequest request(const ForceInventory& owner,ChallengeForceRequestKind kind,u8 force)const{ChallengeForceRequest r;r.scope_=request_scope_;r.owner_scope_=owner_;r.revision_=owner.revision();r.kind_=kind;r.force_=force;r.port_=port_;r.energy_=energy_;return r;}
    bool can_return(const ForceInventory& owner,const ChallengeForceRequest& token)const{return phase_==ChallengeForcePhase::EditorRequested&&bound(owner)&&token.kind_==ChallengeForceRequestKind::Editor&&token.force_==edit_force_&&token.scope_.lock()==request_scope_&&token.valid(owner);}
    bool resume_editor(const ForceInventory& owner,const ChallengeForceRequest& token,unsigned force,const ChallengeSetupButtons& input){
        if(!can_return(owner,token)||force>=kForceCount||!owner.state()->force_count||!selection_.return_from_edit_nonempty(owner,force))return false;
        rebuild(owner,selection_.force());revision_=owner.revision();input_=input;request_scope_->live=false;request_scope_.reset();phase_=ChallengeForcePhase::Enter;return true;
    }
    ChallengeForceSelectionProfile profile_;ForceSelection selection_;ChallengeSetupButtons input_;std::weak_ptr<const ForceInventoryScope> owner_;std::shared_ptr<challenge_force_detail::RequestScope> request_scope_;u64 revision_=0;u32 updates_=0;s32 energy_=0;s16 cursor_=0;std::array<u8,kForceCount> slots_{};unsigned count_=0;u16 remaining_=0;u8 port_=0,context_=0,visible_=0,interactive_=0,closing_=0,edit_force_=0;bool team_up_=false,random_=false,random_taken_=false;ChallengeForcePhase phase_=ChallengeForcePhase::Closed;
};
} // namespace gf
