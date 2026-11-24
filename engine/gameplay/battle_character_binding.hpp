#pragma once
#include "engine/assets/battle_normal_class.hpp"
#include "engine/gameplay/challenge_character_resources.hpp"
#include "engine/gameplay/startup_actor_material.hpp"
#include "engine/assets/hsd_joint_instances.hpp"
#include "engine/gameplay/scene_model_bindings.hpp"
namespace gf {
struct BattleBindingPending {
    bool all_bank_instances=true,class_callback4b8=true,scalar_variant_setup=true,motion_program=true;
    bool colliders_and_effects=true,placement_and_history=true,full_constructor=true;
};
struct BattleBankInstanceScope {
    std::weak_ptr<const BattleCharacterResource> resource;
    std::weak_ptr<const CharacterModelBinding> model;
};
//Instance existence alone is not current resource authority. Even keeping an
//old model identity alive cannot keep this private binding scope alive on retire.
class BattleBankInstanceReceipt {
public:
    bool current(const std::array<const ForceInventory*,2>& owners)const{
        const auto scope=scope_.lock();if(!scope||scope->model.expired()||!instance_.current())return false;
        const auto resource=scope->resource.lock();return resource&&allocation_.data(owners)==resource;
    }
    bool same_instance(const BattleBankInstanceReceipt& other)const{return instance_.same_instance(other.instance_);}
private:
    std::weak_ptr<const BattleBankInstanceScope> scope_;HsdJointInstanceReceipt instance_;ChallengeCharacterDescriptorReceipt allocation_;friend class BattleCharacterBinding;
};
using BattleBankMaterialContext=SceneModelMaterialContext;
//Preparatory binding of real normal body/rig, source class fields, neutral shared
//fade owner and the motion-bank table. NOT419EC's complete bank of normal/alternate
//instances, a preview actor, a program execution, or a541AC constructor receipt.
class BattleCharacterBinding {
public:
    BattleCharacterBinding()=default;BattleCharacterBinding(const BattleCharacterBinding&)=delete;BattleCharacterBinding& operator=(const BattleCharacterBinding&)=delete;
    BattleCharacterBinding(BattleCharacterBinding&&)=default;BattleCharacterBinding& operator=(BattleCharacterBinding&&)=default;
    bool bind(const ChallengeCharacterDescriptorReceipt& allocation,const std::array<const ForceInventory*,2>& owners,const BattleNormalClassProfile& classes,ByteView pose,ByteView setup,ByteView fade,std::string* error){
        if(attempted_)return fail(error,"fresh one-shot binding required");
        attempted_=true;const auto data=allocation.data(owners);if(!data)return fail(error,"current genuine resource allocation required");
        if(!load(data,classes,pose,setup,fade,error))return false;
        if(allocation.data(owners)!=data){storage_.reset();return fail(error,"allocation retired during preparation");}allocation_=allocation;native_bound_=true;return true;
    }
    //Declared input seam does not mint native allocation/actor authority.
    bool bind_declared(std::shared_ptr<const BattleCharacterResource> data,const BattleNormalClassProfile& classes,ByteView pose,ByteView setup,ByteView fade,std::string* error){
        if(attempted_)return fail(error,"fresh one-shot binding required");
        attempted_=true;return load(std::move(data),classes,pose,setup,fade,error);
    }
    bool bind_with_bank_instances(const ChallengeCharacterDescriptorReceipt& allocation,const std::array<const ForceInventory*,2>& owners,const BattleNormalClassProfile& classes,ByteView pose,ByteView setup,ByteView fade,const AttachmentAlternateProfile& alternate,std::string* error){
        if(attempted_)return fail(error,"fresh one-shot binding required");
        attempted_=true;const auto data=allocation.data(owners);if(!data)return fail(error,"current genuine resource allocation required");
        if(!load(data,classes,pose,setup,fade,error,&alternate))return false;
        if(allocation.data(owners)!=data){storage_.reset();return fail(error,"allocation retired during preparation");}allocation_=allocation;native_bound_=true;return true;
    }
    bool bind_declared_with_bank_instances(std::shared_ptr<const BattleCharacterResource> data,const BattleNormalClassProfile& classes,ByteView pose,ByteView setup,ByteView fade,const AttachmentAlternateProfile& alternate,std::string* error){
        if(attempted_)return fail(error,"fresh one-shot binding required");
        attempted_=true;return load(std::move(data),classes,pose,setup,fade,error,&alternate);
    }
    bool bind_with_bank_instances_and_spawn_colour(const ChallengeCharacterDescriptorReceipt& allocation,const std::array<const ForceInventory*,2>& owners,const BattleNormalClassProfile& classes,ByteView pose,ByteView setup,ByteView fade,const AttachmentAlternateProfile& alternate,std::string* error){
        if(attempted_)return fail(error,"fresh one-shot binding required");
        attempted_=true;const auto data=allocation.data(owners);if(!data)return fail(error,"current genuine resource allocation required");
        if(!load(data,classes,pose,setup,fade,error,&alternate,true))return false;
        if(allocation.data(owners)!=data){storage_.reset();return fail(error,"allocation retired during preparation");}allocation_=allocation;native_bound_=true;return true;
    }
    bool prepared()const{return bool(storage_);}bool current(const std::array<const ForceInventory*,2>& owners)const{return native_bound_&&storage_&&allocation_.data(owners)==storage_->resource&&descriptor_roots_current();}
    const BattleNormalClassRow* class_fields()const{return storage_?&storage_->klass:nullptr;}
    const CharacterPoseRig* rig()const{return storage_?&storage_->model.rig():nullptr;}
    const CharacterMotionBank* motion_bank()const{return storage_?&storage_->motion:nullptr;}
    const HsdSkinAsset* body_asset()const{return storage_?&storage_->model.asset():nullptr;}
    const ActorFadeState* material_state()const{return storage_?&storage_->material.state():nullptr;}
    std::weak_ptr<const CharacterModelBinding> model_binding()const{return storage_?storage_->model.instance_binding():std::weak_ptr<const CharacterModelBinding>{};}
    const HsdJointInstanceBank* bank_instances()const{return storage_&&storage_->instances.instances()?&storage_->instances:nullptr;}
    const BattleBankMaterialContext* bank_material_context()const{return bank_instances()?storage_->bank_material.get():nullptr;}
    const HsdJointInstanceOwner* bound_instance(unsigned index)const{const auto pool=storage_?storage_->descriptor_pool.lock():nullptr;return storage_&&storage_->descriptors_bound&&pool&&index<storage_->descriptors.size()?pool->root(storage_->descriptors[index].lease):nullptr;}
    const SceneDescriptorRecord* bound_descriptor(unsigned index)const{const auto pool=storage_?storage_->descriptor_pool.lock():nullptr;return storage_&&storage_->descriptors_bound&&pool&&index<storage_->descriptors.size()?pool->inspect(storage_->descriptors[index].lease):nullptr;}
    BattleBankInstanceReceipt bank_instance_receipt(unsigned index)const{BattleBankInstanceReceipt r;if(native_bound_&&bank_instances()){r.scope_=storage_->instance_scope;const auto* root=bound_instance(index);r.instance_=storage_->descriptors_bound?(root?root->receipt():HsdJointInstanceReceipt{}):storage_->instances.receipt(index);r.allocation_=allocation_;}return r;}
    BattleBindingPending pending()const{BattleBindingPending p;if(bank_instances())p.all_bank_instances=false;return p;}
    void retire(){storage_.reset();native_bound_=false;allocation_={};attempted_=true;}
private:
    friend class BattleActorConstruction;
    struct DescriptorRoot {SceneDescriptorLease lease;HsdJointInstanceReceipt identity;};
    struct Storage {std::shared_ptr<const BattleCharacterResource> resource;BattleNormalClassRow klass;StartupActorSetupProfile setup;CharacterModelScene model;StartupActorMaterial material;CharacterMotionBank motion;HsdRecordAnimationBank bank;HsdJointInstanceBank instances;std::shared_ptr<const BattleBankMaterialContext> bank_material=std::make_shared<BattleBankMaterialContext>();std::shared_ptr<const BattleBankInstanceScope> instance_scope;std::weak_ptr<SceneDescriptorPool> descriptor_pool;std::vector<DescriptorRoot> descriptors;bool descriptors_bound=false;};
    bool descriptor_roots_current()const{
        if(!storage_||!storage_->descriptors_bound)return bool(storage_);
        const auto pool=storage_->descriptor_pool.lock();if(!pool||!pool->ready()||storage_->descriptors.size()!=storage_->instances.instances())return false;
        for(const auto& d:storage_->descriptors){const auto* root=pool->root(d.lease);if(!root||!root->receipt().same_instance(d.identity)||!root->same_source(storage_->bank)||pool->material_context(d.lease)!=storage_->bank_material.get())return false;}
        return true;
    }
    static bool fail(std::string* error,const char* why){if(error)*error=std::string("battle character binding: ")+why;return false;}
    bool load(std::shared_ptr<const BattleCharacterResource> data,const BattleNormalClassProfile& classes,ByteView pose,ByteView setup,ByteView fade,std::string* error,const AttachmentAlternateProfile* alternate=nullptr,bool spawn_colour=false){
        storage_.reset();if(!data||!data->ready())return fail(error,"owned resource data required");const auto* selected=classes.find(data->selection().model,data->variant());if(!selected||data->variant()||!data->motion_present())return fail(error,"qualified normal source domain/variant0 and nonempty motion required");
        try{auto next=std::make_unique<Storage>();next->resource=std::move(data);next->klass=*selected;const auto& resource=*next->resource;
            if(challenge_payload_hash(resource.model_hsd())!=selected->model_hash||challenge_payload_hash(resource.actor_data())!=selected->actor_data_hash)return fail(error,"class/resource model-data identity");
            if(!startup_actor_setup_load(setup,selected->model_hash,&next->setup,error)||next->setup.model_id!=selected->model||next->setup.domain!=selected->domain||challenge_payload_hash({next->setup.actor_data.data(),next->setup.actor_data.size()})!=selected->actor_data_hash||challenge_payload_hash({next->setup.class_config.data(),next->setup.class_config.size()})!=selected->config_hash)return fail(error,"class configuration/character-data binding");
            if(alternate){
                const auto policy=selected->domain==BattleModelDomain::InitialAi?HsdStaticMaterialPolicy::DiffuseOverwritePair:HsdStaticMaterialPolicy::Legacy;
                if(!next->bank.load(resource.model_hsd(),error,true,policy)||!next->instances.construct(next->bank,*alternate,error)||!next->model.load_with_record_bank(next->bank,resource.model_prefix(),pose,error))return false;
            }else if(!next->model.load(resource.model_hsd(),resource.model_prefix(),pose,error))return false;
            const auto& rig=next->model.rig();const auto& config=next->setup.class_config;const unsigned bones=unsigned(rig.binding.bones.size());if(rig.lane_mask!=config[2])return fail(error,"class lane mask differs from pose binding");
            for(unsigned i=0;i<3;++i)if(rig.cuts[i+1]!=config[12+i])return fail(error,"class lane cuts differ from pose binding");
            for(unsigned i=0;i<bones;++i)if(rig.order[i]!=config[0x8c+i]||rig.mirror[i]!=config[0x6c+i])return fail(error,"class bone order/mirror differs from pose binding");
            ActorFadeProfile profile;if(!(spawn_colour?actor_fade_load_spawn(fade,&profile,error):actor_fade_load(fade,&profile,error))||!next->material.bind(next->model,next->setup,profile))return fail(error,"shared neutral model-bound material owner");
            if(!character_motion_bank_load(resource.motion_bank(),&next->motion,error))return false;
            if(next->model.ready())return fail(error,"preparation must not publish an actor pose");
            if(alternate){
                const auto* normal=next->instances.state(0);const auto& body=next->model.asset();
                if(!normal||normal->record||normal->style!=HsdJointInstanceStyle::Normal||normal->joints.size()!=bones||normal->mesh_properties.size()!=body.scene.meshes.size())return fail(error,"body/full-bank record0 shape mismatch");
                for(const auto& joint:body.scene.joints){const auto found=std::find_if(normal->joints.begin(),normal->joints.end(),[&](const auto& j){return j.joint.archive_offset==joint.archive_offset;});if(found==normal->joints.end()||found->joint.flags!=joint.flags)return fail(error,"body/full-bank source joint mismatch");}
                for(unsigned i=0;i<body.scene.meshes.size();++i){const auto* source=next->instances.source_mesh(normal->mesh_properties[i].first);if(!source||source->pobj_offset!=body.scene.meshes[i].pobj_offset||source->dobj_offset!=body.scene.meshes[i].dobj_offset||source->deformation_required!=body.scene.meshes[i].deformation_required)return fail(error,"body/full-bank source mesh mismatch");}
                next->instance_scope=std::make_shared<const BattleBankInstanceScope>(BattleBankInstanceScope{next->resource,next->model.instance_binding()});
            }
            storage_=std::move(next);if(error)error->clear();return true;
        }catch(const std::exception&){storage_.reset();return fail(error,"uncertain preparation/allocation; no replay");}
    }
    std::unique_ptr<Storage> storage_;ChallengeCharacterDescriptorReceipt allocation_;bool attempted_=false,native_bound_=false;
};
} // namespace gf
