#pragma once
#include "engine/assets/adx.hpp"
namespace gf {
struct PcmPlayerCursor {
    u32 position=0,phase=0;u64 loops=0,rendered=0;bool playing=false;
    bool operator==(const PcmPlayerCursor&)const=default;
};
inline bool pcm_player_audio_valid(const AdxAudio& audio,u32 target_rate){
    return (audio.channels==1||audio.channels==2)&&audio.rate>=8000&&audio.rate<=192000&&
        target_rate>=8000&&target_rate<=192000&&audio.frames&&audio.frames<=kMaxAdxFrames&&
        audio.samples.size()==std::size_t(audio.frames)*audio.channels&&
        (!audio.loop||(audio.loop_start<audio.loop_end&&audio.loop_end<=audio.frames));
}
// Same rational player kernel, also usable with a staged cursor and immutable
// decoded resources. A cursor is numeric DATA, never queue/speaker authority.
inline bool pcm_player_render_cursor(const AdxAudio& audio,u32 target,PcmPlayerCursor* cursor,unsigned frames,
        std::vector<s16>* out,u32* produced=nullptr){
    if(!cursor||!cursor->playing||!out||!frames||frames>4096||!pcm_player_audio_valid(audio,target)||
       cursor->position>=audio.frames||cursor->phase>=target||
       (audio.loop&&cursor->position>=audio.loop_end))return false;
    auto next=*cursor;std::vector<s16> result(std::size_t(frames)*2,0);u32 count=0;
    for(unsigned i=0;i<frames&&next.playing;++i){
        u32 following=next.position+1;
        if(audio.loop&&following>=audio.loop_end)following=audio.loop_start;
        else if(following>=audio.frames)following=audio.frames-1;
        for(unsigned channel=0;channel<2;++channel){const unsigned source=audio.channels==1?0:channel;
            const s64 a=audio.samples[std::size_t(next.position)*audio.channels+source],b=audio.samples[std::size_t(following)*audio.channels+source];
            result[i*2+channel]=s16((a*(target-next.phase)+b*next.phase)/target);
        }
        ++count;++next.rendered;const u64 next_phase=u64(next.phase)+audio.rate;
        const u64 next_position=u64(next.position)+next_phase/target;next.phase=u32(next_phase%target);
        if(audio.loop&&next_position>=audio.loop_end){
            const u32 length=audio.loop_end-audio.loop_start;const u64 past=next_position-audio.loop_end;
            next.loops+=1+past/length;next.position=audio.loop_start+u32(past%length);
        }else if(next_position>=audio.frames)next.playing=false;
        else next.position=u32(next_position);
    }
    *cursor=next;*out=std::move(result);if(produced)*produced=count;return true;
}
// Rational-phase native linear resampling. Loops use the metadata-defined
// [start,end) PCM interval; no prediction history is approximated at a wrap.
class PcmPlayer {
public:
    bool load(AdxAudio audio,u32 target_rate){
        loaded_=state_.playing=false;audio_={};
        if(!pcm_player_audio_valid(audio,target_rate))return false;
        audio_=std::move(audio);target_=target_rate;loaded_=true;stop();return true;
    }
    bool play(){if(!loaded_)return false;stop();state_.playing=true;return true;}
    void stop(){state_={};}
    bool render(unsigned frames,std::vector<s16>* out,u32* produced=nullptr){
        return loaded_&&pcm_player_render_cursor(audio_,target_,&state_,frames,out,produced);
    }
    bool playing()const{return state_.playing;}u64 loops()const{return state_.loops;}u64 rendered()const{return state_.rendered;}
    u32 source_position()const{return state_.position;}u32 phase()const{return state_.phase;}
    const PcmPlayerCursor& cursor()const{return state_;}
private:
    AdxAudio audio_;u32 target_=0;PcmPlayerCursor state_;bool loaded_=false;
};
} // namespace gf
