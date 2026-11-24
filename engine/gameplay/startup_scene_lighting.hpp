#pragma once
// Source40940 baseline +40D64 recipe6 +40B94/40A00 four-channel colour state.
// Existing ambient/infinite lighting kernel is reused; no light AObj invented.
#include "engine/assets/startup_scene_pass.hpp"
#include "engine/graphics/lighting.hpp"
#include "engine/graphics/camera.hpp"
#include "engine/gameplay/scene_light_colours.hpp"
namespace gf {
struct StartupLightChannel {bool active=false;u8 phase=0,counter=0,duration=0;s16 remaining=0;std::array<u32,3> from{},target{},current{};};
class StartupSceneLighting {
public:
    bool load(const SceneLighting& base,const StartupScenePassProfile& p,u32 bank_hash,std::string* error){
        ready_=false;channels_={};frame_=0;
        if(!startup_scene_pass_valid(p)||bank_hash!=p.bank_hash||!base.declared||!menu_light_rig_supported(base)||base.lights.size()!=3)return fail(error,"bound source rig/profile");
        SceneLightColours initial;if(!initialize_scene_light_colours(base,&initial,error))return false;
        base_=current_=base;profile_=p;for(unsigned i=0;i<3;++i){baseline_[i]=initial.baseline[i];for(auto& c:channels_)c.current[i]=initial.baseline[i];}ready_=true;if(error)error->clear();return true;
    }
    bool step(u32 frame,bool source_opcode18,std::string* error){
        if(!ready_||frame!=frame_+1||frame>8192)return refuse(error,"consecutive source frame");
        auto next=channels_;
        if(source_opcode18)for(auto& c:next){c.active=true;c.phase=c.counter=0;c.duration=profile_.forward;c.remaining=profile_.hold;c.from=c.current;c.target=profile_.target_colours;}
        for(auto& c:next)if(c.active){
            if(c.phase==0||c.phase==2){c.counter=u8(c.counter+1);if(c.counter==c.duration){c.current=c.target;++c.phase;}
                else for(unsigned i=0;i<3;++i){u32 rgba=c.current[i]&255u;for(unsigned k=0;k<3;++k){const unsigned shift=24-k*8;const f32 from=f32((c.from[i]>>shift)&255u),to=f32((c.target[i]>>shift)&255u);const f32 v=f32(f32(f32(c.counter)*f32(f32(to-from)/f32(c.duration)))+from);if(!std::isfinite(v)||v<0||v>255)return refuse(error,"source RGB domain");rgba|=u32(v)<<shift;}c.current[i]=rgba;}
            }else if(c.phase==1){c.remaining=std::bit_cast<s16>(u16(u16(c.remaining)-1));if(c.remaining<1){c.phase=2;c.counter=0;c.duration=profile_.backward;c.from=c.current;c.target=baseline_;}}
            else if(c.phase==3){c.phase=0;c.active=false;}
            else return refuse(error,"source phase domain");
        }
        channels_=next;current_=base_;for(unsigned i=0;i<3;++i)for(unsigned k=0;k<4;++k)current_.lights[i].colour[k]=u8(channels_[0].current[i]>>(24-k*8));frame_=frame;if(error)error->clear();return true;
    }
    bool camera_without_fog_contribution(const Camera& camera)const{return ready_&&std::isfinite(camera.near_plane)&&std::isfinite(camera.far_plane)&&camera.near_plane>0&&camera.far_plane>camera.near_plane&&camera.far_plane<=profile_.fog_start;}
    bool ready()const{return ready_;}u32 frame()const{return frame_;}const SceneLighting* rig()const{return ready_?&current_:nullptr;}const std::array<StartupLightChannel,4>* channels()const{return ready_?&channels_:nullptr;}
    const std::array<u32,3>& baseline_colours()const{return baseline_;}
private:
    static bool fail(std::string* error,const char* why){if(error)*error=std::string("startup scene lighting: ")+why;return false;}
    bool refuse(std::string* error,const char* why){ready_=false;return fail(error,why);}
    SceneLighting base_,current_;StartupScenePassProfile profile_;std::array<u32,3> baseline_{};std::array<StartupLightChannel,4> channels_{};u32 frame_=0;bool ready_=false;
};
} // namespace gf
