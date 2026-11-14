#pragma once
// Decoded HSD room renderer. Authored vertices/pose remain immutable; evaluated
// pose, world transforms, visibility and camera-facing matrices are separate.
// The HSD material subset is explicit (including PE and default combiners).
#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>
#include "engine/assets/hsd.hpp"
#include "engine/assets/hsd_anim.hpp"
#include "engine/graphics/projected_triangle.hpp"
#include "engine/graphics/pointer_triangles.hpp"
#include "engine/graphics/render_engine.hpp"
#include "engine/graphics/scene_draw.hpp"
#include "engine/graphics/reflection.hpp"
#include "engine/graphics/transform.hpp"
#include "engine/graphics/frustum_bounds.hpp"
#include "engine/gameplay/menu_pointer_geometry.hpp"
#include "engine/gameplay/menu_motion.hpp"

namespace gf {
class MenuScene {
public:
    bool load(ByteView archive, std::string* error) {
        HsdScene scene;
        if (!hsd_load(archive,&scene,error)) { loaded_=false; scene_={}; return false; }
        return load_scene(std::move(scene),error);
    }
    // Also used by owned fixtures, without proprietary assets.
    bool load_scene(HsdScene scene, std::string* error) {
        pointer_enabled_=pointer_free_mode_=selector_pointer_=false;pointer_geometry_.clear();
        loaded_=false; active_pose_=false; handles_.clear();post_rgb_add_={};post_rgb_multiply_={255,255,255};evaluated_origins_.clear();lighting_cache_valid_=false;menu_art_filter_=0;menu_root_filter_=0;fog_={};fog_valid_=true;
        if (scene.meshes.size()!=scene.mesh_joint.size()) return fail(error,"mesh bindings differ");
        for(const auto& t:scene.textures)if(t.shared_rgba&&(!t.rgba.empty()||u64(t.width)*t.height*4!=t.pixels().size()))return fail(error,"immutable image payload differs");
        if(!scene.source_texture_slots.empty()){
            if(scene.source_texture_slots.size()!=scene.textures.size())return fail(error,"compact texture slots differ");
            std::vector<u32> seen;for(auto slot:scene.source_texture_slots){if(slot>=kHsdMaxNodes||std::find(seen.begin(),seen.end(),slot)!=seen.end())return fail(error,"invalid/duplicate compact texture slot");seen.push_back(slot);}
        }
        if(scene.lighting.declared && !menu_light_rig_supported(scene.lighting))return fail(error,"unsupported light rig (ambient/infinite, <=8 hardware slots)");
        if(scene.menu_motion.present){
            const auto step_ok=[](s32 step){return step>=-32768&&step<=32767;};
            if(scene.menu_motion.bindings.size()!=7||!std::isfinite(scene.menu_motion.angle_unit)||
               std::fabs(scene.menu_motion.angle_unit*65536.0f-6.28318530718f)>0.001f||
               !step_ok(scene.menu_motion.selector_step)||!step_ok(scene.menu_motion.identifier_step))
                return fail(error,"invalid menu motion configuration");
            std::vector<u32> items,roots;
            roots.push_back(scene.menu_motion.selector_joint);
            for(const auto& b:scene.menu_motion.bindings){
                if(b.item>=7||!step_ok(b.step)||std::find(items.begin(),items.end(),b.item)!=items.end())return fail(error,"invalid/duplicate motion item");
                items.push_back(b.item);
                for(u32 root:{b.gear,b.identifier,b.caption}){
                    if(std::find(roots.begin(),roots.end(),root)!=roots.end())return fail(error,"reused menu motion root");
                    roots.push_back(root);
                }
            }
            for(u32 root:roots){
                unsigned count=0;
                for(const auto& j:scene.joints)if(j.archive_offset==root&&j.parent==kHsdNoJoint)++count;
                if(count!=1)return fail(error,"missing/ambiguous menu motion root");
            }
        }
        for(std::size_t i=0;i<scene.joints.size();++i)
            if(scene.joints[i].parent!=kHsdNoJoint && scene.joints[i].parent>=i)
                return fail(error,"parent must precede child");
        for(std::size_t i=0;i<scene.meshes.size();++i) {
            if(!scene.meshes[i].material_supported) return fail(error,"unsupported multiple/custom material stages");
            if(!std::isfinite(scene.meshes[i].shininess)||scene.meshes[i].shininess<0)
                return fail(error,"invalid material shininess");
            if(scene.meshes[i].deformation_required) return fail(error,"deformation requires skin/shape evaluation");
            const auto& mesh=scene.meshes[i];
            if(mesh.generated_uv_required){
                if(mesh.texture_coordinate_mode!=1||!mesh.reflection_uv_supported||mesh.texture==kHsdNoTexture||(mesh.texture_matrix_mask&~u8(1)))
                    return fail(error,"unsupported static texture generator");
                for(const auto& vertex:mesh.vertices)if(vertex.matrix_index!=0||((mesh.texture_matrix_mask&1)&&vertex.texture_matrix_indices[0]!=30))
                    return fail(error,"static reflection requires one owner-normal matrix");
            }else if(mesh.texture_matrix_required)return fail(error,"texture matrix requires explicit evaluation");
            if(scene.mesh_joint[i]>=scene.joints.size()) return fail(error,"invalid mesh joint");
            for(u32 v:scene.meshes[i].indices) if(v>=scene.meshes[i].vertices.size()) return fail(error,"invalid index");
            if(((scene.joints[scene.mesh_joint[i]].flags>>9)&7)>4) return fail(error,"unknown billboard mode");
        }
        scene_=std::move(scene);source_.clear();
        motion_state_={};motion_dirty_=true;focus_item_=7;show_selector_=true;
        selection_=scene_.menu_selection;
        bounds_.clear();
        for(const auto& mesh:scene_.meshes){source_.push_back(mesh.vertices);Bounds3 bounds;for(const auto& v:mesh.vertices)bounds.include(v.position);bounds_.push_back(bounds);}
        world_.resize(scene_.joints.size());hidden_.resize(scene_.joints.size());
        motion_affected_.assign(scene_.joints.size(),false);
        identifier_item_.assign(scene_.joints.size(),7);caption_item_.assign(scene_.joints.size(),7);
        menu_item_.assign(scene_.joints.size(),7);
        caption_valid_.assign(scene_.joints.size(),true);
        selector_joint_.assign(scene_.joints.size(),false);
        for(std::size_t j=0;j<scene_.joints.size();++j){
            const auto& joint=scene_.joints[j];
            if(joint.parent!=kHsdNoJoint){motion_affected_[j]=motion_affected_[joint.parent];
                identifier_item_[j]=identifier_item_[joint.parent];caption_item_[j]=caption_item_[joint.parent];selector_joint_[j]=selector_joint_[joint.parent];}
            if(joint.parent!=kHsdNoJoint)menu_item_[j]=menu_item_[joint.parent];
            for(const auto& b:scene_.menu_motion.bindings){
                if(joint.archive_offset==b.gear||joint.archive_offset==b.identifier)motion_affected_[j]=true;
                if(joint.archive_offset==b.identifier)identifier_item_[j]=b.item;
                if(joint.archive_offset==b.caption)caption_item_[j]=b.item;
                if(joint.archive_offset==b.gear||joint.archive_offset==b.identifier||joint.archive_offset==b.caption)menu_item_[j]=b.item;
            }
            if(joint.archive_offset==scene_.menu_motion.selector_joint&&scene_.menu_motion.present)selector_joint_[j]=true;
        }
        loaded_=true;compose();error_.clear();return true;
    }
    bool loaded() const {return loaded_;}
    const HsdScene& scene() const {return scene_;}
    const std::string& error() const {return error_;}
    const HsdAnimPose* pose() const {return active_pose_?&pose_:nullptr;}
    // Copy the pose: a caller may release/re-evaluate its buffer after this call.
    bool set_pose(const HsdAnimPose* pose) {
        if(pose) {
            if(pose->joints.size()!=scene_.joints.size()) return false;
            for(const auto& joint:pose->joints) for(unsigned k=0;k<3;++k)
                if(!std::isfinite(joint.rotation[k])||!std::isfinite(joint.scale[k])||!std::isfinite(joint.translation[k])) return false;
            for(const auto& joint:pose->joints)if(joint.matrix_override)
                for(f32 value:joint.matrix)if(!std::isfinite(value)||std::fabs(value)>1e7f)return false;
            if(!pose->materials.empty()&&pose->materials.size()!=scene_.meshes.size())return false;
            for(const auto& material:pose->materials)if(material.present){
                auto unit=[](f32 v){return std::isfinite(v)&&v>=0&&v<=1;};
                if(!unit(material.alpha))return false;
                for(unsigned k=0;k<3;++k)if(!unit(material.ambient[k])||!unit(material.diffuse[k])||
                    (material.has_specular&&!unit(material.specular[k])))return false;
                if(material.has_texture){
                    if(!unit(material.texture_weight))return false;
                    for(unsigned k=0;k<2;++k)if(!std::isfinite(material.uv_scale[k])||!std::isfinite(material.uv_offset[k])||
                        std::fabs(material.uv_scale[k])>1e7f||std::fabs(material.uv_offset[k])>1e7f)return false;
                }
            }
            for(unsigned i=0;i<pose->materials.size();++i)if(pose->materials[i].has_texture_colours&&(!pose->materials[i].present||!pose->materials[i].has_texture||!scene_.meshes[i].texture_colour_lerp))return false;
            pose_=*pose;active_pose_=true;
        } else {active_pose_=false;pose_={};}
        if(loaded_) compose();
        return true;
    }
    void set_cull_backface(bool enabled){cull_=enabled;}
    void set_post_rgb_add(std::array<u8,3> rgb){post_rgb_add_=rgb;}
    void set_post_rgb_multiply(std::array<u8,3> rgb){post_rgb_multiply_=rgb;}
    bool set_linear_fog(const LinearFog& fog){fog_valid_=linear_fog_valid(fog);if(!fog_valid_)return false;fog_=fog;return true;}
    void set_only_mesh(long index){only_mesh_=index;pointer_geometry_.clear();}
    void set_show_source_markers(bool enabled){show_markers_=enabled;}
    void set_selection(unsigned item){selection_=item;}
    bool enable_menu_pointer(bool enabled,bool while_selector_hidden=false){pointer_geometry_.clear();pointer_free_mode_=while_selector_hidden;pointer_enabled_=enabled&&loaded_&&scene_.menu_motion.present&&!active_pose_;return !enabled||pointer_enabled_;}
    bool menu_pointer_hit(f32 x,f32 y,unsigned* item)const{if(item)*item=7;return pointer_enabled_&&pointer_geometry_.hit(x,y,item);}
    bool menu_pointer_near(f32 x,f32 y,f32 distance,unsigned* item,int only=-1)const{if(item)*item=7;return pointer_enabled_&&pointer_geometry_.near(x,y,distance,item,only);}
    bool pointer_selector_available()const{if(!loaded_||!motion_enabled())return false;for(bool selector:selector_joint_)if(selector)return true;return false;}
    bool selector_ring_dimensions(f32* diameter,f32* band)const{
        if(diameter)*diameter=0;
        if(band)*band=0;
        if(!diameter||!band||!pointer_selector_available())return false;
        f32 inner=1e7f,outer=0;for(std::size_t m=0;m<scene_.meshes.size();++m)if(selector_joint_[scene_.mesh_joint[m]])for(const auto& v:source_[m]){const f32 r=std::hypot(v.position[0],v.position[1]);if(!std::isfinite(r)||r<=0)return false;inner=std::min(inner,r);outer=std::max(outer,r);}
        if(inner>=outer)return false;
        *diameter=outer*2;*band=outer-inner;return true;
    }
    bool selector_target(const Camera& camera,unsigned item,f32* x,f32* y,f32* diameter)const{
        if(!x||!y||!diameter||item>=7)return false;
        f32 band=0;if(!selector_ring_dimensions(diameter,&band))return false;
        for(const auto& binding:scene_.menu_motion.bindings){if(binding.item!=item)continue;for(const auto& joint:scene_.joints)if(joint.archive_offset==binding.gear)return camera.project(joint.translation,x,y,nullptr);}
        return false;
    }
    //MAIN keeps the original selector-before-caption ordering. Other UI screens
    //may use draw_pointer_selector as a final overlay above their modals.
    bool set_pointer_selector(bool enabled,f32 x=0,f32 y=0,f32 diameter=48){
        selector_pointer_=false;
        if(!enabled)return true;
        f32 normal=0,band=0;
        if(!selector_ring_dimensions(&normal,&band)||!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(diameter)||x<0||y<0||x>=640||y>=480||diameter*.5f-band<1||diameter>normal)return false;
        selector_pointer_=true;selector_x_=x;selector_y_=y;selector_offset_=(diameter-normal)*.5f;return true;
    }
    //Change annulus diameter by an equal radial offset, NOT uniform scaling:
    //source band/stroke thickness, UVs/materials and source rotation are retained.
    unsigned draw_pointer_selector(RenderEngine& render,const Camera& camera,f32 x,f32 y,f32 diameter=48){
        f32 normal=0,band=0;if(!selector_ring_dimensions(&normal,&band)||render.model_pass()||!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(diameter)||x<0||y<0||x>=camera.viewport_width||y>=camera.viewport_height||diameter*.5f-band<1||diameter>normal)return 0;
        const auto saved=render.state();Mat4 identity;if(!render.upload_matrix(0,identity.m))return 0;const auto count=draw_selector_at(render,camera,x,y,(diameter-normal)*.5f);render.state()=saved;return count;
    }
    // Recovered menu mode-2 presentation: keep only the active option's models,
    // and suppress the screen-space selector while its camera transition runs.
    void set_menu_focus(unsigned item,bool show_selector){if(focus_item_!=(item<7?item:7)||show_selector_!=show_selector)pointer_geometry_.clear();focus_item_=item<7?item:7;show_selector_=show_selector;}
    void set_interaction(bool enabled){if(interaction_!=enabled){interaction_=enabled;motion_dirty_=true;cache_valid_=false;pointer_geometry_.clear();}}
    // Evaluated-model adapter only: the scene must already be a single static
    // identity joint with no menu motion/pose. This does NOT admit raw skin or
    // generated coordinates through load_scene; its existing gates stay intact.
    bool set_pretransformed_geometry(const std::vector<std::vector<HsdVertex>>& vertices,const std::vector<bool>& visible){
        if(!loaded_||active_pose_||interaction_||scene_.menu_motion.present||!scene_.menu_seals.empty()||scene_.joints.size()!=1||
           vertices.size()!=source_.size()||visible.size()!=source_.size())return false;
        const auto& j=scene_.joints[0];
        if(j.parent!=kHsdNoJoint)return false;
        for(unsigned k=0;k<3;++k)if(j.rotation[k]!=0||j.translation[k]!=0||j.scale[k]!=1)return false;
        std::vector<Bounds3> bounds(vertices.size());
        for(unsigned m=0;m<vertices.size();++m){
            if(vertices[m].size()!=source_[m].size())return false;
            for(const auto& v:vertices[m]){
                for(unsigned k=0;k<3;++k)if(!std::isfinite(v.position[k])||!std::isfinite(v.normal[k])||std::fabs(v.position[k])>1e7f||std::fabs(v.normal[k])>1e7f)return false;
                for(unsigned k=0;k<2;++k)if(!std::isfinite(v.uv[k])||!std::isfinite(v.uv1[k])||std::fabs(v.uv[k])>1e7f||std::fabs(v.uv1[k])>1e7f)return false;
                bounds[m].include(v.position);
            }
        }
        source_=vertices;bounds_=std::move(bounds);evaluated_origins_.clear();for(unsigned m=0;m<visible.size();++m)scene_.meshes[m].hidden=!visible[m];compose();return true;
    }
    // Evaluated identity copies still need the original joint origins for
    // scene-global specular lighting and deferred joint-depth ordering.
    bool set_pretransformed_geometry_with_origins(const std::vector<std::vector<HsdVertex>>& vertices,const std::vector<bool>& visible,const std::vector<std::array<f32,3>>& origins){
        if(origins.size()!=source_.size())return false;
        for(const auto& origin:origins)for(f32 v:origin)if(!std::isfinite(v)||std::abs(v)>1e7f)return false;
        if(!set_pretransformed_geometry(vertices,visible))return false;
        evaluated_origins_=origins;return true;
    }
    // Evaluated identity-draw-copy material update; does not admit raw models,
    // skin, hierarchy, animation or interaction through a weaker loading path.
    bool set_evaluated_material_alpha(const std::vector<f32>& alpha){
        if(!loaded_||active_pose_||interaction_||scene_.menu_motion.present||!scene_.menu_seals.empty()||scene_.joints.size()!=1||alpha.size()!=scene_.meshes.size())return false;
        const auto& j=scene_.joints[0];if(j.parent!=kHsdNoJoint)return false;
        for(unsigned k=0;k<3;++k)if(j.rotation[k]!=0||j.translation[k]!=0||j.scale[k]!=1)return false;
        for(f32 value:alpha)if(!std::isfinite(value)||value<0||value>1)return false;
        for(unsigned i=0;i<alpha.size();++i)scene_.meshes[i].material_alpha=alpha[i];
        cache_valid_=false;return true;
    }
    void advance_motion(){if(motion_enabled()){menu_motion_step(scene_.menu_motion,selection_,motion_state_);motion_dirty_=true;}}
    const MenuMotionState& motion_state()const{return motion_state_;}
    void set_depth_test(bool enabled){depth_=enabled;}
    // Source material compare override (80009690). Keep PE writes/masks and
    // every other material field; this is not the diagnostic depth-disable path.
    bool set_depth_compare(u8 comparison){if(comparison>7)return false;depth_compare_=comparison;return true;}
    void clear_depth_compare(){depth_compare_=0xff;}
    void set_alpha_test(bool enabled,u8 = 0){alpha_=enabled;}
    unsigned upload_textures(RenderEngine& render,u32 base=1) {
        handles_.assign(scene_.textures.size(),0);unsigned n=0;
        for(std::size_t i=0;i<scene_.textures.size();++i) {
            const auto& t=scene_.textures[i];
            const u32 h=base+static_cast<u32>(i);
            if(render.upload_texture(h,{t.width,t.height,t.pixels().data(),t.wrap_s,t.wrap_t,t.linear})) {handles_[i]=h;++n;}
        }
        return n;
    }
    unsigned upload_textures_shared(RenderEngine& render,u32 base){
        handles_.assign(scene_.textures.size(),0);unsigned n=0;for(unsigned i=0;i<scene_.textures.size();++i){const auto& t=scene_.textures[i];const u32 slot=scene_.source_texture_slots.empty()?i:scene_.source_texture_slots[i];const u32 id=slot>~u32(0)-base?0:base+slot;if(render.upload_texture_shared(id,{t.width,t.height,t.pixels().data(),t.wrap_s,t.wrap_t,t.linear})){handles_[i]=id;++n;}}return n;
    }
    // Existing source-qualified menu roots only; does not select arbitrary mesh
    // fragments or change legacy full-room rendering. World roots enter C440;
    // selector and caption remain separate later source-policy layers.
    unsigned draw_menu_world_model(RenderEngine& render,const Camera& camera,unsigned item,unsigned part){
        if(!motion_enabled()||item>=7||part>1)return 0;
        u32 root=0;for(const auto& b:scene_.menu_motion.bindings)if(b.item==item)root=part?b.identifier:b.gear;if(!root)return 0;
        menu_art_filter_=1;menu_root_filter_=root;const auto n=draw(render,camera);menu_art_filter_=0;menu_root_filter_=0;return n;
    }
    unsigned draw_menu_selector(RenderEngine& render,const Camera& camera){if(!motion_enabled())return 0;const auto saved=render.state();const auto n=draw_selector(render,camera);render.state()=saved;return n;}
    unsigned draw_menu_caption(RenderEngine& render,const Camera& camera){if(!motion_enabled())return 0;menu_art_filter_=2;const auto n=draw(render,camera);menu_art_filter_=0;return n;}
    bool bounds(f32 lo[3],f32 hi[3]) const {
        if(!loaded_||scene_.meshes.empty()) return false;
        for(unsigned k=0;k<3;++k){lo[k]=1e30f;hi[k]=-1e30f;}
        for(const auto& mesh:scene_.meshes) for(const auto& v:mesh.vertices) for(unsigned k=0;k<3;++k) {
            lo[k]=std::min(lo[k],v.position[k]);hi[k]=std::max(hi[k],v.position[k]);
        }
        return true;
    }
    bool focus(f32 centre[3],f32* radius) const {
        if(!loaded_) return false;
        std::vector<f32> distances;f64 sum[3]={};std::size_t n=0;
        for(std::size_t m=0;m<scene_.meshes.size();++m) if(!hidden_[scene_.mesh_joint[m]])
            for(const auto& v:scene_.meshes[m].vertices){++n;for(unsigned k=0;k<3;++k)sum[k]+=v.position[k];}
        if(!n)return false;
        for(unsigned k=0;k<3;++k)centre[k]=static_cast<f32>(sum[k]/n);
        for(std::size_t m=0;m<scene_.meshes.size();++m)if(!hidden_[scene_.mesh_joint[m]])
            for(const auto& v:scene_.meshes[m].vertices){f32 d=0;for(unsigned k=0;k<3;++k)d+=(v.position[k]-centre[k])*(v.position[k]-centre[k]);distances.push_back(std::sqrt(d));}
        std::sort(distances.begin(),distances.end());if(radius)*radius=distances[distances.size()/2];return true;
    }
    unsigned draw(RenderEngine& render,const Camera& camera,PointerTriangles* submitted=nullptr) {
        pointer_geometry_.clear();
        Camera::Basis basis;
        if(!loaded_||!render.scene_context_valid()||!camera.basis(&basis))return 0;
        const auto& lighting=render.scene_lighting()?*render.scene_lighting():scene_.lighting;
        const bool global_lighting=render.scene_lighting()!=nullptr;
        if(!lighting_cache_valid_||global_lighting!=lighting_cache_global_||!scene_lighting_equal(lighting,lighting_cache_)){lighting_cache_=lighting;lighting_cache_global_=global_lighting;lighting_cache_valid_=true;cache_valid_=false;}
        const u8 pass=render.model_pass();
        const bool pointer_frame=pointer_enabled_&&motion_enabled()&&!pass&&focus_item_==7&&(show_selector_||pointer_free_mode_)&&only_mesh_<0&&!menu_art_filter_;
        if(pointer_frame)pointer_geometry_.begin(camera.viewport_width,camera.viewport_height);
        if(pass)for(const auto& mesh:scene_.meshes)if((mesh.render_mode&0x60000000u)==0x20000000u){render.reject_model_pass();fail(nullptr,"unsupported source material pass classification");return 0;}
        std::array<u32,14> camera_key{};unsigned key_index=0;
        for(const f32* vector:{camera.eye,camera.target,camera.up})for(unsigned k=0;k<3;++k)camera_key[key_index++]=f32_to_bits(vector[k]);
        camera_key[key_index++]=f32_to_bits(camera.fov_y_degrees);camera_key[key_index++]=f32_to_bits(camera.near_plane);camera_key[key_index++]=f32_to_bits(camera.far_plane);
        camera_key[key_index++]=static_cast<u32>(camera.viewport_width);camera_key[key_index++]=static_cast<u32>(camera.viewport_height);
        if(!cache_valid_||camera_key!=camera_key_){
            camera_key_=camera_key;cache_valid_=true;
            projected_.resize(scene_.meshes.size());
            for(auto& vertices:projected_)vertices.clear();
            projected_ready_.assign(scene_.meshes.size(),false);
            motion_dirty_=true; // caption basis depends on the camera, not just motion ticks
        }
        update_motion_world(camera);
        if(!fog_valid_){render.reject_model_pass();return 0;}
        const RenderState saved=render.state();
        struct Draw {u32 base,count;RenderState state;f32 depth;std::size_t mesh;unsigned item;};
        std::vector<Draw> draws,captions;
        static constexpr f32 identity[12]={1,0,0,0,0,1,0,0,0,0,1,0};
        render.upload_matrix(0,identity);
        for(std::size_t m=0;m<scene_.meshes.size();++m) {
            if(only_mesh_>=0 && std::size_t(only_mesh_)!=m)continue;
            const HsdMesh& source_mesh=scene_.meshes[m];const u32 j=scene_.mesh_joint[m];
            if(menu_art_filter_==1){u32 root=j;while(scene_.joints[root].parent!=kHsdNoJoint)root=scene_.joints[root].parent;if(scene_.joints[root].archive_offset!=menu_root_filter_)continue;}
            if(menu_art_filter_==2&&caption_item_[j]>=7)continue;
            if(pass){const u32 kind=source_mesh.render_mode&0x60000000u;const u8 category=kind==0?1:kind==0x40000000u?4:2;if(category!=pass)continue;}
            if(source_mesh.hidden||hidden_[j])continue;
            bool inactive_caption=false;
            if(motion_enabled()){
                if(selector_joint_[j])continue;
                if(focus_item_<7&&menu_item_[j]<7&&menu_item_[j]!=focus_item_)continue;
                if(identifier_item_[j]==selection_)continue; // selected static caption replaces spinning identifier
                if(caption_item_[j]<7&&caption_item_[j]!=selection_)continue;
                if(caption_item_[j]<7&&!caption_valid_[j])continue;
            } else if(interaction_)for(const auto& seal:scene_.menu_seals)if(source_mesh.dobj_offset==seal.caption_dobj && seal.item!=selection_)inactive_caption=true;
            if(inactive_caption)continue;
            // Most evaluated models already carry their material state. Avoid
            // deep-copying vertex/index arrays on every visit to every pass.
            HsdMeshProperties posed_mesh=source_mesh;
            const HsdMeshProperties& mesh=posed_mesh;
            if(active_pose_ && m<pose_.materials.size() && pose_.materials[m].present){
                const auto& material=pose_.materials[m];posed_mesh.material_alpha=material.alpha;
                for(unsigned k=0;k<3;++k){posed_mesh.material_ambient[k]=static_cast<u8>(material.ambient[k]*255+0.5f);posed_mesh.material_colour[k]=static_cast<u8>(material.diffuse[k]*255+0.5f);
                    if(material.has_specular)posed_mesh.material_specular[k]=static_cast<u8>(material.specular[k]*255+0.5f);}
                if(material.has_texture)posed_mesh.texture_weight=material.texture_weight;
                if(material.has_texture_colours){posed_mesh.texture_low_colour=material.texture_low_colour;posed_mesh.texture_high_colour=material.texture_high_colour;}
            }
            if(hidden_[j])continue; // NODE hides this node's geometry, not the children
            const u32 mode=(scene_.joints[j].flags & 0xE00u)>>9;
            if(!show_markers_ && mode && mesh.texture==kHsdNoTexture)continue;
            Mat4 matrix=mode?facing(j,mode,basis,camera):world_[j];
            // Raw generated UVs can still refuse an invalid transformed normal.
            // Keep that validation even off-screen; evaluated model copies have
            // already validated their generators before reaching this path.
            if(!mesh.generated_uv_required&&!projected_ready_[m]&&!frustum_bounds_visible(bounds_[m],matrix,camera,basis)){
                projected_[m].clear();projected_ready_[m]=true;continue;
            }
            f32 origin[3]={matrix.at(0,3),matrix.at(1,3),matrix.at(2,3)};
            if((pass||global_lighting)&&evaluated_origins_.size()==scene_.meshes.size())std::copy_n(evaluated_origins_[m].data(),3,origin);
            auto& vertices=projected_[m];
            if(!projected_ready_[m])vertices.clear();
            if(!projected_ready_[m]){
                // Indexed corners repeatedly reference the same source vertex.
                // Transform/light each referenced vertex once per mesh/view.
                shaded_.resize(source_[m].size());
                shaded_ready_.assign(source_[m].size(),false);
                const auto normal_transform=mat4_normal_transform(matrix);
                if(lighting.declared)prepared_lights_.prepare(lighting,origin,camera.eye);
                for(std::size_t i=0;i+2<source_mesh.indices.size();i+=3) {
                  Vertex triangle[3];
                  for(unsigned k=0;k<3;++k){
                    const auto index=source_mesh.indices[i+k];
                    if(!shaded_ready_[index]){
                    const auto& src=source_[m][index];
                    f32 p[3],normal[3];mat4_transform_point(matrix,src.position,p);mat4_transform_normal(normal_transform,src.normal,normal);
                    auto& v=shaded_[index];v=shade(src,normal,mesh,p,origin,camera,lighting,&prepared_lights_);
                    if(mesh.generated_uv_required){
                        f32 uv[2];if(!view_normal_reflection_uv(normal,basis,uv)){cache_valid_=false;fail(nullptr,"invalid static reflection normal");return 0;}
                        v.u=uv[0];v.v=uv[1];
                    }
                    if(active_pose_&&m<pose_.materials.size()){
                        const auto& material=pose_.materials[m];
                        if(material.present&&material.has_texture){
                            v.u=v.u*material.uv_scale[0]+material.uv_offset[0];
                            v.v=v.v*material.uv_scale[1]+material.uv_offset[1];
                        }
                    }
                    v.x=p[0];v.y=p[1];v.z=p[2];shaded_ready_[index]=true;
                    }
                    triangle[k]=shaded_[index];
                  }
                  project_triangle(camera,basis,triangle,vertices);
                }
            }
            projected_ready_[m]=true;
            if(vertices.empty())continue;
            RenderState state=state_for(mesh);
            f32 d=0;for(unsigned k=0;k<3;++k)d+=(origin[k]-basis.eye[k])*basis.forward[k];
            Draw draw{render.append_vertices(vertices.data(),vertices.size()),static_cast<u32>(vertices.size()),state,d,m,menu_item_[j]};
            if(motion_enabled()&&caption_item_[j]==selection_){
                draw.state.depth_test=false;draw.state.depth_write=false;
                captions.push_back(draw);
            }else draws.push_back(draw);
        }
        // Preserve authored order within opaque work, and blend back-to-front.
        if(!pass)std::stable_sort(draws.begin(),draws.end(),[](const Draw& a,const Draw& b){
            if(a.state.blend!=b.state.blend)return !a.state.blend;
            return a.state.blend && a.depth>b.depth;
        });
        unsigned issued=0;bool pointer_ok=true;
        auto capture_pointer=[&](const Draw& draw){if(submitted)submitted->append(projected_[draw.mesh],draw.state.cull_backface,draw.state.cull_frontface);if(pointer_frame&&draw.item<7&&!pointer_geometry_.append(projected_[draw.mesh],draw.item,draw.state.cull_backface,draw.state.cull_frontface,!draw.state.depth_test))pointer_ok=false;};
        for(const auto& draw:draws){render.state()=draw.state;if(render.draw_model(Primitive::Triangles,draw.base,draw.count,draw.depth)){++issued;capture_pointer(draw);}else pointer_ok=false;}
        if(motion_enabled()&&show_selector_&&!menu_art_filter_)issued+=draw_selector(render,camera);
        // Foreground caption is above the screen-space selector. Keeping it in
        // the world pass let the gear graphic paint over the selected text.
        for(const auto& draw:captions){render.state()=draw.state;if(render.draw(Primitive::Triangles,draw.base,draw.count)){++issued;capture_pointer(draw);}else pointer_ok=false;}
        if(interaction_&&!motion_enabled())for(const auto& seal:scene_.menu_seals)if(seal.item==selection_){
            std::vector<Vertex> ring;std::size_t first=0,count=0;
            if(ScreenDraw::ring(camera,seal.centre,seal.radius*1.08f,seal.radius*0.09f,48,
                                Tint{255,80,25,255},ring,&first,&count)){
                render.state()=RenderState{};render.state().cull_backface=false;
                render.state().depth_test=depth_;render.state().depth_write=false;
                const u32 base=render.append_vertices(ring.data(),ring.size());
                if(render.draw(Primitive::Triangles,base,static_cast<u32>(ring.size())))++issued;
            }
        }
        if(pointer_frame)pointer_geometry_.finish(pointer_ok);
        render.state()=saved;return issued;
    }
private:
    bool fail(std::string* error,const char* message){error_=std::string("menu scene: ")+message;if(error)*error=error_;return false;}
    void compose(){
        pointer_geometry_.clear();
        cache_valid_=false;
        motion_dirty_=true;
        for(std::size_t i=0;i<scene_.joints.size();++i){const auto& j=scene_.joints[i];
            const Mat4 local=local_matrix(i);
            world_[i]=j.parent==kHsdNoJoint?local:mat4_multiply(world_[j.parent],local);
            hidden_[i]=active_pose_?pose_.joints[i].hidden:bool(j.flags & 0x10u);
        }
        for(std::size_t m=0;m<scene_.meshes.size();++m){scene_.meshes[m].vertices=source_[m];const auto& matrix=world_[scene_.mesh_joint[m]];const auto normals=mat4_normal_transform(matrix);
            for(auto& v:scene_.meshes[m].vertices){f32 p[3],n[3];mat4_transform_point(matrix,v.position,p);mat4_transform_normal(normals,v.normal,n);for(unsigned k=0;k<3;++k){v.position[k]=p[k];v.normal[k]=n[k];}}
        }
    }
    bool motion_enabled()const{return interaction_&&scene_.menu_motion.present&&!active_pose_;}
    Mat4 local_matrix(std::size_t j)const{
        if(active_pose_){const auto& p=pose_.joints[j];
            if(p.matrix_override){Mat4 m;for(unsigned k=0;k<12;++k)m.m[k]=p.matrix[k];return m;}
            return mat4_from_srt(p.rotation,p.scale,p.translation);
        }
        const auto& p=scene_.joints[j];return mat4_from_srt(p.rotation,p.scale,p.translation);
    }
    void update_motion_world(const Camera& camera){
        if(!motion_dirty_)return;
        motion_dirty_=false;
        for(std::size_t j=0;j<scene_.joints.size();++j){const auto& joint=scene_.joints[j];
            caption_valid_[j]=joint.parent==kHsdNoJoint?true:caption_valid_[joint.parent];
            Mat4 local=local_matrix(j);
            if(motion_enabled())for(const auto& b:scene_.menu_motion.bindings){
                if(joint.archive_offset==b.gear)local=menu_motion_matrix(joint,menu_phase_angle(motion_state_.gear_phase[b.item],scene_.menu_motion.angle_unit),0);
                if(joint.archive_offset==b.identifier)local=menu_motion_matrix(joint,0,motion_state_.identifier_y_phase[b.item]*scene_.menu_motion.angle_unit);
                if(joint.archive_offset==b.caption){
                    Mat4 caption;
                    caption_valid_[j]=menu_caption_matrix(joint.translation,camera.eye,&caption);
                    if(caption_valid_[j])local=caption;
                }
            }
            world_[j]=joint.parent==kHsdNoJoint?local:mat4_multiply(world_[joint.parent],local);
        }
        for(std::size_t m=0;m<scene_.meshes.size()&&m<projected_ready_.size();++m)
            if(motion_affected_[scene_.mesh_joint[m]])projected_ready_[m]=false;
    }
    RenderState state_for(const HsdMeshProperties& mesh)const{
        RenderState state{};
        state.texture=!mesh.texture_stage_inactive&&mesh.texture<handles_.size()?handles_[mesh.texture]:0;
        state.blend=mesh.blend;state.blend_src=mesh.blend_src;state.blend_dst=mesh.blend_dst;
        state.depth_test=depth_&&mesh.depth_test;state.depth_write=depth_&&mesh.depth_write;state.depth_compare=depth_compare_<=7?depth_compare_:mesh.depth_compare;
        state.colour_write=mesh.colour_write;state.alpha_write=mesh.alpha_write;
        state.cull_backface=cull_&&mesh.cull_backface;state.cull_frontface=cull_&&mesh.cull_frontface;
        state.alpha_test=alpha_;state.alpha_ref=mesh.alpha_ref0;state.alpha_ref1=mesh.alpha_ref1;
        state.alpha_compare0=mesh.alpha_compare0;state.alpha_compare1=mesh.alpha_compare1;state.alpha_op=mesh.alpha_op;
        state.texture_colour_mode=mesh.texture_colour_mode;state.texture_alpha_mode=mesh.texture_alpha_mode;state.texture_weight=mesh.texture_weight;
        state.texture_colour_lerp=mesh.texture_colour_lerp;state.texture_low_colour=mesh.texture_low_colour;state.texture_high_colour=mesh.texture_high_colour;
        state.texture_after_lighting=mesh.texture_after_lighting;state.post_rgb_add=post_rgb_add_;state.post_rgb_multiply=post_rgb_multiply_;state.fog=fog_;return state;
    }
    unsigned draw_selector(RenderEngine& render,const Camera& camera){
        if(selector_pointer_)return draw_selector_at(render,camera,selector_x_,selector_y_,selector_offset_);
        const HsdJoint* anchor=nullptr;
        for(const auto& b:scene_.menu_motion.bindings)if(b.item==selection_)
            for(const auto& j:scene_.joints)if(j.archive_offset==b.gear)anchor=&j;
        f32 x=0,y=0,q=0;if(!anchor||!camera.project(anchor->translation,&x,&y,&q))return 0;
        return draw_selector_at(render,camera,x,y,0);
    }
    unsigned draw_selector_at(RenderEngine& render,const Camera& camera,f32 x,f32 y,f32 radial_offset){
        const f32 angle=menu_phase_angle(motion_state_.selector_phase,scene_.menu_motion.angle_unit),c=std::cos(angle),s=std::sin(angle);
        unsigned issued=0;
        for(std::size_t m=0;m<scene_.meshes.size();++m)if(selector_joint_[scene_.mesh_joint[m]]){
            const auto& mesh=scene_.meshes[m];std::vector<Vertex> vertices;const f32 origin[3]={0,0,0};
            for(u32 index:mesh.indices){const auto& src=source_[m][index];
                auto v=shade(src,src.normal,mesh,src.position,origin,camera,scene_.lighting);
                f32 px=src.position[0],py=src.position[1];if(radial_offset!=0){const f32 radius=std::hypot(px,py),factor=(radius+radial_offset)/radius;px*=factor;py*=factor;}
                v.x=x+c*px-s*py;v.y=y-s*px-c*py;v.z=1;
                vertices.push_back(v);
            }
            render.state()=state_for(mesh);render.state().depth_test=false;render.state().depth_write=false;render.state().cull_backface=false;render.state().cull_frontface=false;
            const auto base=render.append_vertices(vertices.data(),vertices.size());
            if(render.draw(Primitive::Triangles,base,static_cast<u32>(vertices.size())))++issued;
        }
        return issued;
    }
    Mat4 facing(u32 j,u32 mode,const Camera::Basis& b,const Camera& camera) const {
        const auto& w=world_[j];Mat4 result;
        f32 scale[3];for(unsigned k=0;k<3;++k)scale[k]=std::sqrt(w.at(0,k)*w.at(0,k)+w.at(1,k)*w.at(1,k)+w.at(2,k)*w.at(2,k));
        const f32 roll=active_pose_?pose_.joints[j].rotation[2]:scene_.joints[j].rotation[2];
        f32 right[3],up[3],forward[3];
        for(unsigned k=0;k<3;++k){right[k]=b.right[k];up[k]=b.up[k];forward[k]=-b.forward[k];}
        if(mode==2){ // vertical: preserve world Y, turn only about Y
            const f32 dx=camera.eye[0]-w.at(0,3),dz=camera.eye[2]-w.at(2,3);
            const f32 len=std::sqrt(dx*dx+dz*dz);
            if(len>1e-6f){right[0]=dz/len;right[1]=0;right[2]=-dx/len;up[0]=0;up[1]=1;up[2]=0;forward[0]=dx/len;forward[1]=0;forward[2]=dz/len;}
        }
        if(mode==3){ // horizontal: retain authored facing until supported explicitly
            return w;
        }
        const f32 c=std::cos(roll),s=std::sin(roll);
        for(unsigned k=0;k<3;++k){
            result.at(k,0)=(right[k]*c+up[k]*s)*scale[0];
            result.at(k,1)=(up[k]*c-right[k]*s)*scale[1];
            result.at(k,2)=forward[k]*scale[2];result.at(k,3)=w.at(k,3);
        }
        return result;
    }
    Vertex shade(const HsdVertex& src,const f32 normal[3],const HsdMeshProperties& mesh,const f32 position[3],
                 const f32 origin[3],const Camera& camera,const SceneLighting& lighting,const lighting_detail::PreparedLightView* prepared=nullptr) const {
        Vertex v;const f32* uv=mesh.texture_uv1?src.uv1:src.uv;
        v.u=uv[0];v.v=uv[1];
        const u32 colour_mode=mesh.render_mode & 3u;
        u32 alpha_mode=(mesh.render_mode>>13)&3u;if(!alpha_mode)alpha_mode=colour_mode?colour_mode:1;
        const f32 dot=std::max(0.0f,normal[0]*0.35f+normal[1]*0.86f+normal[2]*0.37f);
        u8 rgb[3];for(unsigned k=0;k<3;++k){
            f32 value=colour_mode==2?src.colour[k]:mesh.material_colour[k];
            if(colour_mode==3)value*=src.colour[k]/255.0f;
            if((mesh.render_mode&4u) && !lighting.declared){const f32 ambient=mesh.material_ambient[k]/255.0f;value*=std::min(1.0f,ambient+(1-ambient)*dot);}
            rgb[k]=static_cast<u8>(std::clamp(value,0.0f,255.0f)+0.5f);
        }
        v.r=rgb[0];v.g=rgb[1];v.b=rgb[2];
        f32 a=alpha_mode==2?src.colour[3]:mesh.material_alpha*255;
        if(alpha_mode==3)a*=src.colour[3]/255.0f;
        v.a=static_cast<u8>(std::clamp(a,0.0f,255.0f)+0.5f);
        if(lighting.declared){
            const auto channels=light_vertex_prepared(lighting,position,normal,origin,camera.eye,
                mesh.material_ambient,mesh.material_specular,mesh.shininess,
                (mesh.render_mode&4u)!=0,(mesh.render_mode&8u)!=0,prepared);
            for(unsigned k=0;k<4;++k)v.light[k]=channels.diffuse[k];
            for(unsigned k=0;k<3;++k)v.specular[k]=channels.specular[k];
        }
        return v;
    }
    HsdScene scene_{};HsdAnimPose pose_{};
    SceneLighting lighting_cache_;bool lighting_cache_valid_=false,lighting_cache_global_=false;
    std::vector<std::array<f32,3>> evaluated_origins_;
    std::vector<std::vector<HsdVertex>> source_{};
    std::vector<Bounds3> bounds_;
    std::vector<Vertex> shaded_;std::vector<bool> shaded_ready_;
    lighting_detail::PreparedLightView prepared_lights_;
    std::vector<Mat4> world_{};std::vector<bool> hidden_{};std::vector<u32> handles_{};
    bool loaded_=false,active_pose_=false,cull_=true,depth_=true,alpha_=true,show_markers_=false;
    u8 depth_compare_=0xff;
    long only_mesh_=-1;std::string error_{"menu scene: not loaded"};
    std::array<u8,3> post_rgb_add_{};
    std::array<u8,3> post_rgb_multiply_{255,255,255};
    LinearFog fog_;bool fog_valid_=true;
    unsigned selection_=5;
    bool interaction_=true;
    std::array<u32,14> camera_key_{};bool cache_valid_=false;
    std::vector<std::vector<Vertex>> projected_{};std::vector<bool> projected_ready_{};
    MenuMotionState motion_state_{};bool motion_dirty_=true;
    std::vector<bool> motion_affected_{},selector_joint_{},caption_valid_{};
    std::vector<unsigned> identifier_item_{},caption_item_{},menu_item_{};
    MenuPointerGeometry pointer_geometry_;bool pointer_enabled_=false,pointer_free_mode_=false;
    bool selector_pointer_=false;f32 selector_x_=0,selector_y_=0,selector_offset_=0;
    unsigned focus_item_=7;bool show_selector_=true;
    u8 menu_art_filter_=0;u32 menu_root_filter_=0;
};
} // namespace gf
