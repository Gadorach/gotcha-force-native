#pragma once
// Production DATA bridge: accepted source capture -> immutable boundary input ->
// tracked shared-queue receipt. No mutable controller/clock/save callback or actor
// admission. Music remains pending; source manager values remain candidate data.
#include "engine/gameplay/startup_sound_journal.hpp"
#include "engine/gameplay/sound_control_owner.hpp"
#include "engine/gameplay/sound_command_transport.hpp"
#include "engine/gameplay/sound_pcm_boundaries.hpp"
namespace gf {
namespace startup_sound_delivery_detail {
struct Entry {
    SoundPcmEventBoundary boundary;std::shared_ptr<const StartupSoundJournalEntry> captured;
    unsigned source_calls=0,manager_calls=0,ui_calls=0,pending_music=0;
};
struct Page {
    std::array<std::shared_ptr<const Entry>,64> entries;unsigned used=0;u32 first=0;
    std::shared_ptr<const Page> previous;
};
}
class StartupSoundInputTimeline;
class StartupSoundInputView final:public SoundPcmBoundaryProvider {
public:
    bool valid(std::string* error)const override{
        if(producer_.expired()||capture_.expired()||!binding_||!binding_->live()||!controls_.live()||!anchor_.same_owner(controls_)){if(error)*error="startup sound input view: expired/mixed source-control/producer scope";return false;}return true;
    }
    u32 frames()const{return tail_?tail_->first+tail_->used-1:0;}
    u32 available(const SoundCpuControlState&)const override{return frames();}
    std::shared_ptr<const startup_sound_delivery_detail::Entry> find(u32 ordinal)const{
        if(!valid(nullptr)||!ordinal||ordinal>frames())return {};
        auto page=tail_;while(page&&page->first>ordinal)page=page->previous;
        if(!page||ordinal-page->first>=page->used)return {};
        return page->entries[ordinal-page->first];
    }
    bool boundary(u32 ordinal,SoundPcmEventBoundary* out,std::string* error)const override{
        if(out)*out={};
        const auto entry=find(ordinal);if(!out||!entry){if(error)*error="startup sound input view: missing live immutable boundary";return false;}*out=entry->boundary;return true;
    }
    bool controls(const SoundCpuControlState& state,SoundVoiceControlObservation* controls,std::vector<SoundVoiceBankGainObservation>* banks,std::string* error)const override{
        if(controls)*controls={};
        if(banks)banks->clear();
        if(!controls||!banks||!valid(error))return false;
        SoundControlFrameInput frame;if(!controls_.frame(state.parameters.lifecycle.callback+1,&frame,error))return false;*controls=frame.controls;*banks=std::move(frame.banks);return true;
    }
    std::shared_ptr<const SoundPcmBoundaryBinding> binding()const override{return binding_;}
private:
    friend class StartupSoundInputTimeline;
    std::shared_ptr<const startup_sound_delivery_detail::Page> tail_;
    std::weak_ptr<const sound_pcm_boundary_detail::ProducerEpoch> producer_;
    std::weak_ptr<const StartupSoundCaptureScope> capture_;
    std::shared_ptr<const SoundPcmBoundaryBinding> binding_;
    SoundControlSnapshot anchor_,controls_;
};
class StartupSoundInputTimeline {
public:
    StartupSoundInputTimeline()=default;
    StartupSoundInputTimeline(const StartupSoundInputTimeline&)=delete;
    StartupSoundInputTimeline& operator=(const StartupSoundInputTimeline&)=delete;
    StartupSoundInputTimeline(StartupSoundInputTimeline&&)=default;
    StartupSoundInputTimeline& operator=(StartupSoundInputTimeline&&)=default;
    bool initialize(std::shared_ptr<SoundPcmBoundaryBinding> binding,const SoundCommandBanks& commands,const SoundVoiceLibrary& voices,
            const StartupSoundSpatialProfile& spatial,const std::weak_ptr<const StartupSoundCaptureScope>& capture,const SoundControlSnapshot& controls,std::string* error){
        if(producer_||binding_||!binding||capture.expired()||!controls.live()||!startup_sound_spatial_profile_valid(spatial)||!sound_command_voice_bindings_valid(commands,voices))return fail(error,"fresh source binding/capture/control/command-voice resources required");
        auto producer=std::make_shared<const sound_pcm_boundary_detail::ProducerEpoch>();if(!binding->claim(producer,error))return false;
        producer_=std::move(producer);binding_=std::move(binding);commands_=commands;spatial_=spatial;capture_=capture;anchor_=latest_=controls;if(error)error->clear();return true;
    }
    bool append(const StartupSoundCaptureFrame& capture,const StartupSoundCaptureContext& context,const SoundControlSnapshot& controls,std::string* error){
        if(!ready(controls,error)||capture.scope.lock()!=capture_.lock()||capture.frame!=game_.frames()+1||frames()>=8192)return fail(error,"matching consecutive accepted capture/native boundary budget");
        if(!capture.manager_producers&&!capture.manager_actions.empty())return fail(error,"legacy capture cannot contain managed action metadata");
        StartupSoundCommandObservation observation;if(!controls.commands(&observation,error))return false;
        if(observation.gate_declared!=context.commands.gate_declared||observation.settings_declared!=context.commands.settings_declared||observation.disabled!=context.commands.disabled||observation.sfx_index!=context.commands.sfx_index)return fail(error,"captured command settings must come from this native control snapshot");
        StartupSoundCapturePlan plan;const bool ok=capture.manager_producers?startup_sound_capture_managed_commands(commands_,spatial_,capture,context,staging_,&plan,error):startup_sound_capture_commands(commands_,spatial_,capture,context,staging_,&plan,error);if(!ok)return false;
        SoundControlFrameInput mode;if(!controls.frame(1,&mode,error))return false;auto entry=std::make_shared<startup_sound_delivery_detail::Entry>();entry->boundary={frames()+1,plan.candidate,mode.event_mode};entry->source_calls=unsigned(plan.scalar_calls.size());entry->ui_calls=plan.ui_operations;entry->pending_music=unsigned(plan.music.size());
        for(const auto& intent:capture.intents)entry->manager_calls+=intent.origin==StartupSoundIntentOrigin::ActorManagerPre||intent.origin==StartupSoundIntentOrigin::ManagerTail;
        auto page=next_page(entry);if(!game_.append(capture,context,error))return false;entry->captured=game_.view().find(capture.frame);if(!entry->captured)return fail(error,"immutable capture publication failed");
        tail_=std::move(page);staging_=std::move(plan.candidate);staging_.count=0;latest_=controls;if(error)error->clear();return true;
    }
    bool append_idle(const SoundControlSnapshot& controls,std::string* error){
        if(!ready(controls,error)||frames()>=8192)return fail(error,"live idle source boundary/native budget required");
        SoundControlFrameInput mode;if(!controls.frame(1,&mode,error))return false;auto entry=std::make_shared<startup_sound_delivery_detail::Entry>();entry->boundary={frames()+1,staging_,mode.event_mode};tail_=next_page(entry);latest_=controls;if(error)error->clear();return true;
    }
    u32 frames()const{return tail_?tail_->first+tail_->used-1:0;}
    u32 captured_frames()const{return game_.frames();}
    StartupSoundInputView view(const SoundControlSnapshot& latest)const{
        StartupSoundInputView view;view.tail_=tail_;view.producer_=producer_;view.capture_=capture_;view.binding_=binding_;view.anchor_=anchor_;view.controls_=latest;return view;
    }
    StartupSoundInputView view()const{return view(latest_);}
    void reset(){producer_.reset();binding_.reset();tail_.reset();capture_.reset();game_.reset();staging_={};commands_={};spatial_={};anchor_={};latest_={};}
private:
    bool ready(const SoundControlSnapshot& controls,std::string* error)const{if(!producer_||!binding_||!binding_->live()||capture_.expired()||!controls.live()||!anchor_.same_owner(controls))return fail(error,"live same producer/capture/control source required");return true;}
    std::shared_ptr<const startup_sound_delivery_detail::Page> next_page(const std::shared_ptr<const startup_sound_delivery_detail::Entry>& entry)const{
        auto page=std::make_shared<startup_sound_delivery_detail::Page>();if(tail_&&tail_->used<tail_->entries.size())*page=*tail_;else{page->first=frames()+1;page->previous=tail_;}page->entries[page->used++]=entry;return page;
    }
    static bool fail(std::string* error,const char* why){if(error)*error=std::string("startup sound input timeline: ")+why;return false;}
    StartupSoundJournal game_;SoundCommandBanks commands_;StartupSoundSpatialProfile spatial_;StartupSoundCommandQueue staging_;
    std::shared_ptr<const sound_pcm_boundary_detail::ProducerEpoch> producer_;
    std::shared_ptr<SoundPcmBoundaryBinding> binding_;
    std::weak_ptr<const StartupSoundCaptureScope> capture_;
    SoundControlSnapshot anchor_,latest_;
    std::shared_ptr<const startup_sound_delivery_detail::Page> tail_;
};
struct StartupSoundSourceDeliveryState {
    u32 boundary=0,captured_frame=0;u64 device_frames=0,control_frames=0,sequence=0;
    u64 source_call_outcomes=0,manager_actions=0,manager_sound_calls=0,ui_calls=0,pending_music_commands=0;
    StartupSoundState manager_candidate;bool have_manager_candidate=false;
    bool requires_parent_actor_admission=true,requires_original_scene_effect_qualification=true;
};
class StartupSoundSourceCompletion {
public:
    bool observe(const StartupSoundInputView& view,const SoundPcmBoundaryReceipt& receipt,std::string* error){
        if(!view.valid(error)||!receipt.matches(view.binding())||receipt.boundary()>view.frames()||receipt.boundary()<state_.boundary||receipt.sequence()<state_.sequence)return fail(error,"live matching actual tracked receipt/prefix required");
        if(have_source_&&!source_.same_source(receipt))return fail(error,"source session cannot be changed behind completion cursor");
        auto next=state_;
        for(u32 i=state_.boundary+1;i<=receipt.boundary();++i){const auto entry=view.find(i);if(!entry)return fail(error,"immutable covered boundary missing");next.source_call_outcomes+=entry->source_calls;next.manager_sound_calls+=entry->manager_calls;next.ui_calls+=entry->ui_calls;next.pending_music_commands+=entry->pending_music;
            if(entry->captured){const auto& frame=entry->captured->capture;next.captured_frame=frame.frame;next.manager_actions+=frame.manager_actions.size();if(frame.manager_producers){next.manager_candidate=frame.manager_candidate;next.have_manager_candidate=true;}}
        }
        next.boundary=receipt.boundary();next.device_frames=receipt.admitted_device_frames();next.control_frames=receipt.control_frames();next.sequence=receipt.sequence();state_=std::move(next);source_=receipt;have_source_=true;if(error)error->clear();return true;
    }
    const StartupSoundSourceDeliveryState& state()const{return state_;}
    void reset(){state_={};source_={};have_source_=false;}
private:
    static bool fail(std::string* error,const char* why){if(error)*error=std::string("startup source-input completion: ")+why;return false;}
    StartupSoundSourceDeliveryState state_;SoundPcmBoundaryReceipt source_;bool have_source_=false;
};
} // namespace gf
