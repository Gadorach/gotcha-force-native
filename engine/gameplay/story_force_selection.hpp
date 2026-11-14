#pragma once
//Story F8140/F8310/F833C/F8374/F857C/F859C/F861C/F8678.
//Prepared-controller seam only: no resource/widget construction, actual fade,
//campaign, event eligibility, sound delivery or battle-ready authority.
#include "engine/assets/story_map_rebuild.hpp"
#include "engine/gameplay/story_replay.hpp"
#include "engine/gameplay/source_setup_input.hpp"
#include "engine/gameplay/force_selection.hpp"
namespace gf {
struct StoryForceSetupFacts {
    StoryStartAction action=StoryStartAction::Continue;StoryReplayContext replay;
    u32 active_event_row=0,source50=0;s32 source78=0,event_energy_override=-1;
    bool operator==(const StoryForceSetupFacts&)const=default;
};
struct StoryForceSetupPlan {
    StoryForceSetupFacts facts;bool selector_enabled=false;s32 energy=0;
};
//8017F9F4 reads the ACTIVE row's header+8, not the map highlight. These are
//declared values, NOT a genuine run/mission context or checkpoint-load receipt.
inline bool story_force_setup_plan(const StoryMapRebuildProfile& profile,const StoryForceSetupFacts& facts,StoryForceSetupPlan* out){
    if(!out||!story_map_rebuild_profile_valid(profile)||unsigned(facts.action)>2||!facts.replay.valid()||!facts.active_event_row||facts.active_event_row>profile.events.size())return false;
    StoryForceSetupPlan next;next.facts=facts;next.selector_enabled=facts.action==StoryStartAction::Special||facts.source50!=0;
    next.energy=facts.event_energy_override>=0?facts.event_energy_override:facts.action==StoryStartAction::Special?2200:facts.source78;
    //CCE38 returns prefix -1 for energy<=0 (unlimited). Preserve zero overrides
    //and signed fallback values instead of inventing a positive/default budget.
    *out=next;return true;
}
class StoryForceEditorBridge;
namespace story_force_detail {struct RequestScope {bool live=true,claimed=false;};}
class StoryForceEditorRequest {
public:
    bool valid(const ForceInventory& owner)const{const auto scope=scope_.lock();const auto binding=owner_.lock();return scope&&scope->live&&binding&&owner.ready()&&binding==owner.instance_binding().lock();}
    bool current(const ForceInventory& owner)const{return valid(owner)&&revision_==owner.revision();}
    unsigned force()const{return force_;}u8 port()const{return port_;}s32 energy()const{return energy_;}
    const StoryForceSetupFacts& facts()const{return facts_;}
private:
    friend class StoryForceSelection;friend class StoryForceEditorBridge;
    std::weak_ptr<story_force_detail::RequestScope> scope_;std::weak_ptr<const ForceInventoryScope> owner_;
    u64 revision_=0;unsigned force_=0;u8 port_=0;s32 energy_=0;StoryForceSetupFacts facts_;
};
enum class StoryForcePhase {Closed,Enter,Opening,Active,CloseEnter,Closing,CoverEnter,CoverWait,Result,EditorRequested,BackRequested,FinishRequested,Failed};
enum class StoryForceDestination {None,Editor,Back,Stored};
struct StoryForceEvent {
    std::array<u16,2> sounds{};unsigned sound_count=0;bool changed=false,rejected=false;
    u16 request_cover=0; //Black cover length; NOT a fade-completion receipt.
    StoryForceDestination destination=StoryForceDestination::None;StoryForceEditorRequest edit;
};
class StoryForceSelection {
public:
    StoryForceSelection()=default;StoryForceSelection(const StoryForceSelection&)=delete;StoryForceSelection& operator=(const StoryForceSelection&)=delete;
    StoryForceSelection(StoryForceSelection&&)=default;StoryForceSelection& operator=(StoryForceSelection&&)=default;
    bool open_prepared(const StoryMapRebuildProfile& profile,const StoryForceSetupFacts& facts,const ForceInventory& owner,unsigned requested_force,u8 port,const SourceSetupButtons& history){
        *this={};StoryForceSetupPlan plan;
        if(!story_force_setup_plan(profile,facts,&plan)||!plan.selector_enabled||!owner.ready()||requested_force>=kForceCount||port>=4)return false;
        owner_=owner.instance_binding();revision_=owner.revision();facts_=facts;energy_=plan.energy;port_=port;input_=history;rebuild(owner,requested_force);
        //Story has NO Random row. Refuse zero rows rather than loop forever or
        //fabricate a first force. This is a bounded native safety boundary.
        if(!count_||!selection_.open(owner,selected_force(),energy_,ForceCostMode::Full))return false;
        phase_=StoryForcePhase::Enter;return true;
    }
    bool step(const ForceInventory& owner,const std::array<SourceSetupPad,4>& pads,bool declared_cover_finished,StoryForceEvent* out){
        if(!out)return false;
        *out={};if(phase_==StoryForcePhase::Closed||phase_==StoryForcePhase::Failed||suspended()||terminal())return false;
        if(!current(owner)||updates_>=8192)return fail();
        //F8080 samples ONCE before EVERY local phase, not just Active. A held
        //confirm during Opening must not become a fresh edge on activation.
        source_setup_input_step(pads[port_],&input_);++updates_;
        auto sound=[&](u16 cue){out->sounds[out->sound_count++]=cue;};
        switch(phase_){
        case StoryForcePhase::Enter:remaining_=4;visible_=4;interactive_=closing_=0;phase_=StoryForcePhase::Opening;break;
        case StoryForcePhase::Opening:if(--remaining_<=0){interactive_|=4;phase_=StoryForcePhase::Active;}break;
        case StoryForcePhase::Active:{
            const unsigned prior=cursor_;
            if(input_.repeated&1)cursor_=(cursor_+count_-1)%count_;
            else if(input_.repeated&2)cursor_=(cursor_+1)%count_;
            if(cursor_!=prior){out->changed=true;sound(119);}
            if(input_.pressed&0x200){destination_=StoryForceDestination::Back;visible_=interactive_=0;phase_=StoryForcePhase::CoverEnter;sound(123);}
            else if(input_.pressed&0x400){
                unsigned accepted=0;if(!selection_.choose(owner,selected_force())||!selection_.begin_edit(owner,&accepted)||accepted!=selected_force())return fail();
                destination_=StoryForceDestination::Editor;visible_=interactive_=0;phase_=StoryForcePhase::CoverEnter;sound(122);
            }else if(input_.pressed&0x180){
                if(!selection_.choose(owner,selected_force())||!selection_.confirm(owner)){out->rejected=true;sound(125);}
                else{destination_=StoryForceDestination::Stored;visible_=interactive_=0;closing_=4;phase_=StoryForcePhase::CloseEnter;sound(122);}
            }break;}
        case StoryForcePhase::CloseEnter:remaining_=32;phase_=StoryForcePhase::Closing;break;
        case StoryForcePhase::Closing:
            if(--remaining_<0){out->request_cover=60;phase_=StoryForcePhase::CoverWait;}break;
        case StoryForcePhase::CoverEnter:out->request_cover=30;phase_=StoryForcePhase::CoverWait;break;
        case StoryForcePhase::CoverWait:if(declared_cover_finished)phase_=StoryForcePhase::Result;break;
        case StoryForcePhase::Result:
            //F8678 changes major dispatch; its result routine runs NEXT callback.
            out->destination=destination_;
            if(destination_==StoryForceDestination::Editor){
                request_scope_=std::make_shared<story_force_detail::RequestScope>();out->edit=editor_request();phase_=StoryForcePhase::EditorRequested;
            }else phase_=destination_==StoryForceDestination::Stored?StoryForcePhase::FinishRequested:StoryForcePhase::BackRequested;
            break;
        default:return fail();
        }
        return true;
    }
    //Stored immutable roster candidate only; not a campaign/battle-ready receipt.
    bool take_roster(const ForceInventory& owner,ForceBattleRoster* out){if(out)*out={};return out&&phase_==StoryForcePhase::FinishRequested&&current(owner)&&selection_.take(owner,out);}
    bool terminal()const{return phase_==StoryForcePhase::BackRequested||phase_==StoryForcePhase::FinishRequested;}
    bool suspended()const{return phase_==StoryForcePhase::EditorRequested;}
    StoryForcePhase phase()const{return phase_;}s16 remaining()const{return remaining_;}unsigned updates()const{return updates_;}
    unsigned cursor()const{return cursor_;}unsigned selected_force()const{return cursor_<count_?slots_[cursor_]:kForceCount;}
    std::span<const u8> slots()const{return {slots_.data(),count_};}const SourceSetupButtons& input()const{return input_;}
    u8 visible_mask()const{return visible_;}u8 interactive_mask()const{return interactive_;}u8 closing_mask()const{return closing_;}
    s32 energy()const{return energy_;}const StoryForceSetupFacts& facts()const{return facts_;}
private:
    friend class StoryForceEditorBridge;
    bool bound(const ForceInventory& owner)const{const auto scope=owner_.lock();return scope&&owner.ready()&&scope==owner.instance_binding().lock();}
    bool current(const ForceInventory& owner)const{return bound(owner)&&owner.revision()==revision_;}
    bool fail(){if(request_scope_)request_scope_->live=false;phase_=StoryForcePhase::Failed;return false;}
    void rebuild(const ForceInventory& owner,unsigned requested){count_=cursor_=0;for(unsigned i=0;i<kForceCount;++i)if(owner.state()->forces[i].count){if(i==requested)cursor_=count_;slots_[count_++]=u8(i);}}
    StoryForceEditorRequest editor_request()const{StoryForceEditorRequest r;r.scope_=request_scope_;r.owner_=owner_;r.revision_=revision_;r.force_=selected_force();r.port_=port_;r.energy_=energy_;r.facts_=facts_;return r;}
    bool can_return(const ForceInventory& owner,const StoryForceEditorRequest& request)const{return suspended()&&bound(owner)&&request.valid(owner)&&request.scope_.lock()==request_scope_&&request_scope_->claimed&&owner.state()->force_count!=0;}
    bool resume_editor(const ForceInventory& owner,const StoryForceEditorRequest& request,unsigned force,const SourceSetupButtons& history){
        if(!can_return(owner,request)||force>=kForceCount||!selection_.return_from_edit_nonempty(owner,force))return false;
        rebuild(owner,selection_.force());revision_=owner.revision();input_=history;request_scope_->live=false;request_scope_.reset();phase_=StoryForcePhase::Enter;destination_=StoryForceDestination::None;visible_=interactive_=closing_=0;return true;
    }
    ForceSelection selection_;SourceSetupButtons input_;StoryForceSetupFacts facts_;
    std::weak_ptr<const ForceInventoryScope> owner_;std::shared_ptr<story_force_detail::RequestScope> request_scope_;
    u64 revision_=0;u32 updates_=0;s32 energy_=0;s16 remaining_=0;unsigned count_=0,cursor_=0;
    std::array<u8,kForceCount> slots_{};u8 port_=0,visible_=0,interactive_=0,closing_=0;
    StoryForceDestination destination_=StoryForceDestination::None;StoryForcePhase phase_=StoryForcePhase::Closed;
};
} //namespace gf
