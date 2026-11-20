#pragma once
#include "engine/gameplay/native_gred_neutral_b_trial.hpp"
#include "engine/gameplay/scene_physical_pools.hpp"

namespace gf {
enum class GRedClass3TrialStatus {AwaitPaidFrame,Failed,NoDescriptors,NoCarrier,Allocated,HeaderInstalled,Released};

// Opt-in native lab bridge. Owns a class-3 slot and its constructor fields,
// but does not dispatch an initializer, create a child or register a callback.
class NativeGRedClass3TrialBridge {
public:
    NativeGRedClass3TrialBridge()=default;
    NativeGRedClass3TrialBridge(const NativeGRedClass3TrialBridge&)=delete;
    NativeGRedClass3TrialBridge& operator=(const NativeGRedClass3TrialBridge&)=delete;
    bool bind(const NativeGRedNeutralBTrial& trial,const NativeBattleMvp& world,const ScenePhysicalPools& physical){
        if(bound_||world.tick())return false;
        const auto owner=trial.allocation_trial_owner(world);
        const auto pool=physical.class2_pool();
        const auto* recipe=physical.recipe();
        if(!owner||!pool||!recipe||recipe->class2_profile().kind(9)!=16||
           !pool->class3_snapshot()||!pool->collision_snapshot())return false;
        bound_=true;trial_=owner;pool_=pool;return true;
    }
    // True means an exact paid attempt got a known allocator outcome,
    // including ordinary exhaustion. Once checked out it cannot be retried.
    bool try_once(NativeGRedNeutralBTrial& source,const NativeBattleMvp& world,const NativeBattleMvpView& view,
                  std::string* error){
        const auto pool=pool_.lock();
        const auto owner=trial_.lock();
        if(!bound_||status_!=GRedClass3TrialStatus::AwaitPaidFrame||!owner||owner!=source.allocation_trial_owner(world)||
           !pool||!pool->class3_snapshot())return refuse(error,"current trial and pool required");
        NativeGRedNeutralBTrialReadout paid;
        NativeGRedTrialBeamHandoff handoff;
        if(!source.observe(world,&paid)||!paid.paid||!source.take_beam_handoff(world,view,&handoff))
            return refuse(error,"current published paid frame required");
        // Past checkout any refusal is terminal; do not refund stock, read
        // actor matrices, or attempt class 4 without a constructor.
        status_=GRedClass3TrialStatus::Failed;
        GRedBeamPoolCall call;
        constexpr NormalGRedNeutralBSource red{0x0615,0,0};
        if(!handoff.inspect(world,view,&call)||call!=GRedBeamPoolCall{3,8,9,16,1,false})
            return refuse(error,"exact class3 operands required");
        using A=SceneObjectAcquire;
        const auto result=pool->acquire_class3(call.resolved_group,call.append,call.descriptors,&lease_,error);
        if(result==A::Refused)return false;
        const bool allocated=result==A::Allocated;
        GRedBeamFactoryPlan factory;
        if(!normal_gred_beam_factory_plan(red,*paid.paid,true,allocated?GRedBeamPoolSample::Allocated:
                                          GRedBeamPoolSample::Refused,&factory))return refuse(error,"factory outcome mismatch");
        if(!allocated){
            status_=result==A::NoDescriptors?GRedClass3TrialStatus::NoDescriptors:GRedClass3TrialStatus::NoCarrier;
        }else{
            const auto* record=pool->inspect(lease_);
            const auto* models=pool->class3_models(lease_);
            if(!record||!models||record->class02!=3||record->kind83!=call.kind83||record->group01!=call.resolved_group||
               record->active00||models->header_installed||models->descriptor_count99!=call.descriptors)
                return refuse(error,"allocated record does not match the rootless factory");
            handoff_=std::move(handoff);
            status_=GRedClass3TrialStatus::Allocated;
        }
        factory_=factory;
        if(error)error->clear();
        return true;
    }
    GRedClass3TrialStatus status()const{return status_;}
    // Read-only inspection; the factory plan is not an installed header.
    const SceneClass3Lease& lease()const{return lease_;}
    const GRedBeamFactoryPlan& factory()const{return factory_;}
    bool install_trial_header(const NativeGRedNeutralBTrial& source,const NativeBattleMvp& world,
                              const NativeBattleMvpView& view,std::string* error){
        const auto pool=pool_.lock();
        const auto owner=trial_.lock();
        GRedBeamPoolCall call;
        if(!pool||status_!=GRedClass3TrialStatus::Allocated||header_attempted_||!owner||
           owner!=source.allocation_trial_owner(world)||world.paused()||!source.current(world)||
           !handoff_.inspect(world,view,&call)||call!=factory_.call||!source.factory_parent_)
            return refuse(error,"current paid caster, rootless carrier and declared parent fields required");
        constexpr NormalGRedNeutralBSource red{0x0615,0,0};
        GRedBeamFactoryBirthFields birth;
        if(source.factory_parent_->model94!=world.actors()[0].model||
           !normal_gred_beam_factory_birth_fields(red,factory_,*source.factory_parent_,&birth))
            return refuse(error,"exact no-target parent fields required");
        header_attempted_=true;
        if(!pool->install_trial_beam_header(lease_))return refuse(error,"class3 header ownership refused");
        birth_=birth;status_=GRedClass3TrialStatus::HeaderInstalled;
        if(error)error->clear();
        return true;
    }
    // The direct parent and credit are still the bound trial owner, not an
    // inferred pointer from either numeric slot in the copied fields.
    const GRedBeamFactoryBirthFields* installed_trial_birth(const NativeGRedNeutralBTrial& source,
        const NativeBattleMvp& world,const NativeBattleMvpView& view)const{
        const auto pool=pool_.lock();const auto owner=trial_.lock();GRedBeamPoolCall call;
        if(!birth_||status_!=GRedClass3TrialStatus::HeaderInstalled||!pool||!pool->inspect(lease_)||
           !owner||owner!=source.allocation_trial_owner(world)||!handoff_.inspect(world,view,&call))return nullptr;
        return &*birth_;
    }
    bool release_uninitialized(std::string* error){
        const auto pool=pool_.lock();
        if(!pool||status_!=GRedClass3TrialStatus::Allocated)return refuse(error,"no rootless allocation to release");
        status_=GRedClass3TrialStatus::Failed;factory_={};
        if(!pool->release(lease_,error))return false;
        status_=GRedClass3TrialStatus::Released;return true;
    }
private:
    static bool refuse(std::string* error,const char* why){
        if(error)*error=std::string("G-Red class3 trial: ")+why;
        return false;
    }
    bool bound_=false,header_attempted_=false;
    GRedClass3TrialStatus status_=GRedClass3TrialStatus::AwaitPaidFrame;
    std::weak_ptr<const NativeBorgMotionProducer> trial_;
    std::weak_ptr<SceneObjectPool> pool_;
    SceneClass3Lease lease_;
    GRedBeamFactoryPlan factory_;
    NativeGRedTrialBeamHandoff handoff_;
    std::optional<GRedBeamFactoryBirthFields> birth_;
};
}
