#pragma once
#include <cmath>
#include "engine/assets/story_guide_indicator.hpp"
namespace gf {
struct StoryGuideIndicatorState {
    //0unborn,1registered,2retiring-hidden,3hidden,4released. Mirrors source
    //main18 progression after the native constructor, not GC/GPU allocation.
    u8 phase=0,subphase=0,selector=0,render_group=0;s32 cached_characters=0;u32 callbacks=0,requests=0;f32 x=0;
    bool operator==(const StoryGuideIndicatorState&)const=default;
};
struct StoryGuideIndicatorPlan {
    StoryGuideIndicatorState state;bool birth=false,release=false,random_required=false,request_selector=false;
};
//Declared parent fields only. Actual native wrapper below derives these from
//its OWN popup and supplies the existing current byte stream, never a new seed.
inline bool story_guide_indicator_plan(const StoryGuideIndicatorArt& art,const StoryGuideIndicatorState& before,bool birth,s32 characters,f32 panel_width,StoryGuideIndicatorPlan* out){
    if(!out||!art.valid()||before.phase>4||before.subphase>1||before.selector>2||before.callbacks>=8192||characters<-1||characters>1024||!std::isfinite(panel_width)||panel_width<5||panel_width>20)return false;
    StoryGuideIndicatorPlan p;p.state=before;auto& s=p.state;
    if(s.phase==0){if(!birth){*out=p;return true;}s.phase=1;p.birth=true;}
    else if(birth)return false;
    if(s.phase==1){
        if(s.subphase==0&&characters!=0){s.subphase=1;s.x=std::fma(-art.factor,f32(panel_width-art.width_origin),art.offset);s.cached_characters=characters;s.render_group=7;}
        if(s.subphase==1){if(characters==0){s.phase=2;s.render_group=0;}
            else if(s.cached_characters!=characters){s.cached_characters=characters;p.request_selector=true;++s.requests;if(characters==-1)s.selector=1;else p.random_required=true;}}
    }else if(s.phase==2){s.phase=3;s.render_group=0;}else if(s.phase==3){s.phase=4;p.release=true;}
    if(s.phase!=4||p.release)++s.callbacks;
    *out=p;return true;
}
} //namespace gf
