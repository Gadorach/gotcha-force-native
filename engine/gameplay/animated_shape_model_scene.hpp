#pragma once
#include "engine/assets/hsd_shape_material.hpp"
#include "engine/gameplay/hsd_record_animation.hpp"
#include "engine/gameplay/menu_scene.hpp"
namespace gf {
// Explicit local evaluated geometry + the SAME retained MObj/FObj callback
// player and MenuScene pose submission. No shape/material arithmetic fork.
class AnimatedShapeModelScene {
public:
    bool load(const HsdRecordAnimationBank& bank,u16 record,std::string* error){
        *this={};if(!shape_.load(bank,record,error)||!base_player_.bind(shape_.binding_,error))return false;
        renderer_.set_interaction(false);if(!renderer_.load_scene(*shape_.binding_.scene(),error))return false;renderer_.set_show_source_markers(true);loaded_=true;return true;
    }
    bool sample(u32 shape_frame,f32 material_frame,const Mat4& world,std::string* error){
        return sample_impl(shape_frame,false,material_frame,world,error);
    }
    bool sample_fractional(f32 shape_frame,f32 material_frame,const Mat4& world,std::string* error){
        return sample_impl(shape_frame,true,material_frame,world,error);
    }
    unsigned draw(RenderEngine& render,const Camera& camera){return ready_?renderer_.draw(render,camera):0;}
    const HsdAnimPose* pose()const{return ready_?renderer_.pose():nullptr;}
private:
    bool sample_impl(f64 shape_frame,bool fractional,f32 material_frame,const Mat4& world,std::string* error){
        ready_=false;if(!loaded_)return false;
        for(f32 v:world.m)if(!std::isfinite(v)||std::abs(v)>1e7f)return false;
        if(world.m[12]!=0||world.m[13]!=0||world.m[14]!=0||world.m[15]!=1)return false;
        const f64 det=f64(world.m[0])*(f64(world.m[5])*world.m[10]-f64(world.m[6])*world.m[9])-f64(world.m[1])*(f64(world.m[4])*world.m[10]-f64(world.m[6])*world.m[8])+f64(world.m[2])*(f64(world.m[4])*world.m[9]-f64(world.m[5])*world.m[8]);if(!std::isfinite(det)||std::abs(det)<1e-12)return false;
        std::vector<std::vector<HsdVertex>> geometry;const bool sampled=fractional?shape_.sample_shapes_fractional(f32(shape_frame),&geometry,nullptr,error):shape_.sample_shapes(u32(shape_frame),&geometry,nullptr,error);if(!sampled)return false;auto player=base_player_;if(!player.request(material_frame,0,error))return false;auto pose=*player.pose();
        pose.joints[0].matrix_override=true;std::copy_n(world.m,12,pose.joints[0].matrix);for(auto& m:pose.materials){u8 alpha=0;if(!hsd_material_draw_alpha(m.alpha,&alpha))return false;m.alpha=alpha/255.f;}
        // Clear the previous pose before replacing the identity-copy source
        // vertices. The subsequent ordinary pose supplies world+materials.
        std::vector<bool> visible;for(const auto& mesh:shape_.binding_.scene()->meshes)visible.push_back(!mesh.hidden);
        if(!renderer_.set_pose(nullptr)||!renderer_.set_pretransformed_geometry(geometry,visible)||!renderer_.set_pose(&pose))return false;
        ready_=true;return true;
    }
    HsdShapeMaterialRecord shape_;HsdRecordAnimationPlayer base_player_;MenuScene renderer_;bool loaded_=false,ready_=false;
};
} // namespace gf
