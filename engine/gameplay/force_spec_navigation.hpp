#pragma once
#include "engine/core/types.hpp"
namespace gf {
enum class ForceSpecPage:u8 {Description=0,Statistics=1,Moves=2,Component=3};
struct ForceSpecNavigationEvent {bool close=false,changed=false;u16 sound=0;};
// Active-page callback801DCD30 only. Resource/fade/actor demonstration and draw
// callbacks are separate consumers; entering this state does not fulfil them.
class ForceSpecNavigation {
public:
    bool open(u8 mask,ForceSpecPage first){
        *this={};const auto page=unsigned(first);if(!mask||(mask&~15u)||page>=4||!(mask&(1u<<page)))return false;
        mask_=mask;page_=first;active_=true;return true;
    }
    bool step(u32 pressed,ForceSpecNavigationEvent* out){
        if(!out)return false;
        *out={};if(!active_||closing_)return false;
        blink_=u8((blink_+1)%71);
        if(pressed&0x200){closing_=true;out->close=true;out->sound=123;return true;}
        const int direction=pressed&0x20002?1:pressed&0x10001?-1:0;if(!direction)return true;
        const auto old=unsigned(page_);auto page=old;
        for(unsigned i=0;i<4;++i){page=unsigned((int(page)+direction+4)%4);if(mask_&(1u<<page))break;}
        if(page!=old){page_=ForceSpecPage(page);blink_=0;out->changed=true;out->sound=119;}return true;
    }
    ForceSpecPage page()const{return page_;}u8 page_mask()const{return mask_;}u8 blink()const{return blink_;}bool closing()const{return closing_;}
private:
    u8 mask_=0,blink_=0;ForceSpecPage page_=ForceSpecPage::Statistics;bool active_=false,closing_=false;
};
//801DCFA0 retains 32-bit angle accumulators; consumers use the low signed16.
// Rotation is independent held C-stick input, not the edge-only page buttons.
struct ForceSpecAngles {u32 pitch=0x800,yaw=0;};
inline ForceSpecAngles force_spec_rotate(ForceSpecAngles state,u32 held){
    if(held&0x800000u)state.pitch+=0x180;
    if(held&0x400000u)state.pitch-=0x180;
    if(held&0x100000u)state.yaw+=0x180;
    if(held&0x200000u)state.yaw-=0x180;
    return state;
}
} // namespace gf
