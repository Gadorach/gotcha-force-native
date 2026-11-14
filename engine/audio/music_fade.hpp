#pragma once
#include <algorithm>
#include <cmath>
#include "engine/core/types.hpp"
namespace gf {
enum class MusicCue : u32 { Menu,Credits,None };
struct MusicFadeConfig {u32 updates=0;f32 full_scale=0,min_step=0;};
inline bool music_fade_config_valid(const MusicFadeConfig& c){
    return c.updates>0&&c.updates<=600&&std::isfinite(c.full_scale)&&c.full_scale>0&&c.full_scale<=1024&&
        std::isfinite(c.min_step)&&c.min_step>0&&c.min_step<=c.full_scale;
}
// Operations from 800E9B60/800E9654/800E99C8/800E9C78, in source scalar units.
// This does NOT port the subsequent logarithmic CRI/MusyX gain conversion.
namespace music_fade_detail {
template<class Cue,u32 Last,Cue None> class Fade {
public:
    bool configure(const MusicFadeConfig& c,f32 level){
        if(!music_fade_config_valid(c)||!unit(level))return false;
        config_=c;configured_=true;return reset(level);
    }
    bool reset(f32 level){if(!configured_||!unit(level))return false;level_=level;maximum_=value_=config_.full_scale*level;step_=0;pending_=None;age_=0;failed_=false;return true;}
    bool request(Cue cue){
        if(!configured_||failed_||cue==None||u32(cue)>Last)return false;
        step_=(0.0f-value_)/f32(config_.updates);
        if(step_>=0)step_=-config_.min_step; // source fallback, not a forced duration
        pending_=cue;age_=0;return true;
    }
    bool set_level(f32 level,bool immediate=false){
        if(!configured_||failed_||!unit(level))return false;
        if(level==level_&&!immediate)return true;
        level_=level;value_=std::clamp(config_.full_scale*level_,0.0f,maximum_);
        step_=0;pending_=None;age_=0;return true; // immediate control cancels pending fade/cue
    }
    bool update(Cue* switched){
        if(switched)*switched=None;
        if(!configured_||failed_||!switched)return false;
        if(step_==0)return true;
        if(++age_>8192){failed_=true;return false;} // native safety budget, not game timing
        value_+=step_;
        if(value_>=maximum_){value_=maximum_;step_=0;}
        else if(value_<=0){
            value_=0;
            if(pending_!=None){
                *switched=pending_;pending_=None;
                maximum_=config_.full_scale*level_;step_=-step_;age_=0;
            }else step_=0;
        }
        return true;
    }
    f32 envelope()const{const f32 base=config_.full_scale*level_;return base>0?std::clamp(value_/base,0.0f,1.0f):0;}
    f32 value()const{return value_;}f32 slope()const{return step_;}f32 maximum()const{return maximum_;}
    Cue pending()const{return pending_;}bool failed()const{return failed_;}
private:
    static bool unit(f32 v){return std::isfinite(v)&&v>=0&&v<=1;}
    MusicFadeConfig config_;f32 level_=0,value_=0,maximum_=0,step_=0;Cue pending_=None;
    u32 age_=0;bool configured_=false,failed_=false;
};
} // namespace music_fade_detail
// Existing two-cue domain is unchanged; scene cues use a separate enum/domain.
using MusicFade=music_fade_detail::Fade<MusicCue,1,MusicCue::None>;
} // namespace gf
