#pragma once
#include "engine/core/types.hpp"
namespace gf {
struct OptionTransitionConfig {
    u32 main_frame=0,entry_frame=0,option_frame=0,step=0;
};
inline bool option_transition_valid(const OptionTransitionConfig& c){
    return c.main_frame<=100000&&c.entry_frame<c.option_frame&&c.option_frame<=100000&&
           c.step>0&&c.step<=c.option_frame-c.entry_frame;
}
enum class OptionTransitionPhase { Main,Entering,UI,UnportedUI=UI,Leaving };
// Original camera counters: 800C008C (enter) and 800C028C (return). Resource,
// memory-card and settings UI stages are deliberately NOT simulated here.
class OptionTransition {
public:
    bool configure(const OptionTransitionConfig& config){
        if(!option_transition_valid(config))return false;
        config_=config;configured_=true;phase_=OptionTransitionPhase::Main;frame_=config.main_frame;return true;
    }
    bool begin(u8 back_mask){
        if(!configured_||active())return false;
        previous_back_=back_mask;phase_=OptionTransitionPhase::Entering;frame_=config_.entry_frame;return true;
    }
    // True only after completing return to the main camera. B during entry or
    // return is consumed, not queued/replayed when the UI stub becomes active.
    bool update(u8 back_mask){
        const bool back=(back_mask&~previous_back_)!=0;previous_back_=back_mask;
        if(phase_==OptionTransitionPhase::Entering){
            frame_+=config_.step;
            if(frame_>=config_.option_frame){frame_=config_.option_frame;phase_=OptionTransitionPhase::UnportedUI;}
        }else if(phase_==OptionTransitionPhase::UnportedUI){
            if(back)phase_=OptionTransitionPhase::Leaving;
        }else if(phase_==OptionTransitionPhase::Leaving){
            if(frame_-config_.entry_frame<=config_.step){frame_=config_.main_frame;phase_=OptionTransitionPhase::Main;return true;}
            frame_-=config_.step;
        }
        return false;
    }
    bool active()const{return phase_!=OptionTransitionPhase::Main;}
    bool request_leave(){
        if(phase_!=OptionTransitionPhase::UI)return false;
        phase_=OptionTransitionPhase::Leaving;return true;
    }
    OptionTransitionPhase phase()const{return phase_;}
    u32 frame()const{return frame_;}
private:
    OptionTransitionConfig config_{};bool configured_=false;
    OptionTransitionPhase phase_=OptionTransitionPhase::Main;u32 frame_=0;u8 previous_back_=0;
};
} // namespace gf
