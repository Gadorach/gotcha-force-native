#pragma once
// Source800088A4 removes/rebinds/requests animation onto ONE global main CObj.
// Special scene descriptors do NOT replace its aspect/static projection state.
#include <span>
#include "engine/assets/hsd_camera.hpp"
#include "engine/gameplay/startup_director.hpp"
namespace gf {
struct StartupCameraInput {u8 scene=255;ByteView archive;};
struct StartupCameraPoseState {Camera camera;f32 source_aspect=0,roll=0;u32 main_binding=0;u8 scene=255;bool initialized=false;};
struct StartupCameraState:StartupCameraPoseState {u32 frame=0;};
// Separate requested-time entry: never truncate a fractional source counter or
// silently change the established integer-frame observation/API contract.
struct StartupCameraRequestedState:StartupCameraPoseState {f32 frame=0;};
namespace startup_camera_detail {
inline u32 hash(ByteView file){u32 h=2166136261u;for(std::size_t i=0;i<file.size;++i){h^=file.data[i];h*=16777619u;}return h;}
inline bool value_requested(const HsdAnimTrack& t,f32 frame,f32 end,f32* out,std::string* error){
    if(!out||!std::isfinite(frame)||frame<0||frame>hsd_anim_detail::kMaxFrame||t.data.empty()||t.data.size()>hsd_anim_detail::kMaxTrackBytes||!std::isfinite(t.startframe)||t.startframe<0||t.startframe>32767||std::floor(t.startframe)!=t.startframe)return false;
    hsd_anim_detail::TrackMachine m;m.data=t.data.data();m.length=t.data.size();m.frac_value=t.frac_value;m.frac_slope=t.frac_slope;m.time=f32(t.startframe+frame);m.float_time=true;
    if(!hsd_anim_detail::track_step(m,error,0)||m.stopped||!m.callbacks)return false;
    if(frame>=end&&m.op_intrp==6){if(!hsd_anim_detail::track_step(m,error,1)||m.stopped)return false;}
    const f32 result=f32(m.value);if(!std::isfinite(result))return false;*out=result;return true;
}
inline bool value(const HsdAnimTrack& t,u32 frame,f32 end,f32* out,std::string* error){
    if(frame>hsd_anim_detail::kMaxFrame)return false;
    return value_requested(t,f32(frame),end,out,error);
}
inline bool unique(const std::vector<HsdAnimTrack>& tracks){u32 channels=0;for(const auto& t:tracks){if(t.channel>=32||(channels&(u32(1)<<t.channel)))return false;channels|=u32(1)<<t.channel;}return true;}
} // namespace startup_camera_detail
class StartupCameraSet {
public:
    bool load(std::span<const StartupCameraInput> inputs,std::string* error){
        loaded_=false;clips_.clear();initial_={};
        auto fail=[&](const char* why){if(error)*error=std::string("startup camera: ")+why;return false;};
        if(inputs.empty()||inputs.size()>33)return fail("source count");
        std::vector<Entry> next;unsigned main=0;bool found=false;std::size_t bytes=0;
        for(const auto& input:inputs){if((input.scene!=255&&input.scene>=32)||!input.archive.data||input.archive.size>(64u<<20)||input.archive.size>(96u<<20)-bytes)return fail("scene/byte domain");bytes+=input.archive.size;
            for(const auto& e:next)if(e.scene==input.scene)return fail("duplicate source binding");
            Entry e;e.scene=input.scene;e.hash=startup_camera_detail::hash(input.archive);if(!hsd_camera_clip_load(input.archive,0,&e.clip,error))return false;
            const auto& c=e.clip;if(!c.has_base||c.base_end_frame!=c.end_frame||(c.has_eye&&c.eye_end_frame!=c.base_end_frame)||(c.has_interest&&c.interest_end_frame!=c.base_end_frame)||std::floor(c.end_frame)!=c.end_frame||c.roll!=0||!startup_camera_detail::unique(c.animation.base)||!startup_camera_detail::unique(c.animation.eye)||!startup_camera_detail::unique(c.animation.interest))return fail("unqualified mixed ends/roll/duplicate channel lifecycle");
            for(const auto* tracks:{&c.animation.eye,&c.animation.interest}){bool path=false,xyz=false;for(const auto& t:*tracks){path|=t.channel==4;xyz|=t.channel>=5&&t.channel<=7;}if(path&&xyz)return fail("mixed path/XYZ callback ordering");}
            if(input.scene==255){main=unsigned(next.size());found=true;}next.push_back(std::move(e));
        }
        if(!found)return fail("missing main descriptor");
        const auto& c=next[main].clip;StartupCameraState initial;initial.source_aspect=c.aspect;initial.main_binding=next[main].hash;initial.roll=c.roll;initial.initialized=true;
        for(unsigned k=0;k<3;++k){initial.camera.eye[k]=c.animation.static_eye[k];initial.camera.target[k]=c.animation.static_interest[k];}
        initial.camera.fov_y_degrees=c.animation.fov;initial.camera.near_plane=c.animation.near_plane;initial.camera.far_plane=c.animation.far_plane;
        if(!initial.camera.basis(nullptr))return fail("main camera basis");
        clips_=std::move(next);initial_=initial;loaded_=true;return true;
    }
    bool initialize(StartupCameraState* out)const{if(!loaded_||!out)return false;*out=initial_;return true;}
    bool observations(StartupDirectorObservations* out)const{
        if(!loaded_||!out)return false;
        out->camera_end_valid.fill(false);out->camera_end.fill(0);
        for(const auto& e:clips_){const unsigned i=e.scene==255?32:e.scene;out->camera_end_valid[i]=true;out->camera_end[i]=e.clip.base_end_frame;}return true;
    }
    bool sample(u8 scene,u32 frame,const StartupCameraState& baseline,StartupCameraState* out,std::string* error)const{
        return sample_impl(scene,frame,baseline,out,error);
    }
    bool sample_requested(u8 scene,f32 frame,const StartupCameraPoseState& baseline,StartupCameraRequestedState* out,std::string* error)const{
        return sample_impl(scene,frame,baseline,out,error);
    }
    const HsdCameraClip* clip(u8 scene)const{if(!loaded_)return nullptr;for(const auto& e:clips_)if(e.scene==scene)return &e.clip;return nullptr;}
    const StartupCameraState& initial()const{return initial_;}
private:
    template<class Time,class State>
    bool sample_impl(u8 scene,Time frame,const StartupCameraPoseState& baseline,State* out,std::string* error)const{
        auto fail=[&](const char* why){if(error)*error=std::string("startup camera sample: ")+why;return false;};
        const f64 requested=frame;
        if(!loaded_||!out||!baseline.initialized||baseline.main_binding!=initial_.main_binding||baseline.source_aspect!=initial_.source_aspect||baseline.roll!=0||!std::isfinite(requested)||requested<0||requested>hsd_anim_detail::kMaxFrame)return fail("state/binding/time domain");
        for(unsigned k=0;k<3;++k)if(baseline.camera.up[k]!=initial_.camera.up[k])return fail("unsupported global camera orientation override");
        for(unsigned k=0;k<3;++k)if(!std::isfinite(baseline.camera.eye[k])||!std::isfinite(baseline.camera.target[k])||std::abs(baseline.camera.eye[k])>1e7f||std::abs(baseline.camera.target[k])>1e7f)return fail("baseline coordinate domain");
        const Entry* source=nullptr;for(const auto& e:clips_)if(e.scene==scene)source=&e;if(!source)return fail("missing explicit scene");
        const auto& clip=source->clip;State next;static_cast<StartupCameraPoseState&>(next)=baseline;
        for(const auto& t:clip.animation.base){f32 v=0;if(!startup_camera_detail::value_requested(t,f32(frame),clip.base_end_frame,&v,error))return false;
            switch(t.channel){case 9:next.roll=v;break;case 10:next.camera.fov_y_degrees=v;break;case 11:next.camera.near_plane=v;break;case 12:next.camera.far_plane=v;break;default:return fail("base channel");}}
        auto window=[&](const std::vector<HsdAnimTrack>& tracks,const HsdAnimSpline& spline,f32* xyz,f32 end){
            for(const auto& t:tracks){f32 v=0;if(!startup_camera_detail::value_requested(t,f32(frame),end,&v,error))return false;
                if(t.channel==4){f64 p[3];if(!hsd_anim_detail::spline_arc_point(spline,v,p))return false;for(unsigned k=0;k<3;++k)xyz[k]=f32(p[k]);}
                else if(t.channel>=5&&t.channel<=7)xyz[t.channel-5]=v;else return false;}
            return true;
        };
        if(!window(clip.animation.eye,clip.animation.eye_spline,next.camera.eye,clip.eye_end_frame)||!window(clip.animation.interest,clip.animation.interest_spline,next.camera.target,clip.interest_end_frame))return fail("window tracks/path");
        for(unsigned k=0;k<3;++k)if(!std::isfinite(next.camera.eye[k])||!std::isfinite(next.camera.target[k])||std::abs(next.camera.eye[k])>1e7f||std::abs(next.camera.target[k])>1e7f)return fail("sampled coordinate domain");
        if(next.roll!=0||!std::isfinite(next.camera.fov_y_degrees)||next.camera.fov_y_degrees<=0||next.camera.fov_y_degrees>=179||!std::isfinite(next.camera.near_plane)||!std::isfinite(next.camera.far_plane)||next.camera.near_plane<=0||next.camera.far_plane<=next.camera.near_plane||!next.camera.basis(nullptr))return fail("roll/projection/basis");
        next.scene=scene;next.frame=frame;*out=std::move(next);return true;
    }
    struct Entry {u8 scene=255;u32 hash=0;HsdCameraClip clip;};
    std::vector<Entry> clips_;StartupCameraState initial_;bool loaded_=false;
};
} // namespace gf
