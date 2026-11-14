#pragma once
#include "engine/gameplay/scene_archive_pool.hpp"
#include "engine/gameplay/scene_object_pool.hpp"
namespace gf {
//06FB4's normal LoadJointDesc attachment, consuming a genuine prepared root.
//This low-level operation does NOT dispatch a widget, invent scene45, advance a
//phase/timer/counter, publish a world or qualify SDK refcount/callback behavior.
class SceneModelBindings {
public:
    static bool attach_object_descriptor(SceneObjectPool& objects,const SceneObjectLease& object,unsigned slot,const SceneArchiveLease& archive,u16 record,HsdJointInstanceOwner&& root,std::string* error){
        if(!objects.owns(object)||slot>=objects.storage_->state.records[object.index()].descriptor_count99){if(error)*error="scene model binding: current object/descriptor slot required";return false;}
        return attach_normal(*objects.storage_->descriptors,objects.storage_->leases[object.index()][slot],archive,record,std::move(root),error);
    }
    static const HsdJointInstanceOwner* object_root(const SceneObjectPool& objects,const SceneObjectLease& object,unsigned slot){return objects.owns(object)&&slot<objects.storage_->state.records[object.index()].descriptor_count99?objects.storage_->descriptors->root(objects.storage_->leases[object.index()][slot]):nullptr;}
    static bool attach_normal(SceneDescriptorPool& pool,const SceneDescriptorLease& descriptor,const SceneArchiveLease& archive,u16 record,HsdJointInstanceOwner&& root,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("scene model binding: ")+why;return false;};
        const auto* bank=archive.bank();const auto* state=root.state();
        if(!pool.owns(descriptor)||!bank||!archive.belongs_to(pool)||!state||state->style!=HsdJointInstanceStyle::Normal||state->record!=record||record>=bank->records()||!root.same_source(*bank)||state->animation_requested||state->world_evaluated)return reject("same current domain/archive/normal unrequested root required");
        if(!pool.can_release(descriptor))return reject("existing root cleanup is not qualified");
        auto& r=pool.storage_->state.records[descriptor.index()];auto& target=pool.storage_->roots[descriptor.index()];
        //Native typed ownership release replaces ONLY the admitted sole-owned,
        //default/unrequested metadata subtree; no opaque SDK hook is invoked.
        target.retire();target=std::move(root);r.native_root0c=true;
        store_be32(r.fields.payload10_1f.data()+4,record);r.fields.flag0b=0;r.fields.payload10_1f[0]=1;
        if(error){error->clear();}return true;
    }
};
} // namespace gf
