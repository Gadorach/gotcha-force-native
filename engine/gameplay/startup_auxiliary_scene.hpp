#pragma once
// Static hierarchy for the source-selected, offline-projected efct record51.
// Root USER_DEF matrix replaces root SRT; child SRT preserved. No fade-mobj,
// animation/controller, parent mutations, scene lighting or visible startup hook.
#include "engine/assets/hsd_skin.hpp"
#include "engine/assets/startup_auxiliary.hpp"
#include "engine/gameplay/menu_scene.hpp"
#include <memory>
namespace gf {
class StartupAuxiliaryModelScene {
public:
    bool load(ByteView model,const StartupAuxiliaryProfile& p,std::string* error){
        loaded_=ready_=false;skin_={};geometry_={};binding_.reset();
        if(!startup_auxiliary_profile_valid(p)||!model.data||model.size>(2u<<20))return fail(error,"profile/model budget");
        u32 hash=2166136261u;for(std::size_t i=0;i<model.size;++i){hash^=model.data[i];hash*=16777619u;}if(hash!=p.model_hash)return fail(error,"projected resource hash");
        HsdSkinAsset skin;hsd_detail::Reader reader;if(!hsd_skin_load(model,&skin,error)||!hsd_anim_detail::open_container(model,&reader,error))return false;
        if(model.be32(0)!=model.size||model.be32(12)!=1||model.be32(16)!=0)return fail(error,"single published resource extent");
        const auto symbols=32+reader.block.size+std::size_t(model.be32(8))*4;const auto strings=symbols+8;const auto name=model.be32(symbols+4);
        if(!model.contains(strings+name,11)||std::string(reinterpret_cast<const char*>(model.data+strings+name),11)!=std::string("scene_data\0",11))return fail(error,"published scene_data identity");
        u32 table=0,record=0,root=0,next=0,animation=0,material_animation=0;
        if(!reader.ptr(model.be32(symbols),&table)||!table||!reader.ptr(table,&record)||!record||!reader.ptr(record,&root)||root!=p.root||!reader.ptr(table+4,&next)||next||!reader.ptr(record+4,&animation)||animation||!reader.ptr(record+8,&material_animation)||material_animation)return fail(error,"selected published record/animation binding");
        if(skin.scene.model_roots.size()!=1||skin.scene.model_roots[0]!=p.root||skin.scene.joints.size()!=p.joints||skin.meshes.size()!=p.meshes||!skin.scene.animation_roots.empty()||!skin.scene.material_animation_roots.empty())return fail(error,"projected source-record shape/animation binding");
        for(unsigned j=0;j<skin.scene.joints.size();++j){const auto& node=skin.scene.joints[j];u32 robj=0;
            if(node.model_root!=0||(node.flags&(0x1e7fu|0x2000u|0x4000u|0x20000u|0x3800000u))||!reader.ptr(node.archive_offset+0x3c,&robj)||robj||(j==0?node.parent!=kHsdNoJoint:node.parent>=j))return fail(error,"unsupported joint/hierarchy modifiers");
        }
        std::array<Mat4,8> world;for(unsigned j=1;j<skin.scene.joints.size();++j){const auto& n=skin.scene.joints[j];world[j]=mat4_multiply(world[n.parent],mat4_from_srt(n.rotation,n.scale,n.translation));}
        HsdSkinGeometry initial;if(!hsd_skin_sample(skin,{world.data(),skin.scene.joints.size()},&initial,error))return false;
        HsdScene draw;draw.joints.resize(1);draw.textures=skin.scene.textures;draw.lighting=skin.scene.lighting;
        for(unsigned i=0;i<skin.meshes.size();++i){const auto& meta=skin.meshes[i];auto mesh=skin.scene.meshes[i];
            u32 klass=0,render_desc=0;if(!reader.ptr(mesh.dobj_offset,&klass)||klass||!reader.ptr(mesh.material_offset,&klass)||klass||!reader.ptr(mesh.material_offset+16,&render_desc)||render_desc)return fail(error,"custom DOBJ/material/render descriptor");
            if(meta.kind!=HsdSkinKind::Rigid||!mesh.material_supported||mesh.generated_uv_required||(mesh.texture_matrix_required&&mesh.texture!=kHsdNoTexture))return fail(error,"unported material/skin/UV consumer");
            mesh.vertices=initial.world_vertices[i];mesh.deformation_required=false;mesh.texture_matrix_required=false;draw.meshes.push_back(std::move(mesh));draw.mesh_joint.push_back(0);
        }
        MenuScene scene;scene.set_interaction(false);if(!scene.load_scene(std::move(draw),error))return false;skin_=std::move(skin);scene_=std::move(scene);binding_=std::make_shared<const u32>(hash);loaded_=true;if(error)error->clear();return true;
    }
    bool set_root(const Mat4& root,std::string* error){
        ready_=false;if(!loaded_||!binding_||!hsd_skin_detail::matrix_valid(root))return fail(error,"loaded resource/root domain");
        std::array<Mat4,8> world;world[0]=root;for(unsigned j=1;j<skin_.scene.joints.size();++j){const auto& n=skin_.scene.joints[j];world[j]=mat4_multiply(world[n.parent],mat4_from_srt(n.rotation,n.scale,n.translation));}
        HsdSkinGeometry next;if(!hsd_skin_sample(skin_,{world.data(),skin_.scene.joints.size()},&next,error))return false;geometry_=std::move(next);origins_.resize(skin_.meshes.size());for(unsigned m=0;m<origins_.size();++m)for(unsigned k=0;k<3;++k)origins_[m][k]=world[skin_.scene.mesh_joint[m]].at(k,3);ready_=true;if(error)error->clear();return true;
    }
    unsigned upload_textures(RenderEngine& r,u32 base=1){return loaded_&&binding_?scene_.upload_textures(r,base):0;}
    unsigned upload_textures_shared(RenderEngine& r,u32 base){return loaded_&&binding_?scene_.upload_textures_shared(r,base):0;}
    unsigned draw(RenderEngine& r,const Camera& camera){if(!ready())return 0;if(!scene_.set_pretransformed_geometry_with_origins(geometry_.world_vertices,std::vector<bool>(geometry_.world_vertices.size(),true),origins_)){ready_=false;return 0;}return scene_.draw(r,camera);}
    void discard_pose(){ready_=false;}bool ready()const{return ready_&&bool(binding_);}
    const HsdSkinAsset& asset()const{return skin_;}const HsdSkinGeometry& geometry()const{return geometry_;}
private:
    static bool fail(std::string* e,const char* why){if(e)*e=std::string("startup auxiliary scene: ")+why;return false;}
    HsdSkinAsset skin_;HsdSkinGeometry geometry_;MenuScene scene_;std::shared_ptr<const u32> binding_;bool loaded_=false,ready_=false;
    std::vector<std::array<f32,3>> origins_;
};
} // namespace gf
