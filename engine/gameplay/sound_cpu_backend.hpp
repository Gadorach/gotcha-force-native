#pragma once
// Native 64-slot backend, source2170A8/216F94/217028 priority/free ordering.
// Real native allocation, packet application, running state and per-voice PCM.
// No device/queue/manager ACK; pending dpop MUST be synchronized before render.
#include "engine/gameplay/sound_voice_parameters.hpp"
#include "engine/audio/dsp_voice_subframe.hpp"
#include "engine/audio/dpop_frame.hpp"
namespace gf {
struct SoundCpuBackendEpoch {};
using SoundCpuBackendScope=std::weak_ptr<const SoundCpuBackendEpoch>;
struct SoundCpuLease {SoundVoiceLifecycleIdentity identity;SoundCpuBackendScope scope;};
struct SoundCpuAcquisition {bool acquired=false,stolen=false;SoundCpuLease lease,victim;};
struct SoundCpuDpop {u8 backend_slot=0;std::array<s16,9> lanes{};};
struct SoundCpuSubframe {
    std::array<std::array<s32,32>,9> lanes{};unsigned processed_voices=0;
    bool requires_bus_queue_consumer=true,requires_dpop_sync=false;
};
struct SoundCpuDropFrame {
    std::array<std::array<s32,160>,9> lanes{};DpopFrame rolloff;
    unsigned synchronized_slots=0,processed_voices=0;u64 frame=0;
    bool requires_bus_queue_consumer=true;
};
struct SoundCpuBackendSnapshot {
    std::array<s16,32> heads{},tails{};std::array<u8,64> free{};u32 free_count=0;
    std::array<u8,64> priority{};std::array<bool,64> allocated{},running{},started{},pending_dpop{};
    std::array<s16,64> next{},previous{};std::array<SoundVoiceLifecycleIdentity,64> owners{};
    u64 last_generation=0,callback=0,subframes=0;bool drop_frame_mode=false;
};
class SoundCpuBackend {
public:
    SoundCpuBackend():epoch_(std::make_shared<const SoundCpuBackendEpoch>()){
        heads_.fill(-1);tails_.fill(-1);free_count_=64;for(unsigned i=0;i<64;++i)free_[i]=u8(i);
    }
    SoundCpuBackendScope scope()const{return epoch_;}
    bool valid()const{
        if(!epoch_||free_count_>64||heads_[0]!=-1||tails_[0]!=-1)return false;
        std::array<bool,64> seen{};unsigned active=0;
        for(unsigned priority=1;priority<32;++priority){s16 previous=-1;unsigned count=0;
            for(s16 id=heads_[priority];id!=-1;id=slots_[unsigned(id)].next){if(id<0||id>=64||seen[unsigned(id)]||++count>64)return false;const auto& s=slots_[unsigned(id)];
                if(!s.allocated||s.priority!=priority||s.previous!=previous||!s.voice.ready()||s.owner.token!=u32(id)+1||s.owner.slot>=64||s.owner.event>=64||!s.owner.event_generation||!s.owner.voice_generation||s.owner.voice_generation>last_generation_)return false;
                for(unsigned j=0;j<64;++j)if(seen[j]&&slots_[j].owner.voice_generation==s.owner.voice_generation)return false;
                seen[unsigned(id)]=true;previous=id;++active;
            }
            if(previous!=tails_[priority])return false;
        }
        if(active+free_count_!=64)return false;
        for(unsigned i=0;i<free_count_;++i){const auto id=free_[i];if(id>=64||seen[id]||slots_[id].allocated||slots_[id].priority||slots_[id].next!=-1||slots_[id].previous!=-1)return false;seen[id]=true;}
        return true;
    }
    bool acquire(u8 priority,SoundVoiceLifecycleIdentity owner,const SoundVoiceSeed& seed,SoundCpuAcquisition* out,std::string* error){
        return acquire_initialized(priority,owner,[&](SoundVoiceSeed* value,std::string*){*value=seed;return true;},out,error);
    }
    template<class Initialize>
    bool acquire_initialized(u8 priority,SoundVoiceLifecycleIdentity owner,Initialize&& initialize,SoundCpuAcquisition* out,std::string* error){
        if(out)*out={};
        if(!out||!valid()||priority<1||priority>31||owner.token||owner.event>=64||owner.slot>=64||!owner.event_generation||owner.voice_generation<=last_generation_||owner.event_generation>=owner.voice_generation)return fail(error,"acquisition scope/priority/owner generation");
        s16 slot=free_count_?s16(free_[free_count_-1]):-1;
        if(slot==-1)for(unsigned p=1;p<priority;++p)if(tails_[p]!=-1){slot=tails_[p];break;}
        if(slot==-1){if(error)error->clear();return true;} // source pool outcome, not guard refusal
        SoundVoiceSeed seed;if(!initialize(&seed,error))return false;
        DspVoiceSubframe voice;if(!load(seed,&voice,error))return false;
        SoundCpuAcquisition acquired;acquired.acquired=true;auto& s=slots_[unsigned(slot)];
        if(s.allocated){acquired.stolen=true;acquired.victim={s.owner,epoch_};if(s.voice.running())s.pending_dpop=true;unlink(slot);}
        else --free_count_;
        owner.token=u32(slot)+1;s.allocated=true;s.priority=priority;s.owner=owner;s.started=false;s.voice=std::move(voice);s.previous=-1;s.next=heads_[priority];
        if(s.next==-1)tails_[priority]=slot;else slots_[unsigned(s.next)].previous=slot;heads_[priority]=slot;last_generation_=owner.voice_generation;acquired.lease={owner,epoch_};*out=acquired;if(error)error->clear();return true;
    }
    bool release(const SoundCpuLease& lease,std::string* error){
        if(!same_scope(lease.scope)||!valid())return fail(error,"foreign/expired release scope");
        const auto id=lookup(lease.identity);if(id==-1)return fail(error,"stale/foreign release owner");
        auto& s=slots_[unsigned(id)];if(s.voice.running())s.pending_dpop=true;free_slot(id);if(error)error->clear();return true;
    }
    bool matches(const SoundVoiceLifecycleState& state)const{
        if(!valid()||!sound_voice_lifecycle_valid(state))return false;
        unsigned owned=0;for(unsigned i=0;i<64;++i)if(sound_voice_backend_owned(state.voices.voices[i])){if(lookup(sound_voice_lifecycle_identity(state,u16(i)))==-1)return false;++owned;}
        return owned==64-free_count_;
    }
    bool observe(const SoundCpuBackendScope& scope,const SoundVoiceLifecycleState& state,std::vector<SoundVoiceLifecycleBackendState>* out,std::string* error)const{
        if(out)out->clear();
        if(!out||!same_scope(scope)||!matches(state)||state.callback==std::numeric_limits<u64>::max()||state.callback!=callback_)return fail(error,"observation scope/owner/callback state");
        std::vector<SoundVoiceLifecycleBackendState> result;
        for(s16 event=state.events.head;event!=-1;event=state.events.events[unsigned(event)].next12){const auto& e=state.events.events[unsigned(event)];if(e.busy03||e.flag01)continue;
            for(s16 child=e.child14;child!=-1;child=state.voices.voices[unsigned(child)].next){const auto slot=u16(child);if(state.voices.voices[slot].stolen25||state.envelopes[slot].phase>=5)continue;const auto identity=sound_voice_lifecycle_identity(state,slot);const auto id=lookup(identity);if(id==-1)return fail(error,"missing actual native backend owner");result.push_back({callback_+1,identity,u16(slots_[unsigned(id)].voice.running()?1:0)});}
        }
        *out=std::move(result);if(error)error->clear();return true;
    }
    bool apply_callback(const SoundCpuBackendScope& scope,u64 callback,std::span<const SoundVoiceParameterPacket> packets,std::string* error){
        if(!same_scope(scope)||!valid()||callback_==std::numeric_limits<u64>::max()||callback!=callback_+1||packets.size()>64)return fail(error,"packet scope/callback/budget");
        auto candidate=*this;std::array<bool,64> touched{};
        for(const auto& packet:packets){if(packet.callback!=callback||packet.identity.slot>=64||packet.identity.event>=64||!packet.identity.event_generation||!packet.identity.voice_generation||packet.identity.token<1||packet.identity.token>64||(packet.stop_and_free&&(packet.publish||packet.start||packet.pitch_write))||(!packet.publish&&(packet.start||packet.pitch_write)))return fail(error,"packet identity/operation coherence");
            if(!packet.publish&&!packet.stop_and_free)continue; // old done/stolen child cannot access token's new owner
            const s16 id=candidate.lookup(packet.identity);if(id==-1||touched[unsigned(id)])return fail(error,"stale/foreign/duplicate packet owner");touched[unsigned(id)]=true;auto& s=candidate.slots_[unsigned(id)];
            if(packet.stop_and_free){s.pending_dpop=true;if(!s.voice.stop(error))return false;candidate.free_slot(id);continue;}
            if(packet.start&&s.started)return fail(error,"duplicate source initial-start packet");
            if(!packet.pitch_write&&packet.src_q16!=s.voice.linear_state().ratio)return fail(error,"clean packet changed retained SRC");
            if(!s.voice.set_parameters(packet.ve_volume,packet.ve_delta,packet.mix,packet.src_q16,error))return false;
            if(packet.start){if(!s.voice.start(error))return false;s.started=true;}
        }
        candidate.callback_=callback;if(!candidate.valid())return fail(error,"derived callback pool topology");*this=std::move(candidate);if(error)error->clear();return true;
    }
    bool render_subframe(const SoundCpuBackendScope& scope,SoundCpuSubframe* out,std::string* error){
        if(drop_frame_mode_){if(out)*out={};return fail(error,"explicit drop-frame mode cannot bypass its rolloff via legacy subframes");}
        return render_plain(scope,out,error);
    }
    bool render_drop_frame(const SoundCpuBackendScope& scope,SoundCpuDropFrame* out,std::string* error){
        if(out)*out={};
        if(!out||!same_scope(scope)||!valid()||subframes_%5!=0||subframes_>std::numeric_limits<u64>::max()-5)return fail(error,"drop-frame scope/state/aligned audio-domain frame required");
        auto candidate=*this;std::vector<std::array<s16,9>> contributions;
        for(const auto& slot:candidate.slots_)if(slot.pending_dpop)contributions.push_back(slot.physical_dpop);
        DpopPlan prepared;if(!dpop_prepare(candidate.dpop_,contributions,&prepared,error))return false;
        for(auto& slot:candidate.slots_)slot.pending_dpop=false;
        SoundCpuDropFrame result;result.rolloff=prepared.frame;result.synchronized_slots=unsigned(contributions.size());result.frame=prepared.state.frames;
        for(unsigned subframe=0;subframe<5;++subframe){SoundCpuSubframe pcm;if(!candidate.render_plain(scope,&pcm,error))return false;result.processed_voices+=pcm.processed_voices;
            for(unsigned lane=0;lane<9;++lane)for(unsigned i=0;i<32;++i){const auto position=subframe*32+i;result.lanes[lane][position]=dpop_detail::add(pcm.lanes[lane][i],dpop_detail::sample(prepared.frame,lane,position));}
        }
        candidate.dpop_=prepared.state;candidate.drop_frame_mode_=true;*this=std::move(candidate);*out=result;if(error)error->clear();return true;
    }
    const DpopState& dpop_state()const{return dpop_;}
private:
    bool render_plain(const SoundCpuBackendScope& scope,SoundCpuSubframe* out,std::string* error){
        if(out)*out={};
        if(!out||!same_scope(scope)||!valid()||subframes_==std::numeric_limits<u64>::max())return fail(error,"render scope/state/budget");
        for(const auto& s:slots_)if(s.pending_dpop){out->requires_dpop_sync=true;return fail(error,"pending source dpop synchronization/consumer required");}
        auto candidate=*this;SoundCpuSubframe result;
        for(auto& s:candidate.slots_)if(s.allocated){const auto before=s.voice.mix_state().mix;DspVoiceSubframePcm voice;if(!s.voice.render(&voice,error))return false;if(!voice.processed)continue;++result.processed_voices;
            for(unsigned lane=0;lane<9;++lane){for(unsigned i=0;i<32;++i)result.lanes[lane][i]+=voice.lanes[lane][i];if(before[lane*2]||before[lane*2+1])s.physical_dpop[lane]=s.voice.mix_state().dpop[lane];}
        }
        ++candidate.subframes_;*this=std::move(candidate);*out=result;if(error)error->clear();return true;
    }
public:
    std::vector<SoundCpuDpop> pending_dpop()const{std::vector<SoundCpuDpop> result;for(unsigned i=0;i<64;++i)if(slots_[i].pending_dpop)result.push_back({u8(i),slots_[i].physical_dpop});return result;}
    SoundCpuBackendSnapshot snapshot()const{
        SoundCpuBackendSnapshot result;result.heads=heads_;result.tails=tails_;result.free=free_;result.free_count=free_count_;result.last_generation=last_generation_;result.callback=callback_;result.subframes=subframes_;result.drop_frame_mode=drop_frame_mode_;
        for(unsigned i=0;i<64;++i){const auto& s=slots_[i];result.priority[i]=s.priority;result.allocated[i]=s.allocated;result.running[i]=s.voice.running();result.started[i]=s.started;result.pending_dpop[i]=s.pending_dpop;result.next[i]=s.next;result.previous[i]=s.previous;result.owners[i]=s.owner;}return result;
    }
private:
    struct Slot {bool allocated=false,started=false,pending_dpop=false;u8 priority=0;s16 previous=-1,next=-1;SoundVoiceLifecycleIdentity owner;DspVoiceSubframe voice;std::array<s16,9> physical_dpop{};};
    bool same_scope(const SoundCpuBackendScope& scope)const{const auto p=scope.lock();return p&&epoch_&&p==epoch_;}
    s16 lookup(const SoundVoiceLifecycleIdentity& owner)const{if(owner.token<1||owner.token>64)return -1;const auto id=u16(owner.token-1);return slots_[id].allocated&&slots_[id].owner==owner?s16(id):-1;}
    void unlink(s16 id){auto& s=slots_[unsigned(id)];if(s.previous==-1)heads_[s.priority]=s.next;else slots_[unsigned(s.previous)].next=s.next;if(s.next==-1)tails_[s.priority]=s.previous;else slots_[unsigned(s.next)].previous=s.previous;s.next=s.previous=-1;}
    void free_slot(s16 id){auto& s=slots_[unsigned(id)];unlink(id);s.allocated=false;s.started=false;s.priority=0;s.owner={};s.voice={};free_[free_count_++]=u8(id);}
    static bool load(const SoundVoiceSeed& seed,DspVoiceSubframe* out,std::string* error){
        if(!seed.resource||seed.dsp>=seed.resource->bank.dsp.size())return fail(error,"qualified seed/resource/DSP required");
        const auto& d=seed.resource->bank.dsp[seed.dsp];DspAdpcmInfo info;if(!dsp_voice_header({d.header.data(),d.header.size()},&info,error))return false;const auto bytes=dsp_detail::payload_bytes(info.nibbles);
        if(d.offset>seed.resource->payload.size()||bytes>seed.resource->payload.size()-d.offset)return fail(error,"seed payload span");
        if(!out->load({d.header.data(),d.header.size()},{seed.resource->payload.data()+d.offset,std::size_t(bytes)},seed.initial_src_q16,error))return false;
        return out->set_parameters(seed.initial_ve,0,seed.initial_mix,seed.initial_src_q16,error);
    }
    static bool fail(std::string* error,const char* why){if(error)*error=std::string("sound CPU backend: ")+why;return false;}
    std::shared_ptr<const SoundCpuBackendEpoch> epoch_;std::array<Slot,64> slots_;
    std::array<s16,32> heads_{},tails_{};std::array<u8,64> free_{};u32 free_count_=0;u64 last_generation_=0,callback_=0,subframes_=0;
    DpopState dpop_;bool drop_frame_mode_=false;
};
} // namespace gf
