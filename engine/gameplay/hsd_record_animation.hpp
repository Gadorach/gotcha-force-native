#pragma once
// Retained default HSD joint/material AObjs for an explicitly published record.
// Reuses the common FObj machine; scene activation/lifetime remain controller policy.
#include "engine/assets/hsd_record_animation.hpp"
#include "engine/animation/hsd_material.hpp"
namespace gf {
struct HsdRecordTrackTrace {u32 channel=0,state=0,flags=0,op=0,interpolation=0;std::size_t cursor=0;f64 time=0,value=0;u64 callbacks=0;};
struct HsdRecordObjectTrace {f32 frame=0;bool stopped=false;std::vector<HsdRecordTrackTrace> tracks;};
class HsdRecordAnimationPlayer {
public:
    HsdRecordAnimationPlayer()=default;
    HsdRecordAnimationPlayer(const HsdRecordAnimationPlayer&)=default;
    HsdRecordAnimationPlayer& operator=(const HsdRecordAnimationPlayer&)=default;
    HsdRecordAnimationPlayer(HsdRecordAnimationPlayer&& other)noexcept{*this=std::move(other);}
    HsdRecordAnimationPlayer& operator=(HsdRecordAnimationPlayer&& other)noexcept{
        if(this!=&other){data_=std::move(other.data_);objects_=std::move(other.objects_);pose_=std::move(other.pose_);frame_=other.frame_;updates_=other.updates_;ready_=other.ready_;requested_=other.requested_;other.ready_=other.requested_=false;}
        return *this;
    }
    bool bind(const HsdRecordAnimationBinding& binding,std::string* error){
        ready_=requested_=false;data_.reset();objects_.clear();pose_={};frame_=0;updates_=0;
        if(!binding.data_)return fail(error,"explicit record binding required");
        data_=binding.data_;ready_=true;if(error)error->clear();return true;
    }
    bool request(f32 start,f32 rate,std::string* error){
        return request_impl(start,start,rate,error);
    }
    //80008744 and800086B8 request independent joint and material FIRST_PLAY
    //times on the SAME model instance. Explicit selectors use zero playback rate;
    //ordinary request/advance semantics and their guards are unchanged.
    bool request_selectors(f32 joint_time,f32 material_time,std::string* error){
        if(!time(material_time))return refuse(error,"material selector time domain");
        return request_impl(joint_time,material_time,0,error);
    }
private:
    bool request_impl(f32 start,f32 material_start,f32 rate,std::string* error){
        if(!ready_||requested_||!time(start)||!valid_rate(rate))return refuse(error,"fresh instance/request domain; reload required for re-request");
        auto pose=rest();std::vector<ObjectState> states;states.reserve(data_->objects.size());
        for(const auto& source:data_->objects){const f32 object_start=source.kind==hsd_record_detail::Object::Joint?start:material_start;ObjectState state;state.frame=object_start;state.first=true;state.texture=source.texture;
            for(const auto& track:source.tracks){hsd_anim_detail::TrackMachine machine;machine.data=track.data.data();machine.length=track.data.size();machine.frac_value=track.frac_value;machine.frac_slope=track.frac_slope;machine.time=f32(track.startframe+object_start);machine.float_time=true;state.tracks.push_back(machine);}states.push_back(std::move(state));}
        for(unsigned i=0;i<states.size();++i)if(!interpret(i,states[i],pose,rate,error))return refuse(error,"initial AObj callback failed");
        pose_=std::move(pose);objects_=std::move(states);frame_=start;requested_=true;updates_=0;if(error)error->clear();return true;
    }
public:
    bool advance(f32 rate,std::string* error){
        if(!ready_||!requested_||!valid_rate(rate)||updates_>=8192||!time(f32(frame_+rate)))return refuse(error,"advance/state/work budget");
        auto states=objects_;auto pose=pose_;
        for(unsigned i=0;i<states.size();++i)if(!interpret(i,states[i],pose,rate,error))return refuse(error,"AObj callback failed");
        objects_=std::move(states);pose_=std::move(pose);frame_=f32(frame_+rate);++updates_;if(error)error->clear();return true;
    }
    // Explicit HSD_ReqAnim-style re-request for UI state selectors. The original
    // request() remains fresh-only; failed owners cannot revive through this API.
    bool request_again(f32 start,f32 rate,std::string* error){
        if(!ready_||!requested_||!data_)return refuse(error,"live requested owner required");
        auto next=*this;next.requested_=false;next.objects_.clear();next.pose_={};next.frame_=0;next.updates_=0;
        if(!next.request(start,rate,error))return refuse(error,"explicit re-request failed");
        *this=std::move(next);return true;
    }
    bool request_again_selectors(f32 joint_time,f32 material_time,std::string* error){
        if(!ready_||!requested_||!data_)return refuse(error,"live requested owner required");
        auto next=*this;next.requested_=false;next.objects_.clear();next.pose_={};next.frame_=0;next.updates_=0;
        if(!next.request_selectors(joint_time,material_time,error))return refuse(error,"explicit selector request failed");
        *this=std::move(next);return true;
    }
    const HsdAnimPose* pose()const{return ready_&&requested_?&pose_:nullptr;}
    f32 frame()const{return frame_;}
    //Read-only constants of the accepted retained TObj states, in texture-object
    //order. This is scalar state inspection, not a renderer/device receipt.
    std::vector<std::array<u32,3>> texture_constants()const{
        std::vector<std::array<u32,3>> result;if(!ready_||!requested_||!data_)return result;
        for(unsigned i=0;i<objects_.size();++i)if(data_->objects[i].kind==hsd_record_detail::Object::Texture){const auto& s=objects_[i].texture;result.push_back({s.high,s.low,s.tev1});}
        return result;
    }
    std::vector<HsdRecordObjectTrace> trace()const{
        std::vector<HsdRecordObjectTrace> result;if(!ready_||!data_||!requested_)return result;
        for(unsigned i=0;i<objects_.size();++i){const auto& s=objects_[i];HsdRecordObjectTrace object;object.frame=s.frame;object.stopped=s.stopped;
            for(unsigned k=0;k<s.tracks.size();++k){const auto& m=s.tracks[k];object.tracks.push_back({data_->objects[i].tracks[k].channel,m.state,m.flags,m.op,m.op_intrp,m.pos,m.time,m.value,m.callbacks});}result.push_back(std::move(object));}return result;
    }
private:
    struct ObjectState {f32 frame=0;bool first=false,stopped=false;std::vector<hsd_anim_detail::TrackMachine> tracks;HsdTextureState texture;};
    static bool time(f32 t){return std::isfinite(t)&&t>=0&&t<=hsd_anim_detail::kMaxFrame;}
    static bool valid_rate(f32 rate){return std::isfinite(rate)&&rate>=0&&rate<=16;}
    static bool fail(std::string* error,const char* why){if(error)*error=std::string("hsd record player: ")+why;return false;}
    bool refuse(std::string* error,const char* why){ready_=false;return fail(error,why);}
    HsdAnimPose rest()const{
        HsdAnimPose pose;const auto& scene=data_->scene;pose.joints.resize(scene.joints.size());pose.materials.resize(scene.meshes.size());
        for(unsigned i=0;i<scene.joints.size();++i){const auto& j=scene.joints[i];auto& p=pose.joints[i];for(unsigned k=0;k<3;++k){p.rotation[k]=j.rotation[k];p.translation[k]=j.translation[k];p.scale[k]=j.scale[k];}p.hidden=(j.flags&0x10u)!=0;}
        for(unsigned i=0;i<scene.meshes.size();++i){const auto& mesh=scene.meshes[i];auto& p=pose.materials[i];p.alpha=mesh.material_alpha;p.has_specular=true;for(unsigned k=0;k<3;++k){p.ambient[k]=mesh.material_ambient[k]/255.f;p.diffuse[k]=mesh.material_colour[k]/255.f;p.specular[k]=mesh.material_specular[k]/255.f;}}
        return pose;
    }
    bool apply(unsigned object,unsigned track,ObjectState& state,HsdAnimPose& pose,f32 rate,std::string* error)const{
        auto& machine=state.tracks[track];const auto before=machine.callbacks;
        if(!hsd_anim_detail::track_step(machine,error,rate)||machine.stopped)return false;
        if(before==machine.callbacks)return true;
        const f32 value=f32(machine.value);if(!std::isfinite(value)||std::abs(value)>1e7f)return false;
        const auto& source=data_->objects[object];const u32 channel=source.tracks[track].channel;
        if(source.kind==hsd_record_detail::Object::Material){for(u32 target:source.meshes)if(!hsd_material_callback(channel,value,&pose.materials[target]))return false;return true;}
        if(source.kind==hsd_record_detail::Object::Texture)return source.retain_tev1?hsd_texture_callback_retaining_tev1(channel,value,&state.texture):hsd_texture_callback(channel,value,&state.texture);
        auto& joint=pose.joints[source.target];
        if(channel>=1&&channel<=3)joint.rotation[channel-1]=value;
        else if(channel==4){f64 point[3];if(!source.path.valid||!hsd_anim_detail::spline_arc_point(source.path,value,point))return false;for(unsigned k=0;k<3;++k){if(!std::isfinite(point[k])||std::abs(point[k])>1e7)return false;joint.translation[k]=f32(point[k]);}}
        else if(channel>=5&&channel<=7)joint.translation[channel-5]=value;
        else if(channel>=8&&channel<=10)joint.scale[channel-8]=value;
        else if(channel==11)joint.hidden=value<=.5f;
        else if(channel==12){joint.hidden=value<=.5f;for(unsigned child=source.target+1;child<pose.joints.size();++child){u32 parent=data_->scene.joints[child].parent;bool descendant=false;for(unsigned guard=0;parent!=kHsdNoJoint&&guard<pose.joints.size();++guard){if(parent==source.target){descendant=true;break;}parent=data_->scene.joints[parent].parent;}if(descendant)pose.joints[child].hidden=value<=.5f;}}
        else return false;
        return true;
    }
    bool interpret(unsigned object,ObjectState& state,HsdAnimPose& pose,f32 rate,std::string* error)const{
        if(state.stopped)return true;
        const f32 step=state.first?0:rate;if(!state.first)state.frame=f32(state.frame+rate);state.first=false;if(!time(state.frame))return false;
        for(unsigned i=0;i<state.tracks.size();++i)if(!apply(object,i,state,pose,step,error))return false;
        if(state.frame>=data_->objects[object].end){
            for(unsigned i=0;i<state.tracks.size();++i){if(state.tracks[i].op_intrp==6&&!apply(object,i,state,pose,rate,error))return false;state.tracks[i].state=0;}state.stopped=true;
        }
        const auto& source=data_->objects[object];
        if(source.kind==hsd_record_detail::Object::Texture){
            std::array<f32,2> uv{},offset{};if(!hsd_texture_uv(state.texture.translate,state.texture.scale,state.texture.repeat,&uv,&offset))return false;
            for(auto index:source.meshes){auto& material=pose.materials[index];material.present=material.has_texture=true;material.texture_weight=state.texture.blend;
                for(unsigned k=0;k<2;++k){material.uv_scale[k]=uv[k];material.uv_offset[k]=offset[k];}
                material.has_texture_colours=state.texture.colour_lerp;material.texture_low_colour=state.texture.low;material.texture_high_colour=state.texture.high;
            }
        }
        return true;
    }
    std::shared_ptr<const hsd_record_detail::Instance> data_;std::vector<ObjectState> objects_;HsdAnimPose pose_;f32 frame_=0;u32 updates_=0;bool ready_=false,requested_=false;
};
} // namespace gf
