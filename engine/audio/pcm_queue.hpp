#pragma once
#include <deque>
#include <memory>
#include "engine/audio/pcm_mixer.hpp"
namespace gf {
class StoryGuideVoicePcmSource;
struct PcmVoiceTailObservation {bool output_available=false;u64 queued_frames=0,slot_frames=0;};
//Exact already-weighted selected-player suffix removed by a successful discard.
//Contains queue-relative leading gaps and tracked terminal padding, but no
//untracked trailing blocks. DATA only: not raw PCM, source generation or heard.
struct PcmSuspendedVoiceTail {std::vector<s64> stereo;u64 discarded_frames=0;};
// Sole native sink-queue owner. Watermark/chunk/work limits remain native policy,
// not original device latency. A bounded tail journal supports exact bus cuts:
// restore ONLY the precisely discarded, not-yet-consumed surviving contributions.
class PcmQueue {
    friend class StoryGuideVoicePcmSource;
public:
    explicit PcmQueue(IAudioBackend& sink):sink_(sink){}
    bool start(){if(started_||failed_||busy_||sink_.frames_available()!=0)return false;started_=true;return true;}
    template<class Prepare,class Complete> bool pump(PcmMixer& mixer,Prepare prepare,Complete complete){
        if(!started_||failed_||busy_)return false;
        if(!sink_.available())return true;
        busy_=true;unsigned work=0;
        while(work<4096){const u64 queued=sink_.frames_available();
            if(queued>history_frames_){busy_=false;failed_=true;return false;} // foreign/inconsistent queue
            if(queued>=2048)break;
            const unsigned frames=unsigned(std::min<u64>(1024,2048-queued));
            std::vector<s16> mixed;PcmContributions block;
            if(!mixer.begin(frames)||!prepare(frames,mixer)||!mixer.finish(&mixed,&block)){
                (void)complete(false);busy_=false;failed_=true;return false;}
            if(!block.bus_mask){if(!complete(true)){busy_=false;failed_=true;return false;}break;}
            generated_+=frames;
            if(!sink_.try_submit(mixed.data(),frames)){(void)complete(false);busy_=false;failed_=true;return false;}
            submitted_+=frames;append(std::move(block),0);
            if(!complete(true)){busy_=false;failed_=true;return false;}
            work+=frames;
        }
        busy_=false;return true;
    }
    bool cut(AudioBus bus){return unsigned(bus)<3?cut_mask(1u<<unsigned(bus)):false;}
    //Physical native voice player0/1 only. Legacy aggregate Voice data in the
    //unplayed tail is a pre-discard refusal, never a fallback to cutting both.
    bool cut_voice_slot(unsigned slot){return slot<2?cut_impl(0,1u<<slot):false;}
    bool suspend_voice_slot(unsigned slot,PcmSuspendedVoiceTail* out){
        if(!out||slot>=2)return false;
        PcmSuspendedVoiceTail next;if(!cut_impl(0,1u<<slot,&next))return false;*out=std::move(next);return true;
    }
    bool clear(){return cut_mask(7);}
    bool failed()const{return failed_;}u64 generated()const{return generated_;}
    u64 submitted()const{return submitted_;}u64 restored()const{return restored_;}u64 discarded()const{return discarded_;}
    u64 journal_frames()const{return history_frames_;}
    //Native unplayed-tail observation, not a generation/played/SDK EOF token.
    //Counts tracked block frames conservatively, including terminal zero padding.
    bool observe_voice_slot(unsigned slot,PcmVoiceTailObservation* out)const{
        if(!out||slot>=2||!started_||failed_||busy_)return false;
        PcmVoiceTailObservation p;if(!sink_.available()){*out=p;return true;}p.output_available=true;p.queued_frames=sink_.frames_available();if(p.queued_frames>history_frames_)return false;u64 remaining=p.queued_frames;for(auto it=history_.rbegin();it!=history_.rend()&&remaining;++it){const u64 count=std::min<u64>(remaining,it->pcm.frames-it->first);if(it->pcm.bus_mask&(1u<<unsigned(AudioBus::Voice))){if(!it->pcm.voice_slots.source_weighted||!pcm_voice_slots_valid(it->pcm))return false;if(it->pcm.voice_slots.mask&(1u<<slot))p.slot_frames+=count;}remaining-=count;}if(remaining)return false;*out=p;return true;
    }
private:
    struct Entry{PcmContributions pcm;unsigned first=0;};
    bool cut_mask(unsigned mask){return cut_impl(mask,0);}
    bool voice_tail_supported(u64 frames)const{
        for(auto it=history_.rbegin();it!=history_.rend()&&frames;++it){const auto n=std::min<u64>(frames,it->pcm.frames-it->first);if((it->pcm.bus_mask&(1u<<unsigned(AudioBus::Voice)))&&(!it->pcm.voice_slots.source_weighted||!pcm_voice_slots_valid(it->pcm)))return false;frames-=n;}return frames==0;
    }
    bool cut_impl(unsigned mask,unsigned voice_slots,PcmSuspendedVoiceTail* suspension=nullptr){
        if(!started_||failed_||busy_)return false;
        if(!sink_.available())return !suspension; // never manufacture a sink transaction
        const auto queued=sink_.frames_available();
        if(queued>history_frames_){failed_=true;return false;}
        if(!queued){history_.clear();history_frames_=0;return true;}
        if(voice_slots&&!voice_tail_supported(queued))return false;
        busy_=true;u64 dropped=0;
        if(!sink_.discard_queued_exact(&dropped)||dropped>queued||dropped>history_frames_||sink_.frames_available()!=0){
            busy_=false;failed_=true;return false;}
        discarded_+=dropped;trim(dropped);
        PcmSuspendedVoiceTail retained;
        if(suspension){
            unsigned offset=0,last=0;for(const auto& entry:history_){const unsigned n=entry.pcm.frames-entry.first;if((entry.pcm.bus_mask&(1u<<unsigned(AudioBus::Voice)))&&(entry.pcm.voice_slots.mask&voice_slots))last=offset+n;offset+=n;}
            retained.stereo.assign(std::size_t(last)*2,0);retained.discarded_frames=dropped;offset=0;
            for(const auto& entry:history_){const unsigned n=entry.pcm.frames-entry.first;if(offset<last&&(entry.pcm.bus_mask&(1u<<unsigned(AudioBus::Voice))))for(unsigned slot=0;slot<2;++slot)if((voice_slots&(1u<<slot))&&(entry.pcm.voice_slots.mask&(1u<<slot))){const auto& lane=entry.pcm.voice_slots.lane[slot];std::copy_n(lane.begin()+std::size_t(entry.first)*2,std::size_t(std::min(n,last-offset))*2,retained.stereo.begin()+std::size_t(offset)*2);}offset+=n;}
        }
        bool surviving=false;for(auto& entry:history_){if(voice_slots){for(unsigned slot=0;slot<2;++slot)if((voice_slots&(1u<<slot))&&!pcm_drop_voice_slot(&entry.pcm,slot)){busy_=false;failed_=true;return false;}}else entry.pcm.bus_mask&=~mask;surviving|=entry.pcm.bus_mask!=0;}
        if(!surviving){history_.clear();history_frames_=0;busy_=false;if(suspension)*suspension=std::move(retained);return true;}
        std::vector<s16> restore;restore.reserve(std::size_t(dropped)*2);
        for(const auto& entry:history_){std::vector<s16> part;
            if(!pcm_encode(entry.pcm,entry.first,entry.pcm.frames-entry.first,&part)){busy_=false;failed_=true;return false;}
            restore.insert(restore.end(),part.begin(),part.end());}
        if(!dropped||restore.size()!=dropped*2||!sink_.try_submit(restore.data(),std::size_t(dropped))){busy_=false;failed_=true;return false;}
        // Transport re-submission is NOT new source generation/admission. Never
        // repeat this transaction if submission fails or acceptance is uncertain.
        submitted_+=dropped;restored_+=dropped;busy_=false;if(suspension)*suspension=std::move(retained);return true;
    }
    void trim(u64 keep){
        while(history_frames_>keep){auto& front=history_.front();const auto available=front.pcm.frames-front.first;
            const auto remove=unsigned(std::min<u64>(available,history_frames_-keep));front.first+=remove;history_frames_-=remove;
            if(front.first==front.pcm.frames)history_.pop_front();}
    }
    void append(PcmContributions block,unsigned first){history_frames_+=block.frames-first;history_.push_back({std::move(block),first});trim(2048);}
    IAudioBackend& sink_;std::deque<Entry> history_;u64 history_frames_=0,generated_=0,submitted_=0,restored_=0,discarded_=0;
    bool started_=false,failed_=false,busy_=false;
    //Opt-in exclusive native two-player owner, never used by legacy callers.
    std::weak_ptr<const void> guide_voice_owner_;
};
} // namespace gf
