#pragma once
#include "engine/audio/pcm_player.hpp"
#include "engine/audio/pcm_mixer.hpp"
namespace gf {
// Native queue-paced single-cue producer. CreditsMusic coordinates two instances;
// the optional menu-only fallback cuts/flushes on unsupported destinations.
class MenuMusic {
public:
    bool load(AdxAudio audio,u32 output_rate){
        failed_=scoped_=pending_=false;accepted_=generated_=wraps_=0;pending_frames_=0;return player_.load(std::move(audio),output_rate);
    }
    // Source-only control for a shared PcmQueue owner. Never touches the sink.
    bool set_active(bool active){
        if(failed_||pending_)return false;
        if(active==scoped_)return true;
        if(!active)player_.stop();else if(!player_.play()){failed_=true;return false;}
        scoped_=active;return true;
    }
    bool set_scope(bool active,IAudioBackend& sink){
        const bool changed=active!=scoped_;if(!set_active(active))return false;
        if(changed&&!active&&sink.available()&&!sink.discard_queued()){failed_=true;return false;}
        return true;
    }
    bool prepare(unsigned frames,PcmMixer& mixer,f32 envelope=1.0f){
        if(failed_||pending_)return false;
        if(!scoped_||!player_.playing())return true;
        std::vector<s16> source;u32 produced=0;const auto old_wraps=player_.loops();
        if(!frames||!player_.render(frames,&source,&produced)||!mixer.add(AudioBus::Bgm,source,envelope)){failed_=true;return false;}
        generated_+=produced;wraps_+=player_.loops()-old_wraps;pending_=true;pending_frames_=frames;return true;
    }
    bool complete(bool accepted){
        if(!pending_)return !failed_;
        pending_=false;
        if(!accepted){failed_=true;player_.stop();return false;}
        accepted_+=pending_frames_;pending_frames_=0;return true;
    }
    bool pump(PcmMixer& mixer,IAudioBackend& sink,f32 envelope=1.0f){
        if(failed_)return false;
        if(!scoped_||!sink.available()||!player_.playing())return true;
        unsigned work=0;
        while(sink.frames_available()<2048&&player_.playing()&&work<4096){
            const unsigned frames=unsigned(std::min<u64>(1024,2048-sink.frames_available()));
            std::vector<s16> mixed;
            if(!mixer.begin(frames)||!prepare(frames,mixer,envelope)||!mixer.finish(&mixed)){
                failed_=true;return false;}
            // Never retry a failed/uncertain submission after advancing source.
            if(!sink.try_submit(mixed.data(),frames)){(void)complete(false);return false;}
            if(!complete(true))return false;
            work+=frames;
        }
        return true;
    }
    bool failed()const{return failed_;}bool scoped()const{return scoped_;}
    u64 accepted()const{return accepted_;}u64 generated()const{return generated_;}u64 wraps()const{return wraps_;}
    u32 source_position()const{return player_.source_position();}
private:
    PcmPlayer player_;bool failed_=false,scoped_=false,pending_=false;unsigned pending_frames_=0;u64 accepted_=0,generated_=0,wraps_=0;
};
} // namespace gf
