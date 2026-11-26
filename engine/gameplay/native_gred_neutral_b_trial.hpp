#pragma once
#include "engine/gameplay/borgs/normal_gred_beam_custody.hpp"
#include "engine/gameplay/borgs/normal_gred_neutral_b_visit.hpp"
#include "engine/gameplay/native_battle_mvp_view.hpp"
#include "engine/gameplay/native_gred_ground_dash_demo.hpp"

namespace gf {
struct NativeGRedNeutralBTrialReadout {
    u32 tick=0;
    WeaponStockModeZeroState stock;
    std::optional<GRedNeutralBReleasePlan> paid;
};
// Inspection of the checked-out practice request. It does not authorize an
// allocation, publish a constructor header or own an allocator result.
class NativeGRedTrialBeamHandoff {
public:
    bool inspect(const NativeBattleMvp& world,const NativeBattleMvpView& view,GRedBeamPoolCall* out)const{
        u64 observed=0;
        if(!out||!publication_||!view.source_pose_publication(world,frame_,&observed)||observed!=publication_)return false;
        *out=call_;return true;
    }
private:
    friend class NativeGRedNeutralBTrial;
    friend class NativeGRedClass3TrialBridge;
    NativeBorgSourcePose frame_;
    GRedBeamPoolCall call_;
    u64 publication_=0;
};

// A single stationary, explicitly no-target practice shot through the actual
// native movement acceptance and shared renderer. It does not admit combat,
// resolve a target, service a reload/pose exit, or take a pool allocation.
class NativeGRedNeutralBTrial {
    friend class NativeGRedClass3TrialBridge;
    static constexpr NormalGRedNeutralBSource source_{0x0615,0,0};
    struct Pending {
        CharacterProgramVm vm;
        NativeBorgMotionOffer offer;
        std::optional<GRedNeutralBReleasePlan> release;
        f32 aim_clock=30;
    };
public:
    bool begin(IFileSystem& files,const std::string& art,const std::string& ground_inputs,
               const NativeBattleMvp& world,const NativeBorgAnimationSeed& seed,
               const GRedNeutralBLocalLibrary& library,s16 declared_rounds,std::string* error,
               const GRedBeamFactoryParentFields* declared_factory_parent=nullptr,
               const GRedBeamAttackProfile* attack_profile=nullptr){
        retire();
        if(!library.ready()||(attack_profile&&!attack_profile->ready())||declared_rounds<1||declared_rounds>5||world.tick()||
           world.shots_spawned()||world.hits()||!stationary_duel(world))return fail(error,"fresh stationary world and declared trial stock required");
        library_=library;
        const auto& rig=*library_.driver().rig();
        if(rig.lane_mask!=seed.rig.lane_mask||rig.cuts!=seed.rig.cuts||
           rig.order!=seed.rig.order||rig.mirror!=seed.rig.mirror||
           !ground_.bind(files,art,ground_inputs,world,seed,error,NativeGRedGroundDashDemo::Mode::Ground))return fail(error,"current shared movement/renderer binding required");
        const auto& height=*library_.height();
        CharacterPoseHeightFields fields;fields.scale_b4=1;
        std::vector<CharacterProgramOperation> ops;
        if(!vm_.load_fresh_actor_directional_controls(*library_.program(),{0,1,height.zero},error)||
           !controls_.bind(vm_,library_.driver(),height,fields,{},CharacterProgramDirectionalControlMode::DeclaredHeightFields,error)||
           !vm_.request_with_actor_controls(0,0,3,-1,1,library_.driver(),controls_,&ops,error,{0,0}))
            return fail(error,"exact local idle pose seed required");
        for(unsigned i=0;i<2;++i){
            const auto& lane=vm_.state().lanes[i];
            const auto* selected=controls_.snapshot(lane.motion);
            if(!selected||selected->lane()!=i||lane.program||lane.descriptor_group||lane.descriptor_choice)
                return fail(error,"exact local idle lanes required");
        }
        stock_={declared_rounds,declared_rounds,180,0};
        // MVP does not carry these source offsets. Keep an explicit owned
        // lab sample; never infer one from the render matrices or the slot.
        if(declared_factory_parent)factory_parent_=*declared_factory_parent;
        if(attack_profile)attack_profile_=*attack_profile;
        initial_=world.actors();ready_=true;if(error)error->clear();return true;
    }
    bool prepare(const NativeBattleMvp& world,const std::weak_ptr<const CharacterModelBinding>& binding,
                 const NativeBorgMovementFrame& camera,NativeBorgMotionOffer* out,std::string* error){
        if(!out||!ready_||pending_||paid_||!current(world)||world.paused())return refuse(error,"trial visit unavailable");
        Pending next;
        CharacterLocalPose local;
        if(!opening_){
            WeaponStockModeZeroPlan available;
            GRedNeutralBStartup startup;
            const GRedNeutralBChoice ordinary{GRedNeutralBRoute::Shot,0,44,0};
            next.vm=vm_;std::vector<CharacterProgramOperation> ops;
            if(!normal_gred_neutral_b_availability(source_,ordinary,stock_,&available)||!available.accepted||
               !normal_gred_neutral_b_start(source_,true,0,&startup)||
               !next.vm.request_with_actor_controls(startup.group,startup.program,u8(startup.mask&3),-1,1,
                                                     library_.driver(),controls_,&ops,error)||
               !controls_.compose_local_pose(next.vm.state(),&local,error))
                return fail(error,"ordinary shot opening");
            next.aim_clock=startup.aim_clock560;
        }else{
            s16 heading=0;
            GRedNeutralBVisitPlan visit;
            const auto facing=world.actors()[0].facing;
            if(!source_heading_s16(facing.x,facing.z,&heading)||
               !normal_gred_neutral_b_phase_one_visit(source_,library_,vm_,controls_,{1,0,true,heading,1,aim_clock_},&visit,error))
                return fail(error,"ordinary shot visit");
            next.vm=visit.vm();local=visit.pose();next.aim_clock=visit.gate().aim_clock560;
            if(visit.gate().release_due){
                GRedNeutralBReleasePlan paid;
                if(!visit.raised_cue()||!visit.propose_debit(stock_,&paid)||!paid.request_beam)
                    return fail(error,"newly due paid shot required");
                next.release=paid;
            }
        }
        if(!ground_.prepare(world,binding,camera,0,0,0,&next.offer,error)||
           next.offer.kind!=NativeBorgMotionKind::Ground||next.offer.action!=BattleMvpAction::Idle||
           next.offer.proposed_position!=next.offer.observed.position||next.offer.facing!=next.offer.observed.facing)
            return fail(error,"stationary movement offer required");
        // Only this coordinator can substitute the exact shot candidate on
        // its private stationary offer. The existing movement owner's commit
        // seals the local data into the matching native frame.
        ground_.pending_->local=std::move(local);
        *out=next.offer;pending_=std::move(next);if(error)error->clear();return true;
    }
    bool commit(const NativeBattleMvp& world,const std::weak_ptr<const CharacterModelBinding>& binding,std::string* error){
        if(!ready_||!pending_)return refuse(error,"no trial visit pending");
        NativeBorgMotionReceipt accepted;
        if(!current(world)||!world.motion_receipt(pending_->offer,&accepted)||
           accepted.contact().position!=pending_->offer.observed.position||accepted.contact().body_hit||
           accepted.contact().terrain_blocked||!ground_.commit(world,binding,error))
            return fail(error,"matching stationary native acceptance required");
        vm_=std::move(pending_->vm);aim_clock_=pending_->aim_clock;opening_=true;
        paid_=pending_->release;
        if(paid_)stock_=paid_->stock;
        pending_.reset();if(error)error->clear();return true;
    }
    bool pose(const NativeBattleMvp& world,NativeBorgSourcePose* out)const{return ready_&&ground_.pose(world,out);}
    bool observe(const NativeBattleMvp& world,NativeGRedNeutralBTrialReadout* out)const{
        if(!out||!ready_)return false;
        NativeBorgSourcePose frame;
        if(!ground_.pose(world,&frame))return false;
        *out={frame.receipt.tick(),stock_,paid_};return true;
    }
    bool take_beam_handoff(const NativeBattleMvp& world,const NativeBattleMvpView& view,NativeGRedTrialBeamHandoff* out){
        if(!out||!ready_||handoff_taken_||!paid_||world.paused()||!current(world))return false;
        NativeGRedTrialBeamHandoff next;
        if(!ground_.pose(world,&next.frame_)||!view.source_pose_publication(world,next.frame_,&next.publication_))return false;
        GRedBeamFactoryPlan request;
        // Keep the source order: carrier request first, then shape allocation,
        // then matrix reads. An absent allocator is not a successful result.
        if(!normal_gred_beam_factory_plan(source_,*paid_,true,GRedBeamPoolSample::Unobserved,&request)||request.propose_header)
            return false;
        next.call_=request.call;handoff_taken_=true;
        *out=std::move(next);return true;
    }
    void retire(){ready_=opening_=handoff_taken_=false;ground_.reject();pending_.reset();paid_.reset();factory_parent_.reset();attack_profile_.reset();beam_activation_count404_=0;}
private:
    std::shared_ptr<const NativeBorgMotionProducer> allocation_trial_owner(const NativeBattleMvp& world)const{
        return ready_&&world.owns_motion_scope(ground_.world_scope_)?ground_.producer_:nullptr;
    }
    static bool stable(const BattleMvpActor& actor){
        return actor.health&&actor.grounded&&!actor.movement_speed&&!actor.vertical_speed&&
            !actor.attack_ticks&&!actor.hit_flash&&!actor.dash_ticks&&!actor.upper_body_yaw;
    }
    static bool stationary_duel(const NativeBattleMvp& world){
        return world.ready()&&world.result()==BattleMvpResult::Fighting&&world.actors()[0].model==source_.model&&
            world.actors()[1].model==0x000a&&stable(world.actors()[0])&&stable(world.actors()[1]);
    }
    bool current(const NativeBattleMvp& world)const{
        if(!stationary_duel(world)||world.shots_spawned()||world.hits())return false;
        for(unsigned i=0;i<2;++i){
            const auto& a=world.actors()[i];const auto& b=initial_[i];
            if(a.position!=b.position||a.facing!=b.facing||a.health!=b.health)return false;
        }
        return true;
    }
    std::array<const StihGrid*,3> trial_stage_grids()const{
        return {&ground_.grids_[0],&ground_.grids_[1],&ground_.grids_[2]};
    }
    static bool refuse(std::string* error,const char* why){
        if(error)*error=std::string("G-Red neutral-B trial: ")+why;
        return false;
    }
    bool fail(std::string* error,const char* why){retire();return refuse(error,why);}
    NativeGRedGroundDashDemo ground_;
    GRedNeutralBLocalLibrary library_;
    CharacterProgramVm vm_;
    CharacterProgramDirectionalControl controls_;
    std::array<BattleMvpActor,kBattleMvpMaxActors> initial_;
    WeaponStockModeZeroState stock_;
    std::optional<Pending> pending_;
    std::optional<GRedNeutralBReleasePlan> paid_;
    std::optional<GRedBeamFactoryParentFields> factory_parent_;
    std::optional<GRedBeamAttackProfile> attack_profile_;
    u32 beam_activation_count404_=0; // fresh trial only; not a hit or stock counter
    f32 aim_clock_=30;
    bool ready_=false,opening_=false,handoff_taken_=false;
};
}
