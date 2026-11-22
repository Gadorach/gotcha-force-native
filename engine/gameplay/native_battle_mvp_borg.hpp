#pragma once
#include "engine/gameplay/native_battle_mvp.hpp"
#include "engine/gameplay/native_borg_motion_map.hpp"
#include "engine/gameplay/character_model_scene.hpp"
namespace gf {
class NativeGRedGroundDashDemo;
struct NativeBorgSourceMeter {s16 value=0,limit=0;};
struct NativeBorgSourceReadout {
    NativeBorgSourceMeter meter;
    NativeBorgMotionKind kind=NativeBorgMotionKind::Ground;
    BattleMvpAction action=BattleMvpAction::Idle;
    bool ascent=false;
    std::optional<u8> dash_release_ticks; //0 means ready, absent outside continuing active dash
    std::optional<u8> ledge_timer_ticks; //0 means expired on a ledge fall/landing, absent outside its window
    bool ledge_drift_active=false; //Only an accepted ledge-fall update with a direct drift heading.
    bool recovery_drift_active=false; //Only either accepted recovery floor-contact update with sealed drift.
    std::optional<std::array<u32,2>> recovery_seam; //Only an accepted recovery contact that changes triangle.
    bool recovery_push_complete=false; //Accepted steep push that finishes the lower recovery lane.
    bool recovery_push_exit_drift=false; //Accepted first falling update with drift after that completing push.
    bool plain_windup_relaunch_ready=false; //Accepted plain handoff window, not a receipt for the next action.
    std::optional<bool> relaunch_floor_damped; //Accepted airborne snap; true only for the negative-speed tail.
    bool dash_interrupted_takeoff=false; //First accepted takeoff after interrupting an existing dash.
    bool landing_dash_started=false; //Accepted phase-12 dash replacing an entered landing.
    bool direct_expiry_relaunch=false; //Accepted plain relaunch directly from a zero-timer walking push.
    bool direct_expiry_relaunch_ready=false; //The accepted expiry push permits asking for soft-Up relaunch next.
    bool windup_from_relaunch_snap=false; //Accepted wind-up floor entry from the preceding airborne relaunch snap.
    bool floor_windup_relaunch_ready=false; //Accepted completed timed floor wind-up; next input may request neutral relaunch.
    bool direct_timed_relaunch=false; //Accepted rising relaunch directly from a still-positive walking push.
    bool direct_timed_relaunch_up=false; //The same timed handoff with the accepted soft Up turn.
    bool relaunch_from_active_snap=false; //Accepted plain relaunch from an already paid active-dash floor snap.
    bool timed_release_push=false; //Accepted inter-gate push in ordinary falling after a timed walking-ledge relaunch tap.
    bool second_timed_release_push=false; //Accepted same-face successor to that timed push.
    bool direct_expiring_relaunch=false; //Accepted plain soft-Up relaunch while the clock goes from one to zero.
    bool direct_expiring_relaunch_ready=false; //The accepted timer-one walking push permits asking for that entry.
    bool direct_expiring_entry_push=false; //Accepted projected soft-Up entry from the timer-one walking push.
    bool expiring_walk_air_dash=false; //Accepted stationary wind-up as the walking-push timer reaches zero.
    bool expiring_walk_air_dash_relaunch=false; //Accepted relaunch instead of active dash after the qualified expiry-step wind-up.
    bool expiring_walk_air_dash_snap=false; //Accepted same-height snap on the expiry-step wind-up entry.
    bool third_timed_release_push=false; //Accepted final successor to the two timed pushes on face 2021.
    bool down_timed_release_push=false; //Accepted 2 -> 1 Down-drift push onto face 2011.
};
class NativeBorgSourcePoseScope {
    friend class NativeGRedGroundDashDemo;
    friend class NativeBattleMvpBorg;
    NativeBorgSourcePoseScope(const NativeBorgMotionReceipt& receipt,NativeBorgSourceReadout readout,
                             std::weak_ptr<const CharacterModelBinding> binding,CharacterLocalPose local)
        :receipt_(receipt),readout_(readout),binding_(std::move(binding)),local_(std::move(local)){}
    NativeBorgSourcePoseScope(const NativeBorgSourcePoseScope&)=delete;
    NativeBorgSourcePoseScope& operator=(const NativeBorgSourcePoseScope&)=delete;
    NativeBorgMotionReceipt receipt_;
    NativeBorgSourceReadout readout_;
    std::weak_ptr<const CharacterModelBinding> binding_;
    CharacterLocalPose local_;
public:
    std::optional<NativeBorgSourceReadout> readout(const NativeBattleMvp& world)const{
        const auto meter=readout_.meter;
        if(!world.motion_receipt_current(receipt_)||meter.limit<=0||meter.value<0||meter.value>meter.limit)return {};
        return readout_;
    }
    std::optional<NativeBorgSourceMeter> meter(const NativeBattleMvp& world)const{
        const auto frame=readout(world);
        return frame?std::optional{frame->meter}:std::nullopt;
    }
};
struct NativeBorgAnimationSeed {
    CharacterPoseRig rig;CharacterLocalPose local;std::weak_ptr<const CharacterModelBinding> binding;
    u32 tick=0,stage_hash=0,floor_hash=0;BattleMvpVec position{};BattlePoint facing{};
    std::weak_ptr<const NativeBorgMotionScope> world_scope;
};
struct NativeBorgSourcePose {
    std::weak_ptr<const CharacterModelBinding> binding;NativeBorgMotionReceipt receipt;
    std::weak_ptr<const NativeBorgSourcePoseScope> frame_scope;
};
// Diagnostic copy from an accepted normal G-Red movement frame. It is not
// the source combat actor's world, a muzzle receipt, or permission to fire.
struct NativeGRedPracticeWorldMatrices {u32 tick=0;Mat4 key0,key7;};
//Presentation for a native MVP gameplay actor, using the shared bone sampler,
//pose evaluation, skinning and renderer. No source constructor/callback receipt.
class NativeBattleMvpBorg {
public:
    bool load(u16 model,ByteView hsd,ByteView prefix,ByteView pose,ByteView motions,std::string* error){
        package_profile_.reset();
        ready_=presentation_ready_=source_pose_active_=false;source_frame_scope_.reset();model_id_=0;world_={};displayed_={};blend_from_={};blend_remaining_={};
        const u32 expected=model==0x615?0xe2b4439au:model==10?0xeab3632au:0;
        if(!expected||!scene_.load(hsd,prefix,pose,error))return false;
        const auto identity=scene_.instance_binding();
        if(!identity||identity->model_hash!=expected){if(error)*error="native MVP: only original normal G-Red/Sasuke models are supported";return false;}
        if(!character_motion_bank_load(motions,&bank_,error))return false;
        map_=native_borg_motion_map(model);
        for(unsigned i=0;i<map_.clips.size();++i){
            if(!map_.user_classified&&i>unsigned(NativeBorgMotion::LegacyJump))continue;
            auto choice=map_.clips[i];if(bank_.groups[choice.group].empty())choice={0,0};
            CharacterMotionSelection selected;if(!character_motion_select(bank_,choice.group,choice.choice,&selected,error)||!hsd_bone_motion_load(selected.archive,scene_.rig().binding,&clips_[i],error))return false;
            if((i==1||map_.user_classified)&&selected.fallback){if(error)*error="native MVP: requested classified/locomotion clip is absent";return false;}
            stride_[i]=120;
            if(i==1){
                HsdBoneMotionSample begin,end;
                if(!hsd_bone_motion_sample(clips_[i],0,&begin,error)||!hsd_bone_motion_sample(clips_[i],u32(clips_[i].end_frame),&end,error))return false;
                if(!begin.bones.empty()&&!end.bones.empty()){
                    const auto& a=begin.bones[0];const auto& b=end.bones[0];const f32 dx=b.value[5]-a.value[5],dz=b.value[7]-a.value[7],distance=std::hypot(dx,dz);
                    if((a.present&b.present&0xa0)&&distance>=20&&distance<=1000)stride_[i]=distance;
                }
            }
        }
        std::array<bool,32> upper{};const auto& rig=scene_.rig();for(unsigned i=rig.cuts[0];i<rig.cuts[1];++i)upper[rig.order[i]]=true;
        torso_=kHsdNoBone;
        for(unsigned i=0;i<rig.binding.bones.size();++i)if(upper[i]){
            const auto parent=rig.binding.bones[i].parent;if(parent!=kHsdNoBone&&!upper[parent]){if(torso_!=kHsdNoBone){if(error)*error="native MVP: ambiguous upper-body root";return false;}torso_=u8(i);}
        }
        if(torso_==kHsdNoBone){if(error)*error="native MVP: upper-body root missing";return false;}
        model_id_=model;ready_=true;last_tick_=std::numeric_limits<u32>::max();return true;
    }
    bool load_package(std::shared_ptr<const BorgGameplayProfile> profile,ByteView hsd,ByteView prefix,ByteView pose,
                      ByteView motions,std::string* error){
        auto fail=[&](std::string why){ready_=false;if(error)*error=std::move(why);return false;};
        if(!profile||!borg_gameplay_profile_valid(*profile))return fail("Borg gameplay: missing or invalid definition");
        const auto movement=profile->movement;
        if(movement==BorgBattleMovement::NormalGRed){
            if(!load(0x0615,hsd,prefix,pose,motions,error))return false;
        }else{
            reset_control_test_pose();ready_=false;model_id_=0;world_={};displayed_={};
            if(!scene_.load(hsd,prefix,pose,error)||!character_motion_bank_load(motions,&bank_,error))return false;
            for(unsigned i=0;i<(movement==BorgBattleMovement::Stationary?1u:4u);++i)
                if(!load_package_clip(profile->locomotion[i],&package_locomotion_[i],error))return false;
            // The authored movement component uses discrete raw clips; it
            // neither inherits a retail lane map nor looks up a retail ID.
            torso_=kHsdNoBone;
            const auto& rig=scene_.rig();
            for(unsigned j=rig.cuts[0];j<rig.cuts[1];++j){
                const auto bone=rig.order[j],parent=rig.binding.bones[bone].parent;
                if(parent!=kHsdNoBone&&std::find(rig.order.begin()+rig.cuts[0],rig.order.begin()+rig.cuts[1],parent)==rig.order.begin()+rig.cuts[1]){
                    if(torso_!=kHsdNoBone){torso_=kHsdNoBone;break;}
                    torso_=bone;
                }
            }
        }
        if(!profile->moves.empty()&&(!(scene_.rig().lane_mask&1u)||scene_.rig().cuts[0]==scene_.rig().cuts[1]))
            return fail("Borg gameplay: action clips require a bound upper lane");
        if(profile->moves.size()>package_moves_.size())return fail("Borg gameplay: action clip budget");
        for(unsigned i=0;i<profile->moves.size();++i){
            const auto& move=profile->moves[i];
            if(!load_package_clip(move.clip,&package_moves_[i],error)||
               package_moves_[i].end_frame<f32(move.pose_start+move.last_marker()))
                return fail("Borg gameplay: unavailable action clip or active frame outside clip for "+move.id);
        }
        for(const auto& shot:profile->projectiles)
            if(shot.socket&&shot.socket->bone>=scene_.rig().binding.bones.size())
                return fail("Borg gameplay: unbound origin bone for "+shot.id);
        for(const auto& fx:profile->movement_fx)
            if(fx.bone>=scene_.rig().binding.bones.size())
                return fail("Borg gameplay: unbound movement effect bone for "+fx.id);
        package_profile_=std::move(profile);ready_=true;return true;
    }
    bool update(const BattleMvpActor& actor,u32 tick,std::string* error){
        if(!ready_||actor.model!=model_id_||actor.source_ground_motion){presentation_ready_=false;if(error)*error="native Borg: accepted source pose required for source movement";return false;}
        if(package_profile_&&package_profile_->movement!=BorgBattleMovement::NormalGRed)return update_package(actor,tick,error);
        const auto motion=native_borg_select_motion(actor,map_);const unsigned base=unsigned(motion);
        const unsigned upper=actor.attack_ticks?2:base;
        if(presentation_ready_&&scene_.ready()&&tick==last_tick_&&base==base_&&upper==upper_&&actor.position==last_position_&&actor.facing==last_facing_&&actor.upper_body_yaw==last_aim_yaw_)return true;
        presentation_ready_=false;
        const bool reset=last_tick_==std::numeric_limits<u32>::max()||tick<last_tick_;
        const u32 elapsed=reset?0:tick-last_tick_;
        if(reset||base!=base_)clock_=0;else clock_+=elapsed;
        if(reset||upper!=upper_)attack_clock_=0;else attack_clock_+=elapsed;
        const bool locomotion=base==1;
        f64 body_time=locomotion?actor.travel_distance/stride_[base]*(u32(clips_[base].end_frame)+1):clock_;
        if(actor.dash_ticks&&map_.user_classified)body_time=native_borg_phase_time(actor.dash_ticks,actor.dash_duration,clips_[base].end_frame);
        else if(motion==NativeBorgMotion::Land)body_time=native_borg_phase_time(actor.landing_ticks,BattleMvpActor::landing_duration,clips_[base].end_frame);
        else if(motion==NativeBorgMotion::Takeoff)body_time=native_borg_phase_time(actor.takeoff_ticks,BattleMvpActor::takeoff_duration,clips_[base].end_frame);
        HsdBoneMotionSample body_a,body_b,upper_a,upper_b;f32 body_fraction=0,upper_fraction=0;
        if(!sample(base,body_time,native_borg_motion_loops(motion),&body_a,&body_b,&body_fraction,error))return false;
        if(upper!=base&&!sample(upper,attack_clock_,false,&upper_a,&upper_b,&upper_fraction,error))return false;
        std::array<CharacterPoseLane,4> lanes;
        for(unsigned lane=0;lane<4;++lane){
            const bool attack=lane==0&&upper!=base;const auto* a=attack?&upper_a:&body_a;const auto* b=attack?&upper_b:&body_b;const f32 fraction=attack?upper_fraction:body_fraction;
            lanes[lane].current=fraction>0?b:a;if(fraction>0){lanes[lane].previous=a;lanes[lane].previous_weight=1-fraction;}
        }
        CharacterLocalPose local;CharacterPoseFrame frame;
        if(!character_pose_sample(scene_.rig(),lanes,&local,error))return false;
        for(unsigned lane=0;lane<4;++lane)if(scene_.rig().lane_mask&(1u<<lane)){
            const unsigned index=lane==0?upper:base,old_index=lane==0?upper_:base_;
            const f32 blend_ticks=native_borg_motion_blend_ticks(NativeBorgMotion(index),map_.user_classified);
            if(reset)blend_remaining_[lane]=0;
            else if(index!=old_index&&!displayed_.bones.empty()){blend_from_[lane]=displayed_;blend_remaining_[lane]=blend_ticks;}
            else blend_remaining_[lane]=std::max(0.f,blend_remaining_[lane]-f32(elapsed));
            if(blend_remaining_[lane]>0&&!character_pose_detail::blend_lane(scene_.rig(),lane,blend_remaining_[lane]/blend_ticks,blend_from_[lane].bones,local.bones))return false;
        }
        const auto root=mat4_multiply(mat4_translation(actor.position.data()),mat4_rotation_y(std::atan2(actor.facing.x,actor.facing.z)));
        if(!character_pose_initial_frame(scene_.rig(),root,&frame,error))return false;
        frame.extra_angles[torso_][1]=std::bit_cast<s16>(u16(s32(std::lround(actor.upper_body_yaw*32768.f/3.14159265359f))));
        if(!character_pose_direct_world(scene_.rig(),local,frame,&world_,error)||!scene_.set_world_pose(world_,{},error))return false;
        displayed_=std::move(local);base_=base;upper_=upper;last_tick_=tick;last_position_=actor.position;last_facing_=actor.facing;last_aim_yaw_=actor.upper_body_yaw;
        source_pose_active_=false;source_frame_scope_.reset();presentation_ready_=true;return true;
    }
    bool animation_seed(const BattleMvpActor& actor,u32 tick,NativeBorgAnimationSeed* out)const{
        if(!out||!ready_||!presentation_ready_||!scene_.ready()||actor.source_ground_motion||actor.model!=0x615||model_id_!=0x615||
           tick!=last_tick_||actor.position!=last_position_||actor.facing!=last_facing_)return false;
        *out={scene_.rig(),displayed_,scene_.instance_binding(),tick,0,0,actor.position,actor.facing,{}};return true;
    }
    void refuse_source_pose(){presentation_ready_=false;}
    void reset_control_test_pose(){
        presentation_ready_=false;source_pose_active_=false;source_frame_scope_.reset();
        last_tick_=std::numeric_limits<u32>::max();
    }
    bool update_source_pose(const NativeBattleMvp& accepted,const NativeBorgSourcePose& source,std::string* error){
        const auto& actor=accepted.actors()[0];const auto tick=accepted.tick();
        presentation_ready_=false;
        const auto owned=source.frame_scope.lock();const auto binding=scene_.instance_binding();
        if(!ready_||!owned||!binding||source.binding.lock()!=binding||owned->binding_.lock()!=binding||
           !accepted.motion_receipt_current(source.receipt)||!accepted.motion_receipt_current(owned->receipt_)||
           !actor.source_ground_motion||model_id_!=0x615||actor.model!=0x615){
            if(error)*error="native Borg source pose: foreign/reloaded model binding";
            return false;
        }
        CharacterPoseFrame frame;CharacterWorldPose world;
        const auto root=mat4_multiply(mat4_translation(actor.position.data()),mat4_rotation_y(std::atan2(actor.facing.x,actor.facing.z)));
        CharacterLocalPose overlay;
        const auto* local=&owned->local_;
        if(!character_pose_initial_frame(scene_.rig(),root,&frame,error))return false;
        if(actor.package_move!=BorgBattleActionState::none){
            overlay=*local;local=&overlay;
            if(!apply_package_action(actor,&overlay,&frame,error))return false;
        }
        if(!character_pose_direct_world(scene_.rig(),*local,frame,&world,error)||!scene_.set_world_pose(world,{},error))return false;
        world_=std::move(world);displayed_=*local;last_tick_=tick;last_position_=actor.position;last_facing_=actor.facing;last_aim_yaw_=actor.upper_body_yaw;
        source_pose_active_=true;source_frame_scope_=source.frame_scope;presentation_ready_=true;return true;
    }
    bool observe_source_world_matrices(const NativeBattleMvp& world,NativeGRedPracticeWorldMatrices* out)const{
        if(!out||!current_source_frame(world)||world_.matrices.size()<=7)return false;
        *out={last_tick_,world_.matrices[0],world_.matrices[7]};return true;
    }
    bool source_pose_published(const NativeBattleMvp& world,const NativeBorgSourcePose& pose)const{
        const auto published=current_source_frame(world);
        return published&&pose.frame_scope.lock()==published&&pose.binding.lock()==scene_.instance_binding()&&
            world.motion_receipt_current(pose.receipt);
    }
    std::weak_ptr<const CharacterModelBinding> render_binding()const{return scene_.instance_binding();}
    void upload(RenderEngine& render,u32 base){scene_.upload_textures_shared(render,base);}
    unsigned draw(RenderEngine& render,const Camera& camera){return ready_&&presentation_ready_&&(!source_pose_active_||!source_frame_scope_.expired())?scene_.draw(render,camera):0;}
    bool bone_point(unsigned bone,const BattleMvpVec& offset,BattleMvpVec* out)const{
        if(!ready_||!presentation_ready_||(source_pose_active_&&source_frame_scope_.expired())||!scene_.ready()||!out||bone>=world_.matrices.size())return false;
        mat4_transform_point(world_.matrices[bone],offset.data(),out->data());return true;
    }
    bool package_socket(const BattleMvpActor& actor,u32 tick,u8 move,u16 frame,const BorgBattleSocket& spec,
                        const CharacterLocalPose* accepted_movement,BattleMvpVec* out)const{
        if(!out||!ready_||!package_profile_||actor.model!=model_id_||!presentation_ready_||tick!=last_tick_+1u||
           spec.bone>=scene_.rig().binding.bones.size()||
           bool(accepted_movement)!=(package_profile_->movement==BorgBattleMovement::NormalGRed))return false;
        auto candidate=actor;candidate.package_move=move;candidate.package_frame=frame;
        candidate.upper_body_yaw=native_borg_aim_delta(actor.facing,actor.aim_facing);
        CharacterLocalPose local;CharacterWorldPose world;
        if(accepted_movement){
            local=*accepted_movement;
            if(!compose_package_world(candidate,local,&world,nullptr))return false;
        }else if(!sample_package_local(candidate,package_clock(candidate,tick),&local,nullptr)||
                !compose_package_world(candidate,local,&world,nullptr))return false;
        mat4_transform_point(world.matrices[spec.bone],spec.local.data(),out->data());return true;
    }
    bool bounds(BattleMvpVec* low,BattleMvpVec* high)const{
        if(!ready_||!presentation_ready_||(source_pose_active_&&source_frame_scope_.expired())||!low||!high||!scene_.ready())return false;
        *low={1e7f,1e7f,1e7f};*high={-1e7f,-1e7f,-1e7f};bool any=false;
        for(const auto& mesh:scene_.world_geometry().world_vertices)for(const auto& vertex:mesh){any=true;for(unsigned k=0;k<3;++k){(*low)[k]=std::min((*low)[k],vertex.position[k]);(*high)[k]=std::max((*high)[k],vertex.position[k]);}}
        return any;
    }
private:
    friend class NativeBattleMvpView;
    bool load_package_clip(BorgBattleClip choice,HsdBoneMotionClip* out,std::string* error)const{
        CharacterMotionSelection selected;
        if(!character_motion_select(bank_,choice.group,choice.choice,&selected,error)||selected.fallback){
            if(error)*error="Borg gameplay: exact clip required; no fallback";
            return false;
        }
        return hsd_bone_motion_load(selected.archive,scene_.rig().binding,out,error);
    }
    bool apply_package_action(const BattleMvpActor& actor,CharacterLocalPose* local,CharacterPoseFrame* frame,std::string* error)const{
        if(actor.package_move==BorgBattleActionState::none)return true;
        if(!package_profile_||actor.package_move>=package_profile_->moves.size())return false;
        const auto& clip=package_moves_[actor.package_move];HsdBoneMotionSample sample;
        const auto start=package_profile_->moves[actor.package_move].pose_start;
        if(!hsd_bone_motion_sample(clip,std::min(u32(clip.end_frame),u32(start)+actor.package_frame),&sample,error))return false;
        const auto& rig=scene_.rig();
        std::array<CharacterPoseLane,4> lanes;for(auto& lane:lanes)lane.current=&sample;
        CharacterLocalPose attack;
        if(!character_pose_sample(rig,lanes,&attack,error))return false;
        for(unsigned i=rig.cuts[0];i<rig.cuts[1];++i)local->bones[rig.order[i]]=attack.bones[rig.order[i]];
        if(torso_!=kHsdNoBone)frame->extra_angles[torso_][1]=std::bit_cast<s16>(u16(s32(std::lround(actor.upper_body_yaw*32768.f/3.14159265359f))));
        return true;
    }
    static unsigned package_locomotion_role(const BattleMvpActor& actor){
        return actor.dash_ticks?3:!actor.grounded?2:actor.movement_speed>.05f?1:0;
    }
    u32 package_clock(const BattleMvpActor& actor,u32 tick)const{
        if(last_tick_==std::numeric_limits<u32>::max()||tick<last_tick_||package_locomotion_role(actor)!=base_)return 0;
        return clock_+tick-last_tick_;
    }
    bool sample_package_local(const BattleMvpActor& actor,u32 clock,CharacterLocalPose* out,std::string* error)const{
        const unsigned role=package_locomotion_role(actor);
        const auto& clip=package_locomotion_[role];const u32 length=u32(clip.end_frame)+1;
        HsdBoneMotionSample sample;
        if(!hsd_bone_motion_sample(clip,role<2?clock%length:std::min(clock,length-1),&sample,error))return false;
        std::array<CharacterPoseLane,4> lanes;for(auto& lane:lanes)lane.current=&sample;
        return character_pose_sample(scene_.rig(),lanes,out,error);
    }
    bool compose_package_world(const BattleMvpActor& actor,CharacterLocalPose& local,CharacterWorldPose* out,std::string* error)const{
        CharacterPoseFrame frame;
        const auto root=mat4_multiply(mat4_translation(actor.position.data()),mat4_rotation_y(std::atan2(actor.facing.x,actor.facing.z)));
        return character_pose_initial_frame(scene_.rig(),root,&frame,error)&&apply_package_action(actor,&local,&frame,error)&&
            character_pose_direct_world(scene_.rig(),local,frame,out,error);
    }
    bool update_package(const BattleMvpActor& actor,u32 tick,std::string* error){
        presentation_ready_=false;
        const auto role=package_locomotion_role(actor);const auto clock=package_clock(actor,tick);
        CharacterLocalPose local;
        if(!sample_package_local(actor,clock,&local,error)||!compose_package_world(actor,local,&world_,error)||
           !scene_.set_world_pose(world_,{},error))return false;
        displayed_=std::move(local);base_=role;clock_=clock;last_tick_=tick;last_position_=actor.position;last_facing_=actor.facing;last_aim_yaw_=actor.upper_body_yaw;
        source_pose_active_=false;source_frame_scope_.reset();presentation_ready_=true;return true;
    }
    bool observe_receiver_key2(const BattleMvpActor& actor,u32 tick,Mat4* out)const{
        if(!out||!ready_||!presentation_ready_||!scene_.ready()||source_pose_active_||
           model_id_!=0x000a||actor.model!=model_id_||!actor.health||actor.source_ground_motion||
           tick!=last_tick_||actor.position!=last_position_||actor.facing!=last_facing_||
           actor.upper_body_yaw!=last_aim_yaw_||world_.matrices.size()<=2)return false;
        *out=world_.matrices[2];return true;
    }
    std::shared_ptr<const NativeBorgSourcePoseScope> current_source_frame(const NativeBattleMvp& world)const{
        if(!ready_||!presentation_ready_||!source_pose_active_||!scene_.ready()||last_tick_!=world.tick())return {};
        const auto owned=source_frame_scope_.lock();const auto binding=scene_.instance_binding();
        return owned&&binding&&owned->binding_.lock()==binding&&world.motion_receipt_current(owned->receipt_)?owned:nullptr;
    }
    bool sample(unsigned index,f64 time,bool loop,HsdBoneMotionSample* a,HsdBoneMotionSample* b,f32* fraction,std::string* error)const{
        const auto& clip=clips_[index];const u32 length=std::max(1u,u32(clip.end_frame)+1);
        const f64 frame=loop?std::fmod(time,length):std::min(time,f64(length-1));const u32 first=u32(frame);
        *fraction=f32(frame-first);
        if(!hsd_bone_motion_sample(clip,first,a,error))return false;
        return *fraction==0||hsd_bone_motion_sample(clip,loop?(first+1)%length:std::min(first+1,length-1),b,error);
    }
    CharacterModelScene scene_;CharacterMotionBank bank_;NativeBorgMotionMap map_;std::array<HsdBoneMotionClip,unsigned(NativeBorgMotion::Count)> clips_;std::array<f32,unsigned(NativeBorgMotion::Count)> stride_{};CharacterWorldPose world_;
    CharacterLocalPose displayed_;std::array<CharacterLocalPose,4> blend_from_;std::array<f32,4> blend_remaining_{};
    u16 model_id_=0;u8 torso_=kHsdNoBone;unsigned base_=0,upper_=0;u32 clock_=0,attack_clock_=0,last_tick_=0;bool ready_=false,presentation_ready_=false,source_pose_active_=false;BattleMvpVec last_position_{};BattlePoint last_facing_{};f32 last_aim_yaw_=0;
    std::weak_ptr<const NativeBorgSourcePoseScope> source_frame_scope_;
    std::shared_ptr<const BorgGameplayProfile> package_profile_;
    std::array<HsdBoneMotionClip,4> package_locomotion_;
    std::array<HsdBoneMotionClip,kBorgBattleMoves> package_moves_;
};
}
