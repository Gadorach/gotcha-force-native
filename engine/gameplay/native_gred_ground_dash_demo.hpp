#pragma once
//Movement-only native harness around recovered G-Red ground components.
//Native collider/target proxies are explicit approximations, NOT original
//actor construction, weapon/auxiliary callbacks or whole-frame admission.
#include <memory>
#include "engine/gameplay/native_battle_mvp_borg.hpp"
#include "engine/gameplay/character_motion_driver.hpp"
#include "engine/gameplay/character_program_height_control.hpp"
#include "engine/gameplay/gred_movement_admission.hpp"
#include "engine/gameplay/gred_ground_dash.hpp"
#include "engine/gameplay/gred_ground_walk.hpp"
#include "engine/gameplay/native_gred_ground_input.hpp"
#include "engine/gameplay/gred_vertical_jump.hpp"
#include "engine/gameplay/gred_air_dash.hpp"
#include "engine/gameplay/gred_air_dash_contact.hpp"
#include "engine/gameplay/gred_relaunch.hpp"
#include "engine/gameplay/gred_surface_contact.hpp"
#include "engine/gameplay/character_movement_motion.hpp"
namespace gf {
class NativeGRedGroundDashDemo {
    struct State {
        CharacterProgramVm vm;CharacterProgramHeightControl height;CharacterMovementInputState input;
        CharacterAirResourceState resource;CharacterMovementMotionState motion;GRedGroundDashState dash;
        GRedDashSteering steering;CharacterMovementTurn turn;s16 body=0;f32 previous_y=0;
        u8 walk_selector=0;bool walk_entered=false;
        GRedVerticalJumpState jump;
        GRedAirDashState air_dash;
        GRedRelaunchState relaunch;
        s16 contact_angle=-1;
        std::optional<NativeBorgStaticPush> static_push{};
        std::optional<NativeBorgRelaunchFloor> relaunch_floor{};
        std::optional<NativeBorgAirDashFloor> air_dash_floor{};
    };
    struct Pending {State state;NativeBorgMotionOffer offer;CharacterLocalPose local;u8 interrupted_dash=0,landing_target=0;};
public:
    enum class Mode {DashOnly,Ground,VerticalJump,Jump,AirDash,Relaunch,DashJump,LandingInterrupt,Surface,Steep,FallingSurface,RecoverySurface,WindupSurface,ActiveSurface,RelaunchSurface,RelaunchEntrySurface,RelaunchFloor,AirDashFloor,WindupEntryFloor,WindupPositiveFloor,WindupEntryPush};
    NativeGRedGroundDashDemo()=default;
    NativeGRedGroundDashDemo(const NativeGRedGroundDashDemo&)=delete;
    NativeGRedGroundDashDemo& operator=(const NativeGRedGroundDashDemo&)=delete;
    bool bind(IFileSystem& files,const std::string& art,const std::string& inputs,const NativeBattleMvp& world,
              const NativeBorgAnimationSeed& seed,std::string* error,Mode mode=Mode::DashOnly){
        retire();NativeBorgMotionObservation observed;
        mode_=mode;
        const auto binding=seed.binding.lock();
        if(!world.observe_motion(&observed)||seed.world_scope.lock()!=observed.scope.lock()||seed.tick!=observed.tick||
           seed.position!=observed.position||seed.facing!=observed.facing||!observed.grounded||world.actors()[0].source_ground_motion||
           world.actors()[0].movement_speed||!binding||binding->model_hash!=0xe2b4439au||seed.stage_hash!=0xbdf8e542u||seed.floor_hash!=0x0d913901u)
            return fail(error,"fresh same-world/model/stage pose seed required");
        std::vector<u8> encoded,height,movement,motion,model,motions;
        if(!read(files,inputs+"/locomotion.gfmp",65536,&encoded,error)||!read(files,inputs+"/pose-height.gfhc",28,&height,error)||
           !read(files,inputs+"/movement.gfmi",192,&movement,error)||!read(files,inputs+"/motion.gfmm",364,&motion,error)||
           !read(files,art+"/g-red.hsd",4u<<20,&model,error)||!read(files,art+"/g-red.motions",32u<<20,&motions,error))return false;
        if(character_program_model_hash(view(encoded))!=0x638570ffu||character_program_model_hash(view(model))!=0xe2b4439au||
           !character_program_load_control_flow(view(encoded),0xe2b4439au,&programs_,error)||
           !character_pose_height_profile_load(view(height),&height_,error)||!character_movement_input_load(view(movement),&input_,error)||
           !character_movement_load(view(motion),&motion_,error))return fail(error,"qualified normal source profiles required");
        if(mode_!=Mode::DashOnly&&!motion_.walk_operands)return fail(error,"general ground movement requires GFMM2 walk operands");
        if(moving_mode(mode_)&&!motion_.air_operands)return fail(error,"moving jump requires GFMM3 air operands");
        if(air_dash_mode(mode_)&&!motion_.air_dash_operands)return fail(error,"air dash/relaunch mode requires GFMM4 operands");
        if(surface_mode(mode_)&&!motion_.surface_operands)return fail(error,"source surface contact requires GFMM5 operands");
        if(steep_mode(mode_)&&!motion_.steep_operands)return fail(error,"source steep contact requires GFMM6 operands");
        if(relaunch_floor_mode(mode_)&&!motion_.relaunch_floor_operands)return fail(error,"source null-callback floor contact requires GFMM7 operands");
        CharacterMotionBank bank;if(!character_motion_bank_load(view(motions),&bank,error))return false;
        std::array<std::array<bool,128>,16> needed{};
        for(const auto& descriptor:programs_.descriptors)for(unsigned lane=0;lane<2;++lane){
            const unsigned at=4+lane*8,group=descriptor.bytes[at],choice=descriptor.bytes[at+1];
            if(group>=16||choice>=128)return fail(error,"unreviewed common clip");
            needed[group][choice]=true;
        }
        std::vector<CharacterMotionClipBinding> clips;
        for(unsigned group=0;group<16;++group)for(unsigned choice=0;choice<128;++choice)if(needed[group][choice]){
            CharacterMotionSelection selected;CharacterMotionClipBinding clip;clip.group=u8(group);clip.index=u8(choice);
            if(!character_motion_select(bank,group,choice,&selected,error)||selected.fallback||!hsd_bone_motion_load(selected.archive,seed.rig.binding,&clip.clip,error))return false;
            clips.push_back(std::move(clip));
        }
        if(!driver_.load_frozen_target_transitions(view(model),seed.rig,programs_,clips,seed.local,error))return false;
        constexpr std::array<u32,3> hashes={0x0d913901u,0x01859c01u,0xfe083e8fu};
        const std::array<const char*,3> names={"floor.stih","secondary.stih","tertiary.stih"};
        for(unsigned i=0;i<3;++i){std::vector<u8> bytes;if(!read(files,inputs+"/"+names[i],16u<<20,&bytes,error)||
            character_program_model_hash(view(bytes))!=hashes[i]||!grids_[i].load(view(bytes),error))return fail(error,"qualified static stage473 grids required");}
        state_={};state_.dash.selector=0;state_.motion.position=observed.position;state_.previous_y=observed.position[1];state_.resource.value736=input_.resource_limit;
        if(!source_heading_s16(observed.facing.x,observed.facing.z,&state_.body)||!state_.vm.load_fresh_actor_height_controls(programs_,{},error))return false;
        CharacterPoseHeightFields fields;fields.model_group3e8=6;
        if(!state_.height.bind(state_.vm,height_,fields,{},CharacterProgramHeightControlMode::DeclaredFields,error)||!request(state_,0,error))return false;
        producer_=world.new_motion_producer(mode_==Mode::WindupEntryPush?NativeBorgMotionPolicy::WindupEntryPush:mode_==Mode::WindupPositiveFloor?NativeBorgMotionPolicy::WindupPositiveFloor:mode_==Mode::WindupEntryFloor?NativeBorgMotionPolicy::WindupEntryFloor:mode_==Mode::AirDashFloor?NativeBorgMotionPolicy::AirDashFloor:mode_==Mode::RelaunchFloor?NativeBorgMotionPolicy::RelaunchFloor:mode_==Mode::RelaunchEntrySurface?NativeBorgMotionPolicy::RelaunchEntryPush:mode_==Mode::RelaunchSurface?NativeBorgMotionPolicy::RelaunchPush:mode_==Mode::ActiveSurface?NativeBorgMotionPolicy::ActivePush:mode_==Mode::WindupSurface?NativeBorgMotionPolicy::WindupPush:mode_==Mode::RecoverySurface?NativeBorgMotionPolicy::RecoveryPush:mode_==Mode::FallingSurface?NativeBorgMotionPolicy::FallHistoryPush:mode_==Mode::Steep?NativeBorgMotionPolicy::StaticPush:relaunch_mode(mode_)?NativeBorgMotionPolicy::Relaunch:mode_==Mode::AirDash?NativeBorgMotionPolicy::AirDash:mode_==Mode::Jump?NativeBorgMotionPolicy::Jump:
            mode_==Mode::VerticalJump?NativeBorgMotionPolicy::VerticalJump:NativeBorgMotionPolicy::GroundOnly);
        if(!producer_)return fail(error,"native movement producer lease refused");
        dash_fall_pushes_=relaunch_fall_pushes_=recovery_pushes_=windup_pushes_=active_pushes_=relaunch_surface_pushes_=0;
        relaunch_entry_surface_pushes_=0;
        windup_entry_surface_pushes_=0;
        relaunch_floor_contacts_=0;last_relaunch_floor_programs_={0xffff,0xffff};
        windup_floor_contacts_=active_floor_contacts_=windup_entry_floor_contacts_=positive_windup_floor_contacts_=0;last_air_dash_floor_programs_={0xffff,0xffff};
        last_push_programs_=last_active_push_programs_=last_relaunch_push_programs_={0xffff,0xffff};
        binding_=seed.binding;world_scope_=observed.scope;expected_tick_=observed.tick;accepted_frames_=body_contacts_=air_frames_=landings_=air_dash_starts_=air_dash_active_=relaunches_=dash_jumps_=landing_jumps_=landing_dashes_=surface_landings_=surface_pushes_=0;last_dash_jump_=0;last_surface_angle_=last_push_angle_=0;ready_=true;return true;
    }
    bool prepare(const NativeBattleMvp& world,const std::weak_ptr<const CharacterModelBinding>& current_binding,
                  const NativeBorgMovementFrame& camera,s16 stick_x,s16 stick_y,u32 held,NativeBorgMotionOffer* out,std::string* error){
        if(error)error->clear();
        NativeBorgMotionObservation observed;
        if(!ready_||pending_||!out||!world.observe_motion(&observed)||observed.scope.lock()!=world_scope_.lock()||observed.tick!=expected_tick_||
           current_binding.lock()!=binding_.lock()||binding_.expired()||
           (!observed.grounded&&((mode_!=Mode::VerticalJump&&!moving_mode(mode_))||!state_.jump.selector))||
           (held&~((mode_==Mode::VerticalJump||moving_mode(mode_))?0xf0100u:0xf0000u)))return fail(error,"bound movement frame required; requested feature is not enabled");
        if(mode_==Mode::VerticalJump&&(state_.jump.selector||(held&0x100u))){
            u8 direction=0;
            if(!character_movement_stick_nibble(input_,stick_x,stick_y,input_.low_threshold,&direction)||direction||
               (held&0xf0000u)||state_.walk_selector||state_.dash.selector)return fail(error,"vertical jump requires idle takeoff and neutral airborne direction");
        }
        if(mode_==Mode::Jump&&(state_.jump.selector||(held&0x100u))){
            u8 flick=0;
            if(state_.dash.selector||!character_movement_stick_nibble(input_,stick_x,stick_y,input_.high_threshold,&flick)||flick)
                return fail(error,"dash-to-jump and airborne flick/air-dash callbacks are not enabled");
        }
        if(air_dash_mode(mode_)&&!dash_jump_mode(mode_)&&state_.dash.selector&&(held&0x100u))return fail(error,"dash-to-jump is not enabled");
        auto candidate=std::make_unique<Pending>();candidate->state=state_;auto& s=candidate->state;
        s.motion.position=observed.position;s.motion.flags5e0=s.vm.state().actor.behaviour;s.contact_angle=-1;s.static_push.reset();s.relaunch_floor.reset();s.air_dash_floor.reset();
        auto ground=ground_for(s);
        f32 start_floor=0;
        if(!ground.valid_for(s.motion)||!ground.height(s.motion.position,&start_floor,error)||
           (observed.grounded&&std::abs(start_floor-observed.position[1])>.01f))return fail(error,"source/native current-ground disagreement");
        if(!character_air_resource_recover(s.resource,0,s.motion.flags5e0,s.jump.cached87,input_.resource_limit,input_.recovery_increment,&s.resource))return fail(error,"resource recovery refused");
        if(!character_movement_input_step(input_,s.input,{stick_x,stick_y,held,s.motion.flags5e0},&s.input,error))return fail(error,"unsupported movement input");
        if(moving_mode(mode_)){
            if(s.jump.timer690>0)s.jump.timer690=std::max(0.f,s.jump.timer690-1); //declared native timer768=1
            if(s.air_dash.contact69c>0)s.air_dash.contact69c=std::max(0.f,s.air_dash.contact69c-1);
            if(mode_==Mode::Jump&&s.jump.selector&&(s.input.directions5d8&0xf0u))return fail(error,"buffered air-dash branch is not enabled");
        }
        CharacterMovementHeadingFrame frame;
        if(!native_borg_source_heading_frame(camera,observed.facing,&frame))return fail(error,"native heading observation refused");
        frame.body72=s.body;
        if(air_dash_mode(mode_)&&s.jump.selector){
            GRedMovementAdmissionFields fields;fields.selector=s.jump.selector;fields.flags5e0=s.motion.flags5e0;fields.prior_handlers_declined=true;
            fields.air_ready588=s.jump.air_ready588;GRedMovementAdmission admission;
            if(!gred_movement_admit(input_,s.input,s.resource,fields,&admission,error))return fail(error,"airborne input predicate refused");
            if(admission.changed){
                if(s.jump.selector==7){
                    if(!landing_mode(mode_)||!s.jump.entered||!observed.grounded||s.jump.cached87)
                        return fail(error,"landing interruption needs an accepted landing-entry callback in the enabled mode");
                    if(s.motion.flags5e0&0x81000003u)return fail(error,"landing interruption needs reviewed unblocked upper/lower synchronization");
                    candidate->landing_target=admission.selector;
                    if(admission.selector==4&&admission.reset_direction57c)begin_takeoff(s);
                    else if(admission.selector==12){
                        if(!begin_ground_dash(s,admission,frame,error))return fail(error,"landing dash heading refused");
                    }else return fail(error,"unreviewed landing interruption");
                }else if(admission.selector==15){
                    if(observed.grounded||!s.jump.cached87)return fail(error,"air dash before accepted takeoff is not enabled");
                    CharacterMovementHeading heading;
                    if(!character_movement_heading(input_,motion_,admission.direction58f,CharacterMovementHeadingMode::OctantDash,frame,&heading,error)||!heading.has_direction)return fail(error,"octant air-dash heading refused");
                    if(s.jump.selector==8)s.relaunch={};
                    s.input.directions5d8=admission.remaining_directions;s.air_dash={};
                    s.air_dash.timer568=f32(motion_.turn_counts[source_heading_sector(heading.delta5aa,1)]>>1);
                    s.turn={s.body,heading.delta5aa,0};
                    if(!character_movement_turn_begin(s.turn,s32(s.air_dash.timer568),&s.turn))return fail(error,"windup turn refused");
                    s.jump.selector=15;s.jump.entered=false;s.jump.air_ready588=0;
                }else if(admission.selector==6){
                    if(s.jump.selector==8)s.relaunch={};
                    s.jump.selector=6;s.jump.entered=false;s.jump.air_ready588=0;
                }else if(admission.selector==17){
                    s.air_dash.selector=17;s.air_dash.entered=false;s.air_dash.stage=0;s.air_dash.air_ready588=0;s.air_dash.contact_callback=false;
                    s.jump.selector=17;s.jump.air_ready588=0;
                }else if(relaunch_mode(mode_)&&admission.selector==8){
                    if(observed.grounded||!s.jump.cached87)return fail(error,"relaunch needs accepted airborne state");
                    s.jump.selector=8;s.jump.entered=false;s.jump.air_ready588=0;s.relaunch={};s.air_dash.selector=0;
                }else return fail(error,"air relaunch callback is not enabled");
            }
        }
        if(dash_jump_mode(mode_)&&s.dash.selector&&!candidate->landing_target){
            GRedMovementAdmissionFields fields;fields.selector=s.dash.selector;fields.flags5e0=s.motion.flags5e0;fields.prior_handlers_declined=true;
            GRedMovementAdmission admission;
            if(!gred_movement_admit(input_,s.input,s.resource,fields,&admission,error))return fail(error,"dash interruption input predicate refused");
            if(admission.changed){
                if(admission.selector!=4||!admission.reset_direction57c)return fail(error,"unreviewed ground-dash interruption");
                //59F5C synchronizes upper with the admitted lower selector
                //before55568 captures dispatch state only under this guard.
                //Attacking/blocked upper-lane variants are not acknowledged.
                if(s.motion.flags5e0&0x81000003u)return fail(error,"dash jump needs reviewed unblocked upper/lower synchronization");
                candidate->interrupted_dash=s.dash.selector;
                begin_takeoff(s); //source selection precedes lower/upper dispatch; do NOT advance the old dash first
            }
        }
        bool entry=false;
        NativeBorgMotionKind kind=NativeBorgMotionKind::Ground;
        if(!s.dash.selector&&!s.jump.selector){
            GRedMovementAdmission admission;GRedMovementAdmissionFields fields;fields.prior_handlers_declined=true;fields.flags5e0=s.motion.flags5e0;
            fields.selector=s.walk_selector;
            //Native movement-only harness: no original weapon/auxiliary state is
            //claimed. Unsupported inputs and static-ground mismatches refuse.
            if(!gred_movement_admit(input_,s.input,s.resource,fields,&admission,error))return fail(error,"movement predicate refused");
            if(admission.changed){
                if(admission.selector==12){
                    if(!begin_ground_dash(s,admission,frame,error))return fail(error,"source directional heading refused");
                    entry=true;
                }else if((mode_==Mode::VerticalJump||moving_mode(mode_))&&admission.selector==4){
                    begin_takeoff(s);
                }else if(mode_!=Mode::DashOnly&&admission.selector>=1&&admission.selector<=3){
                    s.walk_selector=admission.selector;s.walk_entered=false;
                }else return fail(error,"source walk/jump callback is not enabled in dash lab");
            }
            if(!s.dash.selector&&!s.jump.selector&&s.walk_selector){
                if(!walk(s,frame,error))return fail(error,"ordinary source walk/stop handler refused");
            }else if(!s.dash.selector&&!s.jump.selector){
                const auto* snapshot=driver_.snapshot(s.vm.state().lanes[1].motion);
                if(!snapshot)return fail(error,"idle source snapshot missing");
                if(snapshot->group()!=0||snapshot->index()!=0){if(!request(s,0,error))return fail(error,"idle program reset refused");}
                else if(!advance(s,error))return fail(error,"idle program update refused");
                s.motion.speed44=s.motion.planar_change4c=0;
            }
        }
        if(s.jump.selector==8){
            if(!relaunch_mode(mode_)||!relaunch(s,held,frame,&kind,error))return fail(error,"source relaunch/early-contact branch refused");
        }else if(s.jump.selector>=15&&s.jump.selector<=17){
            if(!air_dash(s,held,frame,&kind,error)){
                const std::string detail=error?*error:"";fail(error,"source air-dash handler/contact branch refused");
                if(error&&!detail.empty())*error+=" / "+detail;
                return false;
            }
        }else if(s.jump.selector&&!jump(s,held,frame,&kind,error)){
            const std::string detail=error?*error:"";fail(error,"source jump/landing handler refused");
            if(error&&!detail.empty())*error+=" / "+detail;
            return false;
        }
        bool moving=s.dash.selector!=0;
        if(moving){
            GRedGroundDashPlan plan;
            if(!gred_ground_dash_plan(motion_,s.dash,s.motion.speed44,s.motion.planar_change4c,s.vm.state().actor.control1[0],&plan))return fail(error,"unreviewed dash handler branch");
            if(plan.request_program){if(!request(s,plan.program,error))return fail(error,"dash program request refused");}
            else if(plan.advance_program&&!advance(s,error))return fail(error,"dash program update refused");
            s.motion.speed44=plan.speed44;s.motion.planar_change4c=plan.change4c;s.motion.flags5e0=s.vm.state().actor.behaviour;
            s.steering.body72=s.body;s.steering.camera5b0=frame.camera5b0;s.steering.speed44=s.motion.speed44;
            s.steering.direction57c=s.dash.direction_class;s.steering.coarse57d=s.dash.coarse57d;
            //Native target-center proxy for the source horizontal target-hitpoint
            //distance. This is explicit harness policy, not original target ACK.
            const auto& target=world.actors()[1];
            if(!target.health)return fail(error,"live native target required");
            s.steering.distance75c=std::hypot(observed.position[0]-target.position[0],observed.position[2]-target.position[2]);s.steering.distance_declared=true;
            GRedDashSteering steering;
            if(!gred_ground_dash_steer(motion_,s.steering,&steering))return fail(error,"source steering refused");
            s.steering=steering;s.body=steering.body72;s.motion.travel5ae=steering.travel5ae;
            if(plan.move){
                ground=ground_for(s);CharacterMovementMotionState moved;
                if(!character_movement_ground_step(motion_,s.motion,1,ground,&moved,error))return fail(error,"source ground motion refused");
                s.motion=moved;f32 floor=0;if(!ground.height(s.motion.position,&floor,error)||std::abs(floor-s.motion.position[1])>.01f)return fail(error,"airborne/ledge tail is not enabled");
            }
            if(!gred_ground_dash_finish(motion_,plan,s.vm.state().actor.control1[0],s.vm.state().lanes[1].done,held,1,&s.dash))return fail(error,"source dash finish refused");
            if(!s.dash.selector)s.motion.speed44=s.motion.planar_change4c=0; //native harness's common idle reset
        }
        std::array<CharacterProgramMotion,2> motions{s.vm.state().lanes[0].motion,s.vm.state().lanes[1].motion};
        if(!driver_.compose_local_pose(motions,&candidate->local,error))return fail(error,"source pose composition refused");
        f32 x=0,z=0;source_heading_xz(s.body,&x,&z);
        const bool walking=s.walk_selector!=0;
        candidate->offer={observed,s.motion.position,moving||entry||walking||(moving_mode(mode_)&&kind!=NativeBorgMotionKind::Ground)?BattlePoint{x,z}:observed.facing,
            moving||entry?BattleMvpAction::GroundDash:walking?BattleMvpAction::Move:BattleMvpAction::Idle,++serial_};
        candidate->offer.producer=producer_;
        candidate->offer.kind=kind;candidate->offer.vertical_speed=s.motion.vertical48;
        candidate->offer.static_push=s.static_push;
        candidate->offer.relaunch_floor=s.relaunch_floor;
        candidate->offer.air_dash_floor=s.air_dash_floor;
        if(kind!=NativeBorgMotionKind::Ground)candidate->offer.action=native_borg_motion_phase(kind)==4?BattleMvpAction::Land:BattleMvpAction::Jump;
        if(native_borg_motion_air_dash(kind))candidate->offer.action=BattleMvpAction::AirDash;
        if(native_borg_motion_relaunch_entry(kind))candidate->offer.entry_vertical_delta=f32(motion_.air[2]*motion_.rank[2]);
        if(!serial_)return fail(error,"source serial exhausted");
        *out=candidate->offer;pending_=std::move(candidate);return true;
    }
    bool commit(const NativeBattleMvp& world,const std::weak_ptr<const CharacterModelBinding>& current_binding,std::string* error){
        NativeBorgMotionReceipt receipt;
        if(!ready_||!pending_||current_binding.lock()!=binding_.lock()||binding_.expired()||!world.motion_receipt(pending_->offer,&receipt))return fail(error,"source frame has no matching native contact receipt");
        const f32 previous_y=pending_->offer.observed.position[1];
        state_=std::move(pending_->state);state_.motion.position=world.actors()[0].position;state_.previous_y=previous_y;expected_tick_=world.tick();
        ++accepted_frames_;body_contacts_+=receipt.contact().body_hit;
        air_frames_+=native_borg_motion_phase(pending_->offer.kind)==2;
        landings_+=native_borg_motion_phase(pending_->offer.kind)==3;
        air_dash_starts_+=native_borg_motion_windup_entry(pending_->offer.kind);
        air_dash_active_+=native_borg_motion_active_entry(pending_->offer.kind)||native_borg_motion_active_update(pending_->offer.kind);
        relaunches_+=native_borg_motion_relaunch_entry(pending_->offer.kind);
        if(pending_->interrupted_dash){++dash_jumps_;last_dash_jump_=pending_->interrupted_dash;}
        landing_jumps_+=pending_->landing_target==4;landing_dashes_+=pending_->landing_target==12;
        if(native_borg_motion_phase(pending_->offer.kind)==3&&state_.contact_angle>=0){++surface_landings_;last_surface_angle_=state_.contact_angle;}
        if(native_borg_motion_push(pending_->offer.kind)){
            ++surface_pushes_;last_push_angle_=state_.static_push->angle;
            if(pending_->offer.kind==NativeBorgMotionKind::StaticPush){
                dash_fall_pushes_+=state_.jump.falling_after_dash;relaunch_fall_pushes_+=state_.jump.falling_after_relaunch;
            }else if(pending_->offer.kind==NativeBorgMotionKind::AirDashRecoveryPush)++recovery_pushes_;
            else if(native_borg_motion_windup_push(pending_->offer.kind)){
                ++windup_pushes_;windup_entry_surface_pushes_+=pending_->offer.kind==NativeBorgMotionKind::AirDashWindupEntryPush;
            }
            else if(pending_->offer.kind==NativeBorgMotionKind::AirDashActivePush){++active_pushes_;last_active_push_programs_=movement_programs();}
            else {
                ++relaunch_surface_pushes_;last_relaunch_push_programs_=movement_programs();
                relaunch_entry_surface_pushes_+=pending_->offer.kind==NativeBorgMotionKind::RelaunchEntryPush;
            }
            last_push_programs_=movement_programs();
        }
        if(native_borg_motion_relaunch_floor(pending_->offer.kind)){++relaunch_floor_contacts_;last_relaunch_floor_programs_=movement_programs();}
        if(native_borg_motion_air_dash_floor(pending_->offer.kind)){
            if(pending_->offer.kind==NativeBorgMotionKind::AirDashWindupFloor||native_borg_motion_windup_floor_entry(pending_->offer.kind))++windup_floor_contacts_;else ++active_floor_contacts_;
            windup_entry_floor_contacts_+=native_borg_motion_windup_floor_entry(pending_->offer.kind);
            positive_windup_floor_contacts_+=pending_->offer.kind==NativeBorgMotionKind::AirDashWindupPositiveEntryFloor;
            last_air_dash_floor_programs_=movement_programs();
        }
        frame_scope_=std::shared_ptr<const NativeBorgSourcePoseScope>(new NativeBorgSourcePoseScope);
        pose_={std::move(pending_->local),binding_,receipt,frame_scope_};pending_.reset();return true;
    }
    void reject(){retire();}
    bool pose(const NativeBattleMvp& world,NativeBorgSourcePose* out)const{
        if(!out||!ready_||pending_||!frame_scope_||binding_.expired()||!world.motion_receipt_current(pose_.receipt))return false;
        *out=pose_;return true;
    }
    u64 accepted_frames()const{return accepted_frames_;}
    u64 body_contacts()const{return body_contacts_;}
    u64 air_frames()const{return air_frames_;}u64 landings()const{return landings_;}
    s16 resource()const{return state_.resource.value736;}
    u64 air_dash_starts()const{return air_dash_starts_;}u64 air_dash_active_frames()const{return air_dash_active_;}
    u64 relaunches()const{return relaunches_;}
    u64 dash_jumps()const{return dash_jumps_;}u8 last_dash_jump_selector()const{return last_dash_jump_;}
    u64 landing_jumps()const{return landing_jumps_;}u64 landing_dashes()const{return landing_dashes_;}
    bool landing_entry_accepted()const{return ready_&&state_.jump.selector==7&&state_.jump.entered&&!state_.jump.cached87;}
    u64 surface_landings()const{return surface_landings_;}s16 last_surface_angle()const{return last_surface_angle_;}
    u64 surface_pushes()const{return surface_pushes_;}
    s16 last_push_angle()const{return last_push_angle_;}
    u64 dash_fall_pushes()const{return dash_fall_pushes_;}
    u64 relaunch_fall_pushes()const{return relaunch_fall_pushes_;}
    u64 recovery_pushes()const{return recovery_pushes_;}
    u64 windup_pushes()const{return windup_pushes_;}
    u64 windup_entry_surface_pushes()const{return windup_entry_surface_pushes_;}
    u64 active_pushes()const{return active_pushes_;}
    std::array<u16,2> last_active_push_programs()const{return last_active_push_programs_;}
    u64 relaunch_surface_pushes()const{return relaunch_surface_pushes_;}
    u64 relaunch_entry_surface_pushes()const{return relaunch_entry_surface_pushes_;}
    u64 relaunch_floor_contacts()const{return relaunch_floor_contacts_;}
    std::array<u16,2> last_relaunch_floor_programs()const{return last_relaunch_floor_programs_;}
    u64 windup_floor_contacts()const{return windup_floor_contacts_;}
    u64 active_floor_contacts()const{return active_floor_contacts_;}
    u64 windup_entry_floor_contacts()const{return windup_entry_floor_contacts_;}
    u64 positive_windup_floor_contacts()const{return positive_windup_floor_contacts_;}
    std::array<u16,2> last_air_dash_floor_programs()const{return last_air_dash_floor_programs_;}
    std::array<u16,2> last_relaunch_push_programs()const{return last_relaunch_push_programs_;}
    std::array<u16,2> movement_programs()const{return {state_.vm.state().lanes[0].program,state_.vm.state().lanes[1].program};}
    std::array<u16,2> last_push_programs()const{return last_push_programs_;}
    GRedRelaunchState relaunch_stages()const{return state_.relaunch;}
    GRedAirDashState air_dash_stages()const{return state_.air_dash;}
    u8 ascent_stage()const{return state_.jump.ascent_stage;}
    u8 selector()const{return state_.jump.selector?state_.jump.selector:state_.dash.selector?state_.dash.selector:state_.walk_selector;}
    bool map_input(const GamePadState& pad,NativeGRedGroundInput* out,std::string* error)const{
        if(mode_!=Mode::VerticalJump&&!moving_mode(mode_))return ready_&&native_gred_ground_input(input_,pad,out,error);
        auto movement=pad;movement.b=false;NativeGRedGroundInput mapped;
        if(!ready_||!native_gred_ground_input(input_,movement,&mapped,error))return false;
        const bool relaunch_input=relaunch_mode(mode_)&&(state_.jump.selector==6||state_.jump.selector==16||state_.jump.selector==17);
        const bool landing_input=landing_mode(mode_)&&landing_entry_accepted();
        const bool takeoff_input=surface_mode(mode_)&&state_.jump.selector==4; //selector4 samples input but admits no locomotion cancellation
        if((mode_==Mode::Jump||(air_dash_mode(mode_)&&(!state_.jump.cached87||state_.jump.selector==7)))&&
           (state_.jump.selector||pad.b)&&!(landing_input&&!pad.b)){
            //Native convenience mapping: jump/touchdown directions stay below
            //the flick threshold. Accepted landing entry may then admit a real
            //full-deflection ground dash; jump still has priority over a flick.
            mapped.x=s16(std::clamp(s32(mapped.x),-32,32));mapped.y=s16(std::clamp(s32(mapped.y),-32,32));u8 direction=0;
            if(!character_movement_stick_nibble(input_,mapped.x,mapped.y,input_.low_threshold,&direction))return false;
            mapped.held=u32(direction)<<16;
        }
        if(pad.b)mapped.held|=0x100u;
        if(landing_mode(mode_)&&state_.jump.selector==7&&!state_.jump.entered){
            //Preview the same bounded kernel, not a separate host press-edge
            //rule. A still-buffered held jump must not bypass landing entry.
            CharacterMovementInputState preview;GRedMovementAdmission admission;GRedMovementAdmissionFields fields;
            fields.selector=7;fields.flags5e0=state_.vm.state().actor.behaviour;fields.prior_handlers_declined=true;
            if(!character_movement_input_step(input_,state_.input,{mapped.x,mapped.y,mapped.held,fields.flags5e0},&preview,error)||
               !gred_movement_admit(input_,preview,state_.resource,fields,&admission,error))return false;
            if(admission.changed){if(error)*error="source landing input refused: touchdown cannot skip accepted landing entry";return false;}
        }
        if((mode_==Mode::VerticalJump&&(state_.jump.selector||pad.b)&&((mapped.held&0xf0000u)||state_.walk_selector||state_.dash.selector))||
           ((state_.jump.selector||pad.b)&&state_.dash.selector&&!dash_jump_mode(mode_))||
           (pad.b&&!relaunch_input&&!landing_input&&!takeoff_input&&((moving_mode(mode_)&&state_.jump.selector)||(state_.jump.selector==6||state_.jump.selector==7))&&!(state_.input.previous_held&0x100u))){
                 if(error)*error=surface_mode(mode_)?"source movement refused: this input cannot cancel the reviewed phase":
                    mode_==Mode::LandingInterrupt?"source movement refused: takeoff interruption is unsupported":
                    mode_==Mode::DashJump?"source movement refused: takeoff/landing interruption is unsupported":
                   mode_==Mode::Relaunch?"source relaunch refused: takeoff/landing interruption and dash-to-jump are unsupported":
                  moving_mode(mode_)?"source jump refused: dash-to-jump, relaunch and landing interruption are unsupported":
                 "source vertical jump refused: moving takeoff, air direction and relaunch are unsupported";
            return false;
        }
        if(!out)return false;
        *out=mapped;return true;
    }
private:
    static bool positive_windup_floor_mode(Mode mode){return mode==Mode::WindupPositiveFloor||mode==Mode::WindupEntryPush;}
    static bool windup_entry_floor_mode(Mode mode){return mode==Mode::WindupEntryFloor||positive_windup_floor_mode(mode);}
    static bool air_dash_floor_mode(Mode mode){return mode==Mode::AirDashFloor||windup_entry_floor_mode(mode);}
    static bool relaunch_floor_mode(Mode mode){return mode==Mode::RelaunchFloor||air_dash_floor_mode(mode);}
    static bool relaunch_entry_surface_mode(Mode mode){return mode==Mode::RelaunchEntrySurface||relaunch_floor_mode(mode);}
    static bool relaunch_surface_mode(Mode mode){return mode==Mode::RelaunchSurface||relaunch_entry_surface_mode(mode);}
    static bool active_surface_mode(Mode mode){return mode==Mode::ActiveSurface||relaunch_surface_mode(mode);}
    static bool windup_surface_mode(Mode mode){return mode==Mode::WindupSurface||active_surface_mode(mode);}
    static bool recovery_surface_mode(Mode mode){return mode==Mode::RecoverySurface||windup_surface_mode(mode);}
    static bool history_surface_mode(Mode mode){return mode==Mode::FallingSurface||recovery_surface_mode(mode);}
    static bool steep_mode(Mode mode){return mode==Mode::Steep||history_surface_mode(mode);}
    static bool surface_mode(Mode mode){return mode==Mode::Surface||steep_mode(mode);}
    static bool landing_mode(Mode mode){return mode==Mode::LandingInterrupt||surface_mode(mode);}
    static bool dash_jump_mode(Mode mode){return mode==Mode::DashJump||landing_mode(mode);}
    static bool relaunch_mode(Mode mode){return mode==Mode::Relaunch||dash_jump_mode(mode);}
    static bool air_dash_mode(Mode mode){return mode==Mode::AirDash||relaunch_mode(mode);}
    static bool moving_mode(Mode mode){return mode==Mode::Jump||air_dash_mode(mode);}
    bool air_contact(State& s,const CharacterMovementStaticGround& ground,const CharacterMovementMotionState& before,
                     const CharacterMovementMotionState& proposed,CharacterMovementMotionState* out,bool* touchdown,std::string* error,bool moving,bool allow_push=false){
        if(!surface_mode(mode_))return gred_vertical_jump_flat_contact(ground,before,proposed,out,touchdown,error,moving);
        GRedSurfaceContact result;
        //Native movement-only owner declares timer698=0 and7C9=0, not an
        //original actor constructor/hit/size-animation producer. Zero698 uses
        //the normalAC gate; the alternateAE and7C9 branches remain disabled.
        if(!gred_surface_contact(motion_,ground,before,proposed,{0,0,steep_mode(mode_)&&allow_push},&result,error))return false;
        if(result.kind==GRedSurfaceContactKind::Pushed){
            s.static_push=NativeBorgStaticPush{proposed.position,proposed.vertical48,
                {result.surface.height,result.surface.normal,result.surface.triangle},result.angle,motion_.steep_damping};
        }
        *out=result.motion;*touchdown=result.kind==GRedSurfaceContactKind::Landed;
        if(*touchdown)s.contact_angle=result.angle;
        return true;
    }
    static void begin_takeoff(State& s){
        s.jump={4,0,false};s.walk_selector=0;s.walk_entered=false;
        s.dash.selector=0;s.dash.direction_class=0; //source jump predicate resets57C, not retained coarse57D or body heading
        s.steering.direction57c=0;s.steering.completed547=false;
        s.relaunch={};s.air_dash.selector=0;
    }
    bool begin_ground_dash(State& s,const GRedMovementAdmission& admission,const CharacterMovementHeadingFrame& frame,std::string* error){
        CharacterMovementHeading heading;u8 program=0,klass=0;
        if(admission.selector!=12||!admission.consume_direction||
           !character_movement_heading(input_,motion_,admission.direction58f,CharacterMovementHeadingMode::CardinalDash,frame,&heading,error)||
           !gred_ground_dash_program(motion_,heading,&program,&klass))return false;
        s.input.directions5d8=admission.remaining_directions;s.dash={};s.dash.direction_class=klass;
        s.dash.coarse57d=u8(source_heading_sector(heading.relative734,2));
        s.steering={};s.steering.travel5ae=source_heading_wrap(s32(frame.camera5b0)-0x8000);
        s.walk_selector=0;s.walk_entered=false;s.jump.selector=0;s.jump.entered=false;s.jump.air_ready588=0;
        s.relaunch={};s.air_dash.selector=0;return true;
    }
    bool relaunch(State& s,u32 held,CharacterMovementHeadingFrame frame,NativeBorgMotionKind* kind,std::string* error){
        GRedRelaunchPlan plan;if(!gred_relaunch_plan(motion_,s.relaunch,s.motion,&plan))return false;
        const auto before=s.motion;const auto ground=ground_for(s);s.motion=plan.motion;
        if(plan.entry){
            s.jump.cached87=1;s.jump.air_ready588=1;s.jump.timer690=motion_.air_ready_delay;
            s.jump.falling_after_dash=false;s.jump.falling_after_relaunch=true;s.jump.falling_coarse57d=s.dash.coarse57d;
        }
        if(!character_air_resource_consume(s.resource,input_.costs[plan.cost_index],input_.recovery_delay,1,&s.resource))return false;
        std::vector<CharacterProgramOperation> operations;
        auto select=[&](u8 program,u8 mask){return s.vm.request_with_actor_controls(1,program,mask,-1,1,driver_,s.height,&operations,error);};
        auto update=[&](u8 mask){return s.vm.update_with_actor_controls(mask,1,driver_,s.height,&operations,error);};
        if(plan.request?!select(plan.program,2):!update(2))return false;
        s.motion.flags5e0=s.vm.state().actor.behaviour;
        if(!gred_relaunch_lower_finished(plan,s.vm.state().lanes[1].done,&s.relaunch))return false;
        CharacterMovementHeading heading;frame.body72=s.body;
        if(!character_movement_heading(input_,motion_,u8((held>>16)&15),CharacterMovementHeadingMode::Held,frame,&heading,error))return false;
        s.turn.body72=s.body;
        if(!character_movement_turn_steer(motion_,s.turn,heading.delta5aa,1,1,1,&s.turn))return false;
        s.body=s.turn.body72;s.motion.travel5ae=s.body;
        if(!gred_jump_acceleration(motion_,heading.has_direction,&s.motion))return false;
        CharacterMovementCeiling ceiling;ceiling.mode=CharacterMovementCeilingMode::DeclaredStatic;
        ceiling.stage_top34=20000;ceiling.query_height668=ground.query_height668;ceiling.previous_y30=s.previous_y;
        CharacterMovementMotionState proposed;bool touchdown=false;
        if(!character_movement_air_step(motion_,s.motion,1,1,true,ceiling,&proposed,error))return false;
        if(relaunch_floor_mode(mode_)){
            GRedSurfaceContact contact;
            if(!gred_relaunch_static_contact(motion_,ground,before,proposed,&contact,error))return false;
            s.motion=contact.motion;
            if(contact.kind==GRedSurfaceContactKind::Pushed)
                s.static_push=NativeBorgStaticPush{proposed.position,proposed.vertical48,
                    {contact.surface.height,contact.surface.normal,contact.surface.triangle},contact.angle,motion_.steep_damping};
            else if(contact.kind==GRedSurfaceContactKind::Snapped)
                s.relaunch_floor=NativeBorgRelaunchFloor{proposed.position,proposed.vertical48,proposed.vertical_change50,
                    proposed.vertical48<0?motion_.relaunch_floor_damping:1.f,
                    {contact.surface.height,contact.surface.normal,contact.surface.triangle},contact.angle};
        }else if(!air_contact(s,ground,before,proposed,&s.motion,&touchdown,error,true,
                    relaunch_surface_mode(mode_)&&(!plan.entry||relaunch_entry_surface_mode(mode_)))||touchdown)return false;
        if(s.static_push)s.static_push->relaunch_acceleration=s.motion.vertical_change50;
        GRedRelaunchUpperPlan upper;
        if(!gred_relaunch_upper_plan(s.relaunch,&upper)||(upper.request?!select(upper.program,1):!update(1)))return false;
        s.relaunch.upper_stage=upper.next_stage;
        *kind=plan.entry?NativeBorgMotionKind::RelaunchEntry:plan.program==15?NativeBorgMotionKind::RelaunchActive:NativeBorgMotionKind::RelaunchWindup;
        if(s.static_push)*kind=plan.entry?NativeBorgMotionKind::RelaunchEntryPush:
            plan.program==15?NativeBorgMotionKind::RelaunchActivePush:NativeBorgMotionKind::RelaunchWindupPush;
        if(s.relaunch_floor)*kind=plan.entry?NativeBorgMotionKind::RelaunchEntryFloor:
            plan.program==15?NativeBorgMotionKind::RelaunchActiveFloor:NativeBorgMotionKind::RelaunchWindupFloor;
        return true;
    }
    bool air_dash(State& s,u32 held,CharacterMovementHeadingFrame frame,NativeBorgMotionKind* kind,std::string* error){
        const u8 selector=s.air_dash.selector;const bool entry=!s.air_dash.entered;
        GRedAirDashPlan plan;if(!gred_air_dash_plan(motion_,s.air_dash,s.motion,&plan))return false;
        const auto before=s.motion;const auto ground=ground_for(s);s.motion=plan.motion;s.jump.cached87=1;
        std::vector<CharacterProgramOperation> operations;
        auto select=[&](u8 program,u8 mask){return s.vm.request_with_actor_controls(1,program,mask,-1,1,driver_,s.height,&operations,error);};
        auto update=[&](u8 mask){return s.vm.update_with_actor_controls(mask,1,driver_,s.height,&operations,error);};
        if(plan.request?!select(plan.program,2):!update(2))return false;
        s.motion.flags5e0=s.vm.state().actor.behaviour;
        if(plan.turn_windup){bool complete=false;if(!character_movement_turn_step(s.turn,1,&s.turn,&complete))return false;s.body=s.turn.body72;}
        if(plan.cost_index!=255&&!plan.cost_after_move&&
           !character_air_resource_consume(s.resource,input_.costs[plan.cost_index],input_.recovery_delay,1,&s.resource))return false;
        if(!gred_air_dash_spent(plan,s.resource.value736,&s.air_dash))return false;
        CharacterMovementHeading heading;frame.body72=s.body;
        if(plan.steer_active){
            if(!character_movement_heading(input_,motion_,u8((held>>16)&15),CharacterMovementHeadingMode::Held,frame,&heading,error))return false;
            s.turn.body72=s.body;
            if(!character_movement_turn_steer(motion_,s.turn,heading.delta5aa,1,1,2,&s.turn))return false;
            s.body=s.turn.body72;s.motion.travel5ae=s.body;
        }else if(selector==17){
            if(s.vm.state().actor.control1[0]){if(error)*error="unreviewed ground-motion recovery control";return false;}
            if(entry){s.turn.rate7e=s.turn.delta5aa=0;}
            if(!character_movement_heading(input_,motion_,u8((held>>16)&15),CharacterMovementHeadingMode::Held,frame,&heading,error)||
               !gred_jump_falling_drift(motion_,heading,&s.motion))return false;
        }
        if(plan.move){
            CharacterMovementCeiling ceiling;ceiling.mode=CharacterMovementCeilingMode::DeclaredStatic;ceiling.stage_top34=20000;
            ceiling.query_height668=ground.query_height668;ceiling.previous_y30=s.previous_y;CharacterMovementMotionState moved;
            if(!character_movement_air_step(motion_,s.motion,1,1,selector==17,ceiling,&moved,error))return false;
            s.motion=moved;
        }
        if(plan.cost_after_move&&!character_air_resource_consume(s.resource,input_.costs[plan.cost_index],input_.recovery_delay,1,&s.resource))return false;
        bool touchdown=false;CharacterMovementMotionState contacted;
        const bool push_enabled=(recovery_surface_mode(mode_)&&selector==17)||(windup_surface_mode(mode_)&&selector==15&&!entry)||
            (active_surface_mode(mode_)&&selector==16&&!entry)||(mode_==Mode::WindupEntryPush&&selector==15&&entry);
        if(air_dash_floor_mode(mode_)&&selector!=17){
            GRedSurfaceContact contact;
            if(!gred_air_dash_static_contact(motion_,ground,before,s.motion,selector,s.air_dash.contact_callback,push_enabled,&contact,error))return false;
            if(contact.kind==GRedSurfaceContactKind::Snapped){
                if(selector==15&&entry&&!windup_entry_floor_mode(mode_)){if(error)*error="wind-up entry floor contact is not enabled";return false;}
                s.air_dash_floor=NativeBorgAirDashFloor{s.motion.position,s.motion.vertical48,s.motion.vertical_change50,
                    {contact.surface.height,contact.surface.normal,contact.surface.triangle},contact.angle};
            }else if(contact.kind==GRedSurfaceContactKind::Pushed)
                s.static_push=NativeBorgStaticPush{s.motion.position,s.motion.vertical48,
                    {contact.surface.height,contact.surface.normal,contact.surface.triangle},contact.angle,motion_.steep_damping};
            touchdown=contact.kind==GRedSurfaceContactKind::Landed;
            if(touchdown)s.contact_angle=contact.angle;
            contacted=contact.motion;
        }else if(!air_contact(s,ground,before,s.motion,&contacted,&touchdown,error,true,push_enabled)){
            if(error&&error->empty())*error="non-flat or unequal-height air-dash contact";
            return false;
        }
        const bool done=s.vm.state().lanes[1].done;
        GRedAirDashContactTail tail;
        if(!gred_air_dash_contact_tail(selector,s.air_dash.contact_callback,done,touchdown,&tail)){
            if(error)*error="unsupported early floor contact or simultaneous recovery-completion/landing";
            return false;
        }
        s.motion=contacted;s.jump.selector=s.air_dash.selector;s.jump.air_ready588=s.air_dash.air_ready588;
        if(tail.landing){s.jump.selector=7;s.jump.entered=false;s.jump.air_ready588=0;s.air_dash.selector=0;}
        else if(tail.falling){s.jump.selector=6;s.jump.entered=false;s.jump.air_ready588=0;s.jump.falling_after_dash=true;s.jump.falling_after_relaunch=false;s.air_dash.selector=0;}
        if(plan.request?!select(plan.program,1):!update(1))return false;
        *kind=touchdown?NativeBorgMotionKind::JumpTouchdown:
            selector==15?(entry?NativeBorgMotionKind::AirDashWindupEntry:NativeBorgMotionKind::AirDashWindup):
            selector==16?(entry?NativeBorgMotionKind::AirDashActiveEntry:NativeBorgMotionKind::AirDashActive):NativeBorgMotionKind::AirDashRecovery;
        if(s.static_push)*kind=selector==15?(entry?NativeBorgMotionKind::AirDashWindupEntryPush:NativeBorgMotionKind::AirDashWindupPush):
            selector==16?NativeBorgMotionKind::AirDashActivePush:NativeBorgMotionKind::AirDashRecoveryPush;
        if(s.air_dash_floor)*kind=selector==15?(entry?NativeBorgMotionKind::AirDashWindupEntryFloor:NativeBorgMotionKind::AirDashWindupFloor):
            entry?NativeBorgMotionKind::AirDashActiveEntryFloor:NativeBorgMotionKind::AirDashActiveFloor;
        if(s.air_dash_floor&&selector==15&&entry&&before.vertical48>0&&positive_windup_floor_mode(mode_))
            *kind=NativeBorgMotionKind::AirDashWindupPositiveEntryFloor;
        return true;
    }
    bool jump(State& s,u32 held,CharacterMovementHeadingFrame frame,NativeBorgMotionKind* kind,std::string* error){
        const bool moving=moving_mode(mode_);
        GRedVerticalJumpPlan plan;
        if(!gred_vertical_jump_plan(motion_,s.jump,s.motion,s.resource.value736,held,&plan,moving))return false;
        s.motion=plan.motion;
        CharacterMovementHeading heading;frame.body72=s.body;
        if(moving&&plan.state.selector==5&&(plan.request||plan.state.ascent_stage==2)){
            if(!character_movement_heading(input_,motion_,u8((held>>16)&15),CharacterMovementHeadingMode::Held,frame,&heading,error))return false;
            plan.state.direction550=heading.has_direction;s.turn.body72=s.body;s.turn.delta5aa=heading.delta5aa;
            if(plan.request){
                s.motion.travel5ae=heading.desired5ac;
                if(heading.has_direction)s.motion.speed44=std::max(s.motion.speed44,motion_.air[3]);
                if(!character_movement_turn_begin(s.turn,motion_.turn_counts[source_heading_sector(heading.delta5aa,1)],&s.turn))return false;
            }else{
                if(!character_movement_turn_steer(motion_,s.turn,heading.delta5aa,1,1,1,&s.turn))return false;
                s.body=s.turn.body72;s.motion.travel5ae=s.body;
            }
        }
        if(plan.cost_index!=255&&!character_air_resource_consume(s.resource,input_.costs[plan.cost_index],input_.recovery_delay,1,&s.resource))return false;
        std::vector<CharacterProgramOperation> operations;
        auto select=[&](u8 program,u8 mask){return s.vm.request_with_actor_controls(1,program,mask,-1,1,driver_,s.height,&operations,error);};
        auto update=[&](u8 mask){return s.vm.update_with_actor_controls(mask,1,driver_,s.height,&operations,error);};
        if(plan.landing_prefix){
            s.air_dash.contact69c=0; //60228 landing entry, not a refill/animation-completion shortcut
            if(!select(2,2))return false;
        }
        if(plan.request?!select(plan.program,2):!update(2))return false;
        s.motion.flags5e0=s.vm.state().actor.behaviour;
        if(moving&&plan.state.selector==5){
            if(!plan.request&&plan.state.ascent_stage==1){bool complete=false;
                if(!character_movement_turn_step(s.turn,1,&s.turn,&complete))return false;
                s.body=s.turn.body72;if(complete)plan.state.ascent_stage=2;
            }
            if(!gred_jump_acceleration(motion_,plan.state.direction550,&s.motion))return false;
        }
        bool touchdown=false;
        if(plan.air_move){
            const auto before=s.motion;const auto ground=ground_for(s);
            CharacterMovementCeiling ceiling;ceiling.mode=CharacterMovementCeilingMode::DeclaredStatic;
            ceiling.stage_top34=20000;ceiling.query_height668=ground.query_height668;ceiling.previous_y30=s.previous_y; //native lab bounds
            CharacterMovementMotionState proposed;
            if(moving&&plan.state.selector==6){
                if(!character_movement_heading(input_,motion_,u8((held>>16)&15),CharacterMovementHeadingMode::Held,frame,&heading,error)||
                   !gred_jump_falling_drift(motion_,heading,&s.motion))return false;
            }
            if(!character_movement_air_step(motion_,s.motion,1,1,plan.clamp_wrapper,ceiling,&proposed,error)||
               !air_contact(s,ground,before,proposed,&s.motion,&touchdown,error,moving,
                      plan.state.selector==6&&(history_surface_mode(mode_)||(!plan.state.falling_after_dash&&!plan.state.falling_after_relaunch)))||
               (touchdown&&plan.state.selector!=6))return false;
        }
        if(!gred_vertical_jump_finish(plan,s.vm.state().lanes[1].done,touchdown,&s.jump))return false;
        //The previously captured upper state runs after lower motion/contact,
        //even when lower completion selected another movement state.
        if(plan.request?!select(plan.program,1):!update(1))return false;
        *kind=plan.state.selector==4?NativeBorgMotionKind::VerticalTakeoff:
            plan.state.selector==7?NativeBorgMotionKind::VerticalLanding:
            touchdown?NativeBorgMotionKind::VerticalTouchdown:NativeBorgMotionKind::VerticalAir;
        if(moving)*kind=NativeBorgMotionKind(unsigned(*kind)+4);
        if(s.static_push)*kind=NativeBorgMotionKind::StaticPush;
        return true;
    }
    static ByteView view(const std::vector<u8>& bytes){return {bytes.data(),bytes.size()};}
    static bool read(IFileSystem& files,const std::string& path,u64 maximum,std::vector<u8>* out,std::string* error){
        u64 size=0;if(!files.file_size(path,&size)||!size||size>maximum||!files.read(path,*out)||out->size()!=size){if(error)*error="source ground inputs: missing/oversized "+path;return false;}return true;
    }
    bool request(State& s,u8 program,std::string* error){
        std::vector<CharacterProgramOperation> operations;
        return s.vm.request_with_actor_controls(1,program,2,-1,1,driver_,s.height,&operations,error)&&s.vm.request_with_actor_controls(1,program,1,-1,1,driver_,s.height,&operations,error);
    }
    bool advance(State& s,std::string* error){
        std::vector<CharacterProgramOperation> operations;
        return s.vm.update_with_actor_controls(2,1,driver_,s.height,&operations,error)&&s.vm.update_with_actor_controls(1,1,driver_,s.height,&operations,error);
    }
    bool walk_flag(State& s,bool value,std::string* error){
        CharacterProgramClassFields observed;
        return s.vm.observe_class_fields(&observed)&&s.vm.write_ground_walk_flag(observed,value,error);
    }
    bool walk_move(State& s,std::string* error){
        s.motion.flags5e0=s.vm.state().actor.behaviour;const auto ground=ground_for(s);CharacterMovementMotionState moved;f32 floor=0;
        if(!character_movement_ground_step(motion_,s.motion,1,ground,&moved,error)||
           !ground.height(moved.position,&floor,error)||std::abs(floor-moved.position[1])>.01f)return false;
        s.motion=moved;return true;
    }
    bool walk(State& s,CharacterMovementHeadingFrame frame,std::string* error){
        const u8 selector=s.walk_selector;const bool entry=!s.walk_entered;s.walk_entered=true;
        std::vector<CharacterProgramOperation> operations;
        auto update=[&](u8 mask){return s.vm.update_with_actor_controls(mask,1,driver_,s.height,&operations,error);};
        auto select=[&](u8 program,u8 mask,int duration=-1){
            CharacterProgramTransition transition;transition.duration=duration;
            return s.vm.request_with_actor_controls(1,program,mask,-1,1,driver_,s.height,&operations,error,transition);
        };
        if(selector==1&&entry){
            CharacterMovementHeading heading;GRedGroundWalkEntry start;s.turn.body72=s.body;
            if(!character_movement_heading(input_,motion_,u8(s.input.previous_held>>16),CharacterMovementHeadingMode::Held,frame,&heading,error)||
               !gred_ground_walk_entry(motion_,heading,s.turn,&start)||!walk_flag(s,true,error))return false;
            s.turn=start.turn;s.motion.speed44=start.speed44;s.motion.planar_change4c=0;s.motion.travel5ae=start.travel5ae;
            if(!select(1,2,start.blend_duration))return false;
            const auto* lower=driver_.snapshot(s.vm.state().lanes[1].motion);if(!lower)return false;
            //F318 reads lower blend duration1B2C, not the current clip frame.
            return select(1,1,std::max(0,int(lower->blend_duration())-1));
        }
        if(selector==1){
            bool complete=false;
            if(!walk_move(s,error)||!update(2)||!character_movement_turn_step(s.turn,1,&s.turn,&complete))return false;
            s.body=s.turn.body72;if(complete){s.walk_selector=2;s.walk_entered=false;}
            return update(1);
        }
        if(selector==2){
            if(entry){s.motion.speed44=motion_.walk_speeds[5];s.motion.planar_change4c=0;if(!walk_flag(s,true,error))return false;}
            CharacterMovementHeading heading;frame.body72=s.body;s.turn.body72=s.body;
            //Native harness explicitly fixes source rate664 and delta1DC8 to1.
            if(!character_movement_heading(input_,motion_,u8(s.input.previous_held>>16),CharacterMovementHeadingMode::Held,frame,&heading,error)||
               !gred_ground_walk_steer(motion_,s.turn,heading.delta5aa,1,1,&s.turn))return false;
            s.body=s.turn.body72;s.motion.travel5ae=s.body;
            return walk_move(s,error)&&update(2)&&update(1);
        }
        if(selector!=3)return false;
        if(entry){
            //Ordinary G-Red only. F6C0's pre-flip changes metadata, not the
            //already-converted frozen pose copied by B94C. Descriptor0 forces
            //both lanes unmirrored; no shared/directional-buffer consumer.
            const auto* descriptor=character_program_detail::descriptor(programs_,1,0);
            if(!descriptor||(descriptor->bytes[6]&3u)!=1u||(descriptor->bytes[14]&3u)!=1u)return false;
            for(const auto& lane:s.vm.state().lanes){const auto* old=driver_.snapshot(lane.motion);if(!old||old->transition_mode()>2)return false;}
            s.motion.speed44=s.motion.planar_change4c=0;
            return select(31,2)&&walk_move(s,error)&&select(31,1);
        }else{
            if(!update(2))return false;
            const auto* lower=driver_.snapshot(s.vm.state().lanes[1].motion);if(!lower)return false;
            if(!lower->blending()){
                if(!walk_flag(s,false,error))return false;
                s.motion.speed44=s.motion.planar_change4c=0;s.walk_selector=0;s.walk_entered=false;s.turn={};
            }
            return gred_ground_stop_speed(motion_,s.motion.speed44,1,&s.motion.speed44)&&walk_move(s,error)&&update(1);
        }
    }
    CharacterMovementStaticGround ground_for(const State& s)const{
        CharacterMovementStaticGround ground;ground.grids={&grids_[0],&grids_[1],&grids_[2]};
        ground.parameters={-100000,100000,500,0,-.5f,20000,-20000,true}; //native lab's declared static query bounds
        ground.actor.fields_declared=true;ground.actor.current24=s.motion.position[1];ground.actor.previous30=s.previous_y;
        ground.actor.offset6d0=s.vm.state().actor.pose_height.offset6d0;
        ground.query_height668=motion_.dimensions[2]*motion_.rank[0];ground.step674=motion_.dimensions[4]*motion_.rank[0];return ground;
    }
    bool fail(std::string* error,const char* why){retire();if(error)*error=std::string("source ground lab: ")+why;return false;}
    void retire(){ready_=false;pending_.reset();frame_scope_.reset();producer_.reset();}
    CharacterProgramAsset programs_;CharacterPoseHeightProfile height_;CharacterMovementInputProfile input_;CharacterMovementProfile motion_;CharacterMotionDriver driver_;
    std::array<StihGrid,3> grids_;State state_;std::unique_ptr<Pending> pending_;
    std::weak_ptr<const CharacterModelBinding> binding_;std::weak_ptr<const NativeBorgMotionScope> world_scope_;
    std::shared_ptr<const NativeBorgSourcePoseScope> frame_scope_;NativeBorgSourcePose pose_;u32 expected_tick_=0;u64 serial_=0;bool ready_=false;
    std::shared_ptr<const NativeBorgMotionProducer> producer_;
    u64 accepted_frames_=0,body_contacts_=0;
    u64 air_frames_=0,landings_=0;
    u64 air_dash_starts_=0,air_dash_active_=0;
    u64 relaunches_=0;
    u64 dash_jumps_=0;u8 last_dash_jump_=0;
    u64 landing_jumps_=0,landing_dashes_=0;
    u64 surface_landings_=0,surface_pushes_=0;s16 last_surface_angle_=0,last_push_angle_=0;
    u64 dash_fall_pushes_=0,relaunch_fall_pushes_=0,recovery_pushes_=0,windup_pushes_=0,active_pushes_=0,relaunch_surface_pushes_=0;
    u64 relaunch_entry_surface_pushes_=0;
    u64 windup_entry_surface_pushes_=0;
    u64 relaunch_floor_contacts_=0;std::array<u16,2> last_relaunch_floor_programs_{0xffff,0xffff};
    u64 windup_floor_contacts_=0,active_floor_contacts_=0;std::array<u16,2> last_air_dash_floor_programs_{0xffff,0xffff};
    u64 windup_entry_floor_contacts_=0;
    u64 positive_windup_floor_contacts_=0;
    std::array<u16,2> last_push_programs_{0xffff,0xffff};
    std::array<u16,2> last_active_push_programs_{0xffff,0xffff};
    std::array<u16,2> last_relaunch_push_programs_{0xffff,0xffff};
    Mode mode_=Mode::DashOnly;
};
}
