#pragma once
// Published-record animation owner feeding the SAME MenuScene/RenderEngine path.
// No source scene policy, actor world inference, audio or alternate renderer.
#include "engine/gameplay/hsd_record_animation.hpp"
#include "engine/gameplay/menu_scene.hpp"
namespace gf {
class AnimatedModelScene {
public:
    AnimatedModelScene()=default;
    AnimatedModelScene(const AnimatedModelScene&)=delete;
    AnimatedModelScene& operator=(const AnimatedModelScene&)=delete;
    AnimatedModelScene(AnimatedModelScene&& other)noexcept{*this=std::move(other);}
    AnimatedModelScene& operator=(AnimatedModelScene&& other)noexcept{
        if(this!=&other){binding_=std::move(other.binding_);player_=std::move(other.player_);renderer_=std::move(other.renderer_);world_=other.world_;loaded_=other.loaded_;ready_=other.ready_;failed_=other.failed_;enabled_=other.enabled_;world_override_=other.world_override_;alpha_override_=other.alpha_override_;alpha_=other.alpha_;other.loaded_=other.ready_=other.enabled_=false;other.failed_=true;}
        return *this;
    }
    bool load(const HsdRecordAnimationBank& bank,u16 record,u8 channels,u16 joint_clip,u16 material_clip,std::string* error){
        return load_impl(bank,record,channels,joint_clip,material_clip,error,false);
    }
    bool load_compact_static(const HsdRecordAnimationBank& bank,u16 record,std::string* error){return load_impl(bank,record,0,0,0,error,false,false,nullptr,true);}
    bool load_paths(const HsdRecordAnimationBank& bank,u16 record,u8 channels,u16 joint_clip,u16 material_clip,std::string* error){
        return load_impl(bank,record,channels,joint_clip,material_clip,error,true);
    }
    bool load_textures(const HsdRecordAnimationBank& bank,u16 record,u8 channels,u16 joint_clip,u16 material_clip,std::string* error){
        return load_impl(bank,record,channels,joint_clip,material_clip,error,false,true);
    }
    bool load_compact_textures(const HsdRecordAnimationBank& bank,u16 record,u8 channels,u16 joint_clip,u16 material_clip,std::string* error){return load_impl(bank,record,channels,joint_clip,material_clip,error,false,true,nullptr,true);}
    bool load_compact_retained_tev1_textures(const HsdRecordAnimationBank& bank,u16 record,u8 channels,u16 joint_clip,u16 material_clip,std::string* error){return load_impl(bank,record,channels,joint_clip,material_clip,error,false,true,nullptr,true,false,true);}
    bool load_compact_alternate_textures(const HsdRecordAnimationBank& bank,u16 record,u8 channels,u16 joint_clip,u16 material_clip,const AttachmentAlternateProfile& style,std::string* error){return load_impl(bank,record,channels,joint_clip,material_clip,error,false,true,&style,true);}
    bool load_compact_translucent_textures(const HsdRecordAnimationBank& bank,u16 record,u8 channels,u16 joint_clip,u16 material_clip,const AttachmentAlternateProfile& style,std::string* error){return load_impl(bank,record,channels,joint_clip,material_clip,error,false,true,&style,true,true);}
    bool load_alternate_textures(const HsdRecordAnimationBank& bank,u16 record,u8 channels,u16 joint_clip,u16 material_clip,const AttachmentAlternateProfile& style,std::string* error){
        return load_impl(bank,record,channels,joint_clip,material_clip,error,false,true,&style);
    }
private:
    bool load_impl(const HsdRecordAnimationBank& bank,u16 record,u8 channels,u16 joint_clip,u16 material_clip,std::string* error,bool paths,bool textures=false,const AttachmentAlternateProfile* style=nullptr,bool compact=false,bool preserve_alpha=false,bool retain_tev1=false){
        ready_=loaded_=failed_=enabled_=false;world_override_=alpha_override_=false;alpha_=1;binding_={};player_={};renderer_={};
        if(!(retain_tev1?bank.select_compact_retained_tev1_textures(record,channels,joint_clip,material_clip,&binding_,error):style?(preserve_alpha?bank.select_translucent_textures(record,channels,joint_clip,material_clip,*style,&binding_,error,compact):bank.select_alternate_textures(record,channels,joint_clip,material_clip,*style,&binding_,error,compact)):compact?(textures?bank.select_compact_textures(record,channels,joint_clip,material_clip,&binding_,error):bank.select_compact_static(record,&binding_,error)):textures?bank.select_textures(record,channels,joint_clip,material_clip,&binding_,error):paths?bank.select_paths(record,channels,joint_clip,material_clip,&binding_,error):bank.select(record,channels,joint_clip,material_clip,&binding_,error))||!player_.bind(binding_,error))return failure(error,"record/animation binding failed");
        renderer_.set_interaction(false);if(!renderer_.load_scene(*binding_.scene(),error))return failure(error,"shared scene load failed");loaded_=true;if(error)error->clear();return true;
    }
public:
    bool request(f32 start,f32 rate,std::string* error){
        if(!loaded_||failed_)return failure(error,"not loaded/failed instance");
        auto next=player_;if(!next.request(start,rate,error)||!publish(next,error))return failure(error,"request/pose failed");player_=std::move(next);return true;
    }
    bool advance(f32 rate,std::string* error){
        if(!loaded_||failed_||!ready_)return failure(error,"not ready/failed instance");
        auto next=player_;if(!next.advance(rate,error)||!publish(next,error))return failure(error,"advance/pose failed");player_=std::move(next);return true;
    }
    bool request_again(f32 start,f32 rate,std::string* error){
        if(!loaded_||failed_||!ready_)return failure(error,"live requested instance required");
        auto next=player_;if(!next.request_again(start,rate,error)||!publish(next,error))return failure(error,"re-request/pose failed");player_=std::move(next);return true;
    }
    bool request_selectors(f32 joint_time,f32 material_time,std::string* error){
        if(!loaded_||failed_)return failure(error,"not loaded/failed instance");
        auto next=player_;if(!next.request_selectors(joint_time,material_time,error)||!publish(next,error))return failure(error,"selector request/pose failed");player_=std::move(next);return true;
    }
    bool request_again_selectors(f32 joint_time,f32 material_time,std::string* error){
        if(!loaded_||failed_||!ready_)return failure(error,"live requested instance required");
        auto next=player_;if(!next.request_again_selectors(joint_time,material_time,error)||!publish(next,error))return failure(error,"selector re-request/pose failed");player_=std::move(next);return true;
    }
    //80007CAC replacement on a static record only. Do not permanently override
    // live AObj material callbacks or change the record's PE/pass semantics.
    bool replace_static_alpha(f32 alpha,std::string* error){
        if(!loaded_||failed_||!ready_||!binding_.objects()||!binding_.objects()->empty()||!std::isfinite(alpha)||alpha<0||alpha>1)return failure(error,"static requested instance/normalized alpha required");
        alpha_=alpha;alpha_override_=true;if(!publish(player_,error))return failure(error,"static alpha pose failed");if(error)error->clear();return true;
    }
    // UI alpha can be separately owned while UV animation remains active.
    bool replace_unanimated_alpha(f32 alpha,std::string* error){
        if(!loaded_||failed_||!ready_||!binding_.objects()||!std::isfinite(alpha)||alpha<0||alpha>1)return failure(error,"requested instance/normalized alpha required");
        for(const auto& object:*binding_.objects())if(object.kind==hsd_record_detail::Object::Material)for(const auto& track:object.tracks)if(track.channel==10)return failure(error,"live alpha callback owns material");
        alpha_=alpha;alpha_override_=true;if(!publish(player_,error))return failure(error,"alpha pose failed");if(error)error->clear();return true;
    }
    //Explicit 80007CAC AFTER an owner's material callbacks (e.g.1939D0).
    //Unlike replace_unanimated_alpha, this deliberately owns the final alpha
    //while retaining the underlying tracks/clocks. Old static/unanimated guards
    //remain unchanged; no draw/pass/PE semantics or animation time is changed.
    bool replace_after_material_callbacks_alpha(f32 alpha,std::string* error){
        if(!loaded_||failed_||!ready_||!binding_.objects()||!std::isfinite(alpha)||alpha<0||alpha>1)return failure(error,"live requested post-callback instance/normalized alpha required");
        alpha_=alpha;alpha_override_=true;if(!publish(player_,error))return failure(error,"post-callback alpha pose failed");if(error)error->clear();return true;
    }
    //800076D0 USER_DEF replaces the root matrix; it does NOT add authored root
    // SRT a second time. Child SRT/animations stay in the shared hierarchy path.
    bool set_root_world(const Mat4& world,std::string* error){
        if(!loaded_||failed_)return failure(error,"not loaded/failed instance");
        for(float v:world.m)if(!std::isfinite(v)||std::abs(v)>1e7f)return failure(error,"root matrix finite/budget");
        if(world.m[12]!=0||world.m[13]!=0||world.m[14]!=0||world.m[15]!=1)return failure(error,"affine root required");
        const f64 det=f64(world.m[0])*(f64(world.m[5])*world.m[10]-f64(world.m[6])*world.m[9])-f64(world.m[1])*(f64(world.m[4])*world.m[10]-f64(world.m[6])*world.m[8])+f64(world.m[2])*(f64(world.m[4])*world.m[9]-f64(world.m[5])*world.m[8]);
        if(!std::isfinite(det)||std::abs(det)<1e-12)return failure(error,"singular supplied root");
        world_=world;world_override_=true;if(player_.pose()&&!publish(player_,error))return failure(error,"root pose failed");if(error)error->clear();return true;
    }
    void set_enabled(bool enabled){enabled_=enabled;}
    // Some authored effects are vertex-coloured, untextured billboards, not
    // diagnostic markers. Explicit source-qualified owners may enable them;
    // legacy standalone/default filtering remains unchanged.
    void set_untextured_billboards(bool enabled){renderer_.set_show_source_markers(enabled);}
    bool set_depth_compare(u8 comparison){return renderer_.set_depth_compare(comparison);}
    void clear_depth_compare(){renderer_.clear_depth_compare();}
    void set_post_rgb_multiply(std::array<u8,3> rgb){renderer_.set_post_rgb_multiply(rgb);}
    bool set_linear_fog(const LinearFog& fog){return renderer_.set_linear_fog(fog);}
    unsigned upload_textures(RenderEngine& render,u32 first=1){return loaded_?renderer_.upload_textures(render,first):0;}
    unsigned upload_textures_shared(RenderEngine& render,u32 first){return loaded_?renderer_.upload_textures_shared(render,first):0;}
    unsigned draw(RenderEngine& render,const Camera& camera,PointerTriangles* submitted=nullptr){return ready_&&!failed_&&enabled_?renderer_.draw(render,camera,submitted):0;}
    void release(){*this=AnimatedModelScene{};}
    bool ready()const{return ready_&&!failed_;}
    const HsdAnimPose* pose()const{return ready()?renderer_.pose():nullptr;}
    const HsdRecordAnimationPlayer& animation()const{return player_;}
    const HsdScene* asset()const{return binding_.scene();}
    bool bounds(f32 lo[3],f32 hi[3])const{return ready()&&renderer_.bounds(lo,hi);}
private:
    bool publish(const HsdRecordAnimationPlayer& candidate,std::string* error){
        ready_=false;const auto* source=candidate.pose();if(!source)return false;auto pose=*source;
        if(world_override_){if(pose.joints.empty())return false;pose.joints[0].matrix_override=true;std::copy_n(world_.m,12,pose.joints[0].matrix);}
        for(unsigned i=0;i<pose.joints.size();++i)if(!(i==0&&world_override_))for(float scale:pose.joints[i].scale)if(std::abs(scale)<1e-6f){if(error)*error="animated model: singular animated scale";return false;}
        for(auto& material:pose.materials){u8 alpha=0;if(alpha_override_)material.alpha=alpha_;if(!hsd_material_draw_alpha(material.alpha,&alpha))return false;material.alpha=alpha/255.f;material.present=true;}
        if(!renderer_.set_pose(&pose)){if(error)*error="animated model: invalid shared renderer pose";return false;}ready_=true;return true;
    }
    bool failure(std::string* error,const char* why){ready_=false;failed_=true;if(error&&error->empty())*error=std::string("animated model: ")+why;return false;}
    HsdRecordAnimationBinding binding_;HsdRecordAnimationPlayer player_;MenuScene renderer_;Mat4 world_;
    bool loaded_=false,ready_=false,failed_=false,enabled_=false,world_override_=false,alpha_override_=false;f32 alpha_=1;
};
} // namespace gf
