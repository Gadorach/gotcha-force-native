#pragma once
#include "engine/assets/force_spec_lifecycle.hpp"
#include "engine/animation/weighted_slide.hpp"
#include "engine/gameplay/screen_fade.hpp"
#include "engine/gameplay/force_spec_navigation.hpp"
#include <memory>
namespace gf {
enum class ForceSpecPhase {Closed,Resources,Opening,RevealWait,Active,CloseStart,CoverWait,ReleaseDelay,Done};
struct ForceSpecLifecycleEvents {bool show=false,retire=false,release=false,page_changed=false;u16 sound=0;};
struct ForceSpecLifecycleScope {};
//801DCA30/1DCD30/1DCE4C, followed by2821C's fade advance. Resource readiness
// is an explicit observation; this controller does not allocate/acknowledge it.
class ForceSpecLifecycle {
public:
    bool open(const ForceSpecLifecycleProfile& profile,u8 pages,ForceSpecPage first){
        *this={};if(!profile.valid()||!navigation_.open(pages,first))return false;
        profile_=profile;phase_=ForceSpecPhase::Resources;scope_=std::make_shared<const ForceSpecLifecycleScope>();return true;
    }
    bool step(bool resources_ready,u32 pressed,ForceSpecLifecycleEvents* out){
        if(!out)return false;
        *out={};if(phase_==ForceSpecPhase::Closed||phase_==ForceSpecPhase::Done||callbacks_==~u64(0))return false;
        previous_phase_=phase_;
        switch(phase_){
        case ForceSpecPhase::Resources:
            if(resources_ready){phase_=ForceSpecPhase::Opening;out->show=true;fade_.request(0,profile_.fade_callbacks,ScreenFadeDirection::Reveal);}break;
        case ForceSpecPhase::Opening:
            if(weighted_slide_advance(profile_.weights,64/profile_.slide_callbacks,index_,sum_))phase_=ForceSpecPhase::RevealWait;
            z_=f32(profile_.start_z+weighted_slide_scale(profile_.distance,sum_,profile_.total));break;
        case ForceSpecPhase::RevealWait:
            if(!fade_.remaining())phase_=ForceSpecPhase::Active;
            break;
        case ForceSpecPhase::Active:{
            ForceSpecNavigationEvent event;if(!navigation_.step(pressed,&event))return false;
            out->sound=event.sound;out->page_changed=event.changed;if(event.close)phase_=ForceSpecPhase::CloseStart;break;}
        case ForceSpecPhase::CloseStart:
            fade_.request(0,profile_.fade_callbacks,ScreenFadeDirection::Cover);phase_=ForceSpecPhase::CoverWait;break;
        case ForceSpecPhase::CoverWait:
            if(!fade_.remaining()){phase_=ForceSpecPhase::ReleaseDelay;delay_=profile_.release_callbacks;out->retire=true;}break;
        case ForceSpecPhase::ReleaseDelay:
            if(!--delay_){phase_=ForceSpecPhase::Done;fade_.clear();out->release=true;}break;
        default:return false;
        }
        fade_.advance();++callbacks_;return true;
    }
    ForceSpecPhase phase()const{return phase_;}const ScreenFade& fade()const{return fade_;}const ForceSpecNavigation& navigation()const{return navigation_;}
    f32 actor_z()const{return z_;}unsigned slide_index()const{return index_;}f32 slide_sum()const{return sum_;}unsigned release_remaining()const{return delay_;}
    bool visible()const{return phase_!=ForceSpecPhase::Closed&&phase_!=ForceSpecPhase::Resources&&phase_!=ForceSpecPhase::ReleaseDelay&&phase_!=ForceSpecPhase::Done;}
    std::weak_ptr<const ForceSpecLifecycleScope> scope()const{return scope_;}
    u64 callbacks()const{return callbacks_;}u32 bank_hash()const{return profile_.bank_hash;}
    ForceSpecPhase previous_phase()const{return previous_phase_;}
private:
    ForceSpecLifecycleProfile profile_;ForceSpecNavigation navigation_;ScreenFade fade_;ForceSpecPhase phase_=ForceSpecPhase::Closed;
    unsigned index_=0,delay_=0;f32 sum_=0,z_=0;
    std::shared_ptr<const ForceSpecLifecycleScope> scope_;u64 callbacks_=0;ForceSpecPhase previous_phase_=ForceSpecPhase::Closed;
};
} // namespace gf
