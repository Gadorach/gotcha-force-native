#pragma once
#include "engine/gameplay/native_gred_neutral_b_trial.hpp"
#include "engine/gameplay/scene_physical_pools.hpp"
#include "engine/gameplay/borgs/normal_gred_beam_transform.hpp"
#include "engine/gameplay/borgs/normal_gred_beam_art.hpp"
#include "engine/gameplay/borgs/normal_gred_beam_shape_tail.hpp"
#include "engine/gameplay/collider_sphere_shape.hpp"

namespace gf {
enum class GRedClass3TrialStatus {AwaitPaidFrame,Failed,NoDescriptors,NoCarrier,Allocated,HeaderInstalled,Released};
enum class GRedTrialShapeStatus {NotAttempted,Failed,NoSlot,Constructed};
struct GRedTrialLaunch {
    GRedBeamMuzzlePlan muzzle;
    GRedBeamFlightSample flight;
    GRedBeamScalePlan scale;
};

// Opt-in native lab bridge. Owns a class-3 slot and its constructor fields,
// with a limited class-4 construction and launch visit. It does not
// execute shape geometry, register a renderer callback or join collision.
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
        bound_=true;trial_=owner;pool_=pool;shape_kind_=recipe->class2_profile().kind(49);return true;
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
    bool initialize_trial_shape_once(const NativeGRedNeutralBTrial& source,const NativeBattleMvp& world,
                                     const NativeBattleMvpView& view,std::string* error){
        const auto pool=pool_.lock();
        const auto* birth=installed_trial_birth(source,world,view);
        if(!pool||!birth||world.paused()||!source.current(world)||shape_status_!=GRedTrialShapeStatus::NotAttempted||
           shape_kind_!=26||!source.attack_profile_||!source.attack_profile_->ready())
            return refuse(error,"current trial carrier and credited shape profile required");
        constexpr NormalGRedNeutralBSource red{0x0615,0,0};
        GRedBeamInitPlan seed;
        GRedBeamShapeOperands operands;
        // Phase 1 here is the accepted stationary trial caster, not an
        // inference about an MVP actor's source constructor or lifecycle.
        if(!normal_gred_beam_first_dispatch(red,factory_,1,world.actors()[0].model,GRedBeamPoolSample::Unobserved,&seed)||
           !normal_gred_beam_shape_operands(red,seed,source.attack_profile_->bank(),&operands)||
           !normal_gred_beam_shape_scope(red,operands))return refuse(error,"shape initialization scope");
        shape_status_=GRedTrialShapeStatus::Failed;
        // Source phase/life seed precedes the real child allocation. There
        // are still no reads of key 7, key 0 or the carrier's +114.
        init_=seed;
        const auto result=pool->acquire_collision(operands.call.group,operands.call.append,
            SceneObjectParentFields{0,s8(birth->credit90_slot)},&shape_,error);
        if(result!=SceneObjectAcquire::Allocated&&result!=SceneObjectAcquire::NoObjects)return false;
        const bool allocated=result==SceneObjectAcquire::Allocated;
        if(!normal_gred_beam_first_dispatch(red,factory_,1,world.actors()[0].model,allocated?
           GRedBeamPoolSample::Allocated:GRedBeamPoolSample::Refused,&init_))return refuse(error,"shape outcome mismatch");
        if(allocated&&!pool->install_trial_beam_shape(lease_,shape_))return refuse(error,"class4 child ownership refused");
        shape_status_=allocated?GRedTrialShapeStatus::Constructed:GRedTrialShapeStatus::NoSlot;
        if(error)error->clear();
        return true;
    }
    GRedTrialShapeStatus shape_status()const{return shape_status_;}
    const SceneCollisionLease& shape_lease()const{return shape_;}
    const GRedBeamInitPlan* trial_shape_init(const NativeGRedNeutralBTrial& source,
        const NativeBattleMvp& world,const NativeBattleMvpView& view)const{
        return (shape_status_==GRedTrialShapeStatus::Constructed||shape_status_==GRedTrialShapeStatus::NoSlot)&&
            installed_trial_birth(source,world,view)?&init_:nullptr;
    }
    ByteView trial_shape_hit_row(const NativeGRedNeutralBTrial& source,const NativeBattleMvp& world,
                                  const NativeBattleMvpView& view)const{
        const auto pool=pool_.lock();
        if(!pool||!pool->trial_beam_shape(shape_)||!trial_shape_init(source,world,view)||!source.attack_profile_)return {};
        return source.attack_profile_->hit_row();
    }
    bool seed_trial_muzzle_once(const NativeGRedNeutralBTrial& source,const NativeBattleMvp& world,
                                const NativeBattleMvpView& view,std::string* error){
        const auto pool=pool_.lock();
        const auto* birth=installed_trial_birth(source,world,view);
        if(!pool||!birth||world.paused()||!source.current(world)||launch_attempted_||
           shape_status_!=GRedTrialShapeStatus::Constructed||init_.route!=GRedBeamInitRoute::MuzzleBranch||
           !pool->trial_beam_shape(shape_)||!source.paid_)
            return refuse(error,"current paid pose and constructed class4 child required before muzzle reads");
        launch_attempted_=true;
        NativeGRedPracticeWorldMatrices keys;
        if(!view.observe_gred_source_world(world,&keys)||keys.tick!=world.tick())
            return refuse(error,"same-update parent and credit matrices required");
        constexpr NormalGRedNeutralBSource red{0x0615,0,0};
        const GRedBeamNoTargetBasis basis{birth->parent8c_slot,birth->credit90_slot,keys.key7,keys.key0};
        GRedTrialLaunch next;
        GRedBeamTransformPlan matrix;
        // Fresh source record has zero matrix storage. A nondegenerate
        // launch must build a basis, not retain or invent a fallback.
        const Mat4 empty114{{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1}};
        if(!normal_gred_beam_no_target_muzzle(red,*source.paid_,birth->targets_c8_cc_null,basis,&next.muzzle)||
           !normal_gred_beam_flight_seed(red,next.muzzle,&next.flight)||
           !normal_gred_beam_scale_seed(red,birth->copied.scale_b4,next.flight.distance180,&next.scale)||
           !normal_gred_beam_transform(red,next.flight,next.scale,empty114,&matrix)||
           matrix.basis==GRedBeamBasisRoute::Retained)return refuse(error,"ordinary launch operands refused");
        std::array<f32,12> frame;
        std::copy_n(matrix.frame114.m,frame.size(),frame.begin());
        if(!pool->publish_trial_beam_frame(lease_,shape_,frame))return refuse(error,"carrier matrix publication refused");
        launch_=next;
        if(error)error->clear();
        return true;
    }
    const GRedTrialLaunch* trial_launch(const NativeGRedNeutralBTrial& source,const NativeBattleMvp& world,
                                       const NativeBattleMvpView& view)const{
        const auto pool=pool_.lock();
        const auto* models=pool?pool->class3_models(lease_):nullptr;
        if(!launch_||!models||!models->trial_frame114||!pool->trial_beam_shape(shape_)||
           !installed_trial_birth(source,world,view))return nullptr;
        return &*launch_;
    }
    // Draw the exact launch pose through the shared model renderer. This is
    // an opt-in observation, not source callback registration or flight.
    bool prepare_trial_beam_art(const NativeGRedNeutralBTrial& source,const NativeBattleMvp& world,
                                const NativeBattleMvpView& view,const NormalGRedBeamArtBank& bank,
                                std::string* error){
        if(beam_art_.ready()||!bank.ready()||!world_frame_current(source,world,view))
            return refuse(error,"current launch and qualified common effect art required");
        const auto pool=pool_.lock();
        const auto* models=pool?pool->class3_models(lease_):nullptr;
        if(!models||!models->trial_frame114||models->descriptor_count99!=1)
            return refuse(error,"current carrier descriptor and matrix required");
        Mat4 frame;
        std::copy_n(models->trial_frame114->begin(),12,frame.m);
        constexpr NormalGRedNeutralBSource red{0x0615,0,0};
        AnimatedModelScene next;
        if(!bank.prepare(red,birth_->copied.team88,&next,error)||!next.set_root_world(frame,error))return false;
        next.set_enabled(true);beam_art_=std::move(next);
        if(error)error->clear();
        return true;
    }
    unsigned draw_trial_beam(const NativeGRedNeutralBTrial& source,const NativeBattleMvp& world,
                             const NativeBattleMvpView& view,RenderEngine& render,const Camera& camera){
        if(!beam_art_.ready()||!world_frame_current(source,world,view))return 0;
        return beam_art_.draw(render,camera);
    }
    bool initialize_trial_sphere_once(const NativeGRedNeutralBTrial& source,const NativeBattleMvp& world,
                                       const NativeBattleMvpView& view,std::string* error){
        const auto pool=pool_.lock();
        const auto* launch=trial_launch(source,world,view);
        if(!pool||!launch||world.paused()||sphere_attempted_||!source.current(world)||!source.attack_profile_)
            return refuse(error,"current carrier frame and child required before sphere init");
        const auto* shape=pool->trial_beam_shape(shape_);
        const auto* models=pool->class3_models(lease_);
        if(!shape||!models||!models->trial_frame114||shape->carrier_matrix_offset60!=0x114||
           shape->descriptor_offset28!=0xf50||shape->credit_slot24!=0)return refuse(error,"exact beam sphere binding required");
        sphere_attempted_=true;
        CharacterCollisionBank bank;
        CharacterCollisionDescriptor descriptor;
        Mat4 frame;
        std::copy_n(models->trial_frame114->begin(),12,frame.m);
        ColliderSphereSeed next;
        if(!bank.load(source.attack_profile_->bank(),error)||!bank.descriptor(CharacterCollisionTable::Attack,15,&descriptor,error)||
           descriptor.flags2!=1||!collider_sphere_seed(descriptor,frame,launch->scale.physical_c0,&next)||
           !next.sweep||next.previous_radius8c)return refuse(error,"exact flag-1 sphere seed refused");
        if(!pool->publish_trial_beam_sphere(lease_,shape_,next))return refuse(error,"class4 sphere publication refused");
        if(error)error->clear();
        return true;
    }
    const ColliderSphereSeed* trial_sphere(const NativeGRedNeutralBTrial& source,const NativeBattleMvp& world,
                                          const NativeBattleMvpView& view)const{
        const auto pool=pool_.lock();
        const auto* shape=pool?pool->trial_beam_shape(shape_):nullptr;
        return shape&&shape->sphere_seed&&trial_launch(source,world,view)?&*shape->sphere_seed:nullptr;
    }
    bool finish_trial_shape_activation_once(NativeGRedNeutralBTrial& source,const NativeBattleMvp& world,
                                            const NativeBattleMvpView& view,const ColliderGeometryProfile& geometry,
                                            std::string* error){
        const auto pool=pool_.lock();
        if(!pool||world.paused()||activation_attempted_||!source.current(world)||!trial_sphere(source,world,view)||
           !collider_geometry_profile_valid(geometry))
            return refuse(error,"current sphere and qualified phase-0 geometry profile required");
        const auto* record=pool->inspect(lease_);
        const auto* models=pool->class3_models(lease_);
        if(!record||!models||!models->trial_frame114||models->trial_token26c||
           source.beam_activation_count404_!=0||!source.attack_profile_)return refuse(error,"fresh activation fields required");
        activation_attempted_=true;
        constexpr NormalGRedNeutralBSource red{0x0615,0,0};
        GRedBeamInitPlan seed;
        GRedBeamShapeOperands operands;
        if(!normal_gred_beam_first_dispatch(red,factory_,1,world.actors()[0].model,GRedBeamPoolSample::Unobserved,&seed)||
           !normal_gred_beam_shape_operands(red,seed,source.attack_profile_->bank(),&operands))
            return refuse(error,"exact credited shape scope required");
        Mat4 frame;
        std::copy_n(models->trial_frame114->begin(),12,frame.m);
        GRedBeamShapeTailInputs in;
        in.activation.scene_token=pool->storage_->trial_scene_token;
        in.activation.credit_count404=source.beam_activation_count404_;
        in.after_shape0_init=in.null_copied_target_cc=true;
        // This trial starts with clear suppression/disable/freeze and credit
        // timer fields; they are not inferred for a live battle actor.
        in.carrier.lifecycle_declared=true;
        in.carrier.registration={true,true,record->active00,record->kind83};
        in.credit={true,true,0,0};
        GRedBeamShapeTailPlan next;
        if(!normal_gred_beam_shape_tail(red,operands,in,geometry,frame,&next,error)||!next.activation_write||
           !pool->publish_trial_beam_activation(lease_,shape_,in.activation.scene_token,next.activation.scene_token,next.phase10))
            return refuse(error,"class4 phase and carrier token publication refused");
        source.beam_activation_count404_=next.activation.credit_count404;
        activation_=next;
        if(error)error->clear();
        return true;
    }
    const GRedBeamShapeTailPlan* trial_shape_activation(const NativeGRedNeutralBTrial& source,const NativeBattleMvp& world,
                                                      const NativeBattleMvpView& view)const{
        return activation_&&trial_sphere(source,world,view)?&*activation_:nullptr;
    }
    bool publish_trial_attack_registry_once(const NativeGRedNeutralBTrial& source,const NativeBattleMvp& world,
                                             const NativeBattleMvpView& view,ScenePhysicalPools& physical,
                                             std::string* error){
        const auto pool=pool_.lock();
        const auto* activated=trial_shape_activation(source,world,view);
        if(!pool||pool!=physical.class2_pool()||world.paused()||registry_attempted_||!source.current(world)||
           !activated||!activated->registration_sampled||activated->degenerate||
           activated->membership.lists!=(1u<<unsigned(CollisionRegistrationList::L477C)))
            return refuse(error,"current trial attack membership and matching arena required");
        registry_attempted_=true;
        if(!physical.publish_trial_attack_registry(shape_))return refuse(error,"isolated registry publication refused");
        registry_published_=true;
        if(error)error->clear();
        return true;
    }
    // Read-only: callers cannot substitute a copied numeric slot or use
    // this inspection to infer the receiver list or admit a hit.
    std::optional<SceneCollisionLease> trial_attack_entry(const NativeGRedNeutralBTrial& source,const NativeBattleMvp& world,
                                                          const NativeBattleMvpView& view,const ScenePhysicalPools& physical)const{
        const auto pool=pool_.lock();
        if(!registry_published_||!pool||pool!=physical.class2_pool()||!source.current(world)||
           !trial_shape_activation(source,world,view))return {};
        const auto entry=physical.current_trial_attack_registry();
        if(!entry||entry->index()!=shape_.index()||entry->generation()!=shape_.generation())return {};
        return entry;
    }
    bool release_uninitialized(std::string* error){
        const auto pool=pool_.lock();
        if(!pool||status_!=GRedClass3TrialStatus::Allocated)return refuse(error,"no rootless allocation to release");
        status_=GRedClass3TrialStatus::Failed;factory_={};
        if(!pool->release(lease_,error))return false;
        status_=GRedClass3TrialStatus::Released;return true;
    }
private:
    bool world_frame_current(const NativeGRedNeutralBTrial& source,const NativeBattleMvp& world,
                             const NativeBattleMvpView& view)const{
        return source.current(world)&&trial_launch(source,world,view);
    }
    static bool refuse(std::string* error,const char* why){
        if(error)*error=std::string("G-Red class3 trial: ")+why;
        return false;
    }
    bool bound_=false,header_attempted_=false,launch_attempted_=false,sphere_attempted_=false,activation_attempted_=false;
    bool registry_attempted_=false,registry_published_=false;
    u8 shape_kind_=255;
    GRedClass3TrialStatus status_=GRedClass3TrialStatus::AwaitPaidFrame;
    GRedTrialShapeStatus shape_status_=GRedTrialShapeStatus::NotAttempted;
    std::weak_ptr<const NativeBorgMotionProducer> trial_;
    std::weak_ptr<SceneObjectPool> pool_;
    SceneClass3Lease lease_;
    SceneCollisionLease shape_;
    GRedBeamFactoryPlan factory_;
    GRedBeamInitPlan init_;
    NativeGRedTrialBeamHandoff handoff_;
    std::optional<GRedBeamFactoryBirthFields> birth_;
    std::optional<GRedTrialLaunch> launch_;
    std::optional<GRedBeamShapeTailPlan> activation_;
    AnimatedModelScene beam_art_;
};
}
