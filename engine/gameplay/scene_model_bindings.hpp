#pragma once
#include "engine/gameplay/scene_archive_pool.hpp"
#include "engine/gameplay/scene_object_pool.hpp"
#include "engine/gameplay/scene_material_animation.hpp"
namespace gf {
//06FB4's normal LoadJointDesc attachment, consuming a genuine prepared root.
//This low-level operation does NOT dispatch a widget, invent scene45, advance a
//phase/timer/counter, publish a world or qualify SDK refcount/callback behavior.
class SceneModelBindings {
public:
    //07D68(-1) -> 092BC: clear BOTH cull bits across the existing default
    //hierarchy. No side/shadow styling, texture removal, alpha or world work.
    static bool disable_fresh_default_culling(SceneDescriptorPool& pool,const SceneDescriptorLease& descriptor,std::string* error){
        const auto* state=pool.root(descriptor)?pool.root(descriptor)->state():nullptr;
        if(!state||pool.material_context(descriptor)||state->style!=HsdJointInstanceStyle::Normal||state->animation_requested||state->world_evaluated||state->side_style_applied||state->stage_shadow_applied||state->child_translucent_applied){if(error)*error="default culling: fresh normal default hierarchy required";return false;}
        auto& mutable_state=pool.storage_->roots[descriptor.index()].storage_->state;
        for(auto& mesh:mutable_state.mesh_properties){mesh.second.pobj_flags&=u16(~0xc000u);mesh.second.cull_backface=mesh.second.cull_frontface=false;}
        if(error){error->clear();}return true;
    }
    //07CD0 AFTER a texture selector: mutate the actual default MObj constants
    //and the retained player's base colour, never substitute its UV sample.
    static bool set_requested_texture_diffuse_rgb(SceneDescriptorPool& pool,const SceneDescriptorLease& descriptor,SceneMaterialAnimationOwner& animation,const std::array<u8,3>& rgb,std::string* error){
        const auto* root=pool.root(descriptor);const auto* state=root?root->state():nullptr;
        if(!root||!state||!animation.same_root(*root)||pool.material_context(descriptor)||state->style!=HsdJointInstanceStyle::Normal||!state->animation_requested||state->world_evaluated||state->side_style_applied||state->stage_shadow_applied||state->child_translucent_applied){if(error)*error="requested diffuse: same default root/texture runtime required";return false;}
        if(!animation.storage_->player.set_texture_only_base_diffuse(rgb,error))return false;
        auto& mutable_state=pool.storage_->roots[descriptor.index()].storage_->state;
        for(auto& mesh:mutable_state.mesh_properties)std::copy(rgb.begin(),rgb.end(),mesh.second.material_colour);
        if(error){error->clear();}return true;
    }
    //07CAC -> 091E4 -> 2437C8 on a default, unrequested owned hierarchy.
    //Only the float material alpha changes, including each child DObj. This is
    //not 07C54 styling, a cache wrapper, diffuse-alpha quantization or AnimAll.
    static bool set_fresh_material_alpha(SceneDescriptorPool& pool,const SceneDescriptorLease& descriptor,f32 alpha,std::string* error){
        const auto* state=pool.root(descriptor)?pool.root(descriptor)->state():nullptr;
        if(!std::isfinite(alpha)||alpha<0||alpha>1||!state||pool.material_context(descriptor)||state->style!=HsdJointInstanceStyle::Normal||state->animation_requested||state->world_evaluated||state->side_style_applied||state->stage_shadow_applied||state->child_translucent_applied||state->dobjs.empty()){if(error)*error="material alpha: bounded fresh normal default hierarchy required";return false;}
        auto& mutable_state=pool.storage_->roots[descriptor.index()].storage_->state;
        for(auto& dobj:mutable_state.dobjs)dobj.material.alpha=alpha;
        for(auto& mesh:mutable_state.mesh_properties)mesh.second.material_alpha=alpha;
        if(error){error->clear();}return true;
    }
    //086B8(time0) -> ReqAnim -> 07C30/AnimAll for the narrowly reviewed fresh,
    //single-root texture-only material tree. The shared player interprets the
    //first callbacks; its TObj runtime remains owned separately from authored
    //mesh metadata. No clock tick, joint pose or world publication occurs.
    static bool request_fresh_material_texture_zero(SceneDescriptorPool& pool,const SceneDescriptorLease& descriptor,const SceneArchiveLease& archive,SceneMaterialAnimationOwner* out,std::string* error){
        return request_fresh_material_texture_impl(pool,descriptor,archive,0,out,error);
    }
    //Explicit integer UI selectors only; legacy time0 entry remains time0-only.
    static bool request_fresh_material_texture_selector(SceneDescriptorPool& pool,const SceneDescriptorLease& descriptor,const SceneArchiveLease& archive,f32 selector,SceneMaterialAnimationOwner* out,std::string* error){
        if(!std::isfinite(selector)||selector<0||selector>9||std::floor(selector)!=selector){if(error)*error="material selector: integer digit0..9 required";return false;}
        return request_fresh_material_texture_impl(pool,descriptor,archive,selector,out,error);
    }
private:
    static bool request_fresh_material_texture_impl(SceneDescriptorPool& pool,const SceneDescriptorLease& descriptor,const SceneArchiveLease& archive,f32 selector,SceneMaterialAnimationOwner* out,std::string* error){
        auto fail=[&](){if(error)*error="fresh material animation: same default root/nonempty texture-only table required";return false;};
        const auto* root=pool.root(descriptor);const auto* state=root?root->state():nullptr;const auto* bank=archive.bank();HsdRecordAnimationTables tables;
        if(!out||out->storage_||!root||!state||!bank||!archive.belongs_to(pool)||!root->same_source(*bank)||pool.material_context(descriptor)||state->style!=HsdJointInstanceStyle::Normal||state->animation_requested||state->world_evaluated||state->side_style_applied||state->stage_shadow_applied||state->child_translucent_applied||state->joints.size()!=1||state->joints[0].joint.parent!=kHsdNoJoint||state->joints[0].world||(state->joints[0].joint.flags&0x00e05020u)||!bank->animation_tables(state->record,&tables)||tables.joint||!tables.material||tables.material_clips!=1||tables.shape)return fail();
        HsdRecordAnimationBinding binding;
        if(!bank->select_textures(state->record,2,0,0,&binding,error))return false;
        const auto* objects=binding.objects();if(!objects||objects->empty()||objects->size()>8)return fail();
        for(const auto& object:*objects){if(object.kind!=hsd_record_detail::Object::Texture)return fail();for(const auto& track:object.tracks)if(track.channel<2||track.channel>9)return fail();}
        auto next=std::make_unique<SceneMaterialAnimationOwner::Storage>();
        if(!next->player.bind(binding,error)||!next->player.request(selector,1,error))return false;
        next->archive=archive;next->descriptor=descriptor;next->root=root->receipt();
        pool.storage_->roots[descriptor.index()].storage_->state.animation_requested=true;
        out->storage_=std::move(next);if(error){error->clear();}return true;
    }
public:
    //Descriptor+10 visibility is NOT root ownership or JObj hidden flags.
    static bool set_fresh_descriptor_visibility(SceneDescriptorPool& pool,const SceneDescriptorLease& descriptor,u8 visible,std::string* error){
        const auto* state=pool.root(descriptor)?pool.root(descriptor)->state():nullptr;
        if(visible>1||!state||pool.material_context(descriptor)||state->style!=HsdJointInstanceStyle::Normal||state->animation_requested||state->world_evaluated||state->side_style_applied||state->stage_shadow_applied||state->child_translucent_applied){if(error)*error="descriptor visibility: fresh normal default root required";return false;}
        pool.storage_->state.records[descriptor.index()].fields.payload10_1f[0]=visible;
        if(error){error->clear();}return true;
    }
    //086B8/07D40 -> 07C30/AnimAll, only on a fresh single unconstrained
    //default root with NULL animation tables. Do not issue ReqAnim on an empty
    //selection: a nonnull empty material table takes a DIFFERENT source branch.
    static bool finish_fresh_empty_material_animation(SceneDescriptorPool& pool,const SceneDescriptorLease& descriptor,const SceneArchiveLease& archive,std::string* error){
        auto fail=[&](){if(error)*error="empty material animation: fresh null-table/no-op traversal required";return false;};
        const auto* root=pool.root(descriptor);const auto* state=root?root->state():nullptr;const auto* bank=archive.bank();HsdRecordAnimationTables tables;
        if(!root||!state||!bank||!archive.belongs_to(pool)||!root->same_source(*bank)||pool.material_context(descriptor)||state->style!=HsdJointInstanceStyle::Normal||state->animation_requested||state->world_evaluated||state->side_style_applied||state->stage_shadow_applied||state->child_translucent_applied||state->joints.size()!=1||!bank->animation_tables(state->record,&tables)||tables.joint||tables.material||tables.shape)return fail();
        const auto& j=state->joints.front();
        //Construct_normal_records already refused RObj/custom/instanced nodes.
        //These root flags take AnimAll's no-parent/no-dirty/no-instance branch.
        if(j.joint.parent!=kHsdNoJoint||j.world||(j.joint.flags&0x00e05020u))return fail();
        if(error){error->clear();}return true;
    }
    //07CD0's diffuse RGB write only. No alpha/category/texture/cache styling,
    //animation attachment, pose evaluation or world publication.
    static bool set_fresh_diffuse_rgb(SceneDescriptorPool& pool,const SceneDescriptorLease& descriptor,const std::array<u8,3>& rgb,std::string* error){
        const auto* state=pool.root(descriptor)?pool.root(descriptor)->state():nullptr;
        if(!state||pool.material_context(descriptor)||state->style!=HsdJointInstanceStyle::Normal||state->animation_requested||state->world_evaluated||state->side_style_applied||state->stage_shadow_applied||state->child_translucent_applied||state->dobjs.empty()){if(error)*error="diffuse RGB: fresh normal default material root required";return false;}
        auto& mutable_state=pool.storage_->roots[descriptor.index()].storage_->state;
        for(auto& mesh:mutable_state.mesh_properties)std::copy(rgb.begin(),rgb.end(),mesh.second.material_colour);
        if(error){error->clear();}return true;
    }
    static bool attach_object_descriptor(SceneObjectPool& objects,const SceneObjectLease& object,unsigned slot,const SceneArchiveLease& archive,u16 record,HsdJointInstanceOwner&& root,std::string* error){
        if(!objects.owns(object)||slot>=objects.storage_->state.records[object.index()].descriptor_count99){if(error)*error="scene model binding: current object/descriptor slot required";return false;}
        return attach_normal(*objects.storage_->descriptors,objects.storage_->leases[object.index()][slot],archive,record,std::move(root),error);
    }
    static const HsdJointInstanceOwner* object_root(const SceneObjectPool& objects,const SceneObjectLease& object,unsigned slot){return objects.owns(object)&&slot<objects.storage_->state.records[object.index()].descriptor_count99?objects.storage_->descriptors->root(objects.storage_->leases[object.index()][slot]):nullptr;}
    static bool attach_object_descriptor(SceneObjectPool& objects,const SceneClass3Lease& object,unsigned slot,const SceneArchiveLease& archive,u16 record,HsdJointInstanceOwner&& root,std::string* error){
        if(!objects.owns(object)||slot>=objects.storage_->class3_models[object.index()].descriptor_count99){if(error)*error="scene model binding: current class3 object/descriptor slot required";return false;}
        return attach_normal(*objects.storage_->descriptors,objects.storage_->class3_leases[object.index()][slot],archive,record,std::move(root),error);
    }
    static const HsdJointInstanceOwner* object_root(const SceneObjectPool& objects,const SceneClass3Lease& object,unsigned slot){return objects.owns(object)&&slot<objects.storage_->class3_models[object.index()].descriptor_count99?objects.storage_->descriptors->root(objects.storage_->class3_leases[object.index()][slot]):nullptr;}
    static bool attach_normal(SceneDescriptorPool& pool,const SceneDescriptorLease& descriptor,const SceneArchiveLease& archive,u16 record,HsdJointInstanceOwner&& root,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("scene model binding: ")+why;return false;};
        const auto* bank=archive.bank();const auto* state=root.state();
        if(!pool.owns(descriptor)||!bank||!archive.belongs_to(pool)||!state||state->style!=HsdJointInstanceStyle::Normal||state->record!=record||record>=bank->records()||!root.same_source(*bank)||state->animation_requested||state->world_evaluated)return reject("same current domain/archive/normal unrequested root required");
        if(!pool.can_release(descriptor))return reject("existing root cleanup is not qualified");
        auto& r=pool.storage_->state.records[descriptor.index()];auto& target=pool.storage_->roots[descriptor.index()];
        //Native typed ownership release replaces ONLY the admitted sole-owned,
        //default/unrequested metadata subtree; no opaque SDK hook is invoked.
        target.retire();target=std::move(root);r.native_root0c=true;r.material_context_required=false;pool.storage_->contexts[descriptor.index()].reset();
        store_be32(r.fields.payload10_1f.data()+4,record);r.fields.flag0b=0;r.fields.payload10_1f[0]=1;
        if(error){error->clear();}return true;
    }
private:
    static bool attach_scoped(SceneDescriptorPool& pool,const SceneDescriptorLease& descriptor,const SceneArchiveLease& archive,u16 record,HsdJointInstanceStyle style,HsdJointInstanceOwner&& root,const std::shared_ptr<const SceneModelMaterialContext>& context,const std::array<u32,2>& initial_cache,std::string* error){
        const auto* bank=archive.bank();const auto* state=root.state();
        if(!pool.owns(descriptor)||!bank||!archive.belongs_to(pool)||!state||state->record!=record||record>=bank->records()||state->style!=style||!root.same_source(*bank)||state->animation_requested||state->world_evaluated||!context||context->source_cache_words()!=initial_cache){if(error)*error="scene scoped model binding: current domain/archive/root/context required";return false;}
        auto& r=pool.storage_->state.records[descriptor.index()];
        if(r.native_root0c||r.fields.unresolved_root_key0c){if(error)*error="scene actor model binding: fresh descriptor root required";return false;}
        pool.storage_->roots[descriptor.index()]=std::move(root);pool.storage_->contexts[descriptor.index()]=context;r.native_root0c=r.material_context_required=true;
        store_be32(r.fields.payload10_1f.data()+4,record);r.fields.flag0b=0;r.fields.payload10_1f[0]=1;
        if(error){error->clear();}return true;
    }
public:
    //07030's actor row-cache entry retains its qualified initial255/0 domain.
    static bool attach_actor(SceneDescriptorPool& pool,const SceneDescriptorLease& descriptor,const SceneArchiveLease& archive,u16 record,HsdJointInstanceStyle style,HsdJointInstanceOwner&& root,const std::shared_ptr<const SceneModelMaterialContext>& context,std::string* error){
        if(!context||context->kind()!=SceneModelMaterialKind::ScopedCache)return false;
        return attach_scoped(pool,descriptor,archive,record,style,std::move(root),context,{255,0},error);
    }
private:
    friend class BattleActorConstruction;
    friend class BattleStageConstruction;
    //Reviewed initial bank-child second root:07C54/07CAC(.4), textures retained.
    //It is neither actor alternate construction nor side/shadow texture removal.
    static bool style_initial_bank_child(SceneDescriptorPool& pool,const SceneDescriptorLease& descriptor,std::string* error){
        if(!pool.owns(descriptor))return false;
        auto& root=pool.storage_->roots[descriptor.index()];const auto* state=root.state();
        if(!state||state->style!=HsdJointInstanceStyle::Normal||state->animation_requested||state->world_evaluated||state->side_style_applied||state->stage_shadow_applied||state->child_translucent_applied||state->joints.empty())return false;
        auto next=*state;
        for(auto& joint:next.joints){if(joint.joint.flags&0x1000u)return false;joint.joint.flags=(joint.joint.flags|0x20080000u)&~0x50140000u;}
        for(auto& d:next.dobjs){
            AttachmentAlternateState changed;if(!attachment_translucent_apply(d.material,&changed,error))return false;
            changed.alpha=bits_to_f32(0x3ecccccdu);d.material=changed;d.flags=changed.dobj_flags;
            for(auto mesh:d.meshes){
                auto p=std::find_if(next.mesh_properties.begin(),next.mesh_properties.end(),[&](const auto& entry){return entry.first==mesh;});if(p==next.mesh_properties.end())return false;
                HsdMeshProperties properties;if(!attachment_alternate_properties(p->second,changed,&properties,error))return false;p->second=properties;
            }
        }
        next.child_translucent_applied=true;root.storage_->state=std::move(next);if(error)error->clear();return true;
    }
    //07030's descriptor-local cache entry. LoadJointDesc binds the existing
    //cache words first;819B4 writes18=255 only AFTER the binding returns.
    static bool attach_descriptor_context(SceneDescriptorPool& pool,const SceneDescriptorLease& descriptor,const SceneArchiveLease& archive,u16 record,HsdJointInstanceOwner&& root,const std::shared_ptr<const SceneModelMaterialContext>& context,std::string* error){
        if(!context||context->kind()!=SceneModelMaterialKind::ScopedCache)return false;
        const auto* d=pool.inspect(descriptor);if(!d)return false;const ByteView fields{d->fields.payload10_1f.data(),16};
        return attach_scoped(pool,descriptor,archive,record,HsdJointInstanceStyle::Normal,std::move(root),context,{fields.be32(8),fields.be32(12)},error);
    }
    //070E4 uses its own colour-mobj class/context domain, not07030's hook.
    static bool attach_colour_context(SceneDescriptorPool& pool,const SceneDescriptorLease& descriptor,const SceneArchiveLease& archive,u16 record,HsdJointInstanceOwner&& root,const std::shared_ptr<const SceneModelMaterialContext>& context,std::string* error){
        const auto* d=pool.inspect(descriptor);if(!d||!context||context->kind()!=SceneModelMaterialKind::ColourWrapper)return false;const ByteView fields{d->fields.payload10_1f.data(),16};
        return attach_scoped(pool,descriptor,archive,record,HsdJointInstanceStyle::Normal,std::move(root),context,{fields.be32(8),fields.be32(12)},error);
    }
    static bool set_descriptor_cache_colour(SceneDescriptorPool& pool,const SceneDescriptorLease& descriptor,const std::shared_ptr<const SceneModelMaterialContext>& context,u32 value){
        return set_descriptor_cache_word(pool,descriptor,context,0,value);
    }
    static bool set_descriptor_cache_word(SceneDescriptorPool& pool,const SceneDescriptorLease& descriptor,const std::shared_ptr<const SceneModelMaterialContext>& context,unsigned word,u32 value){
        if(word>1||(word==1&&(!context||context->kind()!=SceneModelMaterialKind::ColourWrapper)))return false;
        if(!pool.owns(descriptor)||!context||pool.material_context(descriptor)!=context.get())return false;
        auto& fields=pool.storage_->state.records[descriptor.index()].fields;const ByteView words{fields.payload10_1f.data(),16};
        if(context->source_cache_words()!=std::array<u32,2>{words.be32(8),words.be32(12)})return false;
        context->words_[word]=value;store_be32(fields.payload10_1f.data()+8+4*word,value);return true;
    }
    //81B9C's07C54/07CD0/07F38 and optional07D68, on the already-owned
    //second root. No actor alternate alpha store or new instance allocation.
    static bool style_side_root(SceneDescriptorPool& pool,const SceneDescriptorLease& descriptor,s8 cull,std::string* error){
        return style_owned_root(pool,descriptor,cull,false,false,error);
    }
    static bool style_stage_shadow(SceneDescriptorPool& pool,const SceneDescriptorLease& descriptor,bool stage_root,std::string* error){
        return style_owned_root(pool,descriptor,stage_root?-1:0,true,stage_root,error);
    }
    static bool style_owned_root(SceneDescriptorPool& pool,const SceneDescriptorLease& descriptor,s8 cull,bool shadow,bool lower_y,std::string* error){
        if(!pool.owns(descriptor)||(cull!=0&&cull!=-1))return false;
        auto& root=pool.storage_->roots[descriptor.index()];const auto* state=root.state();
        if(!state||state->style!=HsdJointInstanceStyle::Normal||state->animation_requested||state->world_evaluated||state->side_style_applied||state->stage_shadow_applied||state->joints.empty())return false;
        auto next=*state;
        if(shadow){
            //87924/87D2C unref each immediate child before styling. Admit only
            //exclusive default subtrees with no PObj/ROBJ joint-reference owner.
            for(const auto& p:next.mesh_properties){
                u32 klass=0;
                if(p.second.skin_binding||(p.second.pobj_flags&0x3000u)||!hsd_record_detail::optional(root.storage_->source->reader,p.second.pobj_offset,&klass)||klass){if(error)*error="stage shadow: custom/referenced/deforming child cleanup unsupported";return false;}
            }
            next.joints.resize(1);next.joints[0].joint.flags&=0x8fffffffu;
            std::erase_if(next.dobjs,[](const auto& d){return d.joint!=0;});
            std::erase_if(next.mesh_properties,[&](const auto& p){for(const auto& d:next.dobjs)if(std::find(d.meshes.begin(),d.meshes.end(),p.first)!=d.meshes.end())return false;return true;});
        }
        for(auto& joint:next.joints){if(joint.joint.flags&0x1000u){if(error)*error="stage side style: instanced subtree unsupported";return false;}joint.joint.flags=(joint.joint.flags|0x20080000u)&~0x50140000u;}
        for(auto& d:next.dobjs){
            AttachmentAlternateState changed;if(!attachment_translucent_apply(d.material,&changed,error))return false;
            if(shadow)changed.alpha=0.5f; //07CAC source80437C24; side alpha is retained
            d.material=changed;d.flags=changed.dobj_flags;
            for(auto mesh:d.meshes){auto p=std::find_if(next.mesh_properties.begin(),next.mesh_properties.end(),[&](const auto& entry){return entry.first==mesh;});if(p==next.mesh_properties.end())return false;
                HsdMeshProperties changed_mesh;if(!attachment_alternate_properties(p->second,changed,&changed_mesh,error))return false;
                //Diffuse RGB only; authored diffuse alpha and material alpha stay.
                std::fill_n(changed_mesh.material_colour,3,u8(0));
                changed_mesh.texture=kHsdNoTexture;changed_mesh.texture_stage_inactive=false;changed_mesh.texture_matrix_required=false;changed_mesh.generated_uv_required=false;changed_mesh.reflection_uv_supported=false;changed_mesh.texture_matrix_mask=0;changed_mesh.texture_coordinate_mode=0;
                changed_mesh.texture_colour_mode=4;changed_mesh.texture_alpha_mode=3;changed_mesh.texture_weight=1;changed_mesh.texture_colour_lerp=false;changed_mesh.texture_low_colour=0;changed_mesh.texture_high_colour=0xffffffffu;changed_mesh.texture_uv1=false;changed_mesh.texture_after_lighting=false;
                if(cull){changed_mesh.pobj_flags&=u16(~0xc000u);changed_mesh.cull_backface=changed_mesh.cull_frontface=false;}
                p->second=changed_mesh;
            }
        }
        if(lower_y){
            auto& joint=next.joints[0].joint;joint.translation[1]=f32(joint.translation[1]-1.0f);joint.flags|=0x40u; //07834/251B80, after styling
        }
        next.side_style_applied=!shadow;next.stage_shadow_applied=shadow;root.storage_->state=std::move(next);if(error)error->clear();return true;
    }
    struct JointWorld {u32 joint=0;Mat4 matrix;std::optional<bool> visible;};
    //Derived authored world for the narrow static, single-root rigid path.
    //No actor076D0/user-matrix flags, animation request or material substitution.
    static bool prepare_static_root_world(SceneDescriptorPool& pool,const SceneDescriptorLease& lease,const SceneArchiveLease& archive,const HsdJointInstanceReceipt& identity,u16 record,u64 generation,HsdJointInstanceStateSet* out,std::string* error){
        auto fail=[&](){if(error)*error="scene static root: fresh normal rigid authored root required";return false;};
        const auto* root=pool.root(lease);const auto* bank=archive.bank();const auto* state=root?root->state():nullptr;
        if(!out||!root||!bank||!archive.belongs_to(pool)||!root->same_source(*bank)||!root->receipt().same_instance(identity)||pool.material_context(lease)||!state||state->record!=record||state->style!=HsdJointInstanceStyle::Normal||state->world_evaluated||state->animation_requested||state->stage_shadow_applied||state->side_style_applied||state->joints.size()!=1||!generation)return fail();
        const auto& joint=state->joints[0].joint;
        if(joint.parent!=kHsdNoJoint||(joint.flags&(0x03807e30u))||(joint.flags&0x70000000u)!=0x10000000u||(joint.flags&0x1c0000u)!=0x40000u)return fail();
        const auto world=mat4_from_srt(joint.rotation,joint.scale,joint.translation);
        for(float value:world.m)if(!std::isfinite(value)||std::abs(value)>1e7f)return fail();
        auto next=*state;next.joints[0].world=world;next.world_evaluated=true;next.world_generation=generation;
        *out=std::move(next);if(error)error->clear();return true;
    }
    static bool set_published_descriptor_flag(SceneDescriptorPool& pool,const SceneDescriptorLease& lease,const std::shared_ptr<const HsdJointWorldScope>& scope,u8 value){
        const auto* root=pool.root(lease);if(value>1||!root||!root->world_from(scope))return false;
        pool.storage_->state.records[lease.index()].fields.flag0b=value;return true;
    }
    //Authored, unanimated translation-only trees used by the initial stage props.
    //This is not an arbitrary HSD hierarchy/constraint or actor-matrix evaluator.
    static bool prepare_authored_translation_world(SceneDescriptorPool& pool,const SceneDescriptorLease& lease,const SceneArchiveLease& archive,const HsdJointInstanceReceipt& identity,u16 record,u64 generation,HsdJointInstanceStateSet* out,std::string* error){
        auto fail=[&](){if(error)*error="scene authored prop world: same fresh translation-only normal tree required";return false;};
        const auto* root=pool.root(lease);const auto* bank=archive.bank();const auto* state=root?root->state():nullptr;
        if(!out||!root||!bank||!archive.belongs_to(pool)||!root->same_source(*bank)||!root->receipt().same_instance(identity)||pool.material_context(lease)||!state||state->style!=HsdJointInstanceStyle::Normal||state->record!=record||state->world_evaluated||state->animation_requested||state->side_style_applied||state->stage_shadow_applied||state->joints.empty()||state->joints.size()>32||!generation)return fail();
        auto next=*state;
        const std::array<u32,9> flags{0,0x10050180,0x10050080,0x50080,0x10040080,0x30040080,0x80000,0x20080000,0x10040000};
        for(unsigned i=0;i<next.joints.size();++i){
            auto& joint=next.joints[i];const auto& authored=joint.joint;
            if(std::find(flags.begin(),flags.end(),authored.flags)==flags.end())return fail();
            for(unsigned k=0;k<3;++k)if(authored.rotation[k]!=0||authored.scale[k]!=1||!std::isfinite(authored.translation[k])||std::abs(authored.translation[k])>1e7f)return fail();
            const auto local=mat4_translation(authored.translation);
            if(!i){if(authored.parent!=kHsdNoJoint)return fail();joint.world=local;}
            else{if(authored.parent>=i||!next.joints[authored.parent].world)return fail();joint.world=mat4_multiply(*next.joints[authored.parent].world,local);}
            for(f32 value:joint.world->m)if(!std::isfinite(value)||std::abs(value)>1e7f)return fail();
        }
        next.world_evaluated=true;next.world_generation=generation;*out=std::move(next);if(error)error->clear();return true;
    }
    //820C8's first unoccluded branch: source alpha1 and zero additive cache.
    //076D0 replaces the one root world; authored root translation is NOT added.
    static bool prepare_side_identity_world(SceneDescriptorPool& pool,const SceneDescriptorLease& lease,const SceneArchiveLease& archive,const HsdJointInstanceReceipt& identity,const SceneModelMaterialContext* context,u16 record,u64 generation,const Mat4& world,HsdJointInstanceStateSet* out,std::string* error){
        auto fail=[&](){if(error)*error="side world: exact fresh source root/zero-cache/alpha1 branch required";return false;};
        const auto* root=pool.root(lease);const auto* bank=archive.bank();const auto* state=root?root->state():nullptr;
        if(!out||!root||!bank||!archive.belongs_to(pool)||!root->same_source(*bank)||!root->receipt().same_instance(identity)||pool.material_context(lease)!=context||!state||state->record!=record||state->style!=HsdJointInstanceStyle::Normal||state->world_evaluated||state->animation_requested||state->side_style_applied||state->stage_shadow_applied||state->joints.size()!=1||!generation)return fail();
        if(context&&(context->kind()!=SceneModelMaterialKind::ScopedCache||context->source_cache_words()[0]!=255))return fail();
        const auto flags=state->joints[0].joint.flags;
        if((flags!=0x10040080&&flags!=0x20080080)||state->joints[0].joint.parent!=kHsdNoJoint)return fail();
        for(float value:world.m)if(!std::isfinite(value)||std::abs(value)>1e7f)return fail();
        if(world.m[12]||world.m[13]||world.m[14]||world.m[15]!=1)return fail();
        auto next=*state;next.joints[0].world=world;next.joints[0].joint.flags|=0x03800040u;
        for(auto& d:next.dobjs)d.material.alpha=1;
        for(auto& p:next.mesh_properties)p.second.material_alpha=1;
        next.source_zero_colour_cache=true;next.world_evaluated=true;next.world_generation=generation;*out=std::move(next);if(error)error->clear();return true;
    }
    //Candidate metadata only. Source geometry, root identities and material
    //owners remain in their existing descriptor slots.
    static bool prepare_actor_world(SceneDescriptorPool& pool,const SceneDescriptorLease& lease,const SceneArchiveLease& archive,const HsdJointInstanceReceipt& identity,const SceneModelMaterialContext* context,u16 record,u64 generation,std::span<const JointWorld> writes,HsdJointInstanceStateSet* out,std::string* error){
        auto reject=[&](){if(error)*error="scene actor world: same fresh root/material/generation required";return false;};
        const auto* root=pool.root(lease);const auto* bank=archive.bank();const auto* state=root?root->state():nullptr;
        if(!out||!root||!bank||!archive.belongs_to(pool)||!root->same_source(*bank)||!root->receipt().same_instance(identity)||pool.material_context(lease)!=context||!state||state->style!=HsdJointInstanceStyle::Normal||state->record!=record||state->world_evaluated||state->animation_requested||!generation||writes.size()!=state->joints.size())return reject();
        auto next=*state;std::vector<bool> seen(next.joints.size());
        for(const auto& write:writes){
            unsigned index=0;for(;index<next.joints.size();++index)if(next.joints[index].joint.archive_offset==write.joint)break;
            if(index==next.joints.size()||seen[index])return reject();
            seen[index]=true;
            for(f32 value:write.matrix.m)if(!std::isfinite(value)||std::abs(value)>1e7f)return reject();
            if(write.matrix.m[12]!=0||write.matrix.m[13]!=0||write.matrix.m[14]!=0||write.matrix.m[15]!=1)return reject();
            auto& joint=next.joints[index];
            if(write.visible){if(*write.visible)joint.joint.flags&=~0x10u;else joint.joint.flags|=0x10u;}
            //56AA8/076D0 copy affine matrices, OR3800000, then251B80 marks
            //dirty. Every joint in these admitted roots is written; descendant
            //dirty propagation cannot add a different final flag or matrix.
            joint.joint.flags|=0x03800040u;joint.world=write.matrix;
        }
        next.world_evaluated=true;next.world_generation=generation;*out=std::move(next);if(error)error->clear();return true;
    }
    static bool commit_world(SceneDescriptorPool& pool,const SceneDescriptorLease& lease,const HsdJointInstanceReceipt& identity,const std::shared_ptr<const HsdJointWorldScope>& scope,HsdJointInstanceStateSet&& state){
        if(!scope||!scope->live()||!pool.owns(lease))return false;
        auto& root=pool.storage_->roots[lease.index()];if(!root.current()||!root.receipt().same_instance(identity)||root.state()->world_evaluated||!state.world_evaluated)return false;
        auto epoch=std::make_shared<const u64>(state.world_generation);
        root.storage_->state=std::move(state);root.storage_->world_scope=scope;root.storage_->publication_epoch=std::move(epoch);return true;
    }
    static bool bind_queue_links(SceneDescriptorPool& pool,const SceneDescriptorLease& lease,const std::shared_ptr<const HsdJointWorldScope>& scope,const std::array<u32,3>& ids,u64 generation){
        if(!pool.owns(lease)||!generation)return false;
        const auto* root=pool.root(lease);if(!root||!root->world_from(scope))return false;
        std::array<std::optional<u16>,3> links{};
        for(unsigned i=0;i<3;++i)if(ids[i]){const auto n=ids[i]-1;if(n>=pool.storage_->state.records.size()||!pool.storage_->state.records[n].allocated09||!pool.storage_->roots[n].world_from(scope))return false;links[i]=u16(n);}
        auto& record=pool.storage_->state.records[lease.index()];record.model_queue_next=links;record.model_queue_generation=generation;return true;
    }
};
} // namespace gf
