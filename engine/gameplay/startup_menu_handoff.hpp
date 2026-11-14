#pragma once
//800BF8D4 negative-submode ->800BFB54 camera bridge. No input/audio/scene
// ownership or host admission: ordered create/sample/navigation effects only.
#include "engine/assets/startup_menu_handoff.hpp"
#include "engine/gameplay/startup_camera.hpp"
namespace gf {
struct StartupMenuHandoffState {
    f32 frame=0,speed=0;u32 updates=0;bool entered=false,navigation_ready=false;
};
struct StartupMenuHandoffStep {
    bool create_menu_objects=false,camera_sample=false,navigation_enabled=false;
    StartupCameraRequestedState camera;
};
class StartupMenuHandoff {
public:
    bool begin(const StartupMenuHandoffProfile& profile,const StartupCameraSet& cameras,
               const StartupCameraPoseState& baseline,bool source_terminal_result,std::string* error){
        ready_=false;profile_={};state_={};pose_={};
        if(!source_terminal_result||!startup_menu_handoff_profile_valid(profile))return fail(error,"terminal result/profile required");
        const auto* main=cameras.clip(255);
        if(!main||baseline.main_binding!=profile.main_camera_hash||cameras.initial().main_binding!=profile.main_camera_hash||profile.goal>main->base_end_frame)return fail(error,"main camera/profile binding/end");
        // Pure preflight; no first-frame camera or UI effect is published here.
        StartupCameraRequestedState first;
        if(!cameras.sample_requested(255,profile.first_frame,baseline,&first,error))return false;
        static_cast<StartupCameraPoseState&>(pose_)=baseline;
        profile_=profile;state_.speed=profile.initial_speed;ready_=true;if(error)error->clear();return true;
    }
    bool step(const StartupCameraSet& cameras,StartupMenuHandoffStep* out,std::string* error){
        if(out)*out={};
        if(!ready_||!out)return refuse(error,"not ready/destination");
        const auto* main=cameras.clip(255);
        if(!main||cameras.initial().main_binding!=profile_.main_camera_hash||profile_.goal>main->base_end_frame)return refuse(error,"stale/foreign main camera binding");
        if(state_.navigation_ready){if(error)error->clear();return true;} // never replay entry/sample/enable
        if(state_.updates>=8192)return refuse(error,"update budget");
        auto next=state_;StartupMenuHandoffStep effects;
        next.speed=f32(next.speed+profile_.acceleration);if(next.speed>profile_.speed_limit)next.speed=profile_.speed_limit;
        if(!next.entered){next.entered=true;next.frame=profile_.first_frame;effects.create_menu_objects=true;}
        else next.frame=f32(next.frame+next.speed);
        if(next.frame>=profile_.goal){next.frame=profile_.goal;next.navigation_ready=true;effects.navigation_enabled=true;}
        if(!std::isfinite(next.frame)||!std::isfinite(next.speed))return refuse(error,"nonfinite derived counter");
        StartupCameraRequestedState sampled;
        if(!cameras.sample_requested(255,next.frame,pose_,&sampled,error)){ready_=false;return false;}
        ++next.updates;effects.camera_sample=true;effects.camera=sampled;
        state_=next;pose_=sampled;*out=std::move(effects);if(error)error->clear();return true;
    }
    const StartupMenuHandoffState& state()const{return state_;}
    const StartupCameraRequestedState* camera()const{return ready_&&state_.entered?&pose_:nullptr;}
    bool ready()const{return ready_;}
private:
    static bool fail(std::string* error,const char* why){if(error)*error=std::string("startup menu handoff: ")+why;return false;}
    bool refuse(std::string* error,const char* why){ready_=false;return fail(error,why);}
    StartupMenuHandoffProfile profile_;StartupMenuHandoffState state_;StartupCameraRequestedState pose_;bool ready_=false;
};
} // namespace gf
