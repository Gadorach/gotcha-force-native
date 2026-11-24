#pragma once
#include "engine/gameplay/scene_archive_pool.hpp"
#include "engine/gameplay/hsd_record_animation.hpp"
namespace gf {
//Retained shared player for an animation attached to an ACTUAL descriptor/root.
//The player owns only immutable animation bytes and small TObj/MObj runtime
//state. Neither copied poses nor surviving source bytes keep a retired root live.
class SceneMaterialAnimationOwner {
public:
    SceneMaterialAnimationOwner()=default;
    SceneMaterialAnimationOwner(const SceneMaterialAnimationOwner&)=delete;
    SceneMaterialAnimationOwner& operator=(const SceneMaterialAnimationOwner&)=delete;
    SceneMaterialAnimationOwner(SceneMaterialAnimationOwner&&)=default;
    SceneMaterialAnimationOwner& operator=(SceneMaterialAnimationOwner&&)=default;
    bool current()const{return storage_&&storage_->archive.bank()&&storage_->descriptor.current()&&storage_->root.current()&&storage_->player.pose();}
    const HsdAnimPose* pose()const{return current()?storage_->player.pose():nullptr;}
    std::vector<HsdRecordObjectTrace> trace()const{return current()?storage_->player.trace():std::vector<HsdRecordObjectTrace>{};}
    bool same_root(const HsdJointInstanceOwner& root)const{return current()&&storage_->root.same_instance(root.receipt());}
    void retire(){storage_.reset();}
private:
    friend class SceneModelBindings;
    struct Storage {SceneArchiveLease archive;SceneDescriptorLease descriptor;HsdJointInstanceReceipt root;HsdRecordAnimationPlayer player;};
    std::unique_ptr<Storage> storage_;
};
}
