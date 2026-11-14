#pragma once
#include "engine/core/types.hpp"
namespace gf {
enum class ScreenFadeDirection {Reveal,Cover};
//80027AAC/27ADC/27AE8/27C1C. One advance AFTER the scene callback.
// The draw flag is per-callback; reveal's terminal callback retains colour but
// suppresses drawing. Cover keeps publishing opaque colour until clear().
class ScreenFade {
public:
    bool request(u32 rgba,u16 callbacks,ScreenFadeDirection direction){
        if(!callbacks||callbacks>32767||(direction!=ScreenFadeDirection::Reveal&&direction!=ScreenFadeDirection::Cover))return false;
        colour_=rgba;remaining_=total_=callbacks;direction_=direction;active_=true;return true;
    }
    void advance(){
        draw_=false;if(!active_)return;
        if(remaining_)--remaining_;
        if(!remaining_&&direction_==ScreenFadeDirection::Reveal){active_=false;return;}
        const f32 fraction=f32(f32(direction_==ScreenFadeDirection::Reveal?remaining_:total_-remaining_)/f32(total_));
        colour_=(colour_&0xffffff00u)|u8(f32(255.f*fraction));draw_=true;
    }
    // Source clear only changes active/count; the following advance clears draw.
    void clear(){active_=false;remaining_=0;}
    u16 remaining()const{return remaining_;}u32 colour()const{return colour_;}bool draw()const{return draw_;}bool active()const{return active_;}
private:
    u32 colour_=0;u16 remaining_=0,total_=0;ScreenFadeDirection direction_=ScreenFadeDirection::Reveal;bool active_=false,draw_=false;
};
} // namespace gf
