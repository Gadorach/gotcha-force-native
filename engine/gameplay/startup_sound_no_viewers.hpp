#pragma once
// Explicit source059B8 no-active-viewer scope. F0A74/F0C24 return127/64 before
// consuming position/camera. Bias math only changes caller-local scratch, so its
// unconsumed result is elided here, not guessed from a signed-negative selector.
// This is sound-command equivalence, NOT PPC memory-read/FPSCR/full-actor parity.
// The existing general spatial entry/refusals remain unchanged.
#include <memory>
#include "engine/gameplay/startup_sound_spatial.hpp"
namespace gf {
namespace startup_sound_no_viewers_detail {struct Scope {};}
class StartupSoundNoViewerFrame {
public:
    bool live()const{return epoch_&&!scope_.expired();}
    u64 epoch()const{return epoch_;}
    bool render_selector_known()const{return selector_known_;}
    s8 render_selector()const{return -1;} // meaningful only when known()
private:
    friend class StartupSoundNoViewerOwner;
    std::weak_ptr<const startup_sound_no_viewers_detail::Scope> scope_;
    u64 epoch_=0;bool selector_known_=false;
};
class StartupSoundNoViewerOwner {
public:
    StartupSoundNoViewerOwner()=default;
    StartupSoundNoViewerOwner(const StartupSoundNoViewerOwner&)=delete;
    StartupSoundNoViewerOwner& operator=(const StartupSoundNoViewerOwner&)=delete;
    StartupSoundNoViewerOwner(StartupSoundNoViewerOwner&&)=default;
    StartupSoundNoViewerOwner& operator=(StartupSoundNoViewerOwner&&)=default;
    bool initialize(std::string* error){
        if(scope_||failed_)return fail(error,"fresh/reset source scene required");
        //059B8 sets all four signed viewer IDs toFF. No actor20 or camera state
        // is fabricated. It does not itself establish scene28, so keep unknown.
        scope_=std::make_shared<const startup_sound_no_viewers_detail::Scope>();selector_known_=false;if(error)error->clear();return true;
    }
    bool select_render_slot(unsigned slot,std::string* error){
        if(!ready()||slot>=4)return fail(error,"initialized source render slot required");
        //8C440/8C62C/8C88C copy that slot'sFF into scene28. Not a viewer array0.
        selector_known_=true;if(error)error->clear();return true;
    }
    bool frame(u64 epoch,StartupSoundNoViewerFrame* out,std::string* error)const{
        if(out)*out={};
        if(!out||!ready()||!epoch)return fail(error,"live source scene/nonzero epoch/output required");
        out->scope_=scope_;out->epoch_=epoch;out->selector_known_=selector_known_;if(error)error->clear();return true;
    }
    // Source activity needing a real viewer must switch to a qualified general
    // producer. Do not keep returning no-viewer defaults after an unknown write.
    bool require_active_viewer(std::string* error){
        if(!ready())return fail(error,"initialized source scene required");
        failed_=true;scope_.reset();return fail(error,"actual viewer producer/general spatial path required");
    }
    bool ready()const{return bool(scope_)&&!failed_;}
    bool failed()const{return failed_;}
    void reset(){scope_.reset();failed_=selector_known_=false;}
private:
    static bool fail(std::string* error,const char* why){if(error)*error=std::string("startup sound no viewers: ")+why;return false;}
    std::shared_ptr<const startup_sound_no_viewers_detail::Scope> scope_;bool failed_=false,selector_known_=false;
};
inline bool startup_sound_no_viewers_resolve(const StartupSoundSpatialProfile& profile,const StartupSoundNoViewerFrame& frame,
        const StartupSoundPendingCall& call,std::span<const StartupSoundOwnerObservation> owners,StartupSoundScalarCommand* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("startup sound no-viewer command: ")+why;return false;};
    if(!out||!frame.live()||!startup_sound_spatial_profile_valid(profile)||call.cue<0||call.cue>=384||unsigned(call.kind)>unsigned(StartupSoundCallKind::PauseCentered)||owners.size()>48)return fail("live no-viewer/profile/call/owner budget");
    const bool biased=call.kind==StartupSoundCallKind::StartBiased||call.kind==StartupSoundCallKind::UpdateBiased;
    if(f32_to_bits(call.viewer_bias)!=(biased?f32_to_bits(profile.bias):0u))return fail("source bias operand");
    StartupSoundScalarCommand result;result.cue=call.cue;result.owner=call.owner;result.update=call.kind==StartupSoundCallKind::UpdateDirect||call.kind==StartupSoundCallKind::UpdateBiased||call.kind==StartupSoundCallKind::PauseCentered;result.volume=127;result.pan=64;
    if(!call.owner){if(call.kind==StartupSoundCallKind::PauseCentered)return fail("pause requires live owner");}
    else{const StartupSoundOwnerObservation* owner=nullptr;for(const auto& candidate:owners)if(candidate.generation==call.owner){if(owner)return fail("ambiguous native owner generation");owner=&candidate;}
        if(!owner||!owner->live||owner->epoch!=frame.epoch()||owner->actor96<0||owner->actor96>=6)return fail("live same-epoch actor slot required");
        result.actor96=owner->actor96;if(call.kind==StartupSoundCallKind::PauseCentered)result.volume=0;
    }
    // No position/bias/projection diagnostics are manufactured. Only this new
    // opt-in entry elides unused math under the source no-viewer predicate.
    *out=result;if(error)error->clear();return true;
}
inline bool startup_sound_no_viewers_resolve_batch(const StartupSoundSpatialProfile& profile,const StartupSoundNoViewerFrame& frame,
        std::span<const StartupSoundPendingCall> calls,std::span<const StartupSoundOwnerObservation> owners,StartupSoundSpatialBatch* out,std::string* error){
    if(out)*out={};
    if(!out||calls.size()>48){if(error)*error="startup sound no-viewer batch: output/native budget";return false;}
    StartupSoundSpatialBatch candidate;for(const auto& call:calls){if(!startup_sound_no_viewers_resolve(profile,frame,call,owners,&candidate.commands[candidate.count],error))return false;++candidate.count;}
    if(calls.empty()&&(!frame.live()||!startup_sound_spatial_profile_valid(profile)||owners.size()>48)){if(error)*error="startup sound no-viewer batch: empty-frame source/profile/budget";return false;}
    *out=candidate;if(error)error->clear();return true;
}
} // namespace gf
