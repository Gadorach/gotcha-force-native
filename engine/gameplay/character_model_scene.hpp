#pragma once
#include "engine/assets/character_preview_mount.hpp"
#include "engine/assets/borg_reflection_policy.hpp"
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
#include <span>
namespace gf {
inline bool character_zero_normal(const HsdVertex& v){return v.normal[0]==0&&v.normal[1]==0&&v.normal[2]==0;}
struct CharacterModelBinding {u32 model_hash=0;};
inline bool character_reflection_uv(const f32 normal[3],const Camera& camera,f32 out[2]){
    return view_normal_reflection_uv(normal,camera,out);
}
// For the provisional walk lab only. Repair only authored zero normals on
// reflected geometry, not invalid poses or a failed texture generator.
// The derived direction is an approximation of presentation, not retail data.
inline bool character_walk_reflection_uv(const HsdMesh& source,std::vector<HsdVertex>& posed,
                                          const Camera::Basis& basis){
    if(source.vertices.size()!=posed.size()||source.indices.size()%3)return false;
    const auto zero=character_zero_normal;
    std::vector<std::array<f64,3>> area_normals;
    std::vector<std::array<f64,3>> first_face;
    if(std::any_of(posed.begin(),posed.end(),zero)){
        area_normals.resize(posed.size());first_face.resize(posed.size());
        for(std::size_t i=0;i<source.indices.size();i+=3){
            const std::array<u32,3> idx{source.indices[i],source.indices[i+1],source.indices[i+2]};
            if(*std::max_element(idx.begin(),idx.end())>=posed.size())return false;
            const auto& a=posed[idx[0]];const auto& b=posed[idx[1]];const auto& c=posed[idx[2]];
            if(!zero(a)&&!zero(b)&&!zero(c))continue;
            std::array<f64,3> u{},v{},face{};
            for(unsigned k=0;k<3;++k){u[k]=f64(b.position[k])-a.position[k];v[k]=f64(c.position[k])-a.position[k];}
            for(unsigned k=0;k<3;++k){
                const unsigned j=(k+1)%3,l=(k+2)%3;
                face[k]=u[j]*v[l]-u[l]*v[j];
                if(!std::isfinite(face[k]))return false;
            }
            if(face==std::array<f64,3>{})continue;
            // Give the face the same orientation as the neighbouring authored
            // normal where one exists; flat all-zero faces use source winding.
            for(const auto j:idx)if(!zero(posed[j])){
                f64 dot=0;for(unsigned k=0;k<3;++k)dot+=face[k]*posed[j].normal[k];
                if(dot<0)for(auto& value:face)value=-value;
                break;
            }
            for(const auto j:idx)if(zero(posed[j])){
                if(!zero(source.vertices[j]))return false;
                if(first_face[j]==std::array<f64,3>{})first_face[j]=face;
                for(unsigned k=0;k<3;++k)area_normals[j][k]+=face[k];
            }
        }
    }
    for(std::size_t i=0;i<posed.size();++i){
        auto& vertex=posed[i];
        if(zero(vertex)){
            if(!zero(source.vertices[i]))return false;
            if(first_face[i]==std::array<f64,3>{})continue; // no nondegenerate submitted face
            auto normal=area_normals[i];
            if(normal==std::array<f64,3>{})normal=first_face[i];
            const f64 length=std::hypot(normal[0],normal[1],normal[2]);
            if(!std::isfinite(length)||length<=0)return false;
            for(unsigned k=0;k<3;++k)vertex.normal[k]=f32(normal[k]/length);
        }
        if(!view_normal_reflection_uv(vertex.normal,basis,vertex.uv))return false;
    }
    return true;
}
struct CharacterModelRenderState {
    u32 visible_bones=0xffffffffu; //80056AA8 node-only mask; not branch hiding
    bool dynamic_material=false,attachments=false;
};
struct CharacterTextureScroll {u8 bone=0,axis=0;f32 offset=0;};
class CharacterModelScene {
public:
    enum class Inspection { Strict, ProvisionalWalk };
    bool load(ByteView archive,ByteView prefix,ByteView profile,std::string* error,
              HsdStaticMaterialPolicy material=HsdStaticMaterialPolicy::Legacy){
        return load_impl(archive,prefix,profile,nullptr,error,BodyScope::Archive,{},material);
    }
    // Explicit package body scope: validate the selected mesh and skin
    // strictly; do not evaluate other records as this actor's body.
    bool load_record_zero(ByteView archive,ByteView prefix,ByteView profile,std::string* error,
                          HsdStaticMaterialPolicy material=HsdStaticMaterialPolicy::Legacy){
        return load_impl(archive,prefix,profile,nullptr,error,BodyScope::RecordZero,{},material);
    }
    bool load_assembled_record_zero(const HsdRecordAnimationBank& bank,ByteView prefix,ByteView profile,
                                    std::span<const CharacterPreviewMount> mounts,std::string* error){
        if(!bank.data_||mounts.empty()){
            loaded_=ready_=draw_prepared_=false;binding_.reset();
            return failure(error,"assembled body requires its exact bank and mounts");
        }
        const auto& b=*bank.data_;
        return load_impl({b.bytes.data(),b.bytes.size()},prefix,profile,&b,error,BodyScope::RecordZero,mounts);
    }
    //Explicit shared decoded bank entry. Body skin metadata/evaluator remains
    //the SAME implementation; normal/alternate record instances are not actors.
    bool load_with_record_bank(const HsdRecordAnimationBank& bank,ByteView prefix,ByteView profile,std::string* error){
        if(!bank.data_){loaded_=ready_=draw_prepared_=repair_walk_normals_=false;geometry_={};skin_={};rig_={};binding_.reset();return failure(error,"retained decoded bank required");}
        const auto& b=*bank.data_;return load_impl({b.bytes.data(),b.bytes.size()},prefix,profile,&b,error);
    }
    // Only for body inspection: unselected records contribute no skin, bones
    // or draws. Explicit mounts add static same-bank subtrees to the pose.
    bool load_preview_with_record_bank(const HsdRecordAnimationBank& bank,ByteView prefix,ByteView profile,std::string* error,
                                        Inspection inspection=Inspection::Strict,std::span<const CharacterPreviewMount> mounts={}){
        if(!bank.data_)return load_with_record_bank(bank,prefix,profile,error);
        const auto& b=*bank.data_;
        const bool loaded=load_impl({b.bytes.data(),b.bytes.size()},prefix,profile,&b,error,BodyScope::Inspection,mounts);
        repair_walk_normals_=loaded&&inspection==Inspection::ProvisionalWalk;
        return loaded;
    }
private:
    enum class BodyScope {Archive,RecordZero,Inspection};
    struct MountedJoint {u32 parent=0;Mat4 local;};
    bool load_impl(ByteView archive,ByteView prefix,ByteView profile,const hsd_record_detail::Bank* bank,std::string* error,
                   BodyScope scope=BodyScope::Archive,std::span<const CharacterPreviewMount> mounts={},
                   HsdStaticMaterialPolicy material=HsdStaticMaterialPolicy::Legacy){
        loaded_=ready_=draw_prepared_=repair_walk_normals_=false;geometry_={};skin_={};rig_={};binding_.reset();zero_reflection_meshes_.clear();
        mounted_joints_.clear();mesh_bones_.clear();
        auto fail=[&](const char* why){return failure(error,why);};
        if(!character_preview_mounts_valid(mounts))return fail("invalid static mounts");
        const bool body_preview=scope==BodyScope::Inspection;
        const bool record_zero=scope!=BodyScope::Archive;
        const auto selected_record=record_zero?std::optional<u32>(0):std::nullopt;
        HsdSkinAsset full;HsdModelBones bones;CharacterPoseRig rig;
        if(bank){if(!hsd_skin_metadata(bank->scene,bank->reader,&full.joints,&full.meshes,error,selected_record))return false;}
        else if(!hsd_skin_load(archive,&full,error,selected_record,material))return false;
        if(!hsd_bones_load(prefix,archive,&bones,error)||!character_pose_rig_load(profile,bones,&rig,error))return false;
        const auto& source=bank?bank->scene:full.scene;
        if(scope==BodyScope::RecordZero&&(source.model_roots.empty()||source.model_roots[0]!=bones.root_offset))
            return fail("record-zero body root does not match the declared rig");
        const auto n=bones.bones.size();std::vector<u32> remap(source.joints.size(),kHsdNoJoint),indices;
        for(u32 slot=0;slot<n;++slot){u32 old=kHsdNoJoint;
            for(u32 j=0;j<source.joints.size();++j)
                if((!record_zero||source.joints[j].model_root==0)&&source.joints[j].archive_offset==bones.bones[slot].joint_offset){
                    if(old!=kHsdNoJoint)return fail("ambiguous scene bone");
                    old=j;
                }
            if(old==kHsdNoJoint||source.joints[old].model_root!=0||(source.joints[old].flags&0x1e00u))return fail("missing/non-record0/billboard/instance bone");
            remap[old]=slot;indices.push_back(old);
        }
        HsdSkinAsset selected;selected.scene.model_roots={bones.root_offset};if(bank)selected.scene.textures=source.textures;else selected.scene.textures=std::move(full.scene.textures);selected.scene.lighting=source.lighting;
        std::size_t decoded=0;for(const auto& texture:selected.scene.textures){if(texture.pixels().size()>(64u<<20)-decoded)return fail("selected renderer byte budget");decoded+=texture.pixels().size();}
        for(auto old:indices){auto joint=source.joints[old];
            if(joint.parent!=kHsdNoJoint){if(joint.parent>=remap.size()||remap[joint.parent]==kHsdNoJoint)return fail("parent outside selected skeleton");joint.parent=remap[joint.parent];}
            joint.model_root=0;selected.scene.joints.push_back(joint);selected.joints.push_back(full.joints[old]);
        }
        std::vector<u8> mesh_bones;
        auto append_mesh=[&](u32 m,HsdSkinMesh metadata,u8 bone){
            const auto owner=source.mesh_joint[m];
            if(remap[owner]==kHsdNoJoint)return fail("mesh owner outside selected skeleton");
            for(auto& palette:metadata.palettes)for(auto& influence:palette.influences){
                if(influence.joint>=remap.size()||remap[influence.joint]==kHsdNoJoint)return fail("skin influence outside selected skeleton");
                influence.joint=remap[influence.joint];
            }
            const auto& mesh=source.meshes[m];
            const std::size_t bytes=mesh.vertices.size()*sizeof(HsdVertex)+mesh.indices.size()*sizeof(u32);
            if(bytes>(64u<<20)-decoded)return fail("selected renderer byte budget");
            decoded+=bytes;
            if(!mesh.material_supported||(mesh.texture!=kHsdNoTexture&&(mesh.texture_matrix_mask&~u8(1))))return fail("unsupported material/texture-matrix stages");
            if(mesh.generated_uv_required){
                const bool reflection=mesh.texture_coordinate_mode==1&&mesh.reflection_uv_supported;
                const bool body_highlight=mesh.texture_coordinate_mode==2&&
                    (body_preview||material==HsdStaticMaterialPolicy::ImmutableHighlight);
                if((!reflection&&!body_highlight)||mesh.texture==kHsdNoTexture)return fail("unsupported generated coordinates");
                for(const auto& vertex:mesh.vertices){
                    if(mesh.texture_matrix_mask&1){if(vertex.texture_matrix_indices[0]!=vertex.matrix_index+30)return fail("generated UV matrix/position palette mismatch");}
                    else if(vertex.matrix_index!=0)return fail("nonuniform generated UV palette without vertex selector");
                }
            }else if(mesh.texture_matrix_required&&mesh.texture!=kHsdNoTexture)return fail("explicit texture matrix without qualified generator");
            selected.scene.meshes.push_back(mesh);selected.scene.mesh_joint.push_back(remap[owner]);selected.meshes.push_back(std::move(metadata));
            mesh_bones.push_back(bone);return true;
        };
        for(u32 m=0;m<full.meshes.size();++m){const auto owner=source.mesh_joint[m];if(source.joints[owner].model_root!=0)continue;
            if(!append_mesh(m,std::move(full.meshes[m]),u8(remap[owner])))return false;
        }
        std::vector<MountedJoint> mounted;
        for(const auto& mount:mounts){
            if(!bank||mount.record>=bank->records.size()||mount.bone>=n)return fail("static mount record/bone missing");
            const auto& record=bank->records[mount.record];
            if(record.joint_table||record.material_table||record.shape_table)return fail("animated mounted record");
            std::vector<HsdSkinJoint> joints;std::vector<HsdSkinMesh> meshes;
            if(!hsd_skin_metadata(source,bank->reader,&joints,&meshes,error,mount.record))return false;
            std::fill(remap.begin(),remap.end(),kHsdNoJoint);
            unsigned count=0;
            for(u32 j=0;j<source.joints.size();++j){
                auto joint=source.joints[j];
                if(joint.model_root!=mount.record)continue;
                u32 robj=0;
                if(++count>32||(joint.flags&0x00007e30u)||
                   !bank->reader.ptr(joint.archive_offset+0x3c,&robj)||robj)return fail("unqualified mounted joint");
                Mat4 local;
                if(count==1){
                    if(joint.parent!=kHsdNoJoint||joint.archive_offset!=record.root)return fail("mounted root identity");
                    const f32 offset[]{f32(mount.offset[0]),f32(mount.offset[1]),f32(mount.offset[2])};
                    local=mat4_translation(offset);joint.parent=mount.bone;
                }else{
                    if(joint.parent>=j||remap[joint.parent]==kHsdNoJoint)return fail("mounted hierarchy parent order");
                    joint.parent=remap[joint.parent];
                    local=mat4_from_srt(joint.rotation,joint.scale,joint.translation);
                }
                remap[j]=u32(selected.scene.joints.size());
                joint.model_root=0;selected.scene.joints.push_back(joint);selected.joints.push_back(joints[j]);
                mounted.push_back({joint.parent,local});
            }
            const auto before=selected.meshes.size();
            for(u32 m=0;m<meshes.size();++m){
                if(source.joints[source.mesh_joint[m]].model_root!=mount.record)continue;
                if(meshes[m].kind!=HsdSkinKind::Rigid)return fail("mounted record requires rigid joints");
                if(!append_mesh(m,std::move(meshes[m]),mount.bone))return false;
            }
            if(selected.meshes.size()==before)return fail("empty mounted record");
        }
        if(selected.meshes.empty())return fail("empty selected model");
        selected.loaded=true;
        HsdBoneMotionSample values;values.bones.resize(n);for(u32 j=0;j<n;++j)values.bones[j].joint_offset=bones.bones[j].joint_offset;
        std::array<CharacterPoseLane,4> lanes;for(unsigned i=0;i<4;++i)lanes[i].current=&values;
        CharacterLocalPose local;CharacterPoseFrame frame;CharacterWorldPose world;HsdSkinGeometry geometry;
        if(!character_pose_sample(rig,lanes,&local,error)||!character_pose_initial_frame(rig,Mat4{},&frame,error)||
           !character_pose_direct_world(rig,local,frame,&world,error)||!extend_world(mounted,&world.matrices)||
           !hsd_skin_sample(selected,world.matrices,&geometry,error))return false;
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
        scene_=std::move(renderer);skin_=std::move(selected);rig_=std::move(rig);
        mounted_joints_=std::move(mounted);mesh_bones_=std::move(mesh_bones);loaded_=true;error_.clear();return true;
    }
    static bool extend_world(std::span<const MountedJoint> mounts,std::vector<Mat4>* world){
        for(const auto& joint:mounts){
            if(joint.parent>=world->size())return false;
            const auto matrix=mat4_multiply((*world)[joint.parent],joint.local);
            if(!character_pose_detail::matrix(matrix))return false;
            world->push_back(matrix);
        }
        return true;
    }
public:
    bool bind_zero_reflections(std::span<const BorgZeroReflection> rows,std::string* error){
        if(!loaded_||rows.empty()||!borg_zero_reflections_valid(rows)||!zero_reflection_meshes_.empty()||ready_)
            return failure(error,"zero-reflection policy requires a fresh exact body");
        std::vector<unsigned> selected;
        for(const auto& policy:rows){
            std::optional<unsigned> found;
            for(unsigned m=0;m<skin_.scene.meshes.size();++m){
                const auto& mesh=skin_.scene.meshes[m];
                if(mesh.pobj_offset!=policy.pobj)continue;
                if(found||!mesh.generated_uv_required||!mesh.reflection_uv_supported||mesh.texture_coordinate_mode!=1||
                   std::count_if(mesh.vertices.begin(),mesh.vertices.end(),character_zero_normal)!=policy.count)
                    return failure(error,"zero-reflection mesh or authored normal count mismatch");
                found=m;
            }
            if(!found)return failure(error,"zero-reflection exact mesh not found");
            selected.push_back(*found);
        }
        zero_reflection_meshes_=std::move(selected);return true;
    }
    bool supports_texture_scroll(u8 bone,u8 axis)const{
        if(!loaded_||bone>=rig_.binding.bones.size()||axis>1)return false;
        for(unsigned m=0;m<skin_.scene.meshes.size();++m)if(scroll_mesh(m,bone,axis))return true;
        return false;
    }
    bool set_world_pose(const CharacterWorldPose& world,const CharacterModelRenderState& state,std::string* error,
                        std::span<const CharacterTextureScroll> scrolls={}){
        ready_=draw_prepared_=false;
        if(!loaded_||state.dynamic_material||state.attachments)return failure(error,"unloaded model or unported dynamic material/attachments");
        if(world.joint_offsets.size()!=rig_.binding.bones.size()||world.matrices.size()!=world.joint_offsets.size())return failure(error,"pose shape");
        for(unsigned i=0;i<world.joint_offsets.size();++i)if(world.joint_offsets[i]!=rig_.binding.bones[i].joint_offset)return failure(error,"pose identity");
        if(scrolls.size()>64)return failure(error,"texture scroll budget");
        std::array<u8,32> axes{};
        for(const auto& scroll:scrolls){
            if(!std::isfinite(scroll.offset)||std::fabs(scroll.offset)>1||!supports_texture_scroll(scroll.bone,scroll.axis)||
               (axes[scroll.bone]&(1u<<scroll.axis)))return failure(error,"invalid or unbound texture scroll");
            axes[scroll.bone]|=u8(1u<<scroll.axis);
        }
        std::span<const Mat4> matrices=world.matrices;
        std::vector<Mat4> extended;
        if(!mounted_joints_.empty()){
            extended.reserve(world.matrices.size()+mounted_joints_.size());
            extended.assign(world.matrices.begin(),world.matrices.end());
            if(!extend_world(mounted_joints_,&extended))return failure(error,"mounted world domain");
            matrices=extended;
        }
        HsdSkinGeometry evaluated;std::string why;if(!hsd_skin_sample(skin_,matrices,&evaluated,&why))return failure(error,why.c_str());
        if(ground_offset_!=0)
            for(auto& mesh:evaluated.world_vertices)for(auto& vertex:mesh)vertex.position[1]-=ground_offset_;
        for(const auto& scroll:scrolls)for(unsigned m=0;m<evaluated.world_vertices.size();++m)
            if(scroll_mesh(m,scroll.bone,scroll.axis))for(auto& vertex:evaluated.world_vertices[m])
                (skin_.scene.meshes[m].texture_uv1?vertex.uv1:vertex.uv)[scroll.axis]+=scroll.offset;
        geometry_=std::move(evaluated);visibility_.resize(skin_.meshes.size());origins_.resize(skin_.meshes.size());
        for(unsigned m=0;m<origins_.size();++m)for(unsigned k=0;k<3;++k)origins_[m][k]=matrices[skin_.scene.mesh_joint[m]].at(k,3);
        for(unsigned m=0;m<visibility_.size();++m)visibility_[m]=(state.visible_bones&(u32(1)<<mesh_bones_[m]))!=0;
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
    // Presentation tally: how many skinned meshes resolve to a decoded texture,
    // how many are authored untextured stages, and how many name an index the
    // scene does not carry. Diagnostic only; it never changes a draw.
    std::array<unsigned,3> texture_tally()const{
        std::array<unsigned,3> tally{};
        if(!ready_)return tally;
        for(const auto& mesh:skin_.scene.meshes){
            if(mesh.texture==kHsdNoTexture)++tally[1];
            else if(mesh.texture>=skin_.scene.textures.size())++tally[2];
            else ++tally[0];
        }
        return tally;
    }
    unsigned draw(RenderEngine& render,const Camera& camera){
        if(!ready_)return 0;
        Camera::Basis basis;if(!camera.basis(&basis))return 0;
        const auto key=camera.cache_key();
        // Several world passes visit the same evaluated actor. Reflection and
        // identity-geometry publication depend on pose/view, not pass number.
        if(!draw_prepared_||key!=draw_camera_){
            draw_prepared_=false;auto vertices=geometry_.world_vertices;
            for(unsigned m=0;m<vertices.size();++m)if(skin_.scene.meshes[m].generated_uv_required){
                const auto& mesh=skin_.scene.meshes[m];
                if(mesh.texture_coordinate_mode==2){
                    // Body inspection does not bind a source HSD light list.
                    // GG4E08 8023EE44 loads STQ=(1,1,0) from 803ABD24.
                    // GX halves S and T for Q=0, independent of the input.
                    for(auto& vertex:vertices[m])vertex.uv[0]=vertex.uv[1]=.5f;
                }else if(repair_walk_normals_){
                    if(!character_walk_reflection_uv(mesh,vertices[m],basis)){
                        failure(nullptr,"invalid provisional walk reflection geometry");return 0;
                    }
                }else{
                    const bool neutral=std::find(zero_reflection_meshes_.begin(),zero_reflection_meshes_.end(),m)!=zero_reflection_meshes_.end();
                    for(unsigned i=0;i<vertices[m].size();++i){
                        auto& vertex=vertices[m][i];
                        if(neutral&&character_zero_normal(mesh.vertices[i])&&character_zero_normal(vertex)){
                            // Native convention for no authored normal direction.
                            // The underlying normal and all nonzero UV paths stay intact.
                            vertex.uv[0]=vertex.uv[1]=.5f;
                        }else if(!view_normal_reflection_uv(vertex.normal,basis,vertex.uv)){
                            failure(nullptr,"invalid reflection normal/camera");return 0;
                        }
                    }
                }
            }
            if(!scene_.set_pretransformed_geometry_with_origins(vertices,visibility_,origins_)){failure(nullptr,"evaluated identity-geometry binding");return 0;}
            draw_camera_=key;draw_prepared_=true;
        }
        error_.clear();return scene_.draw(render,camera);
    }
    const CharacterPoseRig& rig()const{return rig_;}
    // Authored presentation shift applied to every sampled vertex, so bounds and
    // the draw agree. It never moves the actor, only the drawn body.
    void set_ground_offset(f32 offset){ground_offset_=offset;}
    std::shared_ptr<const CharacterModelBinding> instance_binding()const{return binding_;}
    void discard_pose(){ready_=draw_prepared_=false;}
    const HsdSkinAsset& asset()const{return skin_;}
    const HsdSkinGeometry& world_geometry()const{return geometry_;}
    bool bounds(std::array<f32,3>* low,std::array<f32,3>* high)const{
        if(!ready_||!low||!high)return false;
        bool any=false;
        for(unsigned m=0;m<geometry_.world_vertices.size();++m)if(visibility_[m])
            for(const auto& vertex:geometry_.world_vertices[m]){
                for(unsigned k=0;k<3;++k){
                    (*low)[k]=any?std::min((*low)[k],vertex.position[k]):vertex.position[k];
                    (*high)[k]=any?std::max((*high)[k],vertex.position[k]):vertex.position[k];
                }
                any=true;
            }
        return any;
    }
    const std::string& error()const{return error_;}
    bool ready()const{return ready_;}
private:
    f32 ground_offset_=0;
    bool scroll_mesh(unsigned m,u8 bone,u8 axis)const{
        const auto& mesh=skin_.scene.meshes[m];
        if(skin_.scene.mesh_joint[m]!=bone||!mesh.identity_texture_srt||mesh.generated_uv_required||mesh.texture_matrix_required||mesh.texture_matrix_mask||
           mesh.texture_stage_inactive||mesh.texture>=skin_.scene.textures.size()||
           !(mesh.explicit_uv_axes[mesh.texture_uv1]&(1u<<axis)))return false;
        const auto& texture=skin_.scene.textures[mesh.texture];
        return (axis==0?texture.wrap_s:texture.wrap_t)==1; // one full cycle of GX_REPEAT, not clamp/mirror
    }
    bool failure(std::string* error,const char* why){error_=std::string("character model scene: ")+why;if(error)*error=error_;return false;}
    HsdSkinAsset skin_;CharacterPoseRig rig_;MenuScene scene_;HsdSkinGeometry geometry_;std::vector<bool> visibility_;
    std::vector<MountedJoint> mounted_joints_;std::vector<u8> mesh_bones_;
    std::vector<std::array<f32,3>> origins_;
    bool loaded_=false,ready_=false;std::string error_;
    bool draw_prepared_=false,repair_walk_normals_=false;std::array<u32,17> draw_camera_{};
    std::vector<unsigned> zero_reflection_meshes_;
    std::shared_ptr<const CharacterModelBinding> binding_;
};
} // namespace gf
