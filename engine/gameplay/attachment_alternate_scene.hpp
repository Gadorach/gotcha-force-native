#pragma once
// Explicit alternate COPY of a reviewed normal single-joint bank record. The
// original normal model and legacy bool-alternate refusal remain unchanged.
#include "engine/gameplay/attachment_model_scene.hpp"
#include "engine/gameplay/attachment_alternate.hpp"
#include "engine/animation/hsd_material.hpp"
namespace gf {
class AttachmentAlternateModelScene {
public:
    bool load(ByteView bank,u16 record,ByteView profile,std::string* error){
        loaded_=ready_=false;binding_.reset();skin_={};geometry_={};states_.clear();
        AttachmentAlternateProfile p;AttachmentModelScene normal;hsd_detail::Reader reader;
        if(!attachment_alternate_load(profile,&p,error)||!normal.load(bank,record,error)||!hsd_anim_detail::open_container(bank,&reader,error))return false;
        auto skin=normal.asset();HsdScene draw;draw.joints.resize(1);draw.textures=skin.scene.textures;draw.lighting=skin.scene.lighting;std::vector<AttachmentAlternateState> states;
        for(unsigned i=0;i<skin.meshes.size();++i){const auto& mesh=skin.scene.meshes[i];u32 klass=0,render_desc=0,pe=0,material=0;
            if(!reader.ptr(mesh.dobj_offset,&klass)||klass||!reader.ptr(mesh.material_offset,&klass)||klass||!reader.ptr(mesh.material_offset+12,&material)||!material||!reader.ptr(mesh.material_offset+16,&render_desc)||render_desc||!reader.ptr(mesh.material_offset+20,&pe)||(mesh.render_mode&(0x60000000u|0x04000000u|0x1000u)))return fail(error,"custom/global/null material or non-opaque source");
            AttachmentAlternateState input,style;input.joint_flags=skin.scene.joints[0].flags;input.dobj_flags=2; //8023D9D4 opaque default DOBJ pass
            input.render_mode=mesh.render_mode;input.alpha=mesh.material_alpha;input.has_pe=pe!=0;
            if(pe){if(!reader.has(pe,12))return fail(error,"PE extent");std::copy_n(reader.block.data+pe,12,input.pe.begin());}
            HsdMesh styled;
            if(!attachment_alternate_apply(p,input,&style,error)||!attachment_alternate_mesh(mesh,style,&styled,error))return false;
            if((style.joint_flags&0x70000000u)!=0x20000000u||(style.joint_flags&0x1c0000u)!=0x80000u||style.dobj_flags!=4)return fail(error,"source alternate pass binding");
            // Only the evaluated identity draw copy clears skin/UV requirements.
            styled.deformation_required=styled.generated_uv_required=styled.texture_matrix_required=false;draw.meshes.push_back(std::move(styled));draw.mesh_joint.push_back(0);states.push_back(style);
        }
        MenuScene renderer;renderer.set_interaction(false);if(!renderer.load_scene(std::move(draw),error))return false;
        skin_=std::move(skin);states_=std::move(states);scene_=std::move(renderer);binding_=normal.instance_binding();base_alpha_=p.alpha;loaded_=true;if(error)error->clear();return true;
    }
    bool set_world_fade(const AttachmentBoneTransform& transform,const ActorFadeMaterial& material,std::string* error){
        ready_=false;if(!loaded_||!binding_||!transform.valid||!material.source_class)return fail(error,"transform/material/instance binding");
        return publish_world(transform,material.rgb,base_alpha_,error);
    }
    // Explicit80007C54 style followed by80007CAC/91E4 alpha replacement.
    // No invented fade-mobj token: RGB addition is zero for this separate entry.
    bool set_world_alpha(const AttachmentBoneTransform& transform,f32 alpha,std::string* error){
        ready_=false;if(!loaded_||!binding_||!transform.valid||!std::isfinite(alpha)||alpha<0||alpha>1)return fail(error,"explicit transform/normalized alpha binding");
        return publish_world(transform,{},alpha,error);
    }
    unsigned upload_textures(RenderEngine& render,u32 base=1){return loaded_&&binding_?scene_.upload_textures(render,base):0;}
    unsigned upload_textures_shared(RenderEngine& render,u32 base){return loaded_&&binding_?scene_.upload_textures_shared(render,base):0;}
    unsigned draw(RenderEngine& render,const Camera& camera){
        if(!ready())return 0;
        auto vertices=geometry_.world_vertices;for(unsigned i=0;i<vertices.size();++i)if(skin_.scene.meshes[i].generated_uv_required)for(auto& v:vertices[i])if(!view_normal_reflection_uv(v.normal,camera,v.uv)){ready_=false;return 0;}
        if(!scene_.set_pretransformed_geometry_with_origins(vertices,std::vector<bool>(vertices.size(),true),std::vector<std::array<f32,3>>(vertices.size(),origin_))){ready_=false;return 0;}
        return scene_.draw(render,camera);
    }
    void discard_pose(){ready_=false;}
    bool ready()const{return ready_&&bool(binding_);}
    const HsdSkinAsset& asset()const{return skin_;}
    const HsdSkinGeometry& world_geometry()const{return geometry_;}
    const std::vector<AttachmentAlternateState>& material_states()const{return states_;}
    std::shared_ptr<const AttachmentModelBinding> instance_binding()const{return binding_;}
private:
    bool publish_world(const AttachmentBoneTransform& transform,std::array<u8,3> rgb,f32 alpha,std::string* error){
        HsdSkinGeometry next;const std::array<Mat4,1> world={transform.world};if(!hsd_skin_sample(skin_,world,&next,error))return false;
        u8 byte=0;if(!hsd_material_draw_alpha(alpha,&byte)||!scene_.set_evaluated_material_alpha(std::vector<f32>(states_.size(),f32(byte)/255.f)))return fail(error,"evaluated draw alpha publication");
        for(auto& state:states_)state.alpha=alpha;
        geometry_=std::move(next);for(unsigned k=0;k<3;++k)origin_[k]=transform.world.at(k,3);scene_.set_post_rgb_add(rgb);ready_=true;if(error)error->clear();return true;
    }
    static bool fail(std::string* error,const char* why){if(error)*error=std::string("attachment alternate scene: ")+why;return false;}
    HsdSkinAsset skin_;HsdSkinGeometry geometry_;MenuScene scene_;std::vector<AttachmentAlternateState> states_;
    std::array<f32,3> origin_{};
    std::shared_ptr<const AttachmentModelBinding> binding_;f32 base_alpha_=0;bool loaded_=false,ready_=false;
};
} // namespace gf
