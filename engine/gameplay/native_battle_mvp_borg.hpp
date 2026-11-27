#pragma once
#include "engine/gameplay/borg_trailing_chain.hpp"
#include "engine/gameplay/native_battle_mvp.hpp"
#include "engine/gameplay/native_borg_motion_map.hpp"
#include "engine/gameplay/character_model_scene.hpp"
#include "engine/gameplay/attachment_model_scene.hpp"
#include "engine/gameplay/animated_model_scene.hpp"
#include <map>
namespace gf {
struct ProvisionalBorgJumpClips {BattleMvpMotionChoice takeoff,air,land;};
struct ProvisionalBorgDashClips {BattleMvpMotionChoice forward,left,backward,right;};
struct ProvisionalBorgAirDashClips {BattleMvpMotionChoice windup,burst,recovery;std::array<u16,3> starts{};};
struct ProvisionalBorgGlideClips {BattleMvpMotionChoice entry,held;};
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
        package_profile_.reset();provisional_walk_=provisional_jump_=provisional_dash_=provisional_air_dash_=provisional_glide_=provisional_cruise_=false;
        part_records_.clear();animated_parts_.clear();static_parts_.clear();active_parts_={};mount_turns_={};
        ready_=presentation_ready_=source_pose_active_=false;source_frame_scope_.reset();model_id_=0;world_={};displayed_={};blend_from_={};blend_remaining_={};
        const u32 expected=model==0x615?0xe2b4439au:model==10?0xeab3632au:0;
        if(!expected||!scene_.load(hsd,prefix,pose,error))return false;
        const auto identity=scene_.instance_binding();
        if(!identity||identity->model_hash!=expected){if(error)*error="native MVP: only original normal G-Red/Sasuke models are supported";return false;}
        if(!character_motion_bank_load(motions,&bank_,error))return false;
        map_=native_borg_motion_map(model);
        for(unsigned i=0;i<=unsigned(NativeBorgMotion::AirDash);++i){
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
        package_profile_.reset();provisional_walk_=provisional_jump_=provisional_dash_=provisional_air_dash_=provisional_glide_=provisional_cruise_=false;part_records_.clear();animated_parts_.clear();static_parts_.clear();active_parts_={};mount_turns_={};
        auto fail=[&](std::string why){ready_=false;if(error)*error=std::move(why);return false;};
        if(!profile||!borg_gameplay_profile_valid(*profile))return fail("Borg gameplay: missing or invalid definition");
        const auto movement=profile->movement;
        if(movement==BorgBattleMovement::NormalGRed){
            if(!load(0x0615,hsd,prefix,pose,motions,error))return false;
        }else{
            reset_control_test_pose();ready_=false;model_id_=0;world_={};displayed_={};
            bool body;
            const auto material=profile->body_diffuse_overwrite_pair?
                HsdStaticMaterialPolicy::DiffuseOverwritePair:
                profile->body_immutable_highlight?HsdStaticMaterialPolicy::ImmutableHighlight:
                HsdStaticMaterialPolicy::Legacy;
            if(profile->body_mounts.empty()){
                body=profile->record_zero_body?scene_.load_record_zero(hsd,prefix,pose,error,material):
                                              scene_.load(hsd,prefix,pose,error,material);
            }else{
                HsdRecordAnimationBank bank;
                body=bank.load(hsd,error,true,material)&&scene_.load_assembled_record_zero(bank,prefix,pose,profile->body_mounts,error);
            }
            if(!body)return false;
            if(!profile->zero_reflections.empty()&&!scene_.bind_zero_reflections(profile->zero_reflections,error))return false;
            if(profile->trailing_chain&&!borg_trailing_chain_binds(*profile->trailing_chain,scene_.rig().binding))
                return fail("Borg gameplay: trailing-chain must bind one exact terminal rig branch");
            if(profile->static_body()){
                if(motions.size)return fail("Borg gameplay: static body cannot bind a motion bank");
            }else if(!character_motion_bank_load(motions,&bank_,error))return false;
            if(profile->cruise_entry&&!load_package_clip(*profile->cruise_entry,&package_locomotion_[2],error))return false;
            for(unsigned i=0;i<((profile->static_body()||profile->cruise_entry)?0u:
                              movement==BorgBattleMovement::Stationary?1u:profile->dash_disabled?3u:4u);++i)
                if(!load_package_clip(profile->locomotion[i],&package_locomotion_[i],error,
                       ((i==0&&profile->idle_authored_loop)||(i==1&&profile->walk_authored_loop))?
                           HsdBoneMotionLoadMode::NativeLocomotionLoop:HsdBoneMotionLoadMode::Strict))return false;
            for(unsigned i=0;i<profile->directional_dash.size();++i)
                if(const auto& clip=profile->directional_dash[i];clip&&!load_package_clip(*clip,&package_locomotion_[4+i],error))return false;
            if(profile->air_dash_clip&&(!load_package_clip(*profile->air_dash_clip,&package_locomotion_[7],error)||
               package_locomotion_[7].end_frame<f32(profile->air_dash_start)))return fail("Borg gameplay: air dash clip or start refused");
            if((profile->takeoff_clip&&!load_package_clip(*profile->takeoff_clip,&package_locomotion_[8],error))||
               (profile->landing_clip&&!load_package_clip(*profile->landing_clip,&package_locomotion_[9],error)))return false;
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
        ready_=false; // only the complete package can publish, even after the legacy body bind
        if(std::any_of(profile->moves.begin(),profile->moves.end(),[](const auto& move){return !move.static_pose;})&&
           (!(scene_.rig().lane_mask&1u)||scene_.rig().cuts[0]==scene_.rig().cuts[1]))
            return fail("Borg gameplay: action clips require a bound upper lane");
        if(profile->moves.size()>package_moves_.size())return fail("Borg gameplay: action clip budget");
        if(!profile->part_slots.empty()){
            AttachmentModelBank parts;
            HsdRecordAnimationBank animations;
            const bool prepared=!profile->part_materials.empty()||!profile->static_part_hierarchies.empty();
            if(prepared&&(hsd.size>(4u<<20)||!animations.load(hsd,error,true)))return false;
            const auto under_budget=[&](){return animated_parts_.size()+part_records_.size()+static_parts_.size()<kBorgBattlePartRecords;};
            const auto bind=[&](u8 slot,u16 record){
                if(!record)return true;
                if(std::find(profile->static_part_hierarchies.begin(),profile->static_part_hierarchies.end(),record)!=profile->static_part_hierarchies.end()){
                    if(static_parts_.contains(record))return true;
                    if(!under_budget())return false;
                    auto& part=static_parts_[record];
                    if(!part.load_compact_static(animations,record,error)||!part.request(0,0,error))return false;
                    part.set_enabled(true);return true;
                }
                const auto material=std::find_if(profile->part_materials.begin(),profile->part_materials.end(),
                    [record](const auto& m){return m.record==record;});
                if(material!=profile->part_materials.end()){
                    const auto key=std::pair{slot,record};
                    if(animated_parts_.contains(key))return true;
                    if(!under_budget())return false;
                    auto& part=animated_parts_[key];part.clip=*material;
                    if(!part.scene.load_compact_textures(animations,record,2,0,material->clip,error)||
                       !part.scene.supports_material_window(material->last))return false;
                    part.scene.set_enabled(true);
                    return true;
                }
                if(part_records_.contains(record))return true;
                if(!parts.ready()&&!(prepared?parts.load_prepared(animations,error):parts.load(hsd,error)))return false;
                return under_budget()&&part_records_[record].load_shared(parts,record,error);
            };
            for(u8 i=0;i<profile->part_slots.size();++i){
                const auto& slot=profile->part_slots[i];
                if(slot.bone>=scene_.rig().binding.bones.size()||!bind(i,slot.record))
                    return fail("Borg gameplay: unbound bone or unavailable rigid part-record for "+slot.id);
            }
            for(const auto& move:profile->moves)for(const auto& swap:move.part_swaps){
                if(swap.bone&&*swap.bone>=scene_.rig().binding.bones.size())
                    return fail("Borg gameplay: unbound part-swap bone for "+move.id);
                if(!bind(swap.slot,swap.record))return fail("Borg gameplay: unavailable part-swap record or material clip for "+move.id);
            }
        }
        for(unsigned i=0;i<profile->moves.size();++i){
            const auto& move=profile->moves[i];
            if(move.field&&move.field->socket&&move.field->socket->bone>=scene_.rig().binding.bones.size())
                return fail("Borg gameplay: unbound field-socket bone for "+move.id);
            // An action with the authored loop bit samples one bounded
            // window; unlike locomotion it never wraps on the native clock.
            const auto action_mode=move.pose_authored_loop_window?HsdBoneMotionLoadMode::NativeActionLoopWindow:HsdBoneMotionLoadMode::Strict;
            const auto required_end=move.pose_start+(move.pose_followup?unsigned(move.pose_followup->frame-1):
                move.pose_authored_loop_window?unsigned(move.duration-1):move.last_marker());
            if(!move.static_pose&&(!load_package_clip(move.clip,&package_moves_[i],error,action_mode)||
               package_moves_[i].end_frame<f32(required_end)))
                return fail("Borg gameplay: unavailable action clip or active frame outside clip for "+move.id);
            if(const auto& follow=move.pose_followup;follow&&
               (!load_package_clip(follow->clip,&package_followups_[i],error)||
                package_followups_[i].end_frame<f32(follow->start+move.duration-1-follow->frame)))
                return fail("Borg gameplay: unavailable exact follow-up clip or incomplete follow-up window for "+move.id);
            for(const auto& spin:move.part_spins)
                if(spin.bone>=scene_.rig().binding.bones.size())
                    return fail("Borg gameplay: unbound part-spin bone for "+move.id);
            for(const auto& scroll:move.part_scrolls)
                if(!scene_.supports_texture_scroll(scroll.bone,scroll.axis))
                    return fail("Borg gameplay: part-scroll needs explicit repeating texture coordinates for "+move.id);
            auto& parts=package_parts_[i];
            parts.clear();
            for(const auto& part:move.part_motions){
                HsdBoneMotionClip clip;
                if(part.bone>=scene_.rig().binding.bones.size()||!load_package_clip(part.clip,&clip,error)||
                   clip.bones[part.bone].tracks.empty()||clip.end_frame<f32(u32(part.first)+part.count-1))
                    return fail("Borg gameplay: unavailable decorative clip or bone for "+move.id);
                parts.push_back(std::move(clip));
            }
        }
        for(const auto& shot:profile->projectiles)
            if(shot.socket&&shot.socket->bone>=scene_.rig().binding.bones.size())
                return fail("Borg gameplay: unbound origin bone for "+shot.id);
        for(const auto& fx:profile->movement_fx)
            if(fx.bone>=scene_.rig().binding.bones.size())
                return fail("Borg gameplay: unbound movement effect bone for "+fx.id);
        scene_.set_ground_offset(profile->pose_ground_offset);
        package_profile_=std::move(profile);ready_=true;return true;
    }
    // Explicit no-combat walk lab. Uses the same pose and body renderer as
    // Battle Test, but accepts no gameplay definition or source actor.
    bool load_provisional_walk(ByteView model,ByteView prefix,ByteView motions,
                               const std::array<BattleMvpMotionChoice,2>& roles,std::string* error,
                               const ProvisionalBorgJumpClips* jump=nullptr,std::span<const CharacterPreviewMount> mounts={},
                               const ProvisionalBorgDashClips* dash=nullptr,const ProvisionalBorgAirDashClips* air_dash=nullptr,
                               const ProvisionalBorgGlideClips* glide=nullptr,bool cruise=false,
                               const BattleMvpMotionChoice* cruise_entry=nullptr){
        NativeBattleMvpBorg next;
        HsdRecordAnimationBank bank;
        std::vector<u8> pose;
        if(!bank.load(model,error,true,HsdStaticMaterialPolicy::BodyInspection)||
           !character_pose_whole_body_profile(model,prefix,&pose,error)||
           !next.scene_.load_preview_with_record_bank(bank,prefix,{pose.data(),pose.size()},error,
                                                        CharacterModelScene::Inspection::ProvisionalWalk,mounts))return false;
        if((cruise&&(bool(motions.size)!=bool(cruise_entry)||jump||dash||air_dash||glide))||(!cruise&&cruise_entry)){
            if(error)*error="cruise lab: exact body and motion policy required";
            return false;
        }
        if(cruise&&!cruise_entry){
            next.provisional_walk_=next.provisional_cruise_=next.ready_=true;
            next.reset_control_test_pose();*this=std::move(next);return true;
        }
        if(!character_motion_bank_load(motions,&next.bank_,error))return false;
        auto bind=[&](NativeBorgMotion role,BattleMvpMotionChoice key,
                      HsdBoneMotionLoadMode mode=HsdBoneMotionLoadMode::InspectAuthoredLoop){
            CharacterMotionSelection selected;
            if(!character_motion_select(next.bank_,key.group,key.choice,&selected,error)||selected.fallback){
                if(error&&selected.fallback)*error="movement lab: exact mapped clips required";
                return false;
            }
            if(!hsd_bone_motion_load(selected.archive,next.scene_.rig().binding,&next.clips_[unsigned(role)],error,mode))return false;
            next.stride_[unsigned(role)]=120; // native presentation cadence, not retail scale
            return true;
        };
        if(cruise_entry){
            if(!bind(NativeBorgMotion::LabCruise,*cruise_entry,HsdBoneMotionLoadMode::Strict))return false;
            next.provisional_walk_=next.provisional_cruise_=next.provisional_animated_cruise_=next.ready_=true;
            next.reset_control_test_pose();*this=std::move(next);return true;
        }
        if(!bind(NativeBorgMotion::Idle,roles[0])||!bind(NativeBorgMotion::Walk,roles[1]))return false;
        if(jump&&(!bind(NativeBorgMotion::Takeoff,jump->takeoff)||!bind(NativeBorgMotion::Fall,jump->air)||
                  !bind(NativeBorgMotion::Land,jump->land)))return false;
        if(dash&&(!bind(NativeBorgMotion::DashForward,dash->forward)||!bind(NativeBorgMotion::DashLeft,dash->left)||
                  !bind(NativeBorgMotion::DashBackward,dash->backward)||!bind(NativeBorgMotion::DashRight,dash->right)))return false;
        if(air_dash&&(!jump||!bind(NativeBorgMotion::LabAirWindup,air_dash->windup)||
                      !bind(NativeBorgMotion::AirDash,air_dash->burst)||!bind(NativeBorgMotion::LabAirRecovery,air_dash->recovery)))return false;
        if(air_dash){
            constexpr std::array phase{NativeBorgMotion::LabAirWindup,NativeBorgMotion::AirDash,NativeBorgMotion::LabAirRecovery};
            for(unsigned i=0;i<phase.size();++i){
                const auto role=unsigned(phase[i]), start=unsigned(air_dash->starts[i]);
                if(start>600||f32(start)>next.clips_[role].end_frame){
                    if(error)*error="air dash lab: declared start outside exact clip";
                    return false;
                }
                next.lab_start_frames_[role]=u16(start);
            }
        }
        if(glide&&(!jump||air_dash||!bind(NativeBorgMotion::LabGlideEntry,glide->entry)||!bind(NativeBorgMotion::LabGlide,glide->held)))return false;
        next.provisional_jump_=jump!=nullptr;
        next.provisional_dash_=dash!=nullptr;
        next.provisional_air_dash_=air_dash!=nullptr;
        next.provisional_glide_=glide!=nullptr;
        next.provisional_walk_=next.ready_=true;next.reset_control_test_pose();
        *this=std::move(next);return true;
    }
    bool update(const BattleMvpActor& actor,u32 tick,std::string* error,const IBattleMvpTerrain* terrain=nullptr){
        package_pose_held_=false;
        if(!ready_||actor.model!=model_id_||actor.source_ground_motion){presentation_ready_=false;if(error)*error="native Borg: accepted source pose required for source movement";return false;}
        if(package_profile_&&package_profile_->movement!=BorgBattleMovement::NormalGRed)return update_package(actor,tick,error,terrain);
        if(provisional_cruise_&&(actor.grounded||actor.attack_ticks||actor.dash_ticks)){
            presentation_ready_=false;if(error)*error="cruise lab requires an airborne no-combat pose";return false;
        }
        if(provisional_cruise_&&!provisional_animated_cruise_){
            presentation_ready_=false;
            CharacterLocalPose local;CharacterPoseFrame frame;
            const auto root=mat4_multiply(mat4_translation(actor.position.data()),mat4_rotation_y(std::atan2(actor.facing.x,actor.facing.z)));
            if(!character_pose_initial_local(scene_.rig(),&local,error)||!character_pose_initial_frame(scene_.rig(),root,&frame,error)||
               !character_pose_direct_world(scene_.rig(),local,frame,&world_,error)||!scene_.set_world_pose(world_,{},error))return false;
            base_=unsigned(actor.package_boosting?NativeBorgMotion::LabBoost:NativeBorgMotion::LabCruise);
            presentation_ready_=true;return true;
        }
        if(provisional_walk_&&((!actor.grounded&&!provisional_jump_&&!provisional_cruise_)||actor.attack_ticks||
           (actor.dash_ticks&&(actor.dash_airborne?(!provisional_air_dash_||actor.grounded):(!provisional_dash_||!actor.grounded)))||
           ((actor.lab_air_windup_ticks||actor.lab_air_recovery_ticks)&&(!provisional_air_dash_||actor.grounded))||
           (actor.lab_gliding&&(!provisional_glide_||actor.grounded)))){
            presentation_ready_=false;if(error)*error="movement lab: state outside admitted movement roles";return false;
        }
        const auto motion=provisional_cruise_?NativeBorgMotion::LabCruise:
            provisional_air_dash_&&actor.lab_air_windup_ticks?NativeBorgMotion::LabAirWindup:
            provisional_air_dash_&&actor.lab_air_recovery_ticks?NativeBorgMotion::LabAirRecovery:
            provisional_air_dash_&&actor.dash_ticks&&actor.dash_airborne?NativeBorgMotion::AirDash:
            provisional_dash_&&actor.dash_ticks?native_borg_ground_dash_motion(actor.dash_motion):
            provisional_glide_&&actor.lab_gliding?(actor.lab_glide_entry_ticks?NativeBorgMotion::LabGlideEntry:NativeBorgMotion::LabGlide):
            provisional_jump_&&!actor.grounded?
            (actor.takeoff_ticks?NativeBorgMotion::Takeoff:NativeBorgMotion::Fall):
            provisional_jump_&&actor.landing_ticks?NativeBorgMotion::Land:native_borg_select_motion(actor,map_);
        const unsigned base=unsigned(motion);
        const unsigned upper=actor.attack_ticks?2:base;
        if(presentation_ready_&&scene_.ready()&&tick==last_tick_&&base==base_&&upper==upper_&&actor.position==last_position_&&actor.facing==last_facing_&&actor.upper_body_yaw==last_aim_yaw_&&actor.reaction_yaw==last_reaction_yaw_&&actor.reaction_pitch==last_reaction_pitch_)return true;
        presentation_ready_=false;
        const bool reset=last_tick_==std::numeric_limits<u32>::max()||tick<last_tick_;
        const u32 elapsed=reset?0:tick-last_tick_;
        if(reset||base!=base_)clock_=0;else clock_+=elapsed;
        if(reset||upper!=upper_)attack_clock_=0;else attack_clock_+=elapsed;
        const bool locomotion=base==1;
        f64 body_time=locomotion?actor.travel_distance/stride_[base]*(u32(clips_[base].end_frame)+1):clock_;
        const auto start_frame=lab_start_frames_[base];
        const auto phase_time=[&](unsigned ticks,unsigned duration){
            return start_frame+native_borg_phase_time(ticks,duration,clips_[base].end_frame-start_frame);
        };
        if(actor.dash_ticks&&(map_.user_classified||provisional_dash_||provisional_air_dash_))body_time=phase_time(actor.dash_ticks,actor.dash_duration);
        else if(motion==NativeBorgMotion::LabAirWindup)body_time=phase_time(actor.lab_air_windup_ticks,BattleMvpActor::lab_air_windup);
        else if(motion==NativeBorgMotion::LabAirRecovery)body_time=phase_time(actor.lab_air_recovery_ticks,BattleMvpActor::lab_air_recovery);
        else if(motion==NativeBorgMotion::LabGlideEntry)body_time=native_borg_phase_time(actor.lab_glide_entry_ticks,BattleMvpActor::lab_glide_entry,clips_[base].end_frame);
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
            const bool lab_air_phase=provisional_air_dash_&&(motion==NativeBorgMotion::LabAirWindup||
                motion==NativeBorgMotion::AirDash||motion==NativeBorgMotion::LabAirRecovery);
            const f32 blend_ticks=lab_air_phase||motion==NativeBorgMotion::LabGlideEntry?2.f:
                native_borg_motion_blend_ticks(NativeBorgMotion(index),map_.user_classified);
            if(reset)blend_remaining_[lane]=0;
            else if(index!=old_index&&!displayed_.bones.empty()){blend_from_[lane]=displayed_;blend_remaining_[lane]=blend_ticks;}
            else blend_remaining_[lane]=std::max(0.f,blend_remaining_[lane]-f32(elapsed));
            if(blend_remaining_[lane]>0&&!character_pose_detail::blend_lane(scene_.rig(),lane,blend_remaining_[lane]/blend_ticks,blend_from_[lane].bones,local.bones))return false;
        }
        const auto root=mat4_multiply(mat4_translation(actor.position.data()),mat4_rotation_y(std::atan2(actor.facing.x,actor.facing.z)));
        if(!character_pose_initial_frame(scene_.rig(),root,&frame,error))return false;
        if(provisional_walk_)apply_reaction_pose(actor,frame);
        else apply_reaction_angles(actor,frame);
        if(!character_pose_direct_world(scene_.rig(),local,frame,&world_,error)||!scene_.set_world_pose(world_,{},error))return false;
        displayed_=std::move(local);base_=base;upper_=upper;last_tick_=tick;last_position_=actor.position;last_facing_=actor.facing;last_aim_yaw_=actor.upper_body_yaw;last_reaction_yaw_=actor.reaction_yaw;last_reaction_pitch_=actor.reaction_pitch;
        source_pose_active_=false;source_frame_scope_.reset();presentation_ready_=true;return true;
    }
    bool animation_seed(const BattleMvpActor& actor,u32 tick,NativeBorgAnimationSeed* out)const{
        if(!out||!ready_||!presentation_ready_||!scene_.ready()||actor.source_ground_motion||actor.model!=0x615||model_id_!=0x615||
           tick!=last_tick_||actor.position!=last_position_||actor.facing!=last_facing_)return false;
        *out={scene_.rig(),displayed_,scene_.instance_binding(),tick,0,0,actor.position,actor.facing,{}};return true;
    }
    void refuse_source_pose(){presentation_ready_=false;}
    void reset_control_test_pose(){
        presentation_ready_=false;source_pose_active_=false;package_pose_held_=false;source_frame_scope_.reset();
        active_parts_={};mount_turns_={};trailing_chain_={};
        last_tick_=std::numeric_limits<u32>::max();
    }
    bool update_source_pose(const NativeBattleMvp& accepted,const NativeBorgSourcePose& source,std::string* error){
        return update_source_pose(0,accepted,source,error);
    }
    bool update_source_pose(unsigned index,const NativeBattleMvp& accepted,const NativeBorgSourcePose& source,std::string* error){
        if(index>=accepted.actor_count())return false;
        const auto& actor=accepted.actors()[index];const auto tick=accepted.tick();
        presentation_ready_=false;package_pose_held_=false;
        const auto owned=source.frame_scope.lock();const auto binding=scene_.instance_binding();
        if(!ready_||!owned||!binding||source.binding.lock()!=binding||owned->binding_.lock()!=binding||
           !accepted.motion_receipt_current(source.receipt,index)||!accepted.motion_receipt_current(owned->receipt_,index)||
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
        if(!character_pose_direct_world(scene_.rig(),*local,frame,&world,error)||
           !present_package_pose(actor,tick,*local,frame,world,error))return false;
        world_=std::move(world);displayed_=*local;last_tick_=tick;last_position_=actor.position;last_facing_=actor.facing;last_aim_yaw_=actor.upper_body_yaw;
        last_reaction_yaw_=actor.reaction_yaw;last_reaction_pitch_=actor.reaction_pitch;
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
    void upload(RenderEngine& render,u32 base){
        scene_.upload_textures_shared(render,base);
        for(auto& pair:part_records_)pair.second.upload_textures_shared(render,base);
        for(auto& pair:animated_parts_)pair.second.scene.upload_textures_shared(render,base);
        for(auto& pair:static_parts_)pair.second.upload_textures_shared(render,base);
    }
    unsigned draw(RenderEngine& render,const Camera& camera,unsigned* parts_issued=nullptr,unsigned* body_issued=nullptr){
        if(!ready_||!presentation_ready_||(source_pose_active_&&source_frame_scope_.expired()))return 0;
        const auto count=scene_.draw(render,camera);
        if(body_issued)*body_issued+=count;
        unsigned parts=0;
        for(unsigned i=0;i<active_parts_.size();++i)if(active_parts_[i].record){
            if(const auto entry=static_parts_.find(active_parts_[i].record);entry!=static_parts_.end()){
                if(entry->second.set_root_world(active_parts_[i].transform.world,nullptr))parts+=entry->second.draw(render,camera);
                continue;
            }
            if(const auto animated=animated_parts_.find({u8(i),active_parts_[i].record});animated!=animated_parts_.end()){
                parts+=animated->second.scene.draw(render,camera);continue;
            }
            auto& part=part_records_.at(active_parts_[i].record);
            // A record may be shared by several slots. Publish the slot's
            // own transform immediately before issuing its draw.
            if(part.set_world_static(active_parts_[i].transform,nullptr))parts+=part.draw(render,camera);
        }
        if(parts_issued)*parts_issued+=parts;
        return count+parts;
    }
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
    bool package_pose_held()const{return presentation_ready_&&package_pose_held_;}
private:
    friend class NativeBattleMvpView;
    bool load_package_clip(BorgBattleClip choice,HsdBoneMotionClip* out,std::string* error,
                           HsdBoneMotionLoadMode mode=HsdBoneMotionLoadMode::Strict)const{
        CharacterMotionSelection selected;
        if(!character_motion_select(bank_,choice.group,choice.choice,&selected,error)||selected.fallback){
            if(error)*error="Borg gameplay: exact clip required; no fallback";
            return false;
        }
        return hsd_bone_motion_load(selected.archive,scene_.rig().binding,out,error,mode);
    }
    // Recovered 800300BC leftover: yaw on the upper-body root's Y axis and
    // pitch on X. The pitch axis is an authored native mapping; the source
    // writes +0x282, whose native bone axis is not yet verified on screen.
    static s16 pose_angle(f32 value){
        return std::bit_cast<s16>(u16(s32(std::lround(value*32768.f/3.14159265359f))));
    }
    void apply_reaction_angles(const BattleMvpActor& actor,CharacterPoseFrame& frame)const{
        if(torso_==kHsdNoBone)return;
        frame.extra_angles[torso_][1]=pose_angle(actor.upper_body_yaw+actor.reaction_yaw);
        if(actor.reaction_pitch)frame.extra_angles[torso_][0]=pose_angle(actor.reaction_pitch);
    }
    // The provisional exact-clip path keeps its no-aim rule but still shows a
    // hit reaction: only the reaction angles are added.
    void apply_reaction_pose(const BattleMvpActor& actor,CharacterPoseFrame& frame)const{
        if(torso_==kHsdNoBone||(!actor.reaction_yaw&&!actor.reaction_pitch))return;
        frame.extra_angles[torso_][1]=pose_angle(actor.reaction_yaw);
        if(actor.reaction_pitch)frame.extra_angles[torso_][0]=pose_angle(actor.reaction_pitch);
    }
    bool apply_package_action(const BattleMvpActor& actor,CharacterLocalPose* local,CharacterPoseFrame* frame,std::string* error)const{
        if(actor.package_move==BorgBattleActionState::none)return true;
        if(!package_profile_||actor.package_move>=package_profile_->moves.size())return false;
        const auto& move=package_profile_->moves[actor.package_move];
        if(move.static_pose)return true;
        const auto& follow=move.pose_followup;
        const bool continued=follow&&actor.package_frame>=follow->frame;
        const auto& clip=(continued?package_followups_:package_moves_)[actor.package_move];
        const u32 sample_frame=continued?u32(follow->start)+actor.package_frame-follow->frame:
            u32(move.pose_start)+actor.package_frame;
        HsdBoneMotionSample sample;
        if(!sample_package_clip(clip,std::min(u32(clip.end_frame),sample_frame),&sample,error))return false;
        const auto& rig=scene_.rig();
        std::array<CharacterPoseLane,4> lanes;for(auto& lane:lanes)lane.current=&sample;
        CharacterLocalPose attack;
        if(!character_pose_sample(rig,lanes,&attack,error))return false;
        for(unsigned i=rig.cuts[0];i<rig.cuts[1];++i)local->bones[rig.order[i]]=attack.bones[rig.order[i]];
        apply_reaction_angles(actor,*frame);
        return true;
    }
    unsigned package_locomotion_role(const BattleMvpActor& actor)const{
        // The explicit cruise entry plays once per spawn; boost and B never restart it.
        if(package_profile_->cruise_entry)return 2;
        if(actor.dash_ticks){
            if(actor.dash_airborne)return package_profile_->air_dash_clip?7:3;
            const unsigned direction=unsigned(actor.dash_motion);
            if(direction&&package_profile_->directional_dash[direction-1])return 3+direction;
        }
        if(actor.takeoff_ticks&&!actor.grounded&&package_profile_->takeoff_clip)return 8;
        if(actor.landing_ticks&&actor.grounded&&package_profile_->landing_clip)return 9;
        return actor.dash_ticks||actor.package_boosting?3:!actor.grounded?2:actor.movement_speed>.05f?1:0;
    }
    u32 package_clock(const BattleMvpActor& actor,u32 tick)const{
        if(last_tick_==std::numeric_limits<u32>::max()||tick<last_tick_||package_locomotion_role(actor)!=base_)return 0;
        return clock_+tick-last_tick_;
    }
    bool sample_package_clip(const HsdBoneMotionClip& clip,u32 frame,HsdBoneMotionSample* out,std::string* error)const{
        return hsd_bone_motion_sample(clip,frame,out,error)&&
            character_pose_scale_translations(*out,package_profile_->animated_translation_percent);
    }
    bool sample_package_local(const BattleMvpActor& actor,u32 clock,CharacterLocalPose* out,std::string* error)const{
        const unsigned role=package_locomotion_role(actor);
        if(package_profile_->static_body()){
            const auto& rig=scene_.rig();
            HsdBoneMotionSample sample;
            for(const auto& bone:rig.binding.bones)sample.bones.push_back({bone.joint_offset});
            std::array<CharacterPoseLane,4> lanes;for(auto& lane:lanes)lane.current=&sample;
            return character_pose_sample(rig,lanes,out,error);
        }
        const auto& clip=package_locomotion_[role];const u32 length=u32(clip.end_frame)+1;
        HsdBoneMotionSample sample;
        const unsigned start=role==7?package_profile_->air_dash_start:0;
        if(!sample_package_clip(clip,role<2?clock%length:std::min(clock+start,length-1),&sample,error))return false;
        std::array<CharacterPoseLane,4> lanes;for(auto& lane:lanes)lane.current=&sample;
        return character_pose_sample(scene_.rig(),lanes,out,error);
    }
    bool compose_package_world(const BattleMvpActor& actor,CharacterLocalPose& local,CharacterWorldPose* out,std::string* error)const{
        CharacterPoseFrame frame;
        return build_package_pose(actor,local,&frame,out,error);
    }
    bool build_package_pose(const BattleMvpActor& actor,CharacterLocalPose& local,CharacterPoseFrame* frame,
                            CharacterWorldPose* out,std::string* error)const{
        const auto root=mat4_multiply(mat4_translation(actor.position.data()),mat4_rotation_y(std::atan2(actor.facing.x,actor.facing.z)));
        if(!character_pose_initial_frame(scene_.rig(),root,frame,error)||
           !apply_package_action(actor,&local,frame,error))return false;
        apply_reaction_angles(actor,*frame);
        return character_pose_direct_world(scene_.rig(),local,*frame,out,error);
    }
    bool present_package_pose(const BattleMvpActor& actor,u32 tick,const CharacterLocalPose& local,
                              const CharacterPoseFrame& frame,const CharacterWorldPose& bound,std::string* error,
                              const IBattleMvpTerrain* terrain=nullptr){
        if(!package_profile_)
            return scene_.set_world_pose(bound,{},error);
        const auto present=[&](const BorgBattleMove* move,const CharacterWorldPose& pose,
                               std::span<const CharacterTextureScroll> scrolls={}){
            auto next=trailing_chain_;
            CharacterWorldPose visual;
            const CharacterWorldPose* view=&pose;
            if(const auto& spec=package_profile_->trailing_chain;spec){
                const auto floor=[&](const BattleMvpVec& point,f32* height){
                    return terrain&&terrain->height(point[0],point[2],point[1],height);
                };
                if(!terrain||!next.step(*spec,tick,pose,floor,&visual)){
                    if(error)*error="Borg gameplay: trailing-chain pose refused";
                    return false;
                }
                view=&visual;
            }
            if(!present_package_parts(move,move?actor.package_frame:0,tick,*view,error)||
               !scene_.set_world_pose(*view,{},error,scrolls))return false;
            trailing_chain_=next;return true;
        };
        if(actor.package_move==BorgBattleActionState::none)
            return present(nullptr,bound);
        if(actor.package_move>=package_profile_->moves.size())return false;
        const auto& move=package_profile_->moves[actor.package_move];
        const u32 age=tick-actor.package_action_born;
        std::array<CharacterTextureScroll,kBorgBattlePartAnimations> scrolls;
        for(unsigned i=0;i<move.part_scrolls.size();++i){
            const auto& spec=move.part_scrolls[i];
            const auto phase=f32(age%spec.period)/spec.period;
            scrolls[i]={spec.bone,spec.axis,spec.reverse?-phase:phase};
        }
        const std::span<const CharacterTextureScroll> offsets{scrolls.data(),move.part_scrolls.size()};
        if(move.part_spins.empty()&&move.part_motions.empty())
            return present(&move,bound,offsets);
        auto appearance=frame;
        for(const auto& spin:move.part_spins){
            const u32 phase=age%spin.period;
            const s32 angle=s32(phase*65536u/spin.period);
            auto& target=appearance.extra_angles[spin.bone][spin.axis];
            target=source_heading_wrap(s32(target)+(spin.reverse?-angle:angle));
        }
        auto decorative=local;
        for(unsigned i=0;i<move.part_motions.size();++i){
            const auto& part=move.part_motions[i];HsdBoneMotionSample sample;
            const u32 phase=age%part.count;
            if(!sample_package_clip(package_parts_[actor.package_move][i],u32(part.first)+phase,&sample,error))return false;
            std::array<CharacterPoseLane,4> lanes;for(auto& lane:lanes)lane.current=&sample;
            CharacterLocalPose played;
            if(!character_pose_sample(scene_.rig(),lanes,&played,error))return false;
            decorative.bones[part.bone]=played.bones[part.bone];
        }
        CharacterWorldPose cosmetic;
        // No cosmetic matrices are published through bone_point or launch sockets.
        return character_pose_direct_world(scene_.rig(),decorative,appearance,&cosmetic,error)&&
            present(&move,cosmetic,offsets);
    }
    bool present_package_parts(const BorgBattleMove* move,u16 frame,u32 tick,const CharacterWorldPose& pose,std::string* error){
        for(unsigned i=0;i<package_profile_->part_slots.size();++i){
            const auto& slot=package_profile_->part_slots[i];
            const auto* swap=borg_part_active_swap(u8(i),move,frame);
            const auto record=swap?swap->record:slot.record;
            if(!record){active_parts_[i]={};mount_turns_[i]={};continue;}
            const auto* placement=borg_part_pose(slot,swap);
            const BorgBattlePartPose identity;
            if(!placement)placement=&identity;
            auto degrees=mount_turns_[i].sample(placement->degrees,slot.turn_step,tick);
            if(const auto& spin=slot.mount_spin;spin){
                const f32 advance=f32(tick%spin->period)*360.f/f32(spin->period);
                degrees[spin->axis]=std::remainder(degrees[spin->axis]+(spin->reverse?-advance:advance),360.f);
            }
            AttachmentBoneTransform transform;
            const auto bone=borg_part_bone(slot,swap);
            if(!(slot.basis==BorgPartBasis::WorldZyx?
                   attachment_world_axes_mount_transform(pose.matrices,bone,placement->offset,degrees,&transform,error):
                   attachment_mount_transform(pose.matrices,bone,placement->offset,degrees,&transform,error,
                                              slot.basis==BorgPartBasis::BoneZyx)))return false;
            auto& active=active_parts_[i];
            const bool enter=active.record!=record||tick<active.tick;
            const u32 born=enter?tick:active.born;
            if(const auto animated=animated_parts_.find({u8(i),record});animated!=animated_parts_.end()){
                auto& part=animated->second;const auto& clip=part.clip;const u32 age=tick-born;
                const u32 cycle=u32(clip.last)-clip.first+1;
                const bool restart=enter||(clip.playback==BorgPartMaterialPlayback::Loop&&age/cycle!=(active.tick-born)/cycle);
                unsigned prior=restart?0u:borg_part_material_phase(clip,active.tick-born);
                const unsigned phase=borg_part_material_phase(clip,age);
                if(restart){
                    const bool sample=clip.playback==BorgPartMaterialPlayback::Sample;
                    const bool live=part.scene.ready();
                    if(!(sample?(live?part.scene.request_again_selectors(0,clip.first,error):part.scene.request_selectors(0,clip.first,error))
                               :(live?part.scene.request_again(clip.first,1,error):part.scene.request(clip.first,1,error))))return false;
                }
                for(;prior<phase;++prior)if(!part.scene.advance(1,error))return false;
                if(!part.scene.set_root_world(transform.world,error))return false;
            }
            active={record,transform,born,tick};
        }
        return true;
    }
    bool update_package(const BattleMvpActor& actor,u32 tick,std::string* error,const IBattleMvpTerrain* terrain){
        const bool hold=actor.health&&actor.contact_pose_hold_ticks&&package_profile_->contact_pose_hold_receive&&
            presentation_ready_&&!source_pose_active_&&tick>=last_tick_&&tick-last_tick_<=1&&!displayed_.bones.empty();
        presentation_ready_=false;
        const auto role=package_locomotion_role(actor);const auto clock=package_clock(actor,tick);
        CharacterLocalPose local;
        CharacterPoseFrame frame;
        if(!sample_package_local(actor,clock,&local,error)||!build_package_pose(actor,local,&frame,&world_,error))return false;
        if(hold){
            // Only presentation borrows the last sample. The current root,
            // reaction angles and independent part clocks stay live; world_
            // and the live sampler continue to serve bone/socket queries.
            CharacterWorldPose held;
            if(!character_pose_direct_world(scene_.rig(),displayed_,frame,&held,error)||
               !present_package_pose(actor,tick,displayed_,frame,held,error,terrain))return false;
        }else{
            if(!present_package_pose(actor,tick,local,frame,world_,error,terrain))return false;
            displayed_=std::move(local);
        }
        package_pose_held_=hold;
        base_=role;clock_=clock;last_tick_=tick;last_position_=actor.position;last_facing_=actor.facing;last_aim_yaw_=actor.upper_body_yaw;last_reaction_yaw_=actor.reaction_yaw;last_reaction_pitch_=actor.reaction_pitch;
        source_pose_active_=false;source_frame_scope_.reset();presentation_ready_=true;return true;
    }
    bool observe_receiver_key2(const BattleMvpActor& actor,u32 tick,Mat4* out)const{
        if(!out||!ready_||!presentation_ready_||!scene_.ready()||source_pose_active_||
           model_id_!=0x000a||actor.model!=model_id_||!actor.health||actor.source_ground_motion||
           tick!=last_tick_||actor.position!=last_position_||actor.facing!=last_facing_||
           actor.upper_body_yaw!=last_aim_yaw_||actor.reaction_yaw!=last_reaction_yaw_||actor.reaction_pitch!=last_reaction_pitch_||world_.matrices.size()<=2)return false;
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
    std::array<unsigned,3> texture_tally()const{return scene_.texture_tally();}
    CharacterModelScene scene_;CharacterMotionBank bank_;NativeBorgMotionMap map_;std::array<HsdBoneMotionClip,unsigned(NativeBorgMotion::Count)> clips_;std::array<f32,unsigned(NativeBorgMotion::Count)> stride_{};CharacterWorldPose world_;
    CharacterLocalPose displayed_;std::array<CharacterLocalPose,4> blend_from_;std::array<f32,4> blend_remaining_{};
    std::array<u16,unsigned(NativeBorgMotion::Count)> lab_start_frames_{};
    u16 model_id_=0;u8 torso_=kHsdNoBone;unsigned base_=0,upper_=0;u32 clock_=0,attack_clock_=0,last_tick_=0;bool ready_=false,presentation_ready_=false,source_pose_active_=false;BattleMvpVec last_position_{};BattlePoint last_facing_{};f32 last_aim_yaw_=0,last_reaction_yaw_=0,last_reaction_pitch_=0;
    bool provisional_walk_=false,provisional_jump_=false,provisional_dash_=false,provisional_air_dash_=false,
          provisional_glide_=false,provisional_cruise_=false,provisional_animated_cruise_=false;
    std::weak_ptr<const NativeBorgSourcePoseScope> source_frame_scope_;
    bool package_pose_held_=false;
    std::shared_ptr<const BorgGameplayProfile> package_profile_;
    std::array<HsdBoneMotionClip,10> package_locomotion_;
    std::array<HsdBoneMotionClip,kBorgBattleMoves> package_moves_;
    std::array<HsdBoneMotionClip,kBorgBattleMoves> package_followups_;
    std::array<std::vector<HsdBoneMotionClip>,kBorgBattleMoves> package_parts_;
    struct PresentPart {u16 record=0;AttachmentBoneTransform transform;u32 born=0,tick=0;};
    struct AnimatedPart {BorgBattlePartMaterial clip;AnimatedModelScene scene;};
    std::array<PresentPart,kBorgBattlePartSlots> active_parts_;
    std::array<AttachmentMountTurn,kBorgBattlePartSlots> mount_turns_;
    std::map<u16,AttachmentModelScene> part_records_;
    std::map<u16,AnimatedModelScene> static_parts_;
    BorgTrailingChain trailing_chain_;
    std::map<std::pair<u8,u16>,AnimatedPart> animated_parts_;
};
}
