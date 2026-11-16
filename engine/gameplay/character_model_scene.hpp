#pragma once
// Explicit evaluated model-record0 renderer. Skinning and reflection UVs must
// both succeed before an identity draw copy is published. No actor controller,
// general material classes, attachment program, startup or combat admission.
// A separate explicit entry accepts the reviewed additive-RGB fade-mobj only.
#include "engine/assets/hsd_skin.hpp"
#include "engine/assets/hsd_record_animation.hpp"
#include "engine/gameplay/character_pose.hpp"
#include "engine/gameplay/actor_fade.hpp"
#include "engine/gameplay/menu_scene.hpp"
#include <memory>
namespace gf {
struct CharacterModelBinding {u32 model_hash=0;};
inline bool character_reflection_uv(const f32 normal[3],const Camera& camera,f32 out[2]){
    return view_normal_reflection_uv(normal,camera,out);
}
struct CharacterModelRenderState {
    u32 visible_bones=0xffffffffu; //80056AA8 node-only mask; not branch hiding
    bool dynamic_material=false,attachments=false;
};
class CharacterModelScene {
public:
    bool load(ByteView archive,ByteView prefix,ByteView profile,std::string* error){
        return load_impl(archive,prefix,profile,nullptr,error);
    }
    //Explicit shared decoded bank entry. Body skin metadata/evaluator remains
    //the SAME implementation; normal/alternate record instances are not actors.
    bool load_with_record_bank(const HsdRecordAnimationBank& bank,ByteView prefix,ByteView profile,std::string* error){
        if(!bank.data_){loaded_=ready_=draw_prepared_=false;geometry_={};skin_={};rig_={};binding_.reset();return failure(error,"retained decoded bank required");}
        const auto& b=*bank.data_;return load_impl({b.bytes.data(),b.bytes.size()},prefix,profile,&b,error);
    }
private:
    bool load_impl(ByteView archive,ByteView prefix,ByteView profile,const hsd_record_detail::Bank* bank,std::string* error){
        loaded_=ready_=draw_prepared_=false;geometry_={};skin_={};rig_={};binding_.reset();
        auto fail=[&](const char* why){return failure(error,why);};
        HsdSkinAsset full;HsdModelBones bones;CharacterPoseRig rig;
        if(bank){if(!hsd_skin_metadata(bank->scene,bank->reader,&full.joints,&full.meshes,error))return false;}
        else if(!hsd_skin_load(archive,&full,error))return false;
        if(!hsd_bones_load(prefix,archive,&bones,error)||!character_pose_rig_load(profile,bones,&rig,error))return false;
        const auto& source=bank?bank->scene:full.scene;
        const auto n=bones.bones.size();std::vector<u32> remap(source.joints.size(),kHsdNoJoint),indices;
        for(u32 slot=0;slot<n;++slot){u32 old=kHsdNoJoint;
            for(u32 j=0;j<source.joints.size();++j)if(source.joints[j].archive_offset==bones.bones[slot].joint_offset){if(old!=kHsdNoJoint)return fail("ambiguous scene bone");old=j;}
            if(old==kHsdNoJoint||source.joints[old].model_root!=0||(source.joints[old].flags&0x1e00u))return fail("missing/non-record0/billboard/instance bone");
            remap[old]=slot;indices.push_back(old);
        }
        HsdSkinAsset selected;selected.scene.model_roots={bones.root_offset};if(bank)selected.scene.textures=source.textures;else selected.scene.textures=std::move(full.scene.textures);selected.scene.lighting=source.lighting;
        std::size_t decoded=0;for(const auto& texture:selected.scene.textures){if(texture.pixels().size()>(64u<<20)-decoded)return fail("selected renderer byte budget");decoded+=texture.pixels().size();}
        for(auto old:indices){auto joint=source.joints[old];
            if(joint.parent!=kHsdNoJoint){if(joint.parent>=remap.size()||remap[joint.parent]==kHsdNoJoint)return fail("parent outside selected skeleton");joint.parent=remap[joint.parent];}
            joint.model_root=0;selected.scene.joints.push_back(joint);selected.joints.push_back(full.joints[old]);
        }
        for(u32 m=0;m<full.meshes.size();++m){const auto owner=source.mesh_joint[m];if(source.joints[owner].model_root!=0)continue;
            if(remap[owner]==kHsdNoJoint)return fail("mesh owner outside selected skeleton");
            auto metadata=full.meshes[m];for(auto& palette:metadata.palettes)for(auto& influence:palette.influences){
                if(influence.joint>=remap.size()||remap[influence.joint]==kHsdNoJoint)return fail("skin influence outside selected skeleton");
                influence.joint=remap[influence.joint];
            }
            const auto& mesh=source.meshes[m];
            const std::size_t bytes=mesh.vertices.size()*sizeof(HsdVertex)+mesh.indices.size()*sizeof(u32);
            if(bytes>(64u<<20)-decoded)return fail("selected renderer byte budget");
            decoded+=bytes;
            if(!mesh.material_supported||(mesh.texture!=kHsdNoTexture&&(mesh.texture_matrix_mask&~u8(1))))return fail("unsupported material/texture-matrix stages");
            if(mesh.generated_uv_required){
                if(mesh.texture_coordinate_mode!=1||!mesh.reflection_uv_supported||mesh.texture==kHsdNoTexture)return fail("unsupported generated coordinates");
                for(const auto& vertex:mesh.vertices){
                    if(mesh.texture_matrix_mask&1){if(vertex.texture_matrix_indices[0]!=vertex.matrix_index+30)return fail("reflection matrix/position palette mismatch");}
                    else if(vertex.matrix_index!=0)return fail("nonuniform reflection palette without vertex selector");
                }
            }else if(mesh.texture_matrix_required&&mesh.texture!=kHsdNoTexture)return fail("explicit texture matrix without qualified generator");
            selected.scene.meshes.push_back(mesh);selected.scene.mesh_joint.push_back(remap[owner]);selected.meshes.push_back(std::move(metadata));
        }
        if(selected.meshes.empty())return fail("empty selected model");
        selected.loaded=true;
        HsdBoneMotionSample values;values.bones.resize(n);for(u32 j=0;j<n;++j)values.bones[j].joint_offset=bones.bones[j].joint_offset;
        std::array<CharacterPoseLane,4> lanes;for(unsigned i=0;i<4;++i)lanes[i].current=&values;
        CharacterLocalPose local;CharacterPoseFrame frame;CharacterWorldPose world;HsdSkinGeometry geometry;
        if(!character_pose_sample(rig,lanes,&local,error)||!character_pose_initial_frame(rig,Mat4{},&frame,error)||!character_pose_direct_world(rig,local,frame,&world,error)||!hsd_skin_sample(selected,world.matrices,&geometry,error))return false;
        HsdScene render_scene;render_scene.joints.resize(1);render_scene.textures=selected.scene.textures;render_scene.lighting=selected.scene.lighting;
        for(u32 m=0;m<selected.meshes.size();++m){auto mesh=selected.scene.meshes[m];mesh.vertices=geometry.world_vertices[m];
            // Evaluator preflight succeeded; only this ephemeral identity draw
            // copy clears source skin/texgen requirements. Default MenuScene
            // still refuses the unevaluated original scene.
            mesh.deformation_required=false;mesh.generated_uv_required=false;mesh.texture_matrix_required=false;
            render_scene.meshes.push_back(std::move(mesh));render_scene.mesh_joint.push_back(0);
        }
        MenuScene renderer;renderer.set_interaction(false);
        if(!renderer.load_scene(std::move(render_scene),error))return false;
        u32 hash=2166136261u;for(std::size_t i=0;i<archive.size;++i){hash^=archive.data[i];hash*=16777619u;}
        binding_=std::make_shared<const CharacterModelBinding>(CharacterModelBinding{hash});
        scene_=std::move(renderer);skin_=std::move(selected);rig_=std::move(rig);loaded_=true;error_.clear();return true;
    }
public:
    bool set_world_pose(const CharacterWorldPose& world,const CharacterModelRenderState& state,std::string* error){
        ready_=draw_prepared_=false;
        if(!loaded_||state.dynamic_material||state.attachments)return failure(error,"unloaded model or unported dynamic material/attachments");
        if(world.joint_offsets.size()!=rig_.binding.bones.size()||world.matrices.size()!=world.joint_offsets.size())return failure(error,"pose shape");
        for(unsigned i=0;i<world.joint_offsets.size();++i)if(world.joint_offsets[i]!=rig_.binding.bones[i].joint_offset)return failure(error,"pose identity");
        HsdSkinGeometry evaluated;std::string why;if(!hsd_skin_sample(skin_,world.matrices,&evaluated,&why))return failure(error,why.c_str());
        geometry_=std::move(evaluated);visibility_.resize(skin_.meshes.size());origins_.resize(skin_.meshes.size());
        for(unsigned m=0;m<origins_.size();++m)for(unsigned k=0;k<3;++k)origins_[m][k]=world.matrices[skin_.scene.mesh_joint[m]].at(k,3);
        for(unsigned m=0;m<visibility_.size();++m)visibility_[m]=(state.visible_bones&(u32(1)<<skin_.scene.mesh_joint[m]))!=0;
        scene_.set_post_rgb_add({});ready_=true;error_.clear();return true;
    }
    // Explicit reviewed fade-mobj entry. Legacy dynamic-material requests still
    // refuse; other classes/attachments are not admitted through this adapter.
    bool set_world_pose_fade(const CharacterWorldPose& world,const CharacterModelRenderState& state,
                             const ActorFadeMaterial& material,std::string* error){
        ready_=false;if(!state.dynamic_material||!material.source_class)return failure(error,"missing explicit source fade-mobj binding");
        auto base=state;base.dynamic_material=false;if(!set_world_pose(world,base,error))return false;
        scene_.set_post_rgb_add(material.rgb);return true;
    }
    unsigned upload_textures(RenderEngine& render,u32 base=1){return loaded_?scene_.upload_textures(render,base):0;}
    unsigned upload_textures_shared(RenderEngine& render,u32 base){return loaded_?scene_.upload_textures_shared(render,base):0;}
    unsigned draw(RenderEngine& render,const Camera& camera){
        if(!ready_)return 0;
        Camera::Basis basis;if(!camera.basis(&basis))return 0;
        const auto key=camera.cache_key();
        // Several world passes visit the same evaluated actor. Reflection and
        // identity-geometry publication depend on pose/view, not pass number.
        if(!draw_prepared_||key!=draw_camera_){
            draw_prepared_=false;auto vertices=geometry_.world_vertices;
            for(unsigned m=0;m<vertices.size();++m)if(skin_.scene.meshes[m].generated_uv_required)
                for(auto& vertex:vertices[m])if(!view_normal_reflection_uv(vertex.normal,basis,vertex.uv)){failure(nullptr,"invalid reflection normal/camera");return 0;}
            if(!scene_.set_pretransformed_geometry_with_origins(vertices,visibility_,origins_)){failure(nullptr,"evaluated identity-geometry binding");return 0;}
            draw_camera_=key;draw_prepared_=true;
        }
        error_.clear();return scene_.draw(render,camera);
    }
    const CharacterPoseRig& rig()const{return rig_;}
    std::shared_ptr<const CharacterModelBinding> instance_binding()const{return binding_;}
    void discard_pose(){ready_=draw_prepared_=false;}
    const HsdSkinAsset& asset()const{return skin_;}
    const HsdSkinGeometry& world_geometry()const{return geometry_;}
    const std::string& error()const{return error_;}
    bool ready()const{return ready_;}
private:
    bool failure(std::string* error,const char* why){error_=std::string("character model scene: ")+why;if(error)*error=error_;return false;}
    HsdSkinAsset skin_;CharacterPoseRig rig_;MenuScene scene_;HsdSkinGeometry geometry_;std::vector<bool> visibility_;
    std::vector<std::array<f32,3>> origins_;
    bool loaded_=false,ready_=false;std::string error_;
    bool draw_prepared_=false;std::array<u32,17> draw_camera_{};
    std::shared_ptr<const CharacterModelBinding> binding_;
};
} // namespace gf
