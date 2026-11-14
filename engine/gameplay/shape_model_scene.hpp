#pragma once
#include "engine/assets/hsd_shape.hpp"
#include "engine/gameplay/menu_scene.hpp"
namespace gf {
class ShapeModelScene {
public:
    bool load(const HsdRecordAnimationBank& bank,u16 record,RenderEngine& render,u32 textures,std::string* error,bool compact=false){
        ready_=loaded_=alpha_changed_=false;renderer_={};if(!shape_.load(bank,record,error,compact))return false;renderer_.set_interaction(false);
        if(!renderer_.load_scene(shape_.scene_,error))return false;
        renderer_.set_show_source_markers(true);renderer_.upload_textures_shared(render,textures);loaded_=true;return true;
    }
    bool sample(u32 frame,const Mat4& world,std::string* error){
        return sample_impl(frame,false,world,0,false,error);
    }
    //Separate opt-in for direct requested-time source shapes and replacement
    //material alpha. Not a clock, blend multiplier or source lifetime receipt.
    bool sample_fractional_alpha(f32 frame,const Mat4& world,f32 alpha,std::string* error){
        return sample_impl(frame,true,world,alpha,true,error);
    }
    unsigned draw(RenderEngine& render,const Camera& camera){return ready_?renderer_.draw(render,camera):0;}
private:
    bool sample_impl(f64 frame,bool fractional,const Mat4& world,f32 alpha,bool replace_alpha,std::string* error){
        ready_=false;if(!loaded_)return false;
        if(replace_alpha&&(!std::isfinite(alpha)||alpha<0||alpha>1))return false;
        for(float value:world.m)if(!std::isfinite(value)||std::abs(value)>1e7f)return false;
        if(world.m[12]!=0||world.m[13]!=0||world.m[14]!=0||world.m[15]!=1)return false;
        const f64 det=f64(world.m[0])*(f64(world.m[5])*world.m[10]-f64(world.m[6])*world.m[9])-f64(world.m[1])*(f64(world.m[4])*world.m[10]-f64(world.m[6])*world.m[8])+f64(world.m[2])*(f64(world.m[4])*world.m[9]-f64(world.m[5])*world.m[8]);
        if(!std::isfinite(det)||std::abs(det)<1e-12)return false;
        std::vector<HsdVertex> vertices;const bool sampled=fractional?shape_.sample_fractional(f32(frame),&vertices,nullptr,error):shape_.sample(u32(frame),&vertices,nullptr,error);if(!sampled)return false;
        for(auto& vertex:vertices){f32 p[3],n[3];mat4_transform_point(world,vertex.position,p);mat4_transform_normal(world,vertex.normal,n);for(unsigned k=0;k<3;++k){vertex.position[k]=p[k];vertex.normal[k]=n[k];}}
        const std::vector<std::array<f32,3>> origins={{{world.m[3],world.m[7],world.m[11]}}};
        if(!renderer_.set_pretransformed_geometry_with_origins({vertices},{true},origins))return false;
        if(replace_alpha||alpha_changed_){if(!renderer_.set_evaluated_material_alpha({replace_alpha?alpha:shape_.original_mesh()->material_alpha}))return false;alpha_changed_=replace_alpha;}
        ready_=true;return true;
    }
    HsdShapeRecord shape_;MenuScene renderer_;bool loaded_=false,ready_=false,alpha_changed_=false;
};
} // namespace gf
