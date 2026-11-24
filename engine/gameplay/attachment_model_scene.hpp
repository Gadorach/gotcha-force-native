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
#include "engine/assets/hsd_joint_instances.hpp"
#include "engine/assets/hsd_static_identity_colour.hpp"
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
    bool same_source(const HsdRecordAnimationBank& bank)const{return storage_&&storage_->source&&storage_->source==bank.data_;}
    const HsdSkinAsset* asset()const{return storage_?&storage_->skin:nullptr;}
private:
    friend class AttachmentModelScene;
    struct Storage {std::vector<u8> bytes;std::shared_ptr<const hsd_record_detail::Bank> source;HsdSkinAsset skin;u32 hash=0;};
    std::shared_ptr<const Storage> storage_;
};
class AttachmentModelScene {
public:
    bool load(ByteView bank,u16 record,std::string* error){
        published_mode_=empty_static_=false;published_world_={};origins_.clear();visible_.clear();
        loaded_=ready_=false;binding_.reset();skin_={};geometry_={};
        HsdSkinAsset full;if(!hsd_skin_load(bank,&full,error))return false;
        auto textures=std::move(full.scene.textures);return load_record(bank,record,full,std::move(textures),error);
    }
    bool load_shared(const AttachmentModelBank& bank,u16 record,std::string* error){
        published_mode_=empty_static_=false;published_world_={};origins_.clear();visible_.clear();
        loaded_=ready_=false;binding_.reset();skin_={};geometry_={};
        if(!bank.storage_)return fail(error,"unloaded shared bank");
        const auto& s=*bank.storage_;const auto& bytes=s.source?s.source->bytes:s.bytes;return load_record({bytes.data(),bytes.size()},record,s.skin,s.skin.scene.textures,error);
    }
    //Explicit scoped static hierarchy entry. Legacy single-joint/supplied-matrix
    //entries retain their old refusals. No new actor/preview renderer is created.
    bool load_published_static(const AttachmentModelBank& bank,const HsdRecordAnimationBank& source,const HsdJointInstanceOwner& root,std::string* error,HsdPublishedMaterialPolicy policy=HsdPublishedMaterialPolicy::Strict){
        return load_published(bank,source,root,error,policy,false);
    }
    bool load_published_side_identity(const AttachmentModelBank& bank,const HsdRecordAnimationBank& source,const HsdJointInstanceOwner& root,std::string* error,HsdPublishedMaterialPolicy policy=HsdPublishedMaterialPolicy::Strict){
        return load_published(bank,source,root,error,policy,true);
    }
private:
    bool load_published(const AttachmentModelBank& bank,const HsdRecordAnimationBank& source,const HsdJointInstanceOwner& root,std::string* error,HsdPublishedMaterialPolicy policy,bool side_identity){
        loaded_=ready_=published_mode_=empty_static_=false;binding_.reset();published_world_={};skin_={};geometry_={};origins_.clear();visible_.clear();
        auto fail_here=[&](const char* why){return fail(error,why);};
        if(!bank.storage_||!bank.same_source(source)||!root.same_source(source)||!root.world_current())return fail_here("same prepared bank and current scoped root required");
        const auto receipt=root.world_receipt();const auto* state=root.state();const auto& full=bank.storage_->skin;
        if(!receipt.live()||!state||state->style!=HsdJointInstanceStyle::Normal||state->animation_requested||state->side_style_applied||state->stage_shadow_applied||state->child_translucent_applied||state->joints.empty()||state->joints.size()>32||state->record>=full.scene.model_roots.size()||state->joints[0].joint.archive_offset!=full.scene.model_roots[state->record])return fail_here("normal unanimated bounded published record required");
        if(side_identity&&(!state->source_zero_colour_cache||state->joints.size()!=1||(state->joints[0].joint.flags&0x03800000u)!=0x03800000u))return fail_here("reviewed side zero-colour/one-root matrix publication required");
        if(!side_identity&&state->source_zero_colour_cache)return fail_here("side matrix publication requires its explicit geometry entry");
        try{
            HsdSkinAsset selected;selected.loaded=true;selected.scene.model_roots={full.scene.model_roots[state->record]};selected.scene.textures=full.scene.textures;selected.scene.lighting=full.scene.lighting;
            std::vector<u32> remap(full.joints.size(),kHsdNoJoint);std::vector<Mat4> worlds;worlds.reserve(state->joints.size());
            for(unsigned i=0;i<state->joints.size();++i){
                const auto& j=state->joints[i];const auto index=j.source_index;
                const u32 forbidden=side_identity?0x00007e30u:0x03807e30u;
                if(index>=full.joints.size()||remap[index]!=kHsdNoJoint||full.scene.joints[index].archive_offset!=j.joint.archive_offset||full.scene.joints[index].model_root!=state->record||!j.world||!mat4_affine_bounded(*j.world)||(j.joint.flags&forbidden))return fail_here("same published authored joint map/world required");
                if((!i&&j.joint.parent!=kHsdNoJoint)||(i&&j.joint.parent>=i))return fail_here("published hierarchy parent order");
                for(unsigned k=0;k<3;++k)if(j.joint.rotation[k]!=0||j.joint.scale[k]!=1)return fail_here("qualified static local metadata required");
                remap[index]=i;auto joint=j.joint;joint.model_root=0;selected.scene.joints.push_back(joint);selected.joints.push_back(full.joints[index]);worlds.push_back(*j.world);
            }
            std::vector<std::array<f32,3>> origins;std::vector<bool> visible;std::size_t bytes=0;
            for(unsigned mesh=0;mesh<full.meshes.size();++mesh){
                const auto owner=full.scene.mesh_joint[mesh];if(owner>=remap.size())return fail_here("source mesh owner extent");
                if(remap[owner]==kHsdNoJoint)continue;
                const auto found=std::find_if(state->mesh_properties.begin(),state->mesh_properties.end(),[&](const auto& p){return p.first==mesh;});
                if(found==state->mesh_properties.end()||found->second.pobj_offset!=full.scene.meshes[mesh].pobj_offset||found->second.material_offset!=full.scene.meshes[mesh].material_offset||found->second.dobj_offset!=full.scene.meshes[mesh].dobj_offset||found->second.pobj_flags!=full.scene.meshes[mesh].pobj_flags||found->second.skin_binding!=full.scene.meshes[mesh].skin_binding)return fail_here("same published PObj/material/skin metadata required");
                auto geometry=full.scene.meshes[mesh];static_cast<HsdMeshProperties&>(geometry)=found->second;
                if(!geometry.material_supported&&policy==HsdPublishedMaterialPolicy::StaticIdentityColourSubtract){
                    HsdMeshProperties reduced;if(!hsd_static_identity_colour(bank.storage_->source->reader,geometry,&reduced,error))return false;
                    static_cast<HsdMeshProperties&>(geometry)=reduced;
                }
                if(!geometry.material_supported||(geometry.texture_matrix_mask&~u8(1)))return fail_here("unsupported published material/texture stages");
                if(geometry.generated_uv_required){
                    if(geometry.texture_coordinate_mode!=1||!geometry.reflection_uv_supported||geometry.texture==kHsdNoTexture)return fail_here("unsupported static coordinate generator");
                    for(const auto& v:geometry.vertices)if((geometry.texture_matrix_mask&1)?v.texture_matrix_indices[0]!=v.matrix_index+30:v.matrix_index!=0)return fail_here("reflection palette binding");
                }else if(geometry.texture_matrix_required&&geometry.texture!=kHsdNoTexture)return fail_here("unevaluated published texture matrix");
                auto skin=full.meshes[mesh];
                for(auto& palette:skin.palettes)for(auto& influence:palette.influences){if(influence.joint>=remap.size()||remap[influence.joint]==kHsdNoJoint)return fail_here("cross-record skin influence requires separate source owner");influence.joint=remap[influence.joint];}
                const auto work=geometry.vertices.size()*sizeof(HsdVertex)+geometry.indices.size()*sizeof(u32);
                if(work>(64u<<20)-bytes)return fail_here("published geometry byte budget");
                bytes+=work;selected.scene.meshes.push_back(std::move(geometry));selected.scene.mesh_joint.push_back(remap[owner]);selected.meshes.push_back(std::move(skin));
                std::array<f32,3> origin;for(unsigned k=0;k<3;++k)origin[k]=worlds[remap[owner]].at(k,3);origins.push_back(origin);
                bool hidden=false;u32 joint=remap[owner];for(unsigned n=0; ;++n){if(n>=state->joints.size())return fail_here("visibility parent cycle");hidden|=bool(state->joints[joint].joint.flags&0x10u);joint=state->joints[joint].joint.parent;if(joint==kHsdNoJoint)break;}visible.push_back(!hidden);
            }
            if(selected.meshes.empty()){
                if(!state->mesh_properties.empty()||state->joints.size()!=1||!source.is_empty_static_record(state->record)||!receipt.live())return fail_here("empty geometry requires an explicit authored empty-static record");
                binding_=std::make_shared<const AttachmentModelBinding>(AttachmentModelBinding{bank.hash(),full.scene.model_roots[state->record],state->record});
                skin_=std::move(selected);published_world_=receipt;loaded_=ready_=published_mode_=empty_static_=true;
                if(error){error->clear();}return true;
            }
            if(selected.meshes.size()!=state->mesh_properties.size())return fail_here("complete published geometry required");
            HsdSkinGeometry evaluated;if(!hsd_skin_sample(selected,worlds,&evaluated,error))return false;
            HsdScene draw;draw.joints.resize(1);draw.textures=selected.scene.textures;draw.lighting=selected.scene.lighting;
            for(unsigned i=0;i<selected.meshes.size();++i){auto mesh=selected.scene.meshes[i];mesh.vertices=evaluated.world_vertices[i];mesh.deformation_required=mesh.generated_uv_required=mesh.texture_matrix_required=false;draw.meshes.push_back(std::move(mesh));draw.mesh_joint.push_back(0);}
            MenuScene renderer;renderer.set_interaction(false);if(!renderer.load_scene(std::move(draw),error))return false;
            if(!receipt.live()||!root.world_current())return fail_here("published root retired during geometry preparation");
            binding_=std::make_shared<const AttachmentModelBinding>(AttachmentModelBinding{bank.hash(),full.scene.model_roots[state->record],state->record});
            skin_=std::move(selected);geometry_=std::move(evaluated);scene_=std::move(renderer);origins_=std::move(origins);visible_=std::move(visible);published_world_=receipt;loaded_=ready_=published_mode_=true;
            if(error){error->clear();}return true;
        }catch(const std::exception&){return fail_here("uncertain published static preparation; no draw grant");}
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
    //owner separately qualifies the published root and its lifetime.
    bool set_world_static(const AttachmentBoneTransform& transform,std::string* error){
        ready_=false;
        if(published_mode_||!loaded_||!binding_||!transform.valid||!mat4_affine_bounded(transform.world))return fail(error,"current rigid model and bounded static world required; published hierarchies cannot be repositioned");
        HsdSkinGeometry next;const std::array<Mat4,1> world={transform.world};
        if(!hsd_skin_sample(skin_,world,&next,error))return false;
        geometry_=std::move(next);for(unsigned k=0;k<3;++k)origin_[k]=transform.world.at(k,3);
        scene_.set_post_rgb_add({});ready_=true;if(error)error->clear();return true;
    }
    // Explicit supplied-transform/material seam, NOT parent-instance ownership.
    // Alternate80007C54/7CAC changes render mode as well as alpha: refuse it.
    bool set_world_fade(const AttachmentBoneTransform& transform,const ActorFadeMaterial& material,bool alternate,std::string* error){
        ready_=false;
        if(published_mode_||!loaded_||!binding_||!transform.valid||!material.source_class||alternate)return fail(error,"unloaded/invalid transform/material or unported alternate style");
        HsdSkinGeometry next;const std::array<Mat4,1> world={transform.world};
        if(!hsd_skin_sample(skin_,world,&next,error))return false;
        geometry_=std::move(next);for(unsigned k=0;k<3;++k)origin_[k]=transform.world.at(k,3);scene_.set_post_rgb_add(material.rgb);ready_=true;if(error)error->clear();return true;
    }
    unsigned upload_textures(RenderEngine& render,u32 base=1){return !empty_static_&&loaded_&&binding_&&(!published_mode_||published_world_.live())?scene_.upload_textures(render,base):0;}
    unsigned upload_textures_shared(RenderEngine& render,u32 base){return !empty_static_&&loaded_&&binding_&&(!published_mode_||published_world_.live())?scene_.upload_textures_shared(render,base):0;}
    unsigned draw(RenderEngine& render,const Camera& camera){
        if(!ready()||empty_static_)return 0;
        auto vertices=geometry_.world_vertices;
        for(unsigned i=0;i<vertices.size();++i)if(skin_.scene.meshes[i].generated_uv_required)for(auto& v:vertices[i])
            if(!view_normal_reflection_uv(v.normal,camera,v.uv)){ready_=false;return 0;}
        if(!scene_.set_pretransformed_geometry_with_origins(vertices,published_mode_?visible_:std::vector<bool>(vertices.size(),true),published_mode_?origins_:std::vector<std::array<f32,3>>(vertices.size(),origin_))){ready_=false;return 0;}
        return scene_.draw(render,camera);
    }
    void discard_pose(){ready_=false;}
    bool ready()const{return ready_&&bool(binding_)&&(!published_mode_||published_world_.live());}
    bool authored_empty_static()const{return published_mode_&&empty_static_&&ready();}
    const HsdSkinAsset& asset()const{return skin_;}
    const HsdSkinGeometry& world_geometry()const{return geometry_;}
    std::shared_ptr<const AttachmentModelBinding> instance_binding()const{return binding_;}
private:
    static bool fail(std::string* error,const char* why){if(error)*error=std::string("attachment model scene: ")+why;return false;}
    HsdSkinAsset skin_;HsdSkinGeometry geometry_;MenuScene scene_;std::shared_ptr<const AttachmentModelBinding> binding_;
    std::array<f32,3> origin_{};
    bool loaded_=false,ready_=false,published_mode_=false,empty_static_=false;HsdJointWorldReceipt published_world_;
    std::vector<std::array<f32,3>> origins_;std::vector<bool> visible_;
};
} // namespace gf
