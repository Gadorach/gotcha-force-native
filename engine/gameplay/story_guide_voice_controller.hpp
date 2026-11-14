#pragma once
#include "engine/gameplay/story_guide_voice_pcm.hpp"
#include "engine/gameplay/story_guide_voice_control.hpp"
#include "engine/gameplay/story_guide_voice_request.hpp"
#include "engine/gameplay/option_settings.hpp"
namespace gf {
namespace story_guide_control_detail {struct Scope {bool live=true;};}
//Fresh call/command DATA. No public delivery/played/editor-ready boolean.
struct StoryGuideVoiceCallEvent {
    StoryGuideReference command;u64 sequence=0;u32 key=0,player=0;u8 setting=0,random=0;SourceByteRngState before{},after{};s16 requested_serial=0;
    std::array<SourceAdxEnqueueOutcome,2> enqueue{};std::array<bool,2> empty_alias{};u32 applied_actions=0;bool random_consumed=false,software_committed=false;
    bool requires_audio_consumer()const{return true;}
};
struct StoryGuideVoiceCommandEvent {bool consumed=false;u32 opcode=0,applied_actions=0;std::array<u64,2> generations{};};
struct StoryGuideVoiceControllerSnapshot {
    SourceAdxControlFields fields;std::array<StoryGuideVoicePcmSnapshot,2> backend;std::array<s16,2> requested_at_native_start{};std::array<u64,2> native_start_sequence{};
    u64 calls=0,commands=0,actions=0;unsigned guide_next=0;bool guide_active=false,failed=false;
};
//Native execution of existing source field/ring plans. Borrowed PCM owner must
//outlive this controller; control and audio calls are serialized. Native statuses
//remain separate from SDK status30/observed8/clock/caption/fade cleanup authority.
class StoryGuideVoiceController {
public:
    StoryGuideVoiceController()=default;StoryGuideVoiceController(const StoryGuideVoiceController&)=delete;StoryGuideVoiceController& operator=(const StoryGuideVoiceController&)=delete;
    bool load(const SourceAdxControlProfile& profile,const StoryGuideVoiceLibrary& library,StoryGuideVoicePcmSource& pcm,const SourceByteRngOwner& rng,u32 native_enabled,std::string* error){
        if(loaded_||failed_||closed_||!profile.valid()||!library.ready()||!rng.ready()||!pcm.ready()||pcm.pending_||pcm.pumping_||pcm.controlling_||!pcm.guide_control_owner_.expired()||!pcm.library_.same_resources(library))return fail(error,"fresh source profile/resources/RNG and exclusive serialized backend");
        for(unsigned i=0;i<2;++i){const auto& s=pcm.state_.slots[i];if(s.phase!=StoryGuideVoicePcmPhase::Idle||s.generation||s.gain.control_serial()||s.gain.requested_volume()||s.gain.dirty()||s.gain.settling())return fail(error,"cold native channel construction, not campaign-entry reset/adoption");}
        SourceAdxControlFields initialized;if(!source_adx_initialize_fields_plan(profile,{},&initialized))return fail(error,"native initialized source fields");initialized.enabled=native_enabled;initialized.handle_present={true,true,false};profile_=profile;library_=library;fields_=initialized;pcm_=&pcm;rng_=rng.snapshot();scope_=std::make_shared<story_guide_control_detail::Scope>();pcm.guide_control_owner_=scope_;for(unsigned i=0;i<2;++i)handles_[i]=pcm.handle(i);loaded_=true;if(error)error->clear();return true;
    }
    bool ready()const{if(!loaded_||failed_||closed_||!scope_||!scope_->live||!pcm_||!pcm_->ready()||pcm_->guide_control_owner_.lock()!=scope_)return false;for(const auto& h:handles_)if(!pcm_->current(h))return false;return true;}
    bool begin_guide(const StoryGuideBank& bank,const SourceByteRngOwner& rng,std::string* error){
        if(!boundary()||guide_active_||!bank.ready()||!rng.same_owner(rng_))return false;
        for(unsigned n=0;n<2;++n){StoryGuideCommand c;if(!bank.command(4,n?5:0,&c)||c.opcode!=1||c.operands.size()!=2||c.operands[0]!=14||c.operands[1]!=60+n||!library_.audio(60+n))return fail(error,"fixed bank14 guide commands");}
        bank_=bank;guide_next_=0;guide_active_=true;if(error)error->clear();return true;
    }
    bool end_guide(){if(!boundary())return false;guide_active_=false;bank_={};return true;}
    bool set_enabled(u32 native_enabled){if(!boundary())return false;fields_.enabled=native_enabled;return true;}
    bool issue_guide(const StoryGuideReference& ref,const OptionSettings& settings,SourceByteRngOwner& rng,StoryGuideVoiceCallEvent* out,std::string* error){
        if(out)*out={};
        if(!out||!boundary()||!guide_active_||guide_next_>=2||!rng.same_owner(rng_)||!option_settings_valid(settings)||!story_guide_reference_valid(ref,bank_)||ref.slot!=4||ref.byte_offset!=(guide_next_?5u:0u)||calls_==std::numeric_limits<u64>::max())return false;
        Busy guard(busy_);StoryGuideVoiceCallEvent event;event.command=ref;event.sequence=calls_+1;event.key=60+guide_next_;event.setting=settings.volume[2];event.before=rng.state();
        //EA170 consumes055FC before its fixed arg3=-1 branch. Effects below may
        //fail later: never rewind this real shared RNG or retry the same call.
        if(!rng.next_byte(&event.random))return fail(error,"current shared RNG draw");
        event.random_consumed=true;event.after=rng.state();++calls_;++guide_next_;*out=event;
        StoryGuideVoiceStartPlan plan;if(!story_guide_voice_start_plan(profile_,fields_,event.key,event.setting,&plan))return fail(error,"fresh source start fields");out->player=plan.player;out->requested_serial=plan.fields.players[plan.player].requested_a;out->enqueue=plan.enqueue;out->empty_alias=plan.source_empty_alias;
        for(const auto& action:plan.actions){if(!execute(action,error))return false;++out->applied_actions;if(!ready())return fail(error,"controller invalidated during source call");}
        fields_=std::move(plan.fields);out->software_committed=true;if(error)error->clear();return true;
    }
    bool enqueue(const SourceAdxCommand& command,SourceAdxEnqueueOutcome* outcome,bool* empty_alias,std::string* error){
        if(!outcome||!empty_alias||!boundary()||command.opcode>=4||!(command.known&1)||command.args[0]>=2)return false;
        SourceAdxEnqueuePlan plan;if(!source_adx_enqueue_plan(profile_,fields_.ring,fields_.enabled,command,&plan))return false;fields_.ring=std::move(plan.ring);*outcome=plan.outcome;*empty_alias=plan.source_empty_alias;if(error)error->clear();return true;
    }
    bool consume_one(StoryGuideVoiceCommandEvent* out,std::string* error){
        if(out)*out={};
        if(!out||!boundary()||commands_==std::numeric_limits<u64>::max())return false;
        Busy guard(busy_);for(unsigned i=0;i<2;++i)out->generations[i]=handles_[i].generation();SourceAdxConsumePlan plan;if(!source_adx_consume_plan(profile_,fields_,&plan))return fail(error,"complete known source command head");out->opcode=plan.opcode;if(!plan.consumed){if(error)error->clear();return true;}
        //The third source row belongs to BGM, not a third voice. No routing into
        //the unrelated music owner or silent unsupported archive/member success.
        if(plan.opcode==0&&(plan.fields.staged_archive!=0||(plan.fields.staged_member!=1863&&plan.fields.staged_member!=1864)))return fail(error,"bounded guide archive0/member1863-1864");
        for(const auto& action:plan.actions)if(action.player>=2||(action.kind==SourceAdxActionKind::StartArchiveMember&&(action.archive!=0||(action.member!=1863&&action.member!=1864))))return fail(error,"represented native voice action domain");
        for(const auto& action:plan.actions){if(!execute(action,error))return false;++out->applied_actions;for(unsigned i=0;i<2;++i)out->generations[i]=handles_[i].generation();if(!ready())return fail(error,"controller invalidated during command execution");}
        //A failure after an earlier action keeps that native action, but never
        //publishes the whole command/ring advance or retries uncertain effects.
        fields_=std::move(plan.fields);++commands_;out->consumed=true;if(error)error->clear();return true;
    }
    StoryGuideVoiceControllerSnapshot snapshot()const{StoryGuideVoiceControllerSnapshot s;s.fields=fields_;if(pcm_)for(unsigned i=0;i<2;++i)s.backend[i]=pcm_->snapshot(i);s.requested_at_native_start=requested_at_start_;s.native_start_sequence=start_sequence_;s.calls=calls_;s.commands=commands_;s.actions=actions_;s.guide_next=guide_next_;s.guide_active=guide_active_;s.failed=!ready();return s;}
    void close(){closed_=true;if(scope_)scope_->live=false;scope_.reset();guide_active_=false;}
private:
    struct Busy {bool& busy;explicit Busy(bool& value):busy(value){busy=true;}~Busy(){busy=false;}};
    bool boundary()const{return ready()&&!busy_&&!pcm_->pending_&&!pcm_->pumping_&&!pcm_->controlling_;}
    bool execute(const SourceAdxPendingAction& action,std::string* error){
        if(!ready()||action.player>=2||actions_==std::numeric_limits<u64>::max())return fail(error,"current native action owner/player/budget");
        const unsigned slot=action.player;bool accepted=false;
        switch(action.kind){
        case SourceAdxActionKind::SetSdkVolume:{StoryGuideVoiceGainSchedule schedule;accepted=pcm_->schedule_volume_next_frame(handles_[slot],action.value,&schedule,error);break;}
        case SourceAdxActionKind::StopHandle:accepted=pcm_->stop(handles_[slot],error);if(accepted){handles_[slot]=pcm_->handle(slot);start_sequence_[slot]=0;}break;
        case SourceAdxActionKind::UnpauseHandle:accepted=pcm_->pause_retaining_queued(handles_[slot],0,error);break;
        case SourceAdxActionKind::StartArchiveMember:{if(action.archive!=0||(action.member!=1863&&action.member!=1864))return fail(error,"native guide member mapping");StoryGuideVoiceGeneration next;accepted=pcm_->start(slot,unsigned(action.member-1803),&next,error);if(accepted){handles_[slot]=next;requested_at_start_[slot]=fields_.players[slot].requested_a;start_sequence_[slot]=commands_+1;}break;}
        case SourceAdxActionKind::PauseHandle:accepted=pcm_->pause_retaining_queued(handles_[slot],action.value,error);break;
        }
        if(!accepted)return fail(error,"failed/uncertain native backend action; no replay");
        ++actions_;return true;
    }
    bool fail(std::string* error,const char* why){failed_=true;if(scope_)scope_->live=false;if(error&&error->empty())*error=std::string("guide voice controller: ")+why;return false;}
    SourceAdxControlProfile profile_;StoryGuideVoiceLibrary library_;StoryGuideBank bank_;SourceByteRngSnapshot rng_;StoryGuideVoicePcmSource* pcm_=nullptr;std::array<StoryGuideVoiceGeneration,2> handles_;
    SourceAdxControlFields fields_;std::array<s16,2> requested_at_start_{};std::array<u64,2> start_sequence_{};std::shared_ptr<story_guide_control_detail::Scope> scope_;u64 calls_=0,commands_=0,actions_=0;unsigned guide_next_=0;bool loaded_=false,failed_=false,closed_=false,busy_=false,guide_active_=false;
};
} //namespace gf
