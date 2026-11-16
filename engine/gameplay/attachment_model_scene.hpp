#pragma once
// Explicit single-joint rigid published-record renderer. Source800076D0 replaces
// the root matrix;80253174 uses view*world when(flags&E00)==0. No child state,
// actor/parent lifetime, shared efct hierarchy, alternate material or host hook.
#include <memory>
#include "engine/assets/hsd_skin.hpp"
#include "engine/assets/hsd_record_animation.hpp"
#include "engine/gameplay/actor_fade.hpp"
#include "engine/gameplay/attachment_transform.hpp"
#include "engine/gameplay/menu_scene.hpp"
#include "engine/graphics/affine.hpp"
namespace gf {
struct AttachmentModelBinding {u32 bank_hash=0,root=0;u16 record=0;};
// Immutable decoded bank for multiple rigid records. Pixel payloads are shared
// before any record copy; selection retains original texture-slot identities.
class AttachmentModelBank {
public:
    bool load(ByteView bank,std::string* error){
        storage_.reset();if(!bank.data||!bank.size||bank.size>(4u<<20)){if(error)*error="attachment bank: byte budget";return false;}
        auto s=std::make_shared<Storage>();if(!hsd_skin_load(bank,&s->skin,error)||!hsd_share_texture_images(s->skin.scene))return false;
        s->bytes.assign(bank.data,bank.data+bank.size);s->hash=2166136261u;for(auto byte:s->bytes)s->hash=(s->hash^byte)*16777619u;storage_=std::move(s);return true;
    }
    //Reuse the actual decoded source bank. This prepares renderer geometry and
    //skin metadata only; it creates no HsdJointInstanceOwner or scene descriptor.
    bool load_prepared(const HsdRecordAnimationBank& bank,std::string* error){
        storage_.reset();if(!bank.data_){if(error)*error="attachment bank: current decoded bank required";return false;}
        auto s=std::make_shared<Storage>();s->source=bank.data_;s->skin.scene=s->source->scene;
        if(!hsd_skin_metadata(s->skin.scene,s->source->reader,&s->skin.joints,&s->skin.meshes,error))return false;
        s->skin.loaded=true;s->hash=2166136261u;for(auto byte:s->source->bytes)s->hash=(s->hash^byte)*16777619u;storage_=std::move(s);return true;
    }
    bool ready()const{return bool(storage_);}u32 hash()const{return storage_?storage_->hash:0;}
    const HsdSkinAsset* asset()const{return storage_?&storage_->skin:nullptr;}
private:
    friend class AttachmentModelScene;
    struct Storage {std::vector<u8> bytes;std::shared_ptr<const hsd_record_detail::Bank> source;HsdSkinAsset skin;u32 hash=0;};
    std::shared_ptr<const Storage> storage_;
};
class AttachmentModelScene {
public:
    bool load(ByteView bank,u16 record,std::string* error){
        loaded_=ready_=false;binding_.reset();skin_={};geometry_={};
        HsdSkinAsset full;if(!hsd_skin_load(bank,&full,error))return false;
        auto textures=std::move(full.scene.textures);return load_record(bank,record,full,std::move(textures),error);
    }
    bool load_shared(const AttachmentModelBank& bank,u16 record,std::string* error){
        loaded_=ready_=false;binding_.reset();skin_={};geometry_={};
        if(!bank.storage_)return fail(error,"unloaded shared bank");
        const auto& s=*bank.storage_;const auto& bytes=s.source?s.source->bytes:s.bytes;return load_record({bytes.data(),bytes.size()},record,s.skin,s.skin.scene.textures,error);
    }
private:
    bool load_record(ByteView bank,u16 record,const HsdSkinAsset& full,std::vector<HsdTexture> textures,std::string* error){
        hsd_detail::Reader reader;if(!hsd_anim_detail::open_container(bank,&reader,error))return false;
        // Require the actual published scene_data record, not HSD's diagnostic
        // candidate-root fallback. Names and table entries remain bounded.
        const u32 publics=bank.be32(12),externs=bank.be32(16),size=bank.be32(0);
        if(publics>4096||externs>4096||record>=full.scene.model_roots.size())return fail(error,"record/symbol budget");
        const std::size_t symbols=32+reader.block.size+std::size_t(bank.be32(8))*4;
        const std::size_t strings=symbols+std::size_t(publics+externs)*8;bool found=false;u32 scene=0;
        for(u32 i=0;i<publics;++i){const u32 name=bank.be32(symbols+i*8+4);
            if(strings>size||name>=size-strings)return fail(error,"symbol name offset");
            const auto begin=strings+name;auto end=begin;
            while(end<size&&bank.data[end]){if(end-begin>=256)return fail(error,"symbol name budget");++end;}
            if(end==size)return fail(error,"unterminated symbol name");
            if(std::string(reinterpret_cast<const char*>(bank.data+begin),end-begin)=="scene_data"){
                if(found)return fail(error,"ambiguous scene export");
                found=true;scene=bank.be32(symbols+i*8);
            }
        }
        u32 table=0,descriptor=0,root=0,animation=0,material_animation=0;
        if(!found||!reader.ptr(scene,&table)||!table)return fail(error,"published model table");
        for(unsigned i=0;i<=record;++i)if(!reader.ptr(std::size_t(table)+i*4,&descriptor)||!descriptor)return fail(error,"published model record extent");
        if(!reader.ptr(descriptor,&root)||!root||root!=full.scene.model_roots[record]||
           !reader.ptr(descriptor+4,&animation)||!reader.ptr(descriptor+8,&material_animation)||animation||material_animation)return fail(error,"root identity or animated record requires separate evaluator");
        u32 joint=kHsdNoJoint;
        for(u32 i=0;i<full.scene.joints.size();++i)if(full.scene.joints[i].model_root==record){
            if(joint!=kHsdNoJoint)return fail(error,"multi-joint record requires separate hierarchy");
            joint=i;
        }
        if(joint==kHsdNoJoint||full.scene.joints[joint].archive_offset!=root||full.scene.joints[joint].parent!=kHsdNoJoint)return fail(error,"single published root binding");
        const auto flags=full.scene.joints[joint].flags;u32 child=0,next=0,robj=0;
        if((flags&(0xe00u|0x2000u|0x1000u|0x4000u|0x20u|0x10u))||
           (flags&0x70000000u)!=0x10000000u||(flags&0x1c0000u)!=0x40000u||
           !reader.ptr(root+8,&child)||!reader.ptr(root+12,&next)||!reader.ptr(root+0x3c,&robj)||child||next||robj)return fail(error,"billboard/instance/constraint/hierarchy or non-opaque root requires separate semantics");
        HsdSkinAsset selected;selected.loaded=true;selected.scene.model_roots={root};
        selected.scene.joints={full.scene.joints[joint]};selected.scene.joints[0].model_root=0;selected.joints={full.joints[joint]};
        selected.scene.textures=std::move(textures);selected.scene.lighting=full.scene.lighting;
        std::size_t bytes=0;for(const auto& t:selected.scene.textures){if(t.pixels().size()>(64u<<20)-bytes)return fail(error,"decoded byte budget");bytes+=t.pixels().size();}
        for(u32 i=0;i<full.meshes.size();++i){if(full.scene.mesh_joint[i]!=joint)continue;
            const auto& mesh=full.scene.meshes[i];const auto& skin=full.meshes[i];
            if(skin.kind!=HsdSkinKind::Rigid||skin.palettes.size()!=1||skin.palettes[0].influences.size()!=1||skin.palettes[0].influences[0].joint!=joint||skin.palettes[0].influences[0].weight!=1)return fail(error,"record is not source rigid-root geometry");
            if(!mesh.material_supported||(mesh.texture!=kHsdNoTexture&&(mesh.texture_matrix_mask&~u8(1))))return fail(error,"unsupported material/texture stages");
            if(mesh.generated_uv_required){
                if(mesh.texture_coordinate_mode!=1||!mesh.reflection_uv_supported||mesh.texture==kHsdNoTexture)return fail(error,"unsupported coordinate generator");
                for(const auto& v:mesh.vertices)if((mesh.texture_matrix_mask&1)?v.texture_matrix_indices[0]!=v.matrix_index+30:v.matrix_index!=0)return fail(error,"reflection palette binding");
            }else if(mesh.texture_matrix_required&&mesh.texture!=kHsdNoTexture)return fail(error,"unevaluated texture matrix");
            const auto work=mesh.vertices.size()*sizeof(HsdVertex)+mesh.indices.size()*sizeof(u32);
            if(work>(64u<<20)-bytes)return fail(error,"decoded geometry budget");
            bytes+=work;selected.scene.meshes.push_back(mesh);selected.scene.mesh_joint.push_back(0);selected.meshes.push_back(skin);selected.meshes.back().palettes[0].influences[0].joint=0;
        }
        if(selected.meshes.empty())return fail(error,"empty selected record");
        HsdSkinGeometry geometry;const std::array<Mat4,1> identity{};
        if(!hsd_skin_sample(selected,identity,&geometry,error))return false;
        HsdScene draw;draw.joints.resize(1);draw.textures=selected.scene.textures;draw.lighting=selected.scene.lighting;
        for(u32 i=0;i<selected.meshes.size();++i){auto mesh=selected.scene.meshes[i];mesh.vertices=geometry.world_vertices[i];mesh.deformation_required=mesh.generated_uv_required=mesh.texture_matrix_required=false;draw.meshes.push_back(std::move(mesh));draw.mesh_joint.push_back(0);}
        MenuScene renderer;renderer.set_interaction(false);if(!renderer.load_scene(std::move(draw),error))return false;
        u32 hash=2166136261u;for(std::size_t i=0;i<bank.size;++i){hash^=bank.data[i];hash*=16777619u;}
        binding_=std::make_shared<const AttachmentModelBinding>(AttachmentModelBinding{hash,root,record});
        skin_=std::move(selected);scene_=std::move(renderer);loaded_=true;if(error)error->clear();return true;
    }
public:
    //Authored rigid geometry without an actor fade/material callback. The scene
    //owner separately qualifies the root and its lifetime.
    bool set_world_static(const AttachmentBoneTransform& transform,std::string* error){
        ready_=false;
        if(!loaded_||!binding_||!transform.valid||!mat4_affine_bounded(transform.world))return fail(error,"current rigid model and bounded static world required");
        HsdSkinGeometry next;const std::array<Mat4,1> world={transform.world};
        if(!hsd_skin_sample(skin_,world,&next,error))return false;
        geometry_=std::move(next);for(unsigned k=0;k<3;++k)origin_[k]=transform.world.at(k,3);
        scene_.set_post_rgb_add({});ready_=true;if(error)error->clear();return true;
    }
    // Explicit supplied-transform/material seam, NOT parent-instance ownership.
    // Alternate80007C54/7CAC changes render mode as well as alpha: refuse it.
    bool set_world_fade(const AttachmentBoneTransform& transform,const ActorFadeMaterial& material,bool alternate,std::string* error){
        ready_=false;
        if(!loaded_||!binding_||!transform.valid||!material.source_class||alternate)return fail(error,"unloaded/invalid transform/material or unported alternate style");
        HsdSkinGeometry next;const std::array<Mat4,1> world={transform.world};
        if(!hsd_skin_sample(skin_,world,&next,error))return false;
        geometry_=std::move(next);for(unsigned k=0;k<3;++k)origin_[k]=transform.world.at(k,3);scene_.set_post_rgb_add(material.rgb);ready_=true;if(error)error->clear();return true;
    }
    unsigned upload_textures(RenderEngine& render,u32 base=1){return loaded_&&binding_?scene_.upload_textures(render,base):0;}
    unsigned upload_textures_shared(RenderEngine& render,u32 base){return loaded_&&binding_?scene_.upload_textures_shared(render,base):0;}
    unsigned draw(RenderEngine& render,const Camera& camera){
        if(!ready())return 0;
        auto vertices=geometry_.world_vertices;
        for(unsigned i=0;i<vertices.size();++i)if(skin_.scene.meshes[i].generated_uv_required)for(auto& v:vertices[i])
            if(!view_normal_reflection_uv(v.normal,camera,v.uv)){ready_=false;return 0;}
        if(!scene_.set_pretransformed_geometry_with_origins(vertices,std::vector<bool>(vertices.size(),true),std::vector<std::array<f32,3>>(vertices.size(),origin_))){ready_=false;return 0;}
        return scene_.draw(render,camera);
    }
    void discard_pose(){ready_=false;}
    bool ready()const{return ready_&&bool(binding_);}
    const HsdSkinAsset& asset()const{return skin_;}
    const HsdSkinGeometry& world_geometry()const{return geometry_;}
    std::shared_ptr<const AttachmentModelBinding> instance_binding()const{return binding_;}
private:
    static bool fail(std::string* error,const char* why){if(error)*error=std::string("attachment model scene: ")+why;return false;}
    HsdSkinAsset skin_;HsdSkinGeometry geometry_;MenuScene scene_;std::shared_ptr<const AttachmentModelBinding> binding_;
    std::array<f32,3> origin_{};
    bool loaded_=false,ready_=false;
};
} // namespace gf
