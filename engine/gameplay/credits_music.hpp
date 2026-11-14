#pragma once
#include <array>
#include "engine/gameplay/menu_music.hpp"
#include "engine/audio/music_fade.hpp"
namespace gf {
class CreditsMusic {
public:
    bool load(AdxAudio menu,AdxAudio credits,u32 rate,const MusicFadeConfig& config,f32 gain){
        loaded_=failed_=scoped_=prepared_=false;switches_=0;current_=wanted_=MusicCue::Menu;
        if(!fade_.configure(config,gain)||!streams_[0].load(std::move(menu),rate)||!streams_[1].load(std::move(credits),rate))return false;
        gain_=gain;loaded_=true;return true;
    }
    bool set_active(bool active){
        if(!loaded_||failed_||prepared_)return false;
        if(scoped_==active)return true;
        if(!active){if(!streams_[u32(current_)].set_active(false)){failed_=true;return false;}}
        else{
            current_=wanted_=MusicCue::Menu;fade_.reset(gain_);
            if(!streams_[0].set_active(true)){failed_=true;return false;}
        }
        scoped_=active;return true;
    }
    bool set_scope(bool active,IAudioBackend& sink){
        const bool changed=active!=scoped_;if(!set_active(active))return false;
        if(changed&&!active&&sink.available()&&!sink.discard_queued()){failed_=true;return false;}
        return true;
    }
    bool request(MusicCue cue){
        if(!loaded_||failed_||u32(cue)>1)return false;
        if(cue==wanted_)return true;
        if(!fade_.request(cue))return false;
        wanted_=cue;return true;
    }
    bool set_level(f32 gain,bool immediate=false){if(!loaded_||failed_||prepared_||!fade_.set_level(gain,immediate))return false;gain_=gain;return true;}
    bool advance(bool available,MusicCue* switched){
        if(switched)*switched=MusicCue::None;
        if(!loaded_||failed_||prepared_||!switched)return false;
        if(!scoped_||!available)return true;
        MusicCue next=MusicCue::None;
        if(!fade_.update(&next)){failed_=true;return false;}
        if(next!=MusicCue::None){
            if(!streams_[u32(current_)].set_active(false)||!streams_[u32(next)].set_active(true)){failed_=true;return false;}
            current_=next;++switches_;*switched=next;
        }
        return true;
    }
    bool prepare(unsigned frames,PcmMixer& mixer){
        if(!loaded_||failed_||prepared_)return false;
        if(!scoped_)return true;
        if(!streams_[u32(current_)].prepare(frames,mixer,fade_.envelope())){failed_=true;return false;}
        prepared_=true;return true;
    }
    bool complete(bool accepted){
        if(!prepared_)return !failed_;
        prepared_=false;if(!streams_[u32(current_)].complete(accepted)){failed_=true;return false;}return true;
    }
    // Legacy standalone route remains single-owner. Shared playback uses the
    // source-only advance/prepare/complete API and PcmQueue's selective bus cut.
    bool pump(PcmMixer& mixer,IAudioBackend& sink){
        MusicCue next;if(!advance(sink.available(),&next))return false;
        if(next!=MusicCue::None&&sink.available()&&!sink.discard_queued()){failed_=true;return false;}
        if(!streams_[u32(current_)].pump(mixer,sink,fade_.envelope())){failed_=true;return false;}return true;
    }
    bool scoped()const{return scoped_;}bool failed()const{return failed_;}
    MusicCue current()const{return current_;}MusicCue wanted()const{return wanted_;}
    const MusicFade& fade()const{return fade_;}u64 switches()const{return switches_;}
    u64 accepted()const{return streams_[0].accepted()+streams_[1].accepted();}
    u64 generated()const{return streams_[0].generated()+streams_[1].generated();}
    u64 wraps()const{return streams_[0].wraps()+streams_[1].wraps();}
    u64 cue_accepted(MusicCue cue)const{return u32(cue)<2?streams_[u32(cue)].accepted():0;}
    u32 source_position()const{return streams_[u32(current_)].source_position();}
private:
    std::array<MenuMusic,2> streams_;MusicFade fade_;f32 gain_=1;
    MusicCue current_=MusicCue::Menu,wanted_=MusicCue::Menu;u64 switches_=0;bool loaded_=false,failed_=false,scoped_=false,prepared_=false;
};
} // namespace gf
