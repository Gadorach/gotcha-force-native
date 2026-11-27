#pragma once
#include "engine/assets/borg_trailing_chain.hpp"
#include "engine/assets/borg_reflection_policy.hpp"
#include "engine/assets/character_preview_mount.hpp"
#include "engine/assets/borg_cruise_boost.hpp"
#include "engine/assets/battle_damage_indices.hpp"
#include <optional>
#include "engine/assets/borg_catalog_profile.hpp"
#include <cmath>

namespace gf {
inline constexpr unsigned kBorgBattleMoves=8,kBorgBattleStocks=8,kBorgBattleAttacks=16,kBorgBattleReleases=8,kBorgBattleVolley=8;
inline constexpr unsigned kBorgStockCapacity=255,kBorgReleaseFollowups=5;
inline constexpr unsigned kBorgReleaseSounds=8;
inline constexpr unsigned kBorgBattlePullUnits=40;
inline constexpr unsigned kBorgBattleEmitters=4;
inline constexpr unsigned kBorgBattleAnchorSets=2,kBorgBattleAnchors=8;
inline constexpr unsigned kBorgBattleMovementFx=8;
inline constexpr unsigned kBorgBattlePartAnimations=8;
inline constexpr unsigned kBorgBattlePartSlots=4;
inline constexpr unsigned kBorgBattlePartRecords=12;
inline constexpr unsigned kBorgDamageMatchupRows=20;
inline constexpr unsigned kBorgDamageLadderCells=32;
inline constexpr unsigned kBorgDamageTierCells=64;
inline constexpr unsigned kBorgBattleHoldTicks=600,kBorgBattleRepeatCycles=16;
inline constexpr std::size_t kBorgGameplayBytes=16u<<10;
inline constexpr u8 kBorgReactionBodyRoot=4;
enum class BorgBattleMovement:u8 {Grounded,NormalGRed,Stationary};
enum class BorgBattleButton:u8 {B,X};
enum class BorgBattleContext:u8 {Any,Grounded,Airborne};
enum class BorgBattleActivation:u8 {Press,ReleaseWindow,Tap,DoubleTap};
enum class BorgAttackVisual:u8 {Bullet,Beam,Shuriken,Orb};
struct BorgBattleSocket {u8 bone=0;std::array<f32,3> local{};bool aim_height=false;};
struct BorgProjectileLook {u16 size=0;std::array<u8,3> rgb{};};
struct BorgProjectileRecordLook {u16 record=0,scale_percent=100;};
inline constexpr unsigned kBorgProjectileTrailSamples=5;
struct BorgProjectileRecordTrail {u16 record=0,scale_percent=100;u8 count=0,interval=0;};
struct BorgProjectileAnimation {f32 spin=0;u16 pulse_period=0;u8 pulse_percent=0;};
enum class BorgMovementCue:u8 {Takeoff=1,AirJump=2,Landing=4,Rise=8};
inline constexpr std::array<std::string_view,4> kBorgMovementCueNames{"takeoff","air-jump","landing","rise"};
enum class BorgMovementLook:u8 {Jet,Puff};
struct BorgMovementFx {
    std::string id;BorgMovementCue event=BorgMovementCue::Takeoff;BorgMovementLook style=BorgMovementLook::Puff;
    u8 bone=0,duration=0;u16 size=0;std::array<f32,3> local{};
};
struct BorgWalkPulse {u16 stride=0,start=0,end=0,height=0,ticks=0,damage=0;};
enum class BorgProjectileFloor:u8 {Ignore,Retire,Land,StageRetire,StageClip};
inline bool borg_projectile_uses_stage(BorgProjectileFloor floor){
    return floor==BorgProjectileFloor::StageRetire||floor==BorgProjectileFloor::StageClip;
}
enum class BorgBlastTrigger:u8 {Actor,Floor,Impact,Expiry,Remote,Stage};
enum class BorgProjectileReturn:u8 {None,Actor,Floor,Expiry,Remote,Stage,ExpiryOrProximity};
enum class BorgGuidanceTarget:u8 {Snapshot,Tracking};
struct BorgBattleGuidance {BorgGuidanceTarget target=BorgGuidanceTarget::Snapshot;f32 turn=0;u16 ticks=0;};
struct BorgSustainedBeam {u16 length=0,interval=0;f32 turn=0;};
struct BorgBeamSweep {u16 travel=0,drop=0,impact_radius=0;};
struct BorgBombProximity {u16 radius=0,arming=0;bool blast=false;};
struct BorgBombAttraction {u16 radius=0;f32 acceleration=0,cap=0;};
struct BorgSmoke {u16 radius=0,ticks=0;bool block_aim=false;};
struct BorgInkOverlay {u16 ticks=0;u8 coverage=0,opacity=0;};
struct BorgBlastAccuracy {u16 ticks=0;f32 radians=0;};
struct BorgFlightChildren {u8 projectile=255,count=0;u16 first=0,interval=0;};
enum class BorgImpactLane:u8 {Actor,Floor,Stage};
struct BorgImpactRecord {BorgImpactLane lane=BorgImpactLane::Actor;u16 record=0,scale_percent=100,ticks=0,end_scale=0;};
struct BorgImpactSound {BorgImpactLane lane=BorgImpactLane::Actor;std::string path;};
struct BorgImpactChildren {u8 projectile=255;BorgImpactLane lane=BorgImpactLane::Actor;bool radial=false;};
struct BorgImpactPattern {
    BorgImpactLane lane=BorgImpactLane::Stage;
    u16 count=0,interval=0,radius=0,advance=0,arms=0,first_delay=0;
    f32 degrees=0;
};
struct BorgImpactTrail : BorgImpactPattern {u16 mark_size=0,mark_ticks=0;};
struct BorgImpactHazard : BorgImpactPattern {u16 hit_radius=0,height=0,ticks=0,damage=0,first_query=0;};
struct BorgImpactArea {
    BorgImpactLane lane=BorgImpactLane::Stage;
    f32 initial_radius=0,gain=0,cap=0;
    u16 queries=0,damage=0;
};
struct BorgBattleRewindHit {u8 set=0,speed=0,fallback=255;};
struct BorgProjectileSpeedRamp {f32 gain=0,cap=0;};
struct BorgBattleBlast {
    BorgBlastTrigger trigger=BorgBlastTrigger::Actor;
    u16 radius=0,damage=0;
    bool include_owner=false; // all living bodies in range; false restricts the pulse to opponents
};
struct BorgBattleClip {u8 group=0,choice=0;};
struct BorgBattlePoseFollowup {u16 frame=0;BorgBattleClip clip;u16 start=0;};
struct BorgBattleStockDefinition {
    std::string id;u16 capacity=0,refill_ticks=0; // zero is explicit return-only stock
    bool add_on_debit=false;
};
struct BorgBattleProjectileDefinition {
    std::string id;BorgAttackVisual visual=BorgAttackVisual::Bullet;
    u8 count=0;f32 speed=0,rise=0,gravity=0,spread=0,radius=0;
    u16 lifetime=0,damage=0;BorgProjectileFloor floor=BorgProjectileFloor::Ignore;
    u16 walk_percent=0,walk_ticks=0; // optional accepted-hit effect, never aim-target based
    u16 pull_units=0; // one deferred caster step toward the accepted recipient snapshot
    bool ignore_actors=false;
    std::optional<BorgBattleBlast> blast;
    BorgProjectileReturn stock_return=BorgProjectileReturn::None;
    std::optional<BorgBattleGuidance> guidance;
    std::optional<BorgSustainedBeam> sustained;
    std::optional<BorgBeamSweep> beam_sweep;
    std::optional<BorgBombProximity> proximity;
    std::optional<BorgBombAttraction> attraction;
    std::optional<BorgSmoke> smoke;
    std::optional<BorgInkOverlay> ink;
    std::optional<BorgBlastAccuracy> accuracy;
    bool accuracy_sensitive=false;
    u16 fuse_tail=0;
    u8 blast_children=255;
    std::optional<BorgFlightChildren> flight_children;
    std::optional<BorgImpactChildren> impact_children;
    std::optional<BorgImpactTrail> impact_trail;
    std::optional<BorgImpactHazard> impact_hazard;
    std::optional<BorgImpactArea> impact_area;
    bool impact_world_space=false;
    std::optional<BorgBattleRewindHit> rewind_hit;
    u8 plant_on_stage=255;
    std::optional<BorgBattleSocket> socket;
    std::optional<BorgProjectileLook> look;
    std::optional<BorgProjectileRecordLook> record_look;
    std::optional<BorgProjectileRecordTrail> record_trail;
    std::optional<BorgImpactRecord> impact_record;
    std::optional<BorgImpactSound> impact_sound;
    std::optional<BorgProjectileAnimation> animation;
    bool launch_body_heading=false;
    std::string launch_sound;
    bool continue_after_actor_hit=false; // one accepted contact per round actor slot
    std::optional<BorgProjectileSpeedRamp> speed_ramp;
};
inline bool borg_hit_continuation_valid(const BorgBattleProjectileDefinition& a){
    // A non-terminal actor hit must not silently skip a retirement effect.
    return a.count==1&&a.damage&&!a.ignore_actors&&!a.sustained&&!a.guidance&&!a.blast&&
           a.floor!=BorgProjectileFloor::Land&&a.stock_return==BorgProjectileReturn::None&&
           !a.walk_ticks&&!a.pull_units&&!a.rewind_hit&&a.plant_on_stage==255&&
           !a.flight_children&&!a.impact_children&&!a.impact_trail&&!a.impact_hazard&&!a.impact_area&&
           (!a.impact_record||a.impact_record->lane!=BorgImpactLane::Actor)&&
           (!a.impact_sound||a.impact_sound->lane!=BorgImpactLane::Actor);
}
inline bool borg_impact_lane_reachable(const BorgBattleProjectileDefinition& attack,BorgImpactLane lane){
    switch(lane){
    case BorgImpactLane::Actor:return !attack.ignore_actors;
    case BorgImpactLane::Floor:return attack.floor==BorgProjectileFloor::Retire;
    case BorgImpactLane::Stage:return attack.floor==BorgProjectileFloor::StageRetire;
    }
    return false;
}
inline bool borg_impact_pattern_valid(const BorgBattleProjectileDefinition& a,const BorgImpactPattern& p){
    return !a.sustained&&borg_impact_lane_reachable(a,p.lane)&&p.count&&p.count<=32&&p.interval&&p.interval<=30&&
           p.first_delay<=120&&p.advance<=80&&u32(p.radius)+u32(p.count)*p.advance<=1200&&
           std::isfinite(p.degrees)&&std::abs(p.degrees)<=180&&p.arms&&p.arms<=8;
}
inline bool borg_projectile_return_reachable(const BorgBattleProjectileDefinition& attack,BorgProjectileReturn reason){
    switch(reason){
    case BorgProjectileReturn::None:case BorgProjectileReturn::Expiry:return true;
    case BorgProjectileReturn::Actor:return !attack.ignore_actors;
    case BorgProjectileReturn::Floor:return attack.floor==BorgProjectileFloor::Retire;
    case BorgProjectileReturn::Remote:return attack.blast&&attack.blast->trigger==BorgBlastTrigger::Remote;
    case BorgProjectileReturn::Stage:return attack.floor==BorgProjectileFloor::StageRetire;
    case BorgProjectileReturn::ExpiryOrProximity:return attack.proximity.has_value();
    }
    return false;
}
inline bool borg_blast_trigger_reachable(const BorgBattleProjectileDefinition& attack,BorgBlastTrigger trigger){
    const bool floor=attack.floor==BorgProjectileFloor::Retire;
    switch(trigger){
    case BorgBlastTrigger::Actor:return !attack.ignore_actors;
    case BorgBlastTrigger::Floor:return floor;
    case BorgBlastTrigger::Stage:return attack.floor==BorgProjectileFloor::StageRetire;
    case BorgBlastTrigger::Impact:return floor||attack.floor==BorgProjectileFloor::StageRetire||!attack.ignore_actors;
    case BorgBlastTrigger::Expiry:case BorgBlastTrigger::Remote:return true;
    }
    return false;
}
struct BorgBattleRelease {
    u16 frame=0;u8 stock=255,cost=0,projectile=0;
    std::array<u8,kBorgReleaseFollowups> followups{};
    u8 count=0;
    bool return_pair=false;
    std::string debit_sound{};
};
struct BorgBattleEntryStock {u8 stock=255,minimum=0;};
struct BorgReleaseAimGate {u16 frame=0,timeout=0;f32 half_angle=0;};
struct BorgBattleDetonation {u16 frame=0;u8 projectile=0;bool landed_only=false,first_eligible=false;};
struct BorgBattleHeldFire {
    u8 stock=255,cost=0,projectile=0;
    u16 warmup=0,slow_interval=0,fast_interval=0,cooldown=0;
};
struct BorgBattleMelee {
    u16 first=0,last=0,damage=0;
    f32 reach=0,radius=0,height=0,half_height=0;
};
enum class BorgBattleAttackKind:u8 {Shot=1,Melee=2,Blast=4};
struct BorgBattleGuard {
    u16 first=0,last=0;
    f32 half_angle=0;
    u8 contacts=0,mask=0;
};
struct BorgBattleEmitter {
    std::string id;
    u8 watches=0,projectile=0;
    f32 side=0,height=0,behind=0;
    u16 lifetime=0;
};
struct BorgBattleEmitterDeploy {u16 frame=0;u8 emitter=0;};
struct BorgPowerField {
    u16 frame=0,radius=0,ticks=0,damage_percent=100,visual_percent=100;
    f32 forward=0,height=0;
};
struct BorgSlowField {
    u16 frame=0,radius=0,ticks=0,actor_percent=100,shot_percent=100;
    f32 forward=0,height=0;
    bool break_guidance=false;
};
enum class BorgFieldShape:u8 {Sphere,Box};
enum class BorgFieldScope:u8 {All,Enemies,Allies,Source};
enum class BorgFieldEffectKind:u8 {ActorSpeed,ShotSpeed,ShotDamage,ShotSize,BreakGuidance};
struct BorgFieldEffect {BorgFieldEffectKind kind;BorgFieldScope scope;u16 value;};
struct BorgFieldShotPull {BorgFieldScope scope=BorgFieldScope::Enemies;f32 acceleration=0;};
struct BorgFieldPullPoint {f32 forward=0,height=0;u16 minimum=0;};
struct BorgFieldActorPull {
    BorgFieldScope scope=BorgFieldScope::Enemies;f32 step=0,half_angle=0;
    std::optional<BorgFieldPullPoint> point;
    bool repel=false;
};
struct BorgFieldCoreDamage {BorgFieldScope scope=BorgFieldScope::Enemies;u16 radius=0,damage=0,interval=0;};
struct BorgFieldDebit {std::string stock;u16 cost=0;};
struct BorgFieldDrift {f32 speed=0,deceleration=0;bool active_while_moving=false;};
struct BorgFieldSlot {std::string id;bool on_ready=false;};
inline constexpr unsigned kBorgFieldEffects=8,kBorgFieldTextureSide=32,kBorgFieldTextureBytes=32*32*4;
struct BorgField {
    u16 frame=0,ticks=0;
    f32 forward=0,height=0;
    BorgFieldShape shape=BorgFieldShape::Sphere;
    std::array<u16,3> extent{};
    std::array<u8,4> tint{};
    std::vector<BorgFieldEffect> effects;
    std::string texture;
    bool follow=false;
    std::optional<BorgBattleSocket> socket;
    std::optional<BorgFieldShotPull> shot_pull;
    std::optional<BorgFieldCoreDamage> core_damage;
    std::optional<BorgFieldDebit> debit;
    std::optional<BorgFieldActorPull> actor_pull;
    std::optional<BorgFieldDrift> drift{};
    std::optional<BorgFieldSlot> slot{};
};
struct BorgAnchorFence {u16 radius=0,damage=0,ticks=0;u8 recoil=0;};
struct BorgBattleAnchorSet {std::string id;u8 capacity=0;std::optional<BorgAnchorFence> fence{};bool return_on_reach=false;};
struct BorgBattleAnchorMarker {
    u16 frame=0;u8 set=0,stock=255,cost=0,rewind_speed=0;f32 forward=0,height=0;
};
struct BorgBattlePartSpin {u8 bone=0,axis=0;u16 period=0;bool reverse=false;};
struct BorgBattlePartMotion {u8 bone=0;BorgBattleClip clip;u16 first=0,count=0;};
struct BorgBattlePartScroll {u8 bone=0,axis=0;u16 period=0;bool reverse=false;};
struct BorgBattlePartPose {std::array<f32,3> offset{},degrees{};};
enum class BorgPartBasis:u8 {Bone,WorldZyx,BoneZyx};
struct BorgBattlePartSlot {
    std::string id;u8 bone=0;u16 record=0;std::optional<BorgBattlePartPose> pose;
    std::array<u16,3> turn_step{}; // binary angle units per visible pose update; zero snaps
    BorgPartBasis basis=BorgPartBasis::Bone;
    std::optional<BorgBattlePartSpin> mount_spin; // only axis, period and direction are used
};
struct BorgBattlePartSwap {
    u8 slot=0;u16 first=0,last=0,record=0;
    std::optional<BorgBattlePartPose> pose;
    std::optional<u8> bone;
};
enum class BorgPartMaterialPlayback:u8 {Once,Loop,Sample};
struct BorgBattlePartMaterial {u16 record=0,clip=0,first=0,last=0;BorgPartMaterialPlayback playback=BorgPartMaterialPlayback::Once;};
struct BorgMoveCharge {u16 ticks=0;};
template<unsigned Cells> struct BorgDamageTeamRows {
    std::array<std::array<f32,Cells>,2> sides{};
    u8 declared=0;
    const std::array<f32,Cells>* find(unsigned team)const{
        return declared==3&&team<sides.size()?&sides[team]:nullptr;
    }
};
using BorgDamageRoleRows=BorgDamageTeamRows<2>;
using BorgDamageLadderRows=BorgDamageTeamRows<kBorgDamageLadderCells>;
struct BorgDamageIndexedRole {
    BorgDamageTeamRows<kBorgDamageSettings> setting;
    BorgDamageTeamRows<kBorgDamageRanks> rank;
    bool complete()const{return setting.declared==3&&rank.declared==3;}
    std::array<f32,2> select(u8 team,u8 setting_index,u8 rank_index)const{
        return {setting.sides[team][setting_index],rank.sides[team][rank_index]};
    }
};
struct BorgDamageLadderRates {f32 hp=1.f,force=1.f;};
struct BorgContactTier {u8 charge=0;bool scale=true;};
struct BorgDamageTierTable {u16 idle_ticks=0;std::array<f32,kBorgDamageTierCells> factors{};};
inline constexpr unsigned kBorgContactSoundBands=4,kBorgContactSoundLanes=13;
struct BorgContactSound {u8 lane=0;std::array<std::string,2> paths;};
inline constexpr unsigned kBorgContactImpactLayers=3;
struct BorgContactImpactLayer {u16 record=0,scale_percent=100,ticks=0;};
inline constexpr unsigned kBorgContactPoseHoldMax=7;
struct BorgContactPoseHold {u8 ticks=0;bool both_bodies=false;};
inline constexpr u8 borg_contact_impact_slot(unsigned move,unsigned team,unsigned layer){
    return u8(kBorgBattleAttacks+(move*2+team)*kBorgContactImpactLayers+layer);
}
struct BorgBattleMove {
    std::string id;std::string action_sound; // authored per-move sound; empty plays nothing
    BorgBattleButton button=BorgBattleButton::B;BorgBattleContext context=BorgBattleContext::Any;
    u16 priority=0,duration=0;BorgBattleClip clip;
    bool static_pose=false;
    bool free_release=false;
    u16 pose_start=0;
    bool pose_authored_loop_window=false;
    std::optional<BorgBattlePoseFollowup> pose_followup{};
    std::optional<u16> aim_lock;
    std::vector<BorgBattleRelease> releases;
    std::optional<BorgBattleEntryStock> entry_stock{};
    std::optional<BorgReleaseAimGate> release_gate{};
    BorgBattleActivation activation=BorgBattleActivation::Press;
    u16 minimum_hold=0,maximum_hold=0,tap_window=0;
    u8 hold_cycles=0; // maximum total cycles including entry, zero means no repeat
    f32 entry_range=0; // zero is no proximity predicate; not a contact observation
    std::optional<BorgBattleMelee> melee;
    u16 contact_power=0; // optional base for this move's direct contacts only
    bool contact_minimum_hp=false; // keep positive final direct damage from truncating to zero
    bool damage_credit_ladders=false; // this direct move uses the package's outgoing HP/force rows
    std::optional<BorgContactTier> contact_tier{};
    std::optional<u8> contact_tier_reset_wear{};
    std::array<std::optional<BorgContactSound>,kBorgContactSoundBands> contact_sounds{};
    std::array<std::vector<BorgContactImpactLayer>,2> contact_impacts;
    std::optional<BorgContactPoseHold> contact_pose_hold{};
    std::optional<BorgBattleHeldFire> held_fire;
    std::optional<BorgBattleDetonation> detonation;
    std::optional<BorgBattleGuard> guard;
    std::optional<BorgMoveCharge> charge;
    std::vector<BorgBattleEmitterDeploy> emitters;
    std::optional<BorgPowerField> power_field;
    std::optional<BorgSlowField> slow_field;
    std::optional<BorgField> field;
    std::vector<BorgBattleAnchorMarker> anchors;
    std::vector<BorgBattlePartSpin> part_spins;
    std::vector<BorgBattlePartMotion> part_motions;
    std::vector<BorgBattlePartScroll> part_scrolls;
    std::vector<BorgBattlePartSwap> part_swaps;
    u8 reaction_mode=1; // attacker move-table +0x0E; mode 4 needs a scoped body root
    bool reaction_mode_declared=false;
    bool reaction_linked_owner=false; // late sample of a direct projectile's round-owned caster
    s8 reaction_yaw_offset=0,reaction_pitch_offset=0; // table +0x14/+0x15
    bool reaction_offset_declared=false;
    unsigned last_marker()const{
        if(held_fire)return duration-1; // every frame in the presentation loop must exist
        unsigned deploy=0;
        for(const auto& marker:emitters)deploy=std::max<unsigned>(deploy,marker.frame);
        for(const auto& marker:anchors)deploy=std::max<unsigned>(deploy,marker.frame);
        return std::max<unsigned>({releases.empty()?0u:unsigned(releases.back().frame),melee?melee->last:0u,deploy,
                                    detonation?detonation->frame:0u,guard?guard->last:0u,power_field?power_field->frame:0u,
                                    slow_field?slow_field->frame:0u,field?field->frame:0u});
    }
};
struct BorgGameplayProfile {
    std::string variant,rig_prefix,pose_profile;
    u16 damage_model=0; // optional damage-row id; does not change the motion model
    bool damage_model_declared=false;
    // Declared pair-relation fields (recovered 8002FD7C). Defaults are neutral.
    u8 damage_kind=0;
    bool damage_kind_declared=false;
    u16 damage_attack_key=0;
    bool damage_attack_key_declared=false;
    u16 damage_target_mask=0;
    bool damage_target_mask_declared=false;
    std::vector<u16> damage_immunity; // at most 48 keys
    std::string impact_record_bank;
    BorgBattleMovement movement=BorgBattleMovement::Grounded;
    bool record_zero_body=false;
    bool body_immutable_highlight=false;
    bool body_diffuse_overwrite_pair=false;
    std::vector<CharacterPreviewMount> body_mounts;
    std::vector<BorgZeroReflection> zero_reflections;
    u16 animated_translation_percent=100;
    std::optional<BorgTrailingChainSpec> trailing_chain;
    bool idle_authored_loop=false;
    bool walk_authored_loop=false;
    bool static_cruise_body=false;
    bool static_grounded_body=false;
    bool dash_disabled=false;
    u16 health=0;f32 walk=0,jump=0,dash=0;
    u16 beam_recovery=0;
    u8 air_jump_points=0;
    u8 air_action_budget=0; // combined jump + air-dash budget; zero keeps the jump-only rule
    f32 pose_ground_offset=0; // authored presentation shift for a floating rig; zero leaves the pose
    // Recovered contact scale: credit factor, battle scalar, receiver rank,
    // relative. All default to 1, so a package that does not declare them is
    // unchanged. `damage_chain` is the successive-hit row (retail tier); it is
    // only applied when `damage_chain_count` is non-zero.
    std::array<f32,4> damage_factors{1,1,1,1};
    std::array<f32,4> damage_chain{1,1,1,1};
    u8 damage_chain_count=0;
    bool damage_factors_declared=false;
    struct DamageRow { u16 model=0; std::array<f32,4> factors{1,1,1,1}; };
    struct DamageSideRow { u8 team=0; std::array<f32,4> factors{1,1,1,1}; };
    std::vector<DamageRow> damage_rows; // per receiver model
    std::vector<DamageSideRow> damage_side_rows; // per attacker side
    BorgDamageRoleRows damage_credit;  // difficulty, rank on the attacking team
    BorgDamageRoleRows damage_receive; // class, rank on the receiving team
    BorgDamageIndexedRole damage_credit_indexed,damage_receive_indexed;
    BorgDamageLadderRows damage_credit_hp,damage_credit_force;
    BorgDamageLadderRows damage_receive_hp,damage_receive_force;
    std::optional<BorgDamageTierTable> damage_tier_receive{};
    u16 damage_tier_reset_capacity=0;
    std::optional<u8> contact_sound_band{};
    bool contact_impact_receive=false;
    bool contact_pose_hold_receive=false;
    std::string damage_matchup_group;
    struct DamageMatchupRow {std::string attacking_group;f32 factor=1;};
    std::vector<DamageMatchupRow> damage_matchup_from;
    f32 flight_lift=0;u16 flight_ticks=0;
    std::optional<BorgCruiseBoost> cruise{};
    std::optional<BorgBattleClip> cruise_entry;
    // idle, walk, airborne, dash; stationary only uses idle. Source movement
    // supplies all lower-body poses itself.
    std::array<BorgBattleClip,4> locomotion{};
    std::array<std::optional<BorgBattleClip>,3> directional_dash{}; // back, left, right
    std::optional<BorgBattleClip> air_dash_clip;
    std::optional<BorgBattleClip> takeoff_clip,landing_clip;
    u16 air_dash_start=0;
    bool air_dash_disabled=false;
    std::vector<BorgBattleStockDefinition> stocks;
    std::vector<BorgBattleProjectileDefinition> projectiles;
    std::vector<BorgBattleMove> moves;
    std::vector<BorgBattlePartSlot> part_slots;
    std::vector<u16> static_part_hierarchies;
    std::vector<BorgBattlePartMaterial> part_materials;
    std::vector<BorgBattleEmitter> emitters;
    std::vector<BorgBattleAnchorSet> anchor_sets;
    std::vector<BorgMovementFx> movement_fx;
    std::optional<BorgWalkPulse> walk_pulse;
    std::array<std::string,4> movement_sound{};
    bool static_body()const{return static_cruise_body||static_grounded_body;}
};
inline bool borg_battle_has_indirect_contact(const BorgBattleMove& move){
    return move.held_fire||move.detonation||!move.emitters.empty()||move.field||
           move.power_field||move.slow_field||!move.anchors.empty();
}
inline bool borg_battle_direct_melee(const BorgBattleMove& move){
    return move.melee&&move.releases.empty()&&!borg_battle_has_indirect_contact(move);
}
inline bool borg_battle_pose_followup_valid(const BorgGameplayProfile& profile,const BorgBattleMove& move){
    if(!move.pose_followup)return true;
    const auto& follow=*move.pose_followup;
    return profile.movement==BorgBattleMovement::Grounded&&move.context==BorgBattleContext::Grounded&&
           !move.static_pose&&borg_battle_direct_melee(move)&&
           follow.frame&&follow.frame<=move.melee->first&&follow.frame<move.duration&&
           follow.start<=600&&follow.clip.group<16&&follow.clip.choice<128;
}
inline bool borg_battle_direct_releases(const BorgGameplayProfile& profile,const BorgBattleMove& move){
    if(move.melee||move.releases.empty()||move.releases.size()>kBorgBattleReleases||
       borg_battle_has_indirect_contact(move))return false;
    const auto direct=[&](unsigned index){
        if(index>=profile.projectiles.size())return false;
        const auto& p=profile.projectiles[index];
        return p.damage&&!p.ignore_actors&&!p.blast&&!p.sustained&&!p.beam_sweep&&
               p.blast_children==255&&!p.flight_children&&!p.impact_children&&
                !p.impact_hazard&&!p.impact_area&&!p.rewind_hit&&p.plant_on_stage==255;
    };
    for(const auto& release:move.releases){
        if(!direct(release.projectile)||release.count>release.followups.size())return false;
        for(unsigned i=0;i<release.count;++i)if(!direct(release.followups[i]))return false;
    }
    return true;
}
inline bool borg_battle_reaction_scope_valid(const BorgGameplayProfile& profile,const BorgBattleMove& move){
    if(move.reaction_mode<kBorgReactionBodyRoot)return !move.reaction_linked_owner;
    if(move.reaction_mode!=kBorgReactionBodyRoot)return false;
    return move.reaction_linked_owner?borg_battle_direct_releases(profile,move):borg_battle_direct_melee(move);
}
inline bool borg_battle_contact_damage_valid(const BorgGameplayProfile& profile,const BorgBattleMove& move){
    return move.contact_power<=10000&&
        (!(move.contact_power||move.contact_minimum_hp)||
         borg_battle_direct_melee(move)||borg_battle_direct_releases(profile,move));
}
inline bool borg_battle_contact_sound_valid(const BorgGameplayProfile& profile,const BorgBattleMove& move){
    bool bound=false;
    for(const auto& sound:move.contact_sounds)if(sound){
        bound=true;
        if(sound->lane>=kBorgContactSoundLanes||
           !std::all_of(sound->paths.begin(),sound->paths.end(),borg_package_asset_path_valid))return false;
    }
    if(!bound||borg_battle_direct_melee(move))return true;
    if(!borg_battle_direct_releases(profile,move))return false;
    const auto conflict=[&](unsigned i){
        const auto& sound=profile.projectiles[i].impact_sound;
        return sound&&sound->lane==BorgImpactLane::Actor;
    };
    for(const auto& release:move.releases){
        if(conflict(release.projectile))return false;
        for(unsigned i=0;i<release.count;++i)if(conflict(release.followups[i]))return false;
    }
    return true;
}
inline bool borg_battle_contact_impact_valid(const BorgGameplayProfile& profile,const BorgBattleMove& move){
    const auto& rows=move.contact_impacts;
    if(rows[0].empty()&&rows[1].empty())return true;
    for(const auto& layers:rows){
        if(layers.empty()||layers.size()>kBorgContactImpactLayers)return false;
        for(std::size_t i=0;i<layers.size();++i){
            const auto& part=layers[i];
            if(!part.record||part.record>=256||!part.scale_percent||part.scale_percent>400||!part.ticks||part.ticks>120)
                return false;
            for(std::size_t j=0;j<i;++j)if(part.record==layers[j].record)return false;
        }
    }
    if(borg_battle_direct_melee(move))return true;
    if(!borg_battle_direct_releases(profile,move))return false;
    const auto conflict=[&](unsigned i){
        const auto& impact=profile.projectiles[i].impact_record;
        return impact&&impact->lane==BorgImpactLane::Actor;
    };
    for(const auto& release:move.releases){
        if(conflict(release.projectile))return false;
        for(unsigned i=0;i<release.count;++i)if(conflict(release.followups[i]))return false;
    }
    return true;
}
inline bool borg_battle_contact_pose_hold_valid(const BorgGameplayProfile& profile,const BorgBattleMove& move){
    if(!move.contact_pose_hold)return true;
    const auto& hold=*move.contact_pose_hold;
    return hold.ticks&&hold.ticks<=kBorgContactPoseHoldMax&&profile.movement!=BorgBattleMovement::NormalGRed&&
        (borg_battle_direct_melee(move)||(!hold.both_bodies&&borg_battle_direct_releases(profile,move)));
}
inline const std::array<f32,4>* borg_damage_factors(const BorgGameplayProfile& profile,u16 model,u8 team){
    for(const auto& row:profile.damage_rows)if(row.model==model)return &row.factors;
    for(const auto& row:profile.damage_side_rows)if(row.team==team)return &row.factors;
    return profile.damage_factors_declared?&profile.damage_factors:nullptr;
}

inline bool borg_damage_roles_valid(const BorgGameplayProfile& p){
    if(p.moves.size()>kBorgBattleMoves)return false;
    const auto valid=[](const auto& role){
        if(!role.declared)return true;
        if(role.declared!=3)return false;
        for(const auto& row:role.sides)for(f32 value:row)
            if(!std::isfinite(value)||value<.01f||value>4.f)return false;
        return true;
    };
    const bool outgoing=p.damage_credit_hp.declared||p.damage_credit_force.declared;
    const auto indexed_valid=[&](const BorgDamageIndexedRole& indexed,const BorgDamageRoleRows& role){
        if(!indexed.setting.declared&&!indexed.rank.declared)return true;
        return role.declared==3&&indexed.complete()&&valid(indexed.setting)&&valid(indexed.rank);
    };
    if(!indexed_valid(p.damage_credit_indexed,p.damage_credit)||
       !indexed_valid(p.damage_receive_indexed,p.damage_receive))return false;
    if(!valid(p.damage_credit)||!valid(p.damage_receive)||!valid(p.damage_credit_hp)||
       !valid(p.damage_credit_force)||!valid(p.damage_receive_hp)||
       !valid(p.damage_receive_force)||((p.damage_receive_hp.declared||p.damage_receive_force.declared)&&
                                      !p.damage_receive.declared)||(outgoing&&!p.damage_credit.declared)||
       outgoing!=std::any_of(p.moves.begin(),p.moves.end(),[](const auto& move){return move.damage_credit_ladders;}))
        return false;
    for(const auto& move:p.moves)if(move.damage_credit_ladders&&
        !borg_battle_direct_melee(move)&&!borg_battle_direct_releases(p,move))return false;
    if(const auto& tier=p.damage_tier_receive;tier){
        if(!p.damage_receive.declared||!tier->idle_ticks||tier->idle_ticks>600)return false;
        for(f32 factor:tier->factors)
            if(!std::isfinite(factor)||factor<.01f||factor>4.f)return false;
    }
    if(p.damage_tier_reset_capacity&&(!p.damage_tier_receive||p.damage_tier_reset_capacity>32767))return false;
    for(const auto& move:p.moves){
        if(move.contact_tier&&(!p.damage_credit.declared||p.damage_chain_count||
           (!borg_battle_direct_melee(move)&&!borg_battle_direct_releases(p,move))))return false;
        if(move.contact_tier_reset_wear&&!move.contact_tier)return false;
    }
    return !p.damage_credit.declared||
           (!p.damage_factors_declared&&p.damage_rows.empty()&&p.damage_side_rows.empty());
}

inline std::optional<unsigned> borg_damage_hp_cell(unsigned health,unsigned maximum,u16 raw_base=0){
    if(!maximum||health>maximum)return {};
    const unsigned probe=health>raw_base?health-raw_base:0;
    if(probe>200)return {};
    return std::min(31u,32u-probe*32u/std::min(maximum,200u));
}
inline std::optional<unsigned> borg_damage_force_cell(unsigned force,unsigned cap){
    if(!cap||force>cap)return {};
    return std::min(31u,32u-unsigned(u64(force)*32u/cap));
}
inline f32 borg_damage_ladder_factor(const BorgDamageLadderRows& ladder,u8 team,std::optional<unsigned> cell){
    const auto* row=ladder.find(team);
    return row&&cell&&*cell<row->size()?(*row)[*cell]:1.f;
}

inline f32 borg_damage_matchup(const BorgGameplayProfile& attacker,const BorgGameplayProfile& receiver){
    if(attacker.damage_matchup_group.empty())return 1.f;
    for(const auto& row:receiver.damage_matchup_from)
        if(row.attacking_group==attacker.damage_matchup_group)return row.factor;
    return 1.f;
}

inline u16 borg_damage_amount(const BorgGameplayProfile& profile,u16 base,unsigned chain,
                              u16 model=0,u8 team=0,f32 scalar=1.f,
                              const std::array<f32,2>* receive=nullptr,f32 matchup=1.f,
                              BorgDamageLadderRates response={},BorgDamageLadderRates outgoing={},f32 tier=1.f,
                              const std::array<f32,2>* indexed_credit=nullptr,bool minimum_hp=false){
    f32 value=f32(base);
    if(const auto* credit=profile.damage_credit.find(team);credit&&receive){
        if(indexed_credit)credit=indexed_credit;
        value*=(*credit)[0];
        value*=outgoing.hp;
        value*=outgoing.force;
        value*=(*credit)[1];
        value*=matchup;
        value*=(*receive)[0];
        value*=response.hp;
        value*=response.force;
        value*=(*receive)[1];
        value*=tier;
    }else if(const auto* factors=borg_damage_factors(profile,model,team))
        for(f32 factor:*factors)value*=factor;
    value*=scalar;
    if(profile.damage_chain_count)
        value*=profile.damage_chain[std::min<unsigned>(chain,profile.damage_chain_count-1)];
    if(!(value>0))return 0;
    if(minimum_hp&&value<1.f)return 1;
    return u16(value>65535.f?65535.f:value);
}

inline const BorgBattlePartSwap* borg_part_active_swap(u8 index,const BorgBattleMove* active,u16 frame){
    if(active)for(const auto& swap:active->part_swaps)
        if(swap.slot==index&&frame>=swap.first&&frame<=swap.last)return &swap;
    return nullptr;
}
inline u16 borg_part_record(const BorgBattlePartSlot& slot,u8 index,const BorgBattleMove* active,u16 frame){
    const auto* swap=borg_part_active_swap(index,active,frame);
    return swap?swap->record:slot.record;
}
inline const BorgBattlePartPose* borg_part_pose(const BorgBattlePartSlot& slot,const BorgBattlePartSwap* swap){
    return swap&&swap->pose?&*swap->pose:slot.pose?&*slot.pose:nullptr;
}
inline u8 borg_part_bone(const BorgBattlePartSlot& slot,const BorgBattlePartSwap* swap){
    return swap&&swap->bone?*swap->bone:slot.bone;
}
inline u16 borg_part_material_phase(const BorgBattlePartMaterial& clip,u32 age){
    const u32 span=u32(clip.last)-clip.first;
    return u16(clip.playback==BorgPartMaterialPlayback::Loop?age%(span+1):std::min(age,span));
}
inline bool borg_battle_emission_valid(const BorgGameplayProfile& p,u8 stock,u8 cost,u8 projectile){
    if(projectile>=p.projectiles.size()||(cost?(stock>=p.stocks.size()||cost>p.stocks[stock].capacity||
       (p.stocks[stock].add_on_debit&&cost!=1)):stock!=255))return false;
    const auto& shot=p.projectiles[projectile];
    const bool returns=shot.stock_return!=BorgProjectileReturn::None;
    const bool return_only=cost&&!p.stocks[stock].refill_ticks;
    // Each successfully created carrier may return exactly one paid unit.
    return returns==return_only&&(!returns||cost==shot.count)&&
        (!shot.rewind_hit||shot.rewind_hit->fallback==255||!return_only);
}
inline bool borg_battle_release_valid(const BorgGameplayProfile& p,const BorgBattleRelease& r){
    if(!r.return_pair)return borg_battle_emission_valid(p,r.stock,r.cost,r.projectile);
    return r.count==1&&r.cost==1&&r.stock<p.stocks.size()&&p.stocks[r.stock].capacity==1&&!p.stocks[r.stock].refill_ticks&&
           borg_battle_emission_valid(p,255,0,r.projectile)&&borg_battle_emission_valid(p,255,0,r.followups[0])&&
           p.projectiles[r.projectile].count==1&&p.projectiles[r.followups[0]].count==1;
}
namespace borg_gameplay_detail {
inline bool id(std::string_view text){
    return text!="none"&&text!="unresolved"&&text!="inherit"&&borg_catalog_variant_id_valid(text);
}
inline bool number(std::string_view s,unsigned min,unsigned max,u16* out){
    if(s.empty()||s.size()>5)return false;
    unsigned n=0;const auto r=std::from_chars(s.data(),s.data()+s.size(),n);
    if(r.ec!=std::errc{}||r.ptr!=s.data()+s.size()||n<min||n>max)return false;
    *out=u16(n);return true;
}
inline bool scalar(std::string_view s,f32 min,f32 max,f32* out){
    if(s.empty()||s.size()>16)return false;
    f32 n=0;const auto r=std::from_chars(s.data(),s.data()+s.size(),n);
    if(r.ec!=std::errc{}||r.ptr!=s.data()+s.size()||!std::isfinite(n)||n<min||n>max)return false;
    *out=n;return true;
}
inline bool clip(std::string_view s,BorgBattleClip* out){
    const auto colon=s.find(':');u16 g=0,c=0;
    if(colon==s.npos||!number(s.substr(0,colon),0,15,&g)||!number(s.substr(colon+1),0,127,&c))return false;
    *out={u8(g),u8(c)};return true;
}
template<class Rows> unsigned index(const Rows& rows,std::string_view id){
    const auto it=std::find_if(rows.begin(),rows.end(),[&](const auto& row){return row.id==id;});
    return unsigned(it-rows.begin());
}
}
inline bool borg_gameplay_profile_valid(const BorgGameplayProfile& p){
    const auto clip=[](BorgBattleClip c){return c.group<16&&c.choice<128;};
    const auto finite=[](f32 n,f32 lo,f32 hi){return std::isfinite(n)&&n>=lo&&n<=hi;};
    if(!p.damage_matchup_group.empty()&&
       (!borg_gameplay_detail::id(p.damage_matchup_group)||(!p.damage_credit.declared&&!p.damage_receive.declared)))return false;
    if(p.damage_matchup_from.size()>kBorgDamageMatchupRows||
       (!p.damage_matchup_from.empty()&&(p.damage_matchup_group.empty()||!p.damage_receive.declared)))return false;
    for(unsigned i=0;i<p.damage_matchup_from.size();++i){
        const auto& row=p.damage_matchup_from[i];
        if(!borg_gameplay_detail::id(row.attacking_group)||!finite(row.factor,.01f,4.f))return false;
        for(unsigned j=0;j<i;++j)if(p.damage_matchup_from[j].attacking_group==row.attacking_group)return false;
    }
    if(!borg_gameplay_detail::id(p.variant)||!p.health||p.health>10000||p.beam_recovery>120||unsigned(p.movement)>2||
       !borg_damage_roles_valid(p)||(p.contact_sound_band&&*p.contact_sound_band>=kBorgContactSoundBands)||
       (p.contact_pose_hold_receive&&p.movement==BorgBattleMovement::NormalGRed)||
       p.moves.size()>kBorgBattleMoves||p.stocks.size()>kBorgBattleStocks||p.projectiles.size()>kBorgBattleAttacks||
       p.emitters.size()>kBorgBattleEmitters||
       p.anchor_sets.size()>kBorgBattleAnchorSets||(!p.anchor_sets.empty()&&p.movement!=BorgBattleMovement::Grounded)||
       p.movement_fx.size()>kBorgBattleMovementFx||(!p.movement_fx.empty()&&p.movement==BorgBattleMovement::Stationary)||
       p.air_jump_points>8||(p.air_jump_points&&p.movement!=BorgBattleMovement::Grounded)||
       p.air_action_budget>8||(p.air_action_budget&&(p.movement!=BorgBattleMovement::Grounded||
                                                    p.air_jump_points||p.flight_ticks||p.cruise))||
       (p.flight_ticks?(p.movement!=BorgBattleMovement::Grounded||p.air_jump_points||p.flight_ticks>120||
                        !finite(p.flight_lift,2,16)):p.flight_lift!=0)||
       p.moves.empty()!=(p.movement==BorgBattleMovement::Stationary))return false;
    if(p.movement==BorgBattleMovement::Grounded&&(!finite(p.walk,.4f,16)||!finite(p.jump,2,24)||!finite(p.dash,p.walk,32)))return false;
    if((p.record_zero_body||p.idle_authored_loop)&&p.movement==BorgBattleMovement::NormalGRed)return false;
    if((p.body_immutable_highlight||p.body_diffuse_overwrite_pair)&&p.movement==BorgBattleMovement::NormalGRed)return false;
    if((!p.body_mounts.empty()&&!p.record_zero_body)||!character_preview_mounts_valid(p.body_mounts))return false;
    if((!p.zero_reflections.empty()&&!p.record_zero_body)||!borg_zero_reflections_valid(p.zero_reflections))return false;
    if(p.animated_translation_percent<25||p.animated_translation_percent>200||
       (p.animated_translation_percent!=100&&(!p.record_zero_body||p.static_body()||p.cruise||
                                                p.movement!=BorgBattleMovement::Grounded)))return false;
    if(p.trailing_chain&&(!p.record_zero_body||p.movement!=BorgBattleMovement::Grounded||p.cruise||
                         !borg_trailing_chain_valid(*p.trailing_chain)))return false;
    if(p.walk_authored_loop&&(p.movement!=BorgBattleMovement::Grounded||p.static_body()||p.cruise_entry))return false;
    if(p.static_body()&&((p.static_cruise_body&&p.static_grounded_body)||bool(p.cruise)!=p.static_cruise_body||
                        p.movement!=BorgBattleMovement::Grounded||p.idle_authored_loop||
                        !p.part_slots.empty()||!p.part_materials.empty()))return false;
    if(p.cruise_entry&&(!p.cruise||p.static_body()||!clip(*p.cruise_entry)||p.idle_authored_loop||
                        !p.part_slots.empty()||!p.part_materials.empty()))return false;
    if(p.dash_disabled&&(p.movement!=BorgBattleMovement::Grounded||p.cruise||p.static_body()))return false;
    const bool directional=std::any_of(p.directional_dash.begin(),p.directional_dash.end(),[](const auto& c){return c.has_value();});
    if((directional||p.air_dash_clip||p.air_dash_disabled)&&
       (p.movement!=BorgBattleMovement::Grounded||p.cruise||p.static_body()||p.dash_disabled))return false;
    if((p.air_dash_disabled&&p.air_dash_clip)||(!p.air_dash_clip&&p.air_dash_start)||p.air_dash_start>600||
       (p.air_dash_clip&&!clip(*p.air_dash_clip)))return false;
    for(const auto& c:p.directional_dash)if(c&&!clip(*c))return false;
    if((p.takeoff_clip||p.landing_clip)&&(p.movement!=BorgBattleMovement::Grounded||p.cruise||p.static_body()))return false;
    for(const auto& c:{p.takeoff_clip,p.landing_clip})if(c&&!clip(*c))return false;
    if(const auto& c=p.cruise;c&&(p.movement!=BorgBattleMovement::Grounded||p.air_jump_points||p.flight_ticks||p.walk_pulse||
        !borg_cruise_boost_valid(*c)))return false;
    if(const auto& q=p.walk_pulse;q&&(p.movement!=BorgBattleMovement::Grounded||q->stride<16||q->stride>800||
       q->start<4||q->end<=q->start||q->end>300||!q->height||q->height>150||!q->ticks||q->ticks>60||
       !q->damage||q->damage>10000))return false;
    if(p.movement==BorgBattleMovement::Stationary&&(!p.stocks.empty()||!p.projectiles.empty()))return false;
    const bool needs_impact_bank=std::any_of(p.projectiles.begin(),p.projectiles.end(),[](const auto& a){return a.impact_record.has_value();})||
        std::any_of(p.moves.begin(),p.moves.end(),[](const auto& m){return !m.contact_impacts[0].empty()||!m.contact_impacts[1].empty();});
    if(needs_impact_bank?!borg_package_asset_path_valid(p.impact_record_bank):!p.impact_record_bank.empty())
        return false;
    for(const auto& c:p.locomotion)if(!clip(c))return false;
    for(const auto& s:p.stocks)if(!s.capacity||s.capacity>kBorgStockCapacity||s.refill_ticks>3600||(s.add_on_debit&&!s.refill_ticks))return false;
    for(const auto& a:p.projectiles){
        if(a.continue_after_actor_hit&&!borg_hit_continuation_valid(a))return false;
        if(const auto& ramp=a.speed_ramp;ramp&&(
           a.sustained||a.guidance||a.rise||a.gravity||a.floor==BorgProjectileFloor::Land||
           !finite(ramp->gain,.01f,20.f)||!finite(ramp->cap,1.f,140.f)||ramp->cap<=a.speed))
            return false;
        if(const auto& s=a.impact_sound;s&&(a.sustained||!borg_impact_lane_reachable(a,s->lane)||
                                         !borg_package_asset_path_valid(s->path)))return false;
        if(const auto& r=a.impact_record;r&&(a.sustained||!borg_impact_lane_reachable(a,r->lane)||
                !r->record||r->record>=256||!r->scale_percent||r->scale_percent>400||!r->ticks||r->ticks>120||
                r->end_scale>400||(r->end_scale&&r->ticks<2)))return false;
        if(!a.launch_sound.empty()&&!borg_package_asset_path_valid(a.launch_sound))return false;
        if(a.record_look&&(!a.record_look->record||a.record_look->record>=32||!a.record_look->scale_percent||
                          a.record_look->scale_percent>400||a.visual!=BorgAttackVisual::Orb||a.look||a.animation||a.sustained))
            return false;
        if(const auto& t=a.record_trail;t&&(!a.record_look||!t->record||t->record>=32||!t->scale_percent||
            t->scale_percent>400||!t->count||t->count>kBorgProjectileTrailSamples||!t->interval||t->interval>30))
            return false;
        if(unsigned(a.visual)>3||unsigned(a.floor)>4||(!a.sustained&&a.floor==BorgProjectileFloor::StageClip)||
           (a.launch_body_heading&&(a.sustained||(a.socket&&a.socket->aim_height)))||
           (a.beam_sweep&&(!a.sustained||a.sustained->turn||a.floor!=BorgProjectileFloor::StageClip||
                           (a.socket&&a.socket->aim_height)||a.lifetime<2||
                           a.beam_sweep->travel<1||a.beam_sweep->travel>2400||a.beam_sweep->drop<1||
                           a.beam_sweep->drop>=a.sustained->length||!a.beam_sweep->impact_radius||a.beam_sweep->impact_radius>180))||
           (a.proximity&&(a.floor!=BorgProjectileFloor::Land||a.count!=1||!a.ignore_actors||!a.blast||
                         a.blast->trigger!=BorgBlastTrigger::Expiry||a.guidance||!a.proximity->radius||
                         a.proximity->radius>600||a.proximity->arming>=a.lifetime))||
           (a.attraction&&(!a.proximity||a.attraction->radius<=a.proximity->radius||a.attraction->radius>1800||
                           !finite(a.attraction->acceleration,.125f,8.f)||
                           !finite(a.attraction->cap,a.attraction->acceleration,32.f)))||
           (a.fuse_tail&&(a.fuse_tail>120||a.floor!=BorgProjectileFloor::Land||a.count!=1||!a.ignore_actors||
                          !a.blast||a.blast->trigger!=BorgBlastTrigger::Expiry))||
           (a.blast_children!=255&&(!a.blast||a.blast_children>=p.projectiles.size()))||
           (a.flight_children&&(a.sustained||a.count!=1||a.lifetime<3||a.flight_children->projectile>=p.projectiles.size()||
              !a.flight_children->first||!a.flight_children->interval||!a.flight_children->count||a.flight_children->count>8||
              u32(a.flight_children->first)+u32(a.flight_children->count-1)*a.flight_children->interval>=a.lifetime-1u))||
            (a.impact_children&&(a.sustained||a.count!=1||a.impact_children->projectile>=p.projectiles.size()||
              !borg_impact_lane_reachable(a,a.impact_children->lane)))||
            (a.impact_trail&&(!borg_impact_pattern_valid(a,*a.impact_trail)||
              !a.impact_trail->mark_size||a.impact_trail->mark_size>180||!a.impact_trail->mark_ticks||a.impact_trail->mark_ticks>30))||
             (a.impact_hazard&&(!borg_impact_pattern_valid(a,*a.impact_hazard)||!a.impact_hazard->hit_radius||
               a.impact_hazard->hit_radius>180||a.impact_hazard->height>600||!a.impact_hazard->ticks||
               a.impact_hazard->ticks>60||!a.impact_hazard->damage||a.impact_hazard->damage>10000||a.impact_hazard->first_query>60))||
             (a.impact_area&&(a.sustained||a.blast||a.impact_hazard||a.rewind_hit||a.plant_on_stage!=255||
               !borg_impact_lane_reachable(a,a.impact_area->lane)||
               !finite(a.impact_area->initial_radius,0,1200)||!finite(a.impact_area->gain,.01f,100)||
               !finite(a.impact_area->cap,.01f,1200)||a.impact_area->cap<=a.impact_area->initial_radius||
               !a.impact_area->queries||a.impact_area->queries>60||!a.impact_area->damage||a.impact_area->damage>10000))||
            (a.impact_world_space&&!a.impact_trail&&!a.impact_hazard)||
           (a.smoke&&(!a.blast||a.smoke->radius<8||a.smoke->radius>600||!a.smoke->ticks||a.smoke->ticks>600))||
           (a.ink&&(!a.blast||!a.blast->damage||!a.ink->ticks||a.ink->ticks>300||
                    a.ink->coverage<10||a.ink->coverage>80||!a.ink->opacity))||
           (a.accuracy&&(!a.blast||!a.blast->damage||!a.accuracy->ticks||a.accuracy->ticks>300||
                         !finite(a.accuracy->radians,.01f,.45f)))||
           (a.accuracy_sensitive&&(a.sustained||a.guidance||a.ignore_actors||!a.damage))||
           !a.count||a.count>kBorgBattleVolley||!finite(a.speed,1,140)||
           (a.sustained&&(a.visual!=BorgAttackVisual::Beam||a.count!=1||a.rise||a.gravity||a.spread||!a.damage||
            (a.floor!=BorgProjectileFloor::Ignore&&a.floor!=BorgProjectileFloor::StageClip)||a.ignore_actors||a.blast||a.guidance||a.rewind_hit||a.pull_units||
            a.walk_ticks||a.stock_return!=BorgProjectileReturn::None||a.plant_on_stage!=255||
            a.sustained->length<40||a.sustained->length>2400||!a.sustained->interval||a.sustained->interval>a.lifetime||
            !finite(a.sustained->turn,0,.25f)))||
           (a.socket&&(a.socket->bone>=32||!std::all_of(a.socket->local.begin(),a.socket->local.end(),
                                                     [&](f32 v){return finite(v,-500,500); })))||
           (a.look&&(!a.look->size||a.look->size>180))||
           (a.animation&&(!finite(a.animation->spin,-2,2)||a.animation->pulse_period>240||a.animation->pulse_percent>75||
               (bool(a.animation->pulse_period)!=bool(a.animation->pulse_percent))||
               (a.animation->spin&&a.visual!=BorgAttackVisual::Shuriken&&a.visual!=BorgAttackVisual::Orb)))||
           !finite(a.rise,-100,100)||!finite(a.gravity,0,8)||!finite(a.spread,0,.8f)||!finite(a.radius,1,60)||
            !a.lifetime||a.lifetime>600||
             (!a.damage&&!a.blast&&a.plant_on_stage==255&&!a.rewind_hit&&!a.flight_children&&!a.impact_children&&
               !a.impact_trail&&!a.impact_hazard&&!a.impact_area)||a.damage>10000||
           (a.ignore_actors&&(a.damage||a.walk_ticks||a.pull_units))||!borg_projectile_return_reachable(a,a.stock_return)||
           (a.guidance&&(unsigned(a.guidance->target)>1||!finite(a.guidance->turn,.001f,1.f)||
                        !a.guidance->ticks||a.guidance->ticks>a.lifetime))||
            (a.rewind_hit&&(p.movement!=BorgBattleMovement::Grounded||a.ignore_actors||a.pull_units||
                          a.count!=1||!a.rewind_hit->speed||a.rewind_hit->speed>16||a.rewind_hit->set>=p.anchor_sets.size()||
                          p.anchor_sets[a.rewind_hit->set].fence||p.anchor_sets[a.rewind_hit->set].return_on_reach||
                         (a.rewind_hit->fallback!=255&&(a.rewind_hit->fallback>=p.projectiles.size()||
                            p.projectiles[a.rewind_hit->fallback].sustained||p.projectiles[a.rewind_hit->fallback].rewind_hit||
                            p.projectiles[a.rewind_hit->fallback].plant_on_stage!=255||
                            p.projectiles[a.rewind_hit->fallback].continue_after_actor_hit||
                            p.projectiles[a.rewind_hit->fallback].stock_return!=BorgProjectileReturn::None))))||
           (a.plant_on_stage!=255&&(a.plant_on_stage>=p.anchor_sets.size()||a.floor!=BorgProjectileFloor::StageRetire||
                                   a.rewind_hit||a.count!=1))||
            (a.blast&&(!a.blast->radius||a.blast->radius>1200||(!a.blast->damage&&!a.smoke)||a.blast->damage>10000||
                      (a.blast->include_owner&&p.movement==BorgBattleMovement::NormalGRed)||
                      !borg_blast_trigger_reachable(a,a.blast->trigger)))||
           a.pull_units>kBorgBattlePullUnits||(a.pull_units&&p.movement!=BorgBattleMovement::Grounded)||
            (a.walk_ticks?(a.walk_ticks>600||a.walk_percent<25||a.walk_percent>200):a.walk_percent!=0))return false;
        const auto valid_child=[](const auto& child){
              return !(child.blast_children!=255||child.flight_children||child.impact_children||child.impact_hazard||child.impact_area||child.blast||child.smoke||
                child.sustained||child.socket||child.guidance||child.proximity||
               child.rewind_hit||child.plant_on_stage!=255||child.stock_return!=BorgProjectileReturn::None||
                child.ignore_actors||!child.damage||child.launch_body_heading||child.continue_after_actor_hit);
        };
        if(a.blast_children!=255){
            const auto& child=p.projectiles[a.blast_children];
            if(!valid_child(child)||child.count<2||child.spread)return false;
        }
        if(a.flight_children&&!valid_child(p.projectiles[a.flight_children->projectile]))return false;
        if(a.impact_children){
            const auto& child=p.projectiles[a.impact_children->projectile];
            if(!valid_child(child)||(a.impact_children->radial&&(child.count<2||child.spread)))return false;
        }
    }
    for(const auto& e:p.emitters){
        if(e.watches>=p.moves.size()||e.projectile>=p.projectiles.size()||p.projectiles[e.projectile].sustained||
           (p.moves[e.watches].releases.empty()&&!p.moves[e.watches].held_fire)||
           p.projectiles[e.projectile].stock_return!=BorgProjectileReturn::None||
           p.projectiles[e.projectile].rewind_hit||p.projectiles[e.projectile].plant_on_stage!=255||p.projectiles[e.projectile].socket||
           p.projectiles[e.projectile].impact_area||
           p.projectiles[e.projectile].launch_body_heading||p.projectiles[e.projectile].continue_after_actor_hit||
           !finite(e.side,-120,120)||!finite(e.height,0,240)||!finite(e.behind,0,180)||!e.lifetime||e.lifetime>600)
            return false;
    }
    for(const auto& set:p.anchor_sets)
        if(!set.capacity||set.capacity>kBorgBattleAnchors||
           (set.fence&&(set.return_on_reach||set.capacity<2||!set.fence->radius||set.fence->radius>120||!set.fence->damage||
                        set.fence->damage>10000||!set.fence->ticks||set.fence->ticks>600||set.fence->recoil>40)))return false;
    for(const auto& path:p.movement_sound)if(!path.empty()&&
        (p.movement==BorgBattleMovement::Stationary||!borg_package_asset_path_valid(path)))return false;
    for(const auto& fx:p.movement_fx){
        const auto event=u8(fx.event);
        if((event!=1&&event!=2&&event!=4&&event!=8)||unsigned(fx.style)>1||!fx.size||fx.size>180||fx.bone>=32||
           (fx.event==BorgMovementCue::Rise?fx.duration!=0:(!fx.duration||fx.duration>60))||
           !std::all_of(fx.local.begin(),fx.local.end(),[&](f32 x){return finite(x,-500,500);}))return false;
    }
    std::array<const BorgBattleMove*,2> gestures{};
    if(p.part_slots.size()>kBorgBattlePartSlots)return false;
    const auto part_pose_valid=[&](const auto& pose){
        if(pose)for(unsigned axis=0;axis<3;++axis)
            if(!finite(pose->offset[axis],-1000,1000)||!finite(pose->degrees[axis],-180,180))return false;
        return true;
    };
    for(unsigned i=0;i<p.part_slots.size();++i){
        const auto& part=p.part_slots[i];
        if(!borg_gameplay_detail::id(part.id)||part.bone>=32||part.record>255||!part_pose_valid(part.pose)||
           unsigned(part.basis)>unsigned(BorgPartBasis::BoneZyx))return false;
        for(auto step:part.turn_step)if(step>8192)return false;
        if(part.mount_spin&&(!part.record||part.mount_spin->axis>2||part.mount_spin->period<2||part.mount_spin->period>240))
            return false;
        for(unsigned j=0;j<i;++j)if(p.part_slots[j].id==part.id)return false;
    }
    if(p.static_part_hierarchies.size()>kBorgBattlePartRecords)return false;
    for(unsigned i=0;i<p.static_part_hierarchies.size();++i){
        const auto record=p.static_part_hierarchies[i];
        if(!record||record>255||std::count(p.static_part_hierarchies.begin(),p.static_part_hierarchies.end(),record)!=1||
           std::any_of(p.part_materials.begin(),p.part_materials.end(),[record](const auto& m){return m.record==record;})||
           std::none_of(p.part_slots.begin(),p.part_slots.end(),[record](const auto& s){return s.record==record;}))return false;
    }
    if(p.part_materials.size()>kBorgBattlePartRecords)return false;
    for(unsigned i=0;i<p.part_materials.size();++i){
        const auto& part=p.part_materials[i];
        if(!part.record||part.record>255||part.clip>255||unsigned(part.playback)>2||part.first>part.last||part.last>600||
           part.last-part.first>=240||((part.first==part.last)!=(part.playback==BorgPartMaterialPlayback::Sample)))return false;
        for(unsigned j=0;j<i;++j)if(p.part_materials[j].record==part.record)return false;
        bool used=false;
        for(const auto& slot:p.part_slots)used|=slot.record==part.record;
        for(const auto& move:p.moves)for(const auto& swap:move.part_swaps)used|=swap.record==part.record;
        if(!used)return false;
    }
    unsigned release_sound_count=0;
    std::array<bool,kBorgBattleAttacks> hit_continuation_released{};
    for(const auto& m:p.moves){
        if(!borg_battle_reaction_scope_valid(p,m)||!borg_battle_pose_followup_valid(p,m)||
           !borg_battle_contact_damage_valid(p,m)||!borg_battle_contact_sound_valid(p,m)||
           !borg_battle_contact_impact_valid(p,m)||!borg_battle_contact_pose_hold_valid(p,m))return false;
        if(m.pose_authored_loop_window&&(m.static_pose||!p.record_zero_body||p.movement!=BorgBattleMovement::Grounded))return false;
        if((p.static_body()&&!m.static_pose)||(m.static_pose&&(m.pose_start||!m.part_motions.empty()||
           !m.part_spins.empty()||!m.part_scrolls.empty()||!m.part_swaps.empty())))return false;
        if(p.cruise_entry&&(!m.static_pose||m.context!=BorgBattleContext::Airborne))return false;
        if(p.static_grounded_body&&m.context!=BorgBattleContext::Grounded)return false;
        // A pose-free move may keep ordinary locomotion only as a plain paid
        // projectile, or as a field-only action: the placed field is the
        // visible executable marker and the body keeps its locomotion pose.
        if(m.static_pose&&!p.static_cruise_body&&
           (p.movement!=BorgBattleMovement::Grounded||(m.releases.empty()&&!m.field)||m.held_fire||m.melee||
            m.detonation||m.guard||!m.emitters.empty()||m.power_field||m.slow_field||!m.anchors.empty()||
            (m.field&&!m.releases.empty())||
            (!m.free_release&&std::any_of(m.releases.begin(),m.releases.end(),
                                          [](const auto& r){return !r.cost;}))))return false;
        if(m.charge&&(m.charge->ticks<2||m.charge->ticks>60))return false;
        if(m.free_release&&(m.releases.empty()||m.entry_stock||
           std::any_of(m.releases.begin(),m.releases.end(),[](const auto& r){return r.cost;})))return false;
        if(m.entry_stock&&(m.entry_stock->stock>=p.stocks.size()||!m.entry_stock->minimum||
           m.entry_stock->minimum>p.stocks[m.entry_stock->stock].capacity))return false;
        if(const auto& g=m.release_gate;g&&(
           m.held_fire||m.hold_cycles||m.aim_lock||m.melee||m.guard||m.field||m.power_field||m.slow_field||
           m.detonation||!m.anchors.empty()||!m.emitters.empty()||m.releases.size()!=1||
           m.releases[0].frame!=g->frame||!g->timeout||g->timeout>120||
           !finite(g->half_angle,.01f,1.5707964f)))return false;
        const bool multi=m.activation==BorgBattleActivation::Tap||m.activation==BorgBattleActivation::DoubleTap;
        if(m.duration<2||m.duration>600||(m.releases.empty()&&!m.melee&&!m.held_fire&&!m.detonation&&!m.guard&&!m.power_field&&
           !m.slow_field&&!m.field&&m.emitters.empty()&&m.anchors.empty())||
           m.releases.size()>kBorgBattleReleases||
           unsigned(m.button)>1||unsigned(m.context)>2||!clip(m.clip)||m.pose_start>600||
           !finite(m.entry_range,0,1200)||
           unsigned(m.activation)>3||
           (m.activation==BorgBattleActivation::ReleaseWindow?(m.minimum_hold>m.maximum_hold||m.maximum_hold>kBorgBattleHoldTicks||m.tap_window):
             multi?(m.minimum_hold||!m.maximum_hold||m.maximum_hold>60||!m.tap_window||m.tap_window>60):
                   (m.minimum_hold||m.maximum_hold||m.tap_window))||
           (m.hold_cycles&&(m.hold_cycles<2||m.hold_cycles>kBorgBattleRepeatCycles||m.activation!=BorgBattleActivation::Press||m.melee||
                            m.releases.size()!=1||!m.releases.front().cost)))return false;
        if(m.part_spins.size()+m.part_motions.size()+m.part_scrolls.size()>kBorgBattlePartAnimations)return false;
        std::array<u8,32> axes{};
        for(const auto& spin:m.part_spins){
            if(!spin.bone||spin.bone>=32||spin.axis>2||spin.period<2||spin.period>240||
               (axes[spin.bone]&(1u<<spin.axis)))return false;
            axes[spin.bone]|=u8(1u<<spin.axis);
        }
        u32 animated=0;
        for(const auto& part:m.part_motions){
            if(!part.bone||part.bone>=32||!clip(part.clip)||part.first>600||part.count<2||part.count>240||
               (animated&(1u<<part.bone)))return false;
            animated|=1u<<part.bone;
        }
        axes={};
        for(const auto& scroll:m.part_scrolls){
            if(scroll.bone>=32||scroll.axis>1||scroll.period<2||scroll.period>240||
               (axes[scroll.bone]&(1u<<scroll.axis)))return false;
            axes[scroll.bone]|=u8(1u<<scroll.axis);
        }
        u32 swapped=0;
        for(const auto& swap:m.part_swaps){
            if(swap.slot>=p.part_slots.size()||swap.first>swap.last||swap.last>=m.duration||
               swap.record>255||(swapped&(1u<<swap.slot))||(swap.pose&&(!swap.record||!part_pose_valid(swap.pose)))||
               (swap.bone&&(!swap.record||*swap.bone>=32)))return false;
            swapped|=1u<<swap.slot;
        }
        if(m.held_fire){
            const auto& h=*m.held_fire;
            if(!m.releases.empty()||m.melee||m.detonation||m.guard||!m.emitters.empty()||!m.anchors.empty()||
               m.activation!=BorgBattleActivation::Press||m.hold_cycles||
               !h.warmup||h.warmup>600||!h.fast_interval||h.fast_interval>h.slow_interval||h.slow_interval>600||
               h.cooldown>600||!borg_battle_emission_valid(p,h.stock,h.cost,h.projectile)||p.projectiles[h.projectile].sustained||
               p.projectiles[h.projectile].continue_after_actor_hit)return false;
        }
        if(multi){
            auto& other=gestures[unsigned(m.button)];
            if(other&&(other->maximum_hold!=m.maximum_hold||other->tap_window!=m.tap_window))return false;
            other=&m;
        }
        unsigned deployed=0;
        if((!m.emitters.empty()&&m.hold_cycles)||m.emitters.size()>kBorgBattleEmitters)return false;
        for(const auto& d:m.emitters){
            if(!d.frame||d.frame>=m.duration||d.emitter>=p.emitters.size()||(deployed&(1u<<d.emitter)))return false;
            deployed|=1u<<d.emitter;
        }
        if((!m.anchors.empty()&&m.hold_cycles)||m.anchors.size()>kBorgBattleAnchors)return false;
        unsigned previous_anchor=0;
        for(const auto& a:m.anchors){
            if(a.frame<=previous_anchor||a.frame>=m.duration||a.set>=p.anchor_sets.size()||
               a.rewind_speed>16||(a.rewind_speed&&(a.cost||a.forward||a.height||p.anchor_sets[a.set].fence))||
               !finite(a.forward,-400,400)||!finite(a.height,-120,400)||
                (a.cost?(a.stock>=p.stocks.size()||a.cost>p.stocks[a.stock].capacity||
                 (p.anchor_sets[a.set].return_on_reach?(p.stocks[a.stock].refill_ticks||a.cost!=1):
                    (!p.stocks[a.stock].refill_ticks||(p.stocks[a.stock].add_on_debit&&a.cost!=1)))):a.stock!=255))
                return false;
            previous_anchor=a.frame;
        }
        if(m.guard){
            const auto& g=*m.guard;
            if(!g.first||g.last<g.first||g.last>=m.duration||!finite(g.half_angle,.01f,3.141593f)||
               !g.contacts||g.contacts>16||(g.mask!=1&&g.mask!=2&&g.mask!=3&&g.mask!=7)||m.hold_cycles)return false;
        }
        if(m.detonation){
            const auto& d=*m.detonation;
            if(!d.frame||d.frame>=m.duration||d.projectile>=p.projectiles.size()||m.hold_cycles)return false;
            const auto& shot=p.projectiles[d.projectile];
            if(!shot.blast||shot.blast->trigger!=BorgBlastTrigger::Remote||
               (d.landed_only&&shot.floor!=BorgProjectileFloor::Land))return false;
        }
        if(m.power_field){
            const auto& field=*m.power_field;
            if(!field.frame||field.frame>=m.duration||field.radius<8||field.radius>600||!field.ticks||field.ticks>600||
               field.damage_percent<100||field.damage_percent>300||field.visual_percent<100||field.visual_percent>200||
               (field.damage_percent==100&&field.visual_percent==100)||
               !finite(field.forward,-500,500)||!finite(field.height,-300,500)||m.hold_cycles||m.held_fire)
                return false;
        }
        if(m.slow_field){
            const auto& field=*m.slow_field;
            if(!field.frame||field.frame>=m.duration||field.radius<8||field.radius>600||!field.ticks||field.ticks>600||
               field.actor_percent<25||field.actor_percent>100||field.shot_percent<25||field.shot_percent>100||
               (field.actor_percent==100&&field.shot_percent==100&&!field.break_guidance)||
               !finite(field.forward,-500,500)||!finite(field.height,-300,500)||m.hold_cycles||m.held_fire||m.power_field)
                return false;
        }
        if(m.field){
            const auto& f=*m.field;
            if(m.power_field||m.slow_field||m.held_fire||m.hold_cycles||!f.frame||f.frame>=m.duration||
               !f.ticks||f.ticks>600||!finite(f.forward,-500,500)||!finite(f.height,-300,500)||
               unsigned(f.shape)>1||!f.tint[3]||(!f.texture.empty()&&!borg_package_asset_path_valid(f.texture))||
               (f.socket&&(f.forward||f.height||f.socket->aim_height||f.socket->bone>=32||
                 !std::all_of(f.socket->local.begin(),f.socket->local.end(),[&](f32 x){return finite(x,-500,500);})))||
               (f.effects.empty()&&!f.shot_pull&&!f.core_damage&&!f.actor_pull)||f.effects.size()>kBorgFieldEffects)return false;
            for(auto extent:f.extent)if(extent<8||extent>600||(f.shape==BorgFieldShape::Sphere&&extent!=f.extent[0]))return false;
            if(f.shot_pull&&(unsigned(f.shot_pull->scope)>3||!finite(f.shot_pull->acceleration,.125f,16.f)))return false;
            if(f.drift&&(f.follow||!finite(f.drift->speed,.25f,100.f)||!finite(f.drift->deceleration,.25f,f.drift->speed)||
                std::ceil(f.drift->speed/f.drift->deceleration)>=f.ticks||
                std::ceil(f.drift->speed/f.drift->deceleration)>120))return false;
            if(f.slot&&(!borg_gameplay_detail::id(f.slot->id)||
               (f.slot->on_ready&&(!f.drift||f.drift->active_while_moving))))return false;
            if(f.actor_pull&&(unsigned(f.actor_pull->scope)>3||!finite(f.actor_pull->step,.125f,16.f)||
               !finite(f.actor_pull->half_angle,0,1.5707964f)))return false;
            if(f.actor_pull&&f.actor_pull->point){
                const auto& point=*f.actor_pull->point;
                if(!finite(point.forward,-2000,2000)||!finite(point.height,-600,600)||point.minimum>600)return false;
            }
            if(const auto& core=f.core_damage;core&&(unsigned(core->scope)>3||!core->radius||
               core->radius>*std::min_element(f.extent.begin(),f.extent.end())||!core->damage||core->damage>10000||
               !core->interval||core->interval>=f.ticks||
               (p.movement==BorgBattleMovement::NormalGRed&&core->scope!=BorgFieldScope::Enemies)))return false;
            if(const auto& debit=f.debit;debit&&(debit->cost<1||debit->cost>255||
               std::none_of(p.stocks.begin(),p.stocks.end(),
                            [&](const auto& stock){return stock.id==debit->stock;})))return false;
            u32 seen=0;
            for(const auto& e:f.effects){
                if(unsigned(e.kind)>4||unsigned(e.scope)>3)return false;
                const auto bit=1u<<(4*unsigned(e.kind)+unsigned(e.scope));
                if(seen&bit)return false;
                seen|=bit;
                const unsigned lo=e.kind==BorgFieldEffectKind::BreakGuidance?0:
                                   e.kind>=BorgFieldEffectKind::ShotDamage?100:25;
                const unsigned hi=e.kind==BorgFieldEffectKind::BreakGuidance?0:
                                   e.kind==BorgFieldEffectKind::ShotDamage?300:
                                   e.kind==BorgFieldEffectKind::ShotSpeed?100:200;
                if(e.value<lo||e.value>hi||e.value==100)return false;
            }
        }
        if(m.melee){
            const auto& a=*m.melee;
            if(!a.first||a.last<a.first||a.last>=m.duration||!a.damage||a.damage>10000||
               !finite(a.reach,1,220)||!finite(a.radius,1,40)||!finite(a.height,-200,200)||!finite(a.half_height,1,100))return false;
        }
        if(m.aim_lock&&(m.releases.empty()||*m.aim_lock>m.releases.front().frame||m.held_fire||m.melee))return false;
        unsigned prior=0;
        for(const auto& r:m.releases){
            if(r.frame<=prior||r.frame>=m.duration||!borg_battle_release_valid(p,r))return false;
            if(p.projectiles[r.projectile].continue_after_actor_hit){
                if(r.count||r.return_pair||m.releases.size()!=1||m.hold_cycles||!borg_battle_direct_releases(p,m))return false;
                hit_continuation_released[r.projectile]=true;
            }
            if(!r.debit_sound.empty()&&(!r.cost||++release_sound_count>kBorgReleaseSounds||
                                       !borg_package_asset_path_valid(r.debit_sound)))return false;
            if(m.release_gate&&p.projectiles[r.projectile].sustained)return false;
            if(r.count>r.followups.size())return false;
            if(r.count&&(!r.cost||p.projectiles[r.projectile].rewind_hit||p.projectiles[r.projectile].sustained))return false;
            for(unsigned i=0;i<r.count;++i){
                const auto extra=r.followups[i];
                if(!borg_battle_emission_valid(p,255,0,extra)||p.projectiles[extra].rewind_hit||p.projectiles[extra].sustained||
                   p.projectiles[extra].continue_after_actor_hit)
                    return false;
            }
            if(m.aim_lock&&p.projectiles[r.projectile].guidance)return false;
            if(const auto& shot=p.projectiles[r.projectile];shot.sustained&&
               (m.releases.size()!=1||m.hold_cycles||m.aim_lock||m.melee||shot.lifetime>m.duration-r.frame))return false;
            prior=r.frame;
        }
    }
    for(unsigned i=0;i<p.projectiles.size();++i)
        if(p.projectiles[i].continue_after_actor_hit&&!hit_continuation_released[i])return false;
    for(const auto& m:p.moves)
        if(gestures[unsigned(m.button)]&&m.activation!=BorgBattleActivation::Tap&&m.activation!=BorgBattleActivation::DoubleTap)
            return false;
    return true;
}
// A small executable subset, not the audit sidecars. Releases are ordered
// animation-frame events, each optionally spending before requesting a volley.
// Keep unsupported capabilities out; no interpreted expressions or freeform calls.
inline bool borg_read_gameplay_profile(std::string_view text,BorgGameplayProfile* out,std::string* error){
    using namespace borg_gameplay_detail;
    using borg_package_detail::split_fields;
    unsigned line=1;
    auto fail=[&](std::string_view why){
        if(error)*error="gameplay line "+std::to_string(line)+": "+std::string(why);
        return false;
    };
    if(!out||text.size()>kBorgGameplayBytes||borg_package_detail::line(text)!="GFBORG-GAMEPLAY 1")
        return fail("expected bounded GFBORG-GAMEPLAY 1");
    struct ReleaseRef {unsigned line;std::string move,stock,projectile;u16 frame=0,cost=0;};
    struct EntryStockRef {unsigned line;std::string move,stock;u16 minimum;};
    struct ReleaseGateRef {unsigned line;std::string move;BorgReleaseAimGate spec;};
    struct AdditionalReleaseRef {unsigned line;std::string move,projectile;u16 frame=0;bool legacy=false;};
    struct ReleasePairRef {unsigned line;std::string move;u16 frame=0;};
    struct HitRef {unsigned line;std::string projectile;bool pull=false;u16 amount=0,ticks=0;};
    struct InputRef {unsigned line;std::string move;BorgBattleActivation activation;u16 first=0,second=0;};
    struct MeleeRef {unsigned line;std::string move;BorgBattleMelee shape;};
    struct RangeRef {unsigned line;std::string move;f32 distance;};
    struct HeldFireRef {unsigned line;std::string move,stock,projectile;BorgBattleHeldFire spec;};
    struct BlastRef {unsigned line;std::string projectile;BorgBattleBlast spec;};
    struct ActorContactRef {unsigned line;std::string projectile;bool continuing;};
    struct DetonationRef {unsigned line;std::string move,projectile;u16 frame=0;bool landed_only=false;};
    struct ReturnRef {unsigned line;std::string projectile;BorgProjectileReturn reason;};
    struct GuardRef {unsigned line;std::string move;BorgBattleGuard spec;};
    struct EmitterRef {unsigned line;std::string watches,projectile;BorgBattleEmitter spec;};
    struct DeployRef {unsigned line;std::string move,emitter;u16 frame=0;};
    struct AnchorRef {unsigned line;std::string move,set,stock;BorgBattleAnchorMarker marker;};
    struct AnchorOffsetRef {unsigned line;std::string move;u16 frame;f32 forward,height;};
    struct AnchorFenceRef {unsigned line;std::string set;BorgAnchorFence spec;};
    struct RewindHitRef {unsigned line;std::string projectile,set;u8 speed;};
    struct StageAnchorRef {unsigned line;std::string projectile,set;};
    struct AnchorFallbackRef {unsigned line;std::string projectile,fallback;};
    struct SocketRef {unsigned line;std::string projectile;BorgBattleSocket spec;};
    struct LookRef {unsigned line;std::string projectile;BorgProjectileLook spec;};
    struct RecordLookRef {unsigned line;std::string projectile;BorgProjectileRecordLook spec;};
    struct RecordTrailRef {unsigned line;std::string projectile;BorgProjectileRecordTrail spec;};
    struct ImpactRecordRef {unsigned line;std::string projectile;BorgImpactRecord spec;};
    struct ImpactRecordGrowthRef {unsigned line;std::string projectile;u16 end;};
    struct ImpactSoundRef {unsigned line;std::string projectile;BorgImpactSound spec;};
    struct PoseStartRef {unsigned line;std::string move;u16 frame;};
    struct ContactPowerRef {unsigned line;std::string move;u16 base;};
    struct ContactTierRef {unsigned line;std::string move;BorgContactTier tier;};
    struct ContactTierWearRef {unsigned line;std::string move;u8 wear;};
    struct ContactSoundRef {unsigned line;std::string move;u8 band;BorgContactSound sound;};
    struct ContactImpactRef {unsigned line;std::string move;u8 team;BorgContactImpactLayer spec;};
    struct ContactPoseHoldRef {unsigned line;std::string move;BorgContactPoseHold spec;};
    struct PoseFollowupRef {unsigned line;std::string move;BorgBattlePoseFollowup spec;};
    struct SustainedRef {unsigned line;std::string projectile;BorgSustainedBeam spec;};
    struct BeamSweepRef {unsigned line;std::string projectile;BorgBeamSweep spec;};
    struct ProximityRef {unsigned line;std::string projectile;BorgBombProximity spec;};
    struct AttractionRef {unsigned line;std::string projectile;BorgBombAttraction spec;};
    struct FuseTailRef {unsigned line;std::string projectile;u16 ticks;};
    struct BlastChildrenRef {unsigned line;std::string projectile,child;};
    struct FlightChildrenRef {unsigned line;std::string projectile,child;BorgFlightChildren spec;};
    struct ImpactChildrenRef {unsigned line;std::string projectile,child;BorgImpactChildren spec;};
    struct ImpactTrailRef {unsigned line;std::string projectile;BorgImpactTrail spec;};
    struct ImpactHazardRef {unsigned line;std::string projectile;BorgImpactHazard spec;};
    struct ImpactAreaRef {unsigned line;std::string projectile;BorgImpactArea spec;};
    struct ImpactTimingRef {unsigned line;std::string projectile;u16 trail=0,hazard=0,query=0;};
    struct SmokeRef {unsigned line;std::string projectile;BorgSmoke spec;};
    struct InkRef {unsigned line;std::string projectile;BorgInkOverlay spec;};
    struct AccuracyRef {unsigned line;std::string projectile;BorgBlastAccuracy spec;};
    struct PowerFieldRef {unsigned line;std::string move;BorgPowerField spec;};
    struct SlowFieldRef {unsigned line;std::string move;BorgSlowField spec;};
    struct FieldRef {unsigned line;std::string move;BorgField spec;};
    struct FieldEffectRef {unsigned line;std::string move;BorgFieldEffect spec;};
    struct FieldShotPullRef {unsigned line;std::string move;BorgFieldShotPull spec;};
    struct FieldDriftRef {unsigned line;std::string move;BorgFieldDrift spec;};
    struct FieldSlotRef {unsigned line;std::string move;BorgFieldSlot spec;};
    struct FieldActorPullRef {unsigned line;std::string move;BorgFieldActorPull spec;};
    struct FieldPullPointRef {unsigned line;std::string move;BorgFieldPullPoint spec;};
    struct FieldCoreDamageRef {unsigned line;std::string move;BorgFieldCoreDamage spec;};
    struct FieldDebitRef {unsigned line;std::string move;BorgFieldDebit spec;};
    struct MoveChargeRef {unsigned line;std::string move;BorgMoveCharge spec;};
    struct ReactionModeRef {unsigned line;std::string move;u8 mode;};
    struct ReactionOffsetRef {unsigned line;std::string move;s8 yaw,pitch;};
    struct FreeReleaseRef {unsigned line;std::string move;};
    struct FieldTextureRef {unsigned line;std::string move,path;};
    struct FieldSocketRef {unsigned line;std::string move;BorgBattleSocket socket;};
    struct ProjectileAnimationRef {unsigned line;std::string projectile;BorgProjectileAnimation spec;};
    struct PartSpinRef {unsigned line;std::string move;BorgBattlePartSpin spec;};
    struct PartMotionRef {unsigned line;std::string move;BorgBattlePartMotion spec;};
    struct PartScrollRef {unsigned line;std::string move;BorgBattlePartScroll spec;};
    struct PartSwapRef {unsigned line;std::string move,slot;BorgBattlePartSwap spec;};
    struct PartPoseRef {unsigned line;std::string move,slot;BorgBattlePartPose spec;};
    struct PartTurnRef {unsigned line;std::string slot;std::array<u16,3> step;};
    struct PartBasisRef {unsigned line;std::string slot;BorgPartBasis basis;};
    struct MountSpinRef {unsigned line;std::string slot;BorgBattlePartSpin spec;};
    struct PartBoneRef {unsigned line;std::string move,slot;u8 bone;};
    struct GuidanceRef {unsigned line;std::string projectile;BorgBattleGuidance spec;};
    struct SpeedRampRef {unsigned line;std::string projectile;BorgProjectileSpeedRamp spec;};
    std::vector<ReleaseRef> refs;
    std::vector<EntryStockRef> entry_stocks;
    std::vector<ReleaseGateRef> release_gates;
    std::vector<std::pair<unsigned,std::string>> add_on_debit_stocks;
    std::vector<AdditionalReleaseRef> additional_releases;
    std::vector<ReleasePairRef> release_pairs;
    std::vector<HitRef> hits;
    std::vector<InputRef> inputs;
    std::vector<MeleeRef> melee;
    std::vector<RangeRef> ranges;
    std::vector<HeldFireRef> held_fire;
    std::vector<BlastRef> blasts;
    std::vector<ActorContactRef> passes;
    std::vector<SpeedRampRef> speed_ramps;
    std::vector<DetonationRef> detonations;
    std::vector<std::pair<unsigned,std::string>> first_detonations;
    std::vector<ReturnRef> returns;
    std::vector<GuardRef> guards;
    std::vector<EmitterRef> emitters;
    std::vector<DeployRef> deployments;
    std::vector<AnchorRef> anchors;
    std::vector<AnchorOffsetRef> anchor_offsets;
    std::vector<AnchorFenceRef> anchor_fences;
    std::vector<std::pair<unsigned,std::string>> anchor_returns;
    std::vector<std::pair<std::string,u8>> fence_recoils;
    std::vector<RewindHitRef> rewind_hits;
    std::vector<StageAnchorRef> stage_anchors;
    std::vector<AnchorFallbackRef> anchor_fallbacks;
    std::vector<SocketRef> sockets;
    struct LaunchSoundRef {unsigned line;std::string projectile,path;};
    std::vector<LaunchSoundRef> launch_sounds;
    std::vector<std::pair<unsigned,std::pair<std::string,std::string>>> action_sounds; // line, (move, path)
    struct ReleaseSoundRef {unsigned line;std::string move,path;u16 frame;};
    std::vector<ReleaseSoundRef> release_sounds;
    std::vector<std::pair<unsigned,std::string>> body_launches;
    std::vector<LookRef> looks;
    std::vector<RecordLookRef> record_looks;
    std::vector<RecordTrailRef> record_trails;
    std::vector<ImpactRecordRef> impact_records;
    std::vector<ImpactRecordGrowthRef> impact_record_growth;
    std::vector<ImpactSoundRef> impact_sounds;
    std::vector<PoseStartRef> starts;
    std::vector<ContactPowerRef> contact_powers;
    std::vector<std::pair<unsigned,std::string>> contact_minimum_hp;
    std::vector<ContactTierRef> contact_tiers;
    std::vector<ContactTierWearRef> contact_tier_reset_wear;
    std::vector<ContactSoundRef> contact_sounds;
    std::vector<ContactImpactRef> contact_impacts;
    std::vector<ContactPoseHoldRef> contact_pose_holds;
    std::vector<std::pair<unsigned,std::string>> damage_credit_ladder_moves;
    std::vector<PoseFollowupRef> pose_followups;
    std::vector<std::pair<unsigned,std::string>> pose_loop_windows;
    std::vector<PoseStartRef> aim_locks;
    std::vector<SustainedRef> sustained;
    std::vector<BeamSweepRef> beam_sweeps;
    std::vector<ProximityRef> proximities;
    std::vector<AttractionRef> attractions;
    std::vector<FuseTailRef> fuse_tails;
    std::vector<BlastChildrenRef> blast_children;
    std::vector<FlightChildrenRef> flight_children;
    std::vector<ImpactChildrenRef> impact_children;
    std::vector<ImpactTrailRef> impact_trails;
    std::vector<ImpactHazardRef> impact_hazards;
    std::vector<ImpactAreaRef> impact_areas;
    std::vector<ImpactTimingRef> impact_timings;
    std::vector<std::pair<unsigned,std::string>> impact_world_space;
    std::vector<SmokeRef> smoke;
    std::vector<InkRef> ink;
    std::vector<AccuracyRef> accuracy;
    std::vector<std::pair<unsigned,std::string>> accuracy_sensitive;
    std::vector<PowerFieldRef> power_fields;
    std::vector<SlowFieldRef> slow_fields;
    std::vector<FieldRef> fields;
    std::vector<FieldEffectRef> field_effects;
    std::vector<FieldShotPullRef> field_shot_pulls;
    std::vector<FieldDriftRef> field_drifts;
    std::vector<FieldSlotRef> field_slots;
    std::vector<FieldActorPullRef> field_actor_pulls;
    std::vector<FieldPullPointRef> field_pull_points;
    std::vector<std::pair<unsigned,std::string>> field_repulsions;
    std::vector<FieldCoreDamageRef> field_core_damage;
    std::vector<FieldDebitRef> field_debit;
    std::vector<MoveChargeRef> move_charge;
    std::vector<ReactionModeRef> reaction_mode;
    std::vector<std::pair<unsigned,std::string>> reaction_roots;
    std::vector<ReactionOffsetRef> reaction_offsets;
    std::vector<FreeReleaseRef> free_release;
    std::vector<FieldTextureRef> field_textures;
    std::vector<FieldSocketRef> field_sockets;
    std::vector<std::pair<unsigned,std::string>> field_follows;
    std::vector<ProjectileAnimationRef> animations;
    std::vector<PartSpinRef> spins;
    std::vector<PartMotionRef> part_motions;
    std::vector<PartScrollRef> scrolls;
    std::vector<PartSwapRef> swaps;
    std::vector<PartPoseRef> part_poses;
    std::vector<PartTurnRef> part_turns;
    std::vector<PartBasisRef> part_bases;
    std::vector<MountSpinRef> mount_spins;
    std::vector<PartBoneRef> part_bones;
    std::vector<GuidanceRef> guidance;
    BorgGameplayProfile next;bool has_movement=false;
    unsigned locomotion=0;
    const auto field_scope=[](std::string_view text,BorgFieldScope* out){
        constexpr std::array<std::string_view,4> scopes{"all","enemies","allies","source"};
        const auto it=std::find(scopes.begin(),scopes.end(),text);
        if(it==scopes.end())return false;
        *out=BorgFieldScope(it-scopes.begin());return true;
    };
    const auto impact_pattern=[&](const auto& a,BorgImpactPattern& spec){
        if(a[1]!="actor"&&a[1]!="floor"&&a[1]!="stage")return false;
        spec.lane=a[1]=="actor"?BorgImpactLane::Actor:a[1]=="floor"?BorgImpactLane::Floor:BorgImpactLane::Stage;
        return number(a[2],1,32,&spec.count)&&number(a[3],1,30,&spec.interval)&&
               number(a[4],0,1200,&spec.radius)&&number(a[5],0,80,&spec.advance)&&
               scalar(a[6],-180,180,&spec.degrees)&&number(a[7],1,8,&spec.arms);
    };
    while(!text.empty()){
        ++line;const auto row=borg_package_detail::line(text);
        if(row.empty()||row.starts_with('#'))continue;
        const auto eq=row.find('=');
        if(eq==row.npos)return fail("expected key=value");
        const auto key=row.substr(0,eq),value=row.substr(eq+1);
        if(key=="variant"||key=="rig-prefix"||key=="pose-profile"){
            auto& dest=key=="variant"?next.variant:key=="rig-prefix"?next.rig_prefix:next.pose_profile;
            if(!dest.empty()||!(key=="variant"?id(value):borg_package_asset_path_valid(value)))return fail("invalid or duplicate identity/rig field");
            dest=value;
        }else if(key=="body-motion"){
            if(next.static_body()||(value!="static-cruise"&&value!="static-grounded"))return fail("invalid or duplicate static body policy");
            next.static_cruise_body=value=="static-cruise";next.static_grounded_body=value=="static-grounded";
        }else if(key=="idle-loop"){
            if(next.idle_authored_loop||value!="authored-bit")return fail("invalid or duplicate idle loop policy");
            next.idle_authored_loop=true;
        }else if(key=="walk-loop"){
            if(next.walk_authored_loop||value!="authored-bit")return fail("invalid or duplicate walk loop policy");
            next.walk_authored_loop=true;
        }else if(key=="body-record"){
            if(next.record_zero_body||value!="0")return fail("only explicit record-zero body binding is supported");
            next.record_zero_body=true;
        }else if(key=="body-material"){
            if(next.body_immutable_highlight||next.body_diffuse_overwrite_pair)
                return fail("invalid or duplicate body material policy");
            if(value=="immutable-highlight")next.body_immutable_highlight=true;
            else if(value=="diffuse-overwrite-pair")next.body_diffuse_overwrite_pair=true;
            else return fail("invalid or duplicate body material policy");
        }else if(key=="animated-translation-percent"){
            u16 percent=0;
            if(next.animated_translation_percent!=100||!number(value,25,200,&percent)||percent==100)
                return fail("invalid or duplicate animated translation scale");
            next.animated_translation_percent=percent;
        }else if(key=="zero-reflection"){
            std::array<std::string_view,2> fields;
            BorgZeroReflection policy;
            if(next.zero_reflections.size()>=kBorgZeroReflectionMeshes||!split_fields(value,&fields)||!number(fields[1],1,64,&policy.count))
                return fail("invalid zero-reflection policy or extent");
            const auto offset=fields[0];const auto parsed=std::from_chars(offset.data(),offset.data()+offset.size(),policy.pobj,16);
            if(parsed.ec!=std::errc{}||parsed.ptr!=offset.data()+offset.size()||!borg_zero_reflection_valid(policy))
                return fail("invalid zero-reflection PObj offset");
            next.zero_reflections.push_back(policy);
        }else if(key=="body-mount"){
            std::array<std::string_view,5> fields;
            u16 record=0,bone=0;
            if(next.body_mounts.size()>=kCharacterPreviewMounts||!split_fields(value,&fields)||
               !number(fields[0],1,31,&record)||!number(fields[1],0,31,&bone))
                return fail("invalid body mount");
            CharacterPreviewMount mount{u8(record),u8(bone)};
            for(unsigned i=0;i<3;++i){
                const auto field=fields[i+2];s32 offset=0;
                const auto parsed=std::from_chars(field.data(),field.data()+field.size(),offset);
                if(parsed.ec!=std::errc{}||parsed.ptr!=field.data()+field.size()||offset< -4096||offset>4096)
                    return fail("invalid body mount offset");
                mount.offset[i]=s16(offset);
            }
            next.body_mounts.push_back(mount);
        }else if(key=="trailing-chain"){
            BorgTrailingChainSpec spec;
            if(next.trailing_chain)return fail("duplicate trailing-chain");
            auto bones=value;
            while(true){
                const auto separator=bones.find('|');
                u16 bone=0;
                if(spec.count==spec.bones.size()||!number(bones.substr(0,separator),1,31,&bone))
                    return fail("invalid trailing-chain bone");
                spec.bones[spec.count++]=u8(bone);
                if(separator==std::string_view::npos)break;
                bones.remove_prefix(separator+1);
            }
            if(!borg_trailing_chain_valid(spec))return fail("trailing-chain needs three to eight distinct bones");
            next.trailing_chain=spec;
        }else if(key=="health"){
            if(next.health||!number(value,1,10000,&next.health))return fail("invalid or duplicate health");
        }else if(key=="beam-recovery"){
            if(next.beam_recovery||!number(value,1,120,&next.beam_recovery))return fail("invalid or duplicate beam recovery");
        }else if(key=="powered-flight"){
            std::array<std::string_view,2> a;
            if(next.flight_ticks||!split_fields(value,&a)||!scalar(a[0],2,16,&next.flight_lift)||
               !number(a[1],1,120,&next.flight_ticks))return fail("invalid or duplicate powered-flight policy");
        }else if(key=="air-jump-points"){
            u16 count=0;
            if(next.air_jump_points||!number(value,1,8,&count))return fail("invalid or duplicate air-jump count");
            next.air_jump_points=u8(count);
        }else if(key=="pose-ground-offset"){
            if(!scalar(value,-1000.f,1000.f,&next.pose_ground_offset))
                return fail("invalid pose-ground offset");
        }else if(key=="air-action-budget"){
            u16 count=0;
            if(next.air_action_budget||!number(value,1,8,&count))return fail("invalid or duplicate air-action budget");
            next.air_action_budget=u8(count);
        }else if(key=="damage-factors"){
            std::array<std::string_view,4> a;
            if(next.damage_factors_declared||!split_fields(value,&a))return fail("invalid or duplicate damage factors");
            for(unsigned i=0;i<4;++i)
                if(!scalar(a[i],.01f,4.f,&next.damage_factors[i]))return fail("invalid damage factor");
            next.damage_factors_declared=true;
        }else if(key=="damage-model"){
            u16 declared=0;
            if(next.damage_model_declared||!number(value,0,65535,&declared))
                return fail("invalid or duplicate damage-model id");
            next.damage_model=declared;next.damage_model_declared=true;
        }else if(key=="damage-kind"){
            u16 declared=0;
            if(next.damage_kind_declared||!number(value,0,255,&declared))
                return fail("invalid or duplicate damage-kind");
            next.damage_kind=u8(declared);next.damage_kind_declared=true;
        }else if(key=="damage-attack-key"){
            u16 declared=0;
            if(next.damage_attack_key_declared||!number(value,0,65535,&declared))
                return fail("invalid or duplicate damage-attack-key");
            next.damage_attack_key=declared;next.damage_attack_key_declared=true;
        }else if(key=="damage-mask"){
            u16 declared=0;
            if(next.damage_target_mask_declared||!number(value,0,65535,&declared))
                return fail("invalid or duplicate damage-mask");
            next.damage_target_mask=declared;next.damage_target_mask_declared=true;
        }else if(key=="damage-immunity"){
            u16 key_value=0;
            if(next.damage_immunity.size()>=48||!number(value,0,65535,&key_value))
                return fail("invalid or full damage-immunity list");
            for(u16 existing:next.damage_immunity)
                if(existing==key_value)return fail("duplicate damage-immunity key");
            next.damage_immunity.push_back(key_value);
        }else if(key=="damage-vs"){
            std::array<std::string_view,5> a;
            u16 model=0;
            if(next.damage_rows.size()>=16||!split_fields(value,&a)||
               !number(a[0],0,65535,&model))return fail("invalid damage-vs row");
            for(const auto& row:next.damage_rows)if(row.model==model)return fail("duplicate damage-vs model");
            BorgGameplayProfile::DamageRow row;row.model=model;
            for(unsigned i=0;i<4;++i)
                if(!scalar(a[1+i],.01f,4.f,&row.factors[i]))return fail("invalid damage-vs factor");
            next.damage_rows.push_back(row);
        }else if(key=="damage-credit"||key=="damage-receive"){
            std::array<std::string_view,3> a;u16 team=0;
            auto& rows=key=="damage-credit"?next.damage_credit:next.damage_receive;
            if(!split_fields(value,&a)||!number(a[0],0,1,&team)||(rows.declared&(1u<<team)))
                return fail("invalid or duplicate damage role team");
            for(unsigned i=0;i<2;++i)
                if(!scalar(a[i+1],.01f,4.f,&rows.sides[team][i]))return fail("invalid damage role factor");
            rows.declared|=u8(1u<<team);
        }else if(key=="damage-receive-hp"||key=="damage-receive-force"||
                 key=="damage-credit-hp"||key=="damage-credit-force"){
            std::array<std::string_view,kBorgDamageLadderCells+1> a;u16 team=0;
            auto& ladder=key=="damage-receive-hp"?next.damage_receive_hp:
                key=="damage-receive-force"?next.damage_receive_force:
                key=="damage-credit-hp"?next.damage_credit_hp:next.damage_credit_force;
            if(!split_fields(value,&a)||!number(a[0],0,1,&team)||(ladder.declared&(1u<<team)))
                return fail("invalid or duplicate damage ladder team");
            for(unsigned i=0;i<kBorgDamageLadderCells;++i)
                if(!scalar(a[i+1],.01f,4.f,&ladder.sides[team][i]))return fail("invalid damage ladder factor");
            ladder.declared|=u8(1u<<team);
        }else if(key=="damage-credit-ladders"){
            if(damage_credit_ladder_moves.size()>=kBorgBattleMoves||!id(value))
                return fail("invalid damage-credit-ladders move");
            damage_credit_ladder_moves.emplace_back(line,std::string(value));
        }else if(key=="damage-credit-setting"||key=="damage-receive-setting"||
                 key=="damage-credit-rank"||key=="damage-receive-rank"){
            auto& rows=key.starts_with("damage-credit-")?next.damage_credit_indexed:next.damage_receive_indexed;
            const auto parse_indexed=[&]<unsigned N>(BorgDamageTeamRows<N>& grid){
                std::array<std::string_view,N+1> a;u16 team=0;
                if(!split_fields(value,&a)||!number(a[0],0,1,&team)||(grid.declared&(1u<<team)))return false;
                for(unsigned i=0;i<N;++i)if(!scalar(a[i+1],.01f,4.f,&grid.sides[team][i]))return false;
                grid.declared|=u8(1u<<team);return true;
            };
            if(!(key.ends_with("-setting")?parse_indexed(rows.setting):parse_indexed(rows.rank)))
                return fail("invalid or duplicate indexed damage row");
        }else if(key=="damage-tier-receive"){
            std::array<std::string_view,kBorgDamageTierCells+1> a;BorgDamageTierTable tier;
            if(next.damage_tier_receive||!split_fields(value,&a)||!number(a[0],1,600,&tier.idle_ticks))
                return fail("invalid or duplicate damage tier table");
            for(unsigned i=0;i<tier.factors.size();++i)
                if(!scalar(a[i+1],.01f,4.f,&tier.factors[i]))return fail("invalid damage tier factor");
            next.damage_tier_receive=tier;
        }else if(key=="damage-tier-reset"){
            if(next.damage_tier_reset_capacity||!number(value,1,32767,&next.damage_tier_reset_capacity))
                return fail("invalid or duplicate damage tier reset capacity");
        }else if(key=="damage-matchup-group"){
            if(!id(value)||!next.damage_matchup_group.empty())return fail("invalid or duplicate damage matchup group");
            next.damage_matchup_group=value;
        }else if(key=="damage-matchup-from"){
            std::array<std::string_view,2> a;f32 factor=0;
            if(next.damage_matchup_from.size()>=kBorgDamageMatchupRows||!split_fields(value,&a)||
               !id(a[0])||!scalar(a[1],.01f,4.f,&factor))return fail("invalid damage matchup row");
            for(const auto& row:next.damage_matchup_from)if(row.attacking_group==a[0])
                return fail("duplicate incoming damage matchup group");
            next.damage_matchup_from.push_back({std::string(a[0]),factor});
        }else if(key=="damage-side"){
            std::array<std::string_view,5> a;
            u16 team=0;
            if(next.damage_side_rows.size()>=2||!split_fields(value,&a)||
               !number(a[0],0,1,&team))return fail("invalid damage-side row");
            for(const auto& row:next.damage_side_rows)if(row.team==u8(team))return fail("duplicate damage-side team");
            BorgGameplayProfile::DamageSideRow row;row.team=u8(team);
            for(unsigned i=0;i<4;++i)
                if(!scalar(a[1+i],.01f,4.f,&row.factors[i]))return fail("invalid damage-side factor");
            next.damage_side_rows.push_back(row);
        }else if(key=="damage-chain"){
            std::array<std::string_view,4> a;
            if(next.damage_chain_count||!split_fields(value,&a))return fail("invalid or duplicate damage chain");
            for(unsigned i=0;i<4;++i)
                if(!scalar(a[i],.01f,2.f,&next.damage_chain[i]))return fail("invalid damage chain value");
            next.damage_chain_count=4;
        }else if(key=="movement"){
            if(has_movement)return fail("duplicate movement");
            has_movement=true;
            if(value=="normal-gred-gfmm9")next.movement=BorgBattleMovement::NormalGRed;
            else if(value=="stationary-v1")next.movement=BorgBattleMovement::Stationary;
            else{
                std::array<std::string_view,4> a;
                if(!split_fields(value,&a)||a[0]!="grounded-v1"||!scalar(a[1],.4f,16,&next.walk)||
                   !scalar(a[2],2,24,&next.jump)||!scalar(a[3],next.walk,32,&next.dash))return fail("unsupported movement or walk/jump/dash values");
            }
        }else if(key=="takeoff-clip"||key=="landing-clip"){
            auto& slot=key=="takeoff-clip"?next.takeoff_clip:next.landing_clip;
            BorgBattleClip bound;
            if(slot||!clip(value,&bound))return fail("invalid or duplicate jump transition clip");
            slot=bound;
        }else if(key=="dash-back"||key=="dash-left"||key=="dash-right"){
            auto& slot=next.directional_dash[key=="dash-back"?0:key=="dash-left"?1:2];
            BorgBattleClip bound;
            if(slot||!clip(value,&bound))return fail("invalid or duplicate directional dash clip");
            slot=bound;
        }else if(key=="air-dash"){
            if(next.air_dash_disabled||next.air_dash_clip)return fail("duplicate air dash policy");
            if(value=="none")next.air_dash_disabled=true;
            else{
                std::array<std::string_view,2> a;BorgBattleClip bound;
                if(!split_fields(value,&a)||!clip(a[0],&bound)||!number(a[1],0,600,&next.air_dash_start))
                    return fail("invalid air dash clip or authored start");
                next.air_dash_clip=bound;
            }
        }else if(key=="idle"||key=="walk"||key=="air"||key=="dash"){
            const unsigned at=key=="idle"?0:key=="walk"?1:key=="air"?2:3;
            if((locomotion&(1u<<at))||!clip(value,&next.locomotion[at]))return fail("invalid or duplicate locomotion clip");
            locomotion|=1u<<at;
        }else if(key=="dash-policy"){
            if(next.dash_disabled||value!="disabled")return fail("invalid or duplicate dash policy");
            next.dash_disabled=true;
        }else if(key=="cruise-boost"){
            BorgCruiseBoost c;
            if(next.cruise||!borg_read_cruise_boost(value,&c))return fail("invalid or duplicate cruise-boost");
            next.cruise=c;
        }else if(key=="cruise-entry"){
            BorgBattleClip entry;
            if(next.cruise_entry||!clip(value,&entry))return fail("invalid or duplicate cruise-entry clip");
            next.cruise_entry=entry;
        }else if(key=="stock-clock"){
            std::array<std::string_view,2> a;
            if(add_on_debit_stocks.size()>=kBorgBattleStocks||!split_fields(value,&a)||!id(a[0])||a[1]!="add-on-debit")
                return fail("invalid stock-clock binding");
            add_on_debit_stocks.emplace_back(line,std::string(a[0]));
        }else if(key=="stock"){
            std::array<std::string_view,3> a;BorgBattleStockDefinition s;
            if(!split_fields(value,&a)||!id(a[0])||index(next.stocks,a[0])!=next.stocks.size()||
               next.stocks.size()>=kBorgBattleStocks||!number(a[1],1,kBorgStockCapacity,&s.capacity)||
               (a[2]!="on-return"&&!number(a[2],1,3600,&s.refill_ticks)))
                return fail("invalid or duplicate stock");
            s.id=a[0];next.stocks.push_back(std::move(s));
        }else if(key=="projectile"){
            std::array<std::string_view,11> a;BorgBattleProjectileDefinition p;u16 count=0;
            if(!split_fields(value,&a)||!id(a[0])||index(next.projectiles,a[0])!=next.projectiles.size()||
               next.projectiles.size()>=kBorgBattleAttacks||(a[1]!="bullet"&&a[1]!="beam"&&a[1]!="shuriken"&&a[1]!="orb")||
               !number(a[2],1,kBorgBattleVolley,&count)||
               !scalar(a[3],1,140,&p.speed)||!scalar(a[4],-100,100,&p.rise)||!scalar(a[5],0,8,&p.gravity)||
               !scalar(a[6],0,.8f,&p.spread)||!scalar(a[7],1,60,&p.radius)||!number(a[8],1,600,&p.lifetime)||
               !number(a[9],0,10000,&p.damage)||(a[10]!="ignore-floor"&&a[10]!="stop-on-floor"&&a[10]!="land-on-floor"&&
                                                a[10]!="stop-on-stage"&&a[10]!="clip-on-stage"))
                return fail("invalid or duplicate projectile");
            p.id=a[0];p.visual=a[1]=="beam"?BorgAttackVisual::Beam:a[1]=="shuriken"?BorgAttackVisual::Shuriken:
                a[1]=="orb"?BorgAttackVisual::Orb:BorgAttackVisual::Bullet;
            p.count=u8(count);p.floor=a[10]=="land-on-floor"?BorgProjectileFloor::Land:
                a[10]=="stop-on-floor"?BorgProjectileFloor::Retire:
                 a[10]=="stop-on-stage"?BorgProjectileFloor::StageRetire:
                 a[10]=="clip-on-stage"?BorgProjectileFloor::StageClip:BorgProjectileFloor::Ignore;
            next.projectiles.push_back(std::move(p));
        }else if(key=="blast"){
            std::array<std::string_view,5> a;BorgBattleBlast spec;
            if(blasts.size()>=kBorgBattleAttacks||!split_fields(value,&a)||!id(a[0])||
               (a[1]!="actor"&&a[1]!="floor"&&a[1]!="stage"&&a[1]!="impact"&&a[1]!="expiry"&&a[1]!="remote")||
               !number(a[2],1,1200,&spec.radius)||!number(a[3],0,10000,&spec.damage)||
               (a[4]!="opponent"&&a[4]!="all"))return fail("invalid blast trigger, radius, damage or recipients");
            spec.trigger=a[1]=="actor"?BorgBlastTrigger::Actor:a[1]=="floor"?BorgBlastTrigger::Floor:
                a[1]=="impact"?BorgBlastTrigger::Impact:a[1]=="expiry"?BorgBlastTrigger::Expiry:
                a[1]=="stage"?BorgBlastTrigger::Stage:BorgBlastTrigger::Remote;
            spec.include_owner=a[4]=="all";blasts.push_back({line,std::string(a[0]),spec});
        }else if(key=="field"){
            std::array<std::string_view,13> a;BorgField spec;
            if(fields.size()>=kBorgBattleMoves||!split_fields(value,&a)||!id(a[0])||
               !number(a[1],1,599,&spec.frame)||(a[2]!="sphere"&&a[2]!="box")||
               !number(a[6],1,600,&spec.ticks)||!scalar(a[7],-500,500,&spec.forward)||!scalar(a[8],-300,500,&spec.height))
                return fail("invalid field binding");
            spec.shape=a[2]=="sphere"?BorgFieldShape::Sphere:BorgFieldShape::Box;
            for(unsigned i=0;i<3;++i)if(!number(a[3+i],8,600,&spec.extent[i]))return fail("invalid field extent");
            for(unsigned i=0;i<4;++i){u16 n=0;if(!number(a[9+i],i==3?1:0,255,&n))return fail("invalid field colour/opacity");spec.tint[i]=u8(n);}
            fields.push_back({line,std::string(a[0]),std::move(spec)});
        }else if(key=="walk-pulse"){
            std::array<std::string_view,6> a;BorgWalkPulse spec;
            if(next.walk_pulse||!split_fields(value,&a)||!number(a[0],16,800,&spec.stride)||
               !number(a[1],4,299,&spec.start)||!number(a[2],u16(spec.start+1),300,&spec.end)||
               !number(a[3],1,150,&spec.height)||!number(a[4],1,60,&spec.ticks)||!number(a[5],1,10000,&spec.damage))
                return fail("invalid or duplicate walk-pulse");
            next.walk_pulse=spec;
        }else if(key=="field-slot"){
            std::array<std::string_view,3> a;
            if(field_slots.size()>=kBorgBattleMoves||!split_fields(value,&a)||!id(a[0])||!id(a[1])||
               (a[2]!="on-place"&&a[2]!="on-ready"))return fail("invalid field-slot");
            field_slots.push_back({line,std::string(a[0]),{std::string(a[1]),a[2]=="on-ready"}});
        }else if(key=="field-drift"){
            std::array<std::string_view,4> a;BorgFieldDrift spec;
            if(field_drifts.size()>=kBorgBattleMoves||!split_fields(value,&a)||!id(a[0])||
               !scalar(a[1],.25f,100.f,&spec.speed)||!scalar(a[2],.25f,spec.speed,&spec.deceleration)||
               (a[3]!="active"&&a[3]!="after-stop"))return fail("invalid field-drift");
            spec.active_while_moving=a[3]=="active";
            field_drifts.push_back({line,std::string(a[0]),spec});
        }else if(key=="field-actor-polarity"){
            std::array<std::string_view,2> a;
            if(field_repulsions.size()>=kBorgBattleMoves||!split_fields(value,&a)||!id(a[0])||a[1]!="away")
                return fail("invalid field-actor-polarity");
            field_repulsions.emplace_back(line,std::string(a[0]));
        }else if(key=="field-actor-pull-point"){
            std::array<std::string_view,4> a;BorgFieldPullPoint spec;
            if(field_pull_points.size()>=kBorgBattleMoves||!split_fields(value,&a)||!id(a[0])||
               !scalar(a[1],-2000,2000,&spec.forward)||!scalar(a[2],-600,600,&spec.height)||
               !number(a[3],0,600,&spec.minimum))return fail("invalid field-actor-pull-point");
            field_pull_points.push_back({line,std::string(a[0]),spec});
        }else if(key=="field-actor-pull"){
            std::array<std::string_view,4> a;BorgFieldActorPull spec;
            if(field_actor_pulls.size()>=kBorgBattleMoves||!split_fields(value,&a)||!id(a[0])||
               !field_scope(a[1],&spec.scope)||!scalar(a[2],.125f,16.f,&spec.step)||
               !scalar(a[3],0,1.5707964f,&spec.half_angle))return fail("invalid field-actor-pull");
            field_actor_pulls.push_back({line,std::string(a[0]),spec});
        }else if(key=="field-shot-pull"){
            std::array<std::string_view,3> a;BorgFieldShotPull spec;
            if(field_shot_pulls.size()>=kBorgBattleMoves||!split_fields(value,&a)||!id(a[0])||
               !field_scope(a[1],&spec.scope)||!scalar(a[2],.125f,16.f,&spec.acceleration))return fail("invalid field-shot-pull");
            field_shot_pulls.push_back({line,std::string(a[0]),spec});
        }else if(key=="field-core-damage"){
            std::array<std::string_view,5> a;BorgFieldCoreDamage spec;
            if(field_core_damage.size()>=kBorgBattleMoves||!split_fields(value,&a)||!id(a[0])||
               !field_scope(a[1],&spec.scope)||!number(a[2],1,600,&spec.radius)||
               !number(a[3],1,10000,&spec.damage)||!number(a[4],1,599,&spec.interval))return fail("invalid field-core-damage");
            field_core_damage.push_back({line,std::string(a[0]),spec});
        }else if(key=="field-debit"){
            std::array<std::string_view,3> a;BorgFieldDebit spec;
            if(field_debit.size()>=kBorgBattleMoves||!split_fields(value,&a)||!id(a[0])||!id(a[1])||
               !number(a[2],1,255,&spec.cost))return fail("invalid field debit");
            spec.stock=std::string(a[1]);
            field_debit.push_back({line,std::string(a[0]),spec});
        }else if(key=="move-charge"){
            std::array<std::string_view,2> a;BorgMoveCharge spec;
            if(move_charge.size()>=kBorgBattleMoves||!split_fields(value,&a)||!id(a[0])||
               !number(a[1],2,60,&spec.ticks))return fail("invalid move charge");
            move_charge.push_back({line,std::string(a[0]),spec});
        }else if(key=="reaction-mode"){
            std::array<std::string_view,2> a;u16 mode=0;
            if(reaction_mode.size()>=kBorgBattleMoves||!split_fields(value,&a)||!id(a[0])||
               !number(a[1],0,kBorgReactionBodyRoot,&mode))
                return fail("unsupported or invalid reaction mode");
            reaction_mode.push_back({line,std::string(a[0]),u8(mode)});
        }else if(key=="reaction-root"){
            std::array<std::string_view,2> a;
            if(reaction_roots.size()>=kBorgBattleMoves||!split_fields(value,&a)||!id(a[0])||a[1]!="linked-owner")
                return fail("invalid reaction-root binding");
            reaction_roots.emplace_back(line,std::string(a[0]));
        }else if(key=="reaction-offset"){
            std::array<std::string_view,3> a;int yaw=0,pitch=0;
            const auto integer=[](std::string_view s,int* out){
                if(s.empty()||s.size()>4)return false;
                int n=0;const auto r=std::from_chars(s.data(),s.data()+s.size(),n);
                if(r.ec!=std::errc{}||r.ptr!=s.data()+s.size()||n<-128||n>127)return false;
                *out=n;return true;
            };
            if(reaction_offsets.size()>=kBorgBattleMoves||!split_fields(value,&a)||!id(a[0])||
               !integer(a[1],&yaw)||!integer(a[2],&pitch))
                return fail("invalid reaction offset");
            reaction_offsets.push_back({line,std::string(a[0]),s8(yaw),s8(pitch)});
        }else if(key=="free-release"){
            if(!value.empty()&&(!id(value)||value.find('|')!=value.npos))return fail("invalid free release");
            free_release.push_back({line,std::string(value)});
        }else if(key=="field-effect"){
            constexpr std::array<std::string_view,5> kinds{"actor-speed","shot-speed","shot-damage","shot-size","break-guidance"};
            std::array<std::string_view,4> a;u16 amount=0;BorgFieldScope scope;
            if(field_effects.size()>=kBorgBattleMoves*kBorgFieldEffects||!split_fields(value,&a)||!id(a[0])||
               !number(a[3],0,300,&amount))return fail("invalid field-effect binding");
            const auto kind=std::find(kinds.begin(),kinds.end(),a[1]);
            if(kind==kinds.end()||!field_scope(a[2],&scope))return fail("unknown field effect or scope");
            field_effects.push_back({line,std::string(a[0]),{BorgFieldEffectKind(kind-kinds.begin()),scope,amount}});
        }else if(key=="field-socket"){
            std::array<std::string_view,5> a;BorgBattleSocket spec;u16 bone=0;
            if(field_sockets.size()>=kBorgBattleMoves||!split_fields(value,&a)||!id(a[0])||!number(a[1],0,31,&bone))
                return fail("invalid field-socket binding");
            for(unsigned k=0;k<3;++k)if(!scalar(a[k+2],-500,500,&spec.local[k]))return fail("invalid field-socket offset");
            spec.bone=u8(bone);field_sockets.push_back({line,std::string(a[0]),spec});
        }else if(key=="field-follow"){
            std::array<std::string_view,2> a;
            if(field_follows.size()>=kBorgBattleMoves||!split_fields(value,&a)||!id(a[0])||a[1]!="source")
                return fail("invalid field-follow binding");
            field_follows.emplace_back(line,std::string(a[0]));
        }else if(key=="field-texture"){
            std::array<std::string_view,2> a;
            if(field_textures.size()>=kBorgBattleMoves||!split_fields(value,&a)||!id(a[0])||
               !borg_package_asset_path_valid(a[1]))return fail("invalid field-texture binding");
            field_textures.push_back({line,std::string(a[0]),std::string(a[1])});
        }else if(key=="slow-field"){
            std::array<std::string_view,9> a;BorgSlowField spec;
            if(slow_fields.size()>=kBorgBattleMoves||!split_fields(value,&a)||!id(a[0])||
               !number(a[1],1,599,&spec.frame)||!number(a[2],8,600,&spec.radius)||!number(a[3],1,600,&spec.ticks)||
               !scalar(a[4],-500,500,&spec.forward)||!scalar(a[5],-300,500,&spec.height)||
               !number(a[6],25,100,&spec.actor_percent)||!number(a[7],25,100,&spec.shot_percent)||
               (a[8]!="keep-guidance"&&a[8]!="break-guidance"))return fail("invalid slow-field binding");
            spec.break_guidance=a[8]=="break-guidance";
            slow_fields.push_back({line,std::string(a[0]),spec});
        }else if(key=="power-field"){
            std::array<std::string_view,8> a;BorgPowerField spec;
            if(power_fields.size()>=kBorgBattleMoves||!split_fields(value,&a)||!id(a[0])||
               !number(a[1],1,599,&spec.frame)||!number(a[2],8,600,&spec.radius)||!number(a[3],1,600,&spec.ticks)||
               !scalar(a[4],-500,500,&spec.forward)||!scalar(a[5],-300,500,&spec.height)||
               !number(a[6],100,300,&spec.damage_percent)||!number(a[7],100,200,&spec.visual_percent))
                return fail("invalid power-field binding");
            power_fields.push_back({line,std::string(a[0]),spec});
        }else if(key=="blast-accuracy"){
            std::array<std::string_view,3> a;BorgBlastAccuracy spec;
            if(accuracy.size()>=kBorgBattleAttacks||!split_fields(value,&a)||!id(a[0])||
               !number(a[1],1,300,&spec.ticks)||!scalar(a[2],.01f,.45f,&spec.radians))
                return fail("invalid blast-accuracy binding");
            accuracy.push_back({line,std::string(a[0]),spec});
        }else if(key=="accuracy-sensitive"){
            if(accuracy_sensitive.size()>=kBorgBattleAttacks||!id(value))return fail("invalid accuracy-sensitive binding");
            accuracy_sensitive.emplace_back(line,std::string(value));
        }else if(key=="blast-ink"){
            std::array<std::string_view,4> a;BorgInkOverlay spec;u16 coverage=0,opacity=0;
            if(ink.size()>=kBorgBattleAttacks||!split_fields(value,&a)||!id(a[0])||
               !number(a[1],1,300,&spec.ticks)||!number(a[2],10,80,&coverage)||!number(a[3],1,255,&opacity))
                return fail("invalid blast-ink binding");
            spec.coverage=u8(coverage);spec.opacity=u8(opacity);ink.push_back({line,std::string(a[0]),spec});
        }else if(key=="blast-smoke"){
            std::array<std::string_view,4> a;BorgSmoke spec;
            if(smoke.size()>=kBorgBattleAttacks||!split_fields(value,&a)||!id(a[0])||
               !number(a[1],8,600,&spec.radius)||!number(a[2],1,600,&spec.ticks)||
               (a[3]!="visual"&&a[3]!="block-aim"))return fail("invalid blast-smoke binding");
            spec.block_aim=a[3]=="block-aim";smoke.push_back({line,std::string(a[0]),spec});
        }else if(key=="impact-pattern-timing"){
            std::array<std::string_view,4> a;ImpactTimingRef r{line,{}};
            if(impact_timings.size()>=kBorgBattleAttacks||!split_fields(value,&a)||!id(a[0])||
               !number(a[1],0,120,&r.trail)||!number(a[2],0,120,&r.hazard)||!number(a[3],0,60,&r.query)||
               !(r.trail||r.hazard||r.query))return fail("invalid impact-pattern-timing binding");
            r.projectile=a[0];impact_timings.push_back(std::move(r));
        }else if(key=="impact-pattern-space"){
            std::array<std::string_view,2> a;
            if(impact_world_space.size()>=kBorgBattleAttacks||!split_fields(value,&a)||!id(a[0])||a[1]!="world")
                return fail("invalid impact-pattern-space binding");
            impact_world_space.emplace_back(line,std::string(a[0]));
        }else if(key=="impact-trail"){
            std::array<std::string_view,10> a;BorgImpactTrail spec;
            if(impact_trails.size()>=kBorgBattleAttacks||!split_fields(value,&a)||!id(a[0])||!impact_pattern(a,spec)||
               !number(a[8],1,180,&spec.mark_size)||!number(a[9],1,30,&spec.mark_ticks))
                return fail("invalid impact-trail binding");
            impact_trails.push_back({line,std::string(a[0]),spec});
        }else if(key=="impact-hazard"){
            std::array<std::string_view,12> a;BorgImpactHazard spec;
            if(impact_hazards.size()>=kBorgBattleAttacks||!split_fields(value,&a)||!id(a[0])||!impact_pattern(a,spec)||
               !number(a[8],1,180,&spec.hit_radius)||!number(a[9],0,600,&spec.height)||
               !number(a[10],1,60,&spec.ticks)||!number(a[11],1,10000,&spec.damage))
                return fail("invalid impact-hazard binding");
            impact_hazards.push_back({line,std::string(a[0]),spec});
        }else if(key=="impact-area"){
            std::array<std::string_view,7> a;BorgImpactArea spec;
            if(impact_areas.size()>=kBorgBattleAttacks||!split_fields(value,&a)||!id(a[0])||
               (a[1]!="actor"&&a[1]!="floor"&&a[1]!="stage")||
               !scalar(a[2],0,1200,&spec.initial_radius)||!scalar(a[3],.01f,100,&spec.gain)||
               !scalar(a[4],.01f,1200,&spec.cap)||spec.cap<=spec.initial_radius||
               !number(a[5],1,60,&spec.queries)||!number(a[6],1,10000,&spec.damage))
                return fail("invalid impact-area binding");
            spec.lane=a[1]=="actor"?BorgImpactLane::Actor:a[1]=="floor"?BorgImpactLane::Floor:BorgImpactLane::Stage;
            impact_areas.push_back({line,std::string(a[0]),spec});
        }else if(key=="impact-children"){
            std::array<std::string_view,4> a;BorgImpactChildren spec;
            if(impact_children.size()>=kBorgBattleAttacks||!split_fields(value,&a)||!id(a[0])||!id(a[1])||
               (a[2]!="actor"&&a[2]!="floor"&&a[2]!="stage")||(a[3]!="forward"&&a[3]!="radial"))
                return fail("invalid impact-children binding");
            spec.lane=a[2]=="actor"?BorgImpactLane::Actor:a[2]=="floor"?BorgImpactLane::Floor:BorgImpactLane::Stage;
            spec.radial=a[3]=="radial";impact_children.push_back({line,std::string(a[0]),std::string(a[1]),spec});
        }else if(key=="flight-children"){
            std::array<std::string_view,5> a;BorgFlightChildren spec;u16 count=0;
            if(flight_children.size()>=kBorgBattleAttacks||!split_fields(value,&a)||!id(a[0])||!id(a[1])||
               !number(a[2],1,598,&spec.first)||!number(a[3],1,598,&spec.interval)||!number(a[4],1,8,&count))
                return fail("invalid flight-children binding");
            spec.count=u8(count);flight_children.push_back({line,std::string(a[0]),std::string(a[1]),spec});
        }else if(key=="blast-children"){
            std::array<std::string_view,2> a;
            if(blast_children.size()>=kBorgBattleAttacks||!split_fields(value,&a)||!id(a[0])||!id(a[1]))
                return fail("invalid blast-children binding");
            blast_children.push_back({line,std::string(a[0]),std::string(a[1])});
        }else if(key=="bomb-fuse-tail"){
            std::array<std::string_view,2> a;u16 ticks=0;
            if(fuse_tails.size()>=kBorgBattleAttacks||!split_fields(value,&a)||!id(a[0])||!number(a[1],1,120,&ticks))
                return fail("invalid bomb-fuse-tail binding");
            fuse_tails.push_back({line,std::string(a[0]),ticks});
        }else if(key=="bomb-attraction"){
            std::array<std::string_view,4> a;BorgBombAttraction spec;
            if(attractions.size()>=kBorgBattleAttacks||!split_fields(value,&a)||!id(a[0])||
               !number(a[1],1,1800,&spec.radius)||!scalar(a[2],.125f,8,&spec.acceleration)||!scalar(a[3],spec.acceleration,32,&spec.cap))
                return fail("invalid bomb-attraction binding");
            attractions.push_back({line,std::string(a[0]),spec});
        }else if(key=="bomb-proximity"){
            std::array<std::string_view,4> a;BorgBombProximity spec;
            if(proximities.size()>=kBorgBattleAttacks||!split_fields(value,&a)||!id(a[0])||
               !number(a[1],1,600,&spec.radius)||!number(a[2],0,599,&spec.arming)||
               (a[3]!="blast"&&a[3]!="retire"))return fail("invalid bomb-proximity binding");
            spec.blast=a[3]=="blast";proximities.push_back({line,std::string(a[0]),spec});
        }else if(key=="beam-sweep"){
            std::array<std::string_view,4> a;BorgBeamSweep spec;
            if(beam_sweeps.size()>=kBorgBattleAttacks||!split_fields(value,&a)||!id(a[0])||
               !number(a[1],1,2400,&spec.travel)||!number(a[2],1,2399,&spec.drop)||!number(a[3],1,180,&spec.impact_radius))
                return fail("invalid beam-sweep binding");
            beam_sweeps.push_back({line,std::string(a[0]),spec});
        }else if(key=="sustained-beam"){
            std::array<std::string_view,4> a;BorgSustainedBeam spec;
            if(sustained.size()>=kBorgBattleAttacks||!split_fields(value,&a)||!id(a[0])||
               !number(a[1],40,2400,&spec.length)||!number(a[2],1,600,&spec.interval)||!scalar(a[3],0,.25f,&spec.turn))
                return fail("invalid sustained-beam binding");
            sustained.push_back({line,std::string(a[0]),spec});
        }else if(key=="guidance"){
            std::array<std::string_view,4> a;BorgBattleGuidance spec;
            if(guidance.size()>=kBorgBattleAttacks||!split_fields(value,&a)||!id(a[0])||
               (a[1]!="snapshot"&&a[1]!="tracking")||!scalar(a[2],.001f,1.f,&spec.turn)||
               !number(a[3],1,600,&spec.ticks))return fail("invalid guidance target, turn limit or window");
            spec.target=a[1]=="snapshot"?BorgGuidanceTarget::Snapshot:BorgGuidanceTarget::Tracking;
            guidance.push_back({line,std::string(a[0]),spec});
        }else if(key=="return-stock"){
            std::array<std::string_view,2> a;
            if(returns.size()>=kBorgBattleAttacks||!split_fields(value,&a)||!id(a[0])||
               (a[1]!="actor"&&a[1]!="floor"&&a[1]!="stage"&&a[1]!="expiry"&&a[1]!="remote"&&a[1]!="expiry-or-proximity"))
                return fail("invalid stock-return disposition");
            returns.push_back({line,std::string(a[0]),a[1]=="actor"?BorgProjectileReturn::Actor:
                 a[1]=="floor"?BorgProjectileReturn::Floor:a[1]=="expiry"?BorgProjectileReturn::Expiry:
                 a[1]=="expiry-or-proximity"?BorgProjectileReturn::ExpiryOrProximity:
                a[1]=="stage"?BorgProjectileReturn::Stage:BorgProjectileReturn::Remote});
        }else if(key=="projectile-speed-ramp"){
            std::array<std::string_view,3> a;BorgProjectileSpeedRamp ramp;
            if(speed_ramps.size()>=kBorgBattleAttacks||!split_fields(value,&a)||!id(a[0])||
               !scalar(a[1],.01f,20.f,&ramp.gain)||!scalar(a[2],1.f,140.f,&ramp.cap))
                return fail("invalid projectile-speed-ramp");
            speed_ramps.push_back({line,std::string(a[0]),ramp});
        }else if(key=="actor-contact"){
            std::array<std::string_view,2> a;
            if(passes.size()>=kBorgBattleAttacks||!split_fields(value,&a)||!id(a[0])||
               (a[1]!="ignore"&&a[1]!="continue-once-per-actor"))
                return fail("unsupported actor-contact policy");
            passes.push_back({line,std::string(a[0]),a[1]=="continue-once-per-actor"});
        }else if(key=="action-sound"){
            std::array<std::string_view,2> a;
            if(!split_fields(value,&a)||!id(a[0])||!borg_package_asset_path_valid(a[1]))
                return fail("invalid action-sound move or path");
            action_sounds.push_back({line,{std::string(a[0]),std::string(a[1])}});
        }else if(key=="contact-sound-band"){
            u16 band=0;
            if(next.contact_sound_band||!number(value,0,kBorgContactSoundBands-1,&band))
                return fail("invalid or duplicate contact sound band");
            next.contact_sound_band=u8(band);
        }else if(key=="contact-sound"){
            std::array<std::string_view,5> a;u16 band=0,lane=0;
            if(contact_sounds.size()>=kBorgBattleMoves*kBorgContactSoundBands||!split_fields(value,&a)||
               !id(a[0])||!number(a[1],0,kBorgContactSoundBands-1,&band)||
               !number(a[2],0,kBorgContactSoundLanes-1,&lane)||
               !borg_package_asset_path_valid(a[3])||!borg_package_asset_path_valid(a[4]))
                return fail("invalid contact sound move, band, lane or path");
            contact_sounds.push_back({line,std::string(a[0]),u8(band),
                                      {u8(lane),{std::string(a[3]),std::string(a[4])}}});
        }else if(key=="movement-sound"){
            std::array<std::string_view,2> a;
            if(!split_fields(value,&a)||!borg_package_asset_path_valid(a[1]))return fail("invalid movement-sound path");
            const auto cue=std::find(kBorgMovementCueNames.begin(),kBorgMovementCueNames.end(),a[0]);
            if(cue==kBorgMovementCueNames.end())return fail("unknown movement sound trigger");
            auto& sound=next.movement_sound[cue-kBorgMovementCueNames.begin()];
            if(!sound.empty())return fail("duplicate movement sound binding");
            sound=a[1];
        }else if(key=="movement-effect"){
            std::array<std::string_view,9> a;BorgMovementFx spec;u16 bone=0,life=0;
            if(next.movement_fx.size()>=kBorgBattleMovementFx||!split_fields(value,&a)||!id(a[0])||
               index(next.movement_fx,a[0])!=next.movement_fx.size()||
               (a[1]!="takeoff"&&a[1]!="air-jump"&&a[1]!="landing"&&a[1]!="rise")||
               (a[2]!="jet"&&a[2]!="puff")||!number(a[3],0,31,&bone)||
               !scalar(a[4],-500,500,&spec.local[0])||!scalar(a[5],-500,500,&spec.local[1])||
               !scalar(a[6],-500,500,&spec.local[2])||!number(a[7],1,180,&spec.size)||!number(a[8],0,60,&life))
                return fail("invalid movement-effect binding");
            spec.id=a[0];spec.event=a[1]=="takeoff"?BorgMovementCue::Takeoff:a[1]=="air-jump"?BorgMovementCue::AirJump:
                a[1]=="landing"?BorgMovementCue::Landing:BorgMovementCue::Rise;
            spec.style=a[2]=="jet"?BorgMovementLook::Jet:BorgMovementLook::Puff;spec.bone=u8(bone);spec.duration=u8(life);
            next.movement_fx.push_back(std::move(spec));
        }else if(key=="release-sound"){
            std::array<std::string_view,3> a;u16 frame=0;
            if(release_sounds.size()>=kBorgReleaseSounds||!split_fields(value,&a)||!id(a[0])||
               !number(a[1],1,600,&frame)||!borg_package_asset_path_valid(a[2]))return fail("invalid release sound binding");
            release_sounds.push_back({line,std::string(a[0]),std::string(a[2]),frame});
        }else if(key=="launch-sound"){
            std::array<std::string_view,2> a;
            if(launch_sounds.size()>=kBorgBattleAttacks||!split_fields(value,&a)||!id(a[0])||!borg_package_asset_path_valid(a[1]))
                return fail("invalid launch-sound projectile or path");
            launch_sounds.push_back({line,std::string(a[0]),std::string(a[1])});
        }else if(key=="origin"){
            std::array<std::string_view,6> a;BorgBattleSocket spec;u16 bone=0;
            if(sockets.size()>=kBorgBattleAttacks||!split_fields(value,&a)||!id(a[0])||!number(a[1],0,31,&bone)||
               !scalar(a[2],-500,500,&spec.local[0])||!scalar(a[3],-500,500,&spec.local[1])||!scalar(a[4],-500,500,&spec.local[2])||
               (a[5]!="fixed"&&a[5]!="target-height"))
                return fail("invalid launch bone or local offset");
            spec.bone=u8(bone);spec.aim_height=a[5]=="target-height";sockets.push_back({line,std::string(a[0]),spec});
        }else if(key=="projectile-animation"){
            std::array<std::string_view,4> a;BorgProjectileAnimation spec;u16 pulse=0;
            if(animations.size()>=kBorgBattleAttacks||!split_fields(value,&a)||!id(a[0])||
               !scalar(a[1],-2,2,&spec.spin)||!number(a[2],0,240,&spec.pulse_period)||!number(a[3],0,75,&pulse)||
               (bool(spec.pulse_period)!=bool(pulse)))return fail("invalid projectile animation");
            spec.pulse_percent=u8(pulse);animations.push_back({line,std::string(a[0]),spec});
        }else if(key=="part-motion"){
            std::array<std::string_view,5> a;BorgBattlePartMotion spec;u16 bone=0;
            if(part_motions.size()>=kBorgBattleMoves*kBorgBattlePartAnimations||!split_fields(value,&a)||!id(a[0])||
               !number(a[1],1,31,&bone)||!clip(a[2],&spec.clip)||!number(a[3],0,600,&spec.first)||
               !number(a[4],2,240,&spec.count))return fail("invalid part-motion binding");
            spec.bone=u8(bone);part_motions.push_back({line,std::string(a[0]),spec});
        }else if(key=="part-spin"){
            std::array<std::string_view,5> a;BorgBattlePartSpin spec;u16 bone=0;
            if(spins.size()>=kBorgBattleMoves*kBorgBattlePartAnimations||!split_fields(value,&a)||!id(a[0])||
               !number(a[1],1,31,&bone)||(a[2]!="x"&&a[2]!="y"&&a[2]!="z")||
               !number(a[3],2,240,&spec.period)||(a[4]!="forward"&&a[4]!="reverse"))
                return fail("invalid part-spin bone, axis or period");
            spec.bone=u8(bone);spec.axis=a[2]=="x"?0:a[2]=="y"?1:2;spec.reverse=a[4]=="reverse";
            spins.push_back({line,std::string(a[0]),spec});
        }else if(key=="part-swap-bone"){
            std::array<std::string_view,3> a;u16 bone=0;
            if(part_bones.size()>=kBorgBattleMoves*kBorgBattlePartSlots||!split_fields(value,&a)||
               !id(a[0])||!id(a[1])||!number(a[2],0,31,&bone))return fail("invalid part-swap-bone binding");
            part_bones.push_back({line,std::string(a[0]),std::string(a[1]),u8(bone)});
        }else if(key=="part-hierarchy"){
            u16 record=0;
            if(next.static_part_hierarchies.size()>=kBorgBattlePartRecords||!number(value,1,255,&record)||
               std::find(next.static_part_hierarchies.begin(),next.static_part_hierarchies.end(),record)!=next.static_part_hierarchies.end())
                return fail("invalid or duplicate static part hierarchy");
            next.static_part_hierarchies.push_back(record);
        }else if(key=="part-mount-spin"){
            std::array<std::string_view,4> a;BorgBattlePartSpin spin;
            if(mount_spins.size()>=kBorgBattlePartSlots||!split_fields(value,&a)||!id(a[0])||
               (a[1]!="x"&&a[1]!="y"&&a[1]!="z")||!number(a[2],2,240,&spin.period)||(a[3]!="forward"&&a[3]!="reverse"))
                return fail("invalid part mount spin");
            spin.axis=a[1]=="x"?0:a[1]=="y"?1:2;spin.reverse=a[3]=="reverse";
            mount_spins.push_back({line,std::string(a[0]),spin});
        }else if(key=="part-basis"){
            std::array<std::string_view,2> a;
            if(part_bases.size()>=kBorgBattlePartSlots||!split_fields(value,&a)||!id(a[0])||
               (a[1]!="world-zyx"&&a[1]!="bone-zyx"))
                return fail("invalid part-basis");
            part_bases.push_back({line,std::string(a[0]),a[1]=="world-zyx"?BorgPartBasis::WorldZyx:BorgPartBasis::BoneZyx});
        }else if(key=="part-turn"){
            std::array<std::string_view,4> a;std::array<u16,3> step{};
            if(part_turns.size()>=kBorgBattlePartSlots||!split_fields(value,&a)||!id(a[0]))return fail("invalid part-turn slot");
            for(unsigned axis=0;axis<3;++axis)
                if(!number(a[1+axis],0,8192,&step[axis]))return fail("invalid part-turn step");
            if(step==std::array<u16,3>{})return fail("part-turn requires a nonzero step");
            part_turns.push_back({line,std::string(a[0]),step});
        }else if(key=="part-pose"||key=="part-swap-pose"){
            std::array<std::string_view,7> a;
            std::array<std::string_view,8> b;
            const bool swap=key=="part-swap-pose";
            if(part_poses.size()>=(kBorgBattleMoves+1)*kBorgBattlePartSlots||
               !(swap?split_fields(value,&b):split_fields(value,&a)))return fail("invalid part-pose fields");
            const auto* fields=swap?b.data()+1:a.data();
            if(!id(fields[0])||(swap&&!id(b[0])))return fail("invalid part-pose target");
            BorgBattlePartPose pose;
            for(unsigned axis=0;axis<3;++axis)
                if(!scalar(fields[1+axis],-1000,1000,&pose.offset[axis])||!scalar(fields[4+axis],-180,180,&pose.degrees[axis]))
                    return fail("invalid part-pose offset or angle");
            part_poses.push_back({line,swap?std::string(b[0]):std::string{},std::string(fields[0]),pose});
        }else if(key=="part-material"){
            std::array<std::string_view,5> a;BorgBattlePartMaterial spec;
            if(next.part_materials.size()>=kBorgBattlePartRecords||!split_fields(value,&a)||
               !number(a[0],1,255,&spec.record)||!number(a[1],0,255,&spec.clip)||
               !number(a[2],0,600,&spec.first)||!number(a[3],spec.first,600,&spec.last)||
               (a[4]!="once"&&a[4]!="loop"&&a[4]!="sample"))return fail("invalid part-material binding");
            spec.playback=a[4]=="loop"?BorgPartMaterialPlayback::Loop:a[4]=="sample"?BorgPartMaterialPlayback::Sample:BorgPartMaterialPlayback::Once;
            next.part_materials.push_back(spec);
        }else if(key=="part-record"){
            std::array<std::string_view,3> a;BorgBattlePartSlot part;u16 bone=0;
            if(next.part_slots.size()>=kBorgBattlePartSlots||!split_fields(value,&a)||!id(a[0])||
               !number(a[1],0,31,&bone)||!number(a[2],0,255,&part.record))
                return fail("invalid part-record slot");
            part.id=a[0];part.bone=u8(bone);next.part_slots.push_back(std::move(part));
        }else if(key=="part-swap"){
            std::array<std::string_view,5> a;BorgBattlePartSwap spec;
            if(swaps.size()>=kBorgBattleMoves*kBorgBattlePartSlots||!split_fields(value,&a)||!id(a[0])||!id(a[1])||
               !number(a[2],0,600,&spec.first)||!number(a[3],spec.first,600,&spec.last)||!number(a[4],0,255,&spec.record))
                return fail("invalid part-swap window");
            swaps.push_back({line,std::string(a[0]),std::string(a[1]),spec});
        }else if(key=="part-scroll"){
            std::array<std::string_view,5> a;BorgBattlePartScroll spec;u16 bone=0;
            if(scrolls.size()>=kBorgBattleMoves*kBorgBattlePartAnimations||!split_fields(value,&a)||!id(a[0])||
               !number(a[1],0,31,&bone)||(a[2]!="u"&&a[2]!="v")||!number(a[3],2,240,&spec.period)||
               (a[4]!="forward"&&a[4]!="reverse"))return fail("invalid part-scroll binding");
            spec.bone=u8(bone);spec.axis=a[2]=="u"?0:1;spec.reverse=a[4]=="reverse";
            scrolls.push_back({line,std::string(a[0]),spec});
        }else if(key=="impact-sound"){
            std::array<std::string_view,3> a;
            if(impact_sounds.size()>=kBorgBattleAttacks||!split_fields(value,&a)||!id(a[0])||
               (a[1]!="actor"&&a[1]!="floor"&&a[1]!="stage")||!borg_package_asset_path_valid(a[2]))
                return fail("invalid impact sound");
            const auto lane=a[1]=="floor"?BorgImpactLane::Floor:a[1]=="stage"?BorgImpactLane::Stage:BorgImpactLane::Actor;
            impact_sounds.push_back({line,std::string(a[0]),{lane,std::string(a[2])}});
        }else if(key=="impact-record-bank"){
            if(!next.impact_record_bank.empty()||!borg_package_asset_path_valid(value))return fail("invalid or duplicate impact record archive");
            next.impact_record_bank=value;
        }else if(key=="contact-impact-receive"){
            if(next.contact_impact_receive||value!="normal")return fail("invalid or duplicate contact impact receiver");
            next.contact_impact_receive=true;
        }else if(key=="contact-impact"){
            std::array<std::string_view,5> a;u16 team=0;BorgContactImpactLayer spec;
            if(contact_impacts.size()>=kBorgBattleMoves*2*kBorgContactImpactLayers||!split_fields(value,&a)||!id(a[0])||
               !number(a[1],0,1,&team)||!number(a[2],1,255,&spec.record)||
               !number(a[3],1,400,&spec.scale_percent)||!number(a[4],1,120,&spec.ticks))
                return fail("invalid contact impact layer");
            contact_impacts.push_back({line,std::string(a[0]),u8(team),spec});
        }else if(key=="contact-pose-hold-receive"){
            if(next.contact_pose_hold_receive||value!="normal")return fail("invalid or duplicate contact pose hold receiver");
            next.contact_pose_hold_receive=true;
        }else if(key=="contact-pose-hold"){
            std::array<std::string_view,3> a;u16 ticks=0;
            if(contact_pose_holds.size()>=kBorgBattleMoves||!split_fields(value,&a)||!id(a[0])||
               !number(a[1],1,kBorgContactPoseHoldMax,&ticks)||(a[2]!="receiver"&&a[2]!="bodies"))
                return fail("invalid contact pose hold");
            contact_pose_holds.push_back({line,std::string(a[0]),{u8(ticks),a[2]=="bodies"}});
        }else if(key=="impact-record-growth"){
            std::array<std::string_view,2> a;u16 end=0;
            if(impact_record_growth.size()>=kBorgBattleAttacks||!split_fields(value,&a)||!id(a[0])||!number(a[1],1,400,&end))
                return fail("invalid impact record growth");
            impact_record_growth.push_back({line,std::string(a[0]),end});
        }else if(key=="impact-record"){
            std::array<std::string_view,5> a;BorgImpactRecord spec;
            if(impact_records.size()>=kBorgBattleAttacks||!split_fields(value,&a)||!id(a[0])||
               (a[1]!="actor"&&a[1]!="floor"&&a[1]!="stage")||!number(a[2],1,255,&spec.record)||
               !number(a[3],1,400,&spec.scale_percent)||!number(a[4],1,120,&spec.ticks))return fail("invalid impact record");
            spec.lane=a[1]=="floor"?BorgImpactLane::Floor:a[1]=="stage"?BorgImpactLane::Stage:BorgImpactLane::Actor;
            impact_records.push_back({line,std::string(a[0]),spec});
        }else if(key=="projectile-record-trail"){
            std::array<std::string_view,5> a;BorgProjectileRecordTrail spec;u16 count=0,interval=0;
            if(record_trails.size()>=kBorgBattleAttacks||!split_fields(value,&a)||!id(a[0])||
               !number(a[1],1,31,&spec.record)||!number(a[2],1,kBorgProjectileTrailSamples,&count)||
               !number(a[3],1,30,&interval)||!number(a[4],1,400,&spec.scale_percent))
                return fail("invalid projectile record trail");
            spec.count=u8(count);spec.interval=u8(interval);
            record_trails.push_back({line,std::string(a[0]),spec});
        }else if(key=="projectile-record"){
            std::array<std::string_view,3> a;BorgProjectileRecordLook spec;
            if(record_looks.size()>=kBorgBattleAttacks||!split_fields(value,&a)||!id(a[0])||
               !number(a[1],1,31,&spec.record)||!number(a[2],1,400,&spec.scale_percent))
                return fail("invalid projectile record");
            record_looks.push_back({line,std::string(a[0]),spec});
        }else if(key=="look"){
            std::array<std::string_view,5> a;BorgProjectileLook spec;u16 r=0,g=0,b=0;
            if(looks.size()>=kBorgBattleAttacks||!split_fields(value,&a)||!id(a[0])||!number(a[1],1,180,&spec.size)||
               !number(a[2],0,255,&r)||!number(a[3],0,255,&g)||!number(a[4],0,255,&b))
                return fail("invalid projectile visual size or color");
            spec.rgb={u8(r),u8(g),u8(b)};looks.push_back({line,std::string(a[0]),spec});
        }else if(key=="empty-anchors"){
            std::array<std::string_view,2> a;
            if(anchor_fallbacks.size()>=kBorgBattleAttacks||!split_fields(value,&a)||!id(a[0])||!id(a[1]))
                return fail("invalid empty-anchor fallback");
            anchor_fallbacks.push_back({line,std::string(a[0]),std::string(a[1])});
        }else if(key=="plant-anchor"){
            std::array<std::string_view,2> a;
            if(stage_anchors.size()>=kBorgBattleAttacks||!split_fields(value,&a)||!id(a[0])||!id(a[1]))
                return fail("invalid stage anchor binding");
            stage_anchors.push_back({line,std::string(a[0]),std::string(a[1])});
        }else if(key=="on-hit"){
            std::array<std::string_view,4> modifier;
            std::array<std::string_view,3> pull;
            u16 amount=0,ticks=0;
            if(hits.size()>=kBorgBattleAttacks*2)return fail("hit-effect budget");
            if(split_fields(value,&modifier)&&id(modifier[0])&&modifier[1]=="walk-speed"&&
               number(modifier[2],25,200,&amount)&&number(modifier[3],1,600,&ticks))
                hits.push_back({line,std::string(modifier[0]),false,amount,ticks});
            else if(split_fields(value,&pull)&&id(pull[0])&&pull[1]=="pull-self"&&number(pull[2],1,kBorgBattlePullUnits,&amount))
                hits.push_back({line,std::string(pull[0]),true,amount,0});
            else if(split_fields(value,&modifier)&&id(modifier[0])&&modifier[1]=="rewind-recipient"&&id(modifier[2])&&
                    number(modifier[3],1,16,&amount)&&rewind_hits.size()<kBorgBattleAttacks)
                rewind_hits.push_back({line,std::string(modifier[0]),std::string(modifier[2]),u8(amount)});
            else return fail("unsupported or invalid hit effect");
        }else if(key=="move"){
            std::array<std::string_view,6> a;BorgBattleMove m;
            if(!split_fields(value,&a)||!id(a[0])||index(next.moves,a[0])!=next.moves.size()||next.moves.size()>=kBorgBattleMoves||
               (a[1]!="b"&&a[1]!="x")||!number(a[2],0,1024,&m.priority)||
               (a[3]!="any"&&a[3]!="grounded"&&a[3]!="airborne")||!number(a[4],2,600,&m.duration)||
               (a[5]!="none"&&!clip(a[5],&m.clip))||
               std::any_of(next.moves.begin(),next.moves.end(),[&](const auto& move){return move.priority==m.priority;}))
                return fail("invalid move or ambiguous priority");
            m.id=a[0];m.button=a[1]=="b"?BorgBattleButton::B:BorgBattleButton::X;
            m.static_pose=a[5]=="none";
            m.context=a[3]=="grounded"?BorgBattleContext::Grounded:a[3]=="airborne"?BorgBattleContext::Airborne:BorgBattleContext::Any;
            next.moves.push_back(std::move(m));
        }else if(key=="melee"){
            std::array<std::string_view,8> a;BorgBattleMelee shape;
            if(melee.size()>=kBorgBattleMoves||!split_fields(value,&a)||!id(a[0])||
               !number(a[1],1,599,&shape.first)||!number(a[2],shape.first,599,&shape.last)||
               !scalar(a[3],1,220,&shape.reach)||!scalar(a[4],1,40,&shape.radius)||
               !scalar(a[5],-200,200,&shape.height)||!scalar(a[6],1,100,&shape.half_height)||
               !number(a[7],1,10000,&shape.damage))return fail("invalid melee window or geometry");
            melee.push_back({line,std::string(a[0]),shape});
        }else if(key=="contact-power"){
            std::array<std::string_view,2> a;u16 base=0;
            if(contact_powers.size()>=kBorgBattleMoves||!split_fields(value,&a)||!id(a[0])||
               !number(a[1],1,10000,&base))return fail("invalid contact power");
            contact_powers.push_back({line,std::string(a[0]),base});
        }else if(key=="contact-minimum"){
            std::array<std::string_view,2> a;
            if(contact_minimum_hp.size()>=kBorgBattleMoves||!split_fields(value,&a)||!id(a[0])||a[1]!="1")
                return fail("contact-minimum needs a move and one HP");
            contact_minimum_hp.emplace_back(line,std::string(a[0]));
        }else if(key=="contact-tier"){
            std::array<std::string_view,3> a;u16 charge=0;
            if(contact_tiers.size()>=kBorgBattleMoves||!split_fields(value,&a)||!id(a[0])||
               !number(a[1],0,255,&charge)||(a[2]!="scale"&&a[2]!="bypass"))
                return fail("invalid contact tier");
            contact_tiers.push_back({line,std::string(a[0]),{u8(charge),a[2]=="scale"}});
        }else if(key=="contact-tier-reset"){
            std::array<std::string_view,2> a;u16 wear=0;
            if(contact_tier_reset_wear.size()>=kBorgBattleMoves||!split_fields(value,&a)||!id(a[0])||
               !number(a[1],0,255,&wear))return fail("invalid contact tier reset wear");
            contact_tier_reset_wear.push_back({line,std::string(a[0]),u8(wear)});
        }else if(key=="aim-lock"){
            std::array<std::string_view,2> a;u16 frame=0;
            if(aim_locks.size()>=kBorgBattleMoves||!split_fields(value,&a)||!id(a[0])||!number(a[1],0,599,&frame))
                return fail("invalid aim-lock marker");
            aim_locks.push_back({line,std::string(a[0]),frame});
        }else if(key=="pose-start"){
            std::array<std::string_view,2> a;u16 frame=0;
            if(starts.size()>=kBorgBattleMoves||!split_fields(value,&a)||!id(a[0])||!number(a[1],1,600,&frame))
                return fail("invalid pose start");
            starts.push_back({line,std::string(a[0]),frame});
        }else if(key=="pose-followup"){
            std::array<std::string_view,4> a;BorgBattlePoseFollowup spec;
            if(pose_followups.size()>=kBorgBattleMoves||!split_fields(value,&a)||!id(a[0])||
               !number(a[1],1,599,&spec.frame)||!clip(a[2],&spec.clip)||!number(a[3],0,600,&spec.start))
                return fail("invalid pose follow-up");
            pose_followups.push_back({line,std::string(a[0]),spec});
        }else if(key=="pose-loop-window"){
            std::array<std::string_view,2> a;
            if(pose_loop_windows.size()>=kBorgBattleMoves||!split_fields(value,&a)||!id(a[0])||a[1]!="authored-bit")
                return fail("invalid pose loop window");
            pose_loop_windows.emplace_back(line,std::string(a[0]));
        }else if(key=="anchor-return"){
            std::array<std::string_view,2> a;
            if(anchor_returns.size()>=kBorgBattleAnchorSets||!split_fields(value,&a)||!id(a[0])||a[1]!="on-reach")
                return fail("invalid anchor-return binding");
            anchor_returns.emplace_back(line,std::string(a[0]));
        }else if(key=="anchor-fence"){
            std::array<std::string_view,4> a;BorgAnchorFence spec;
            if(anchor_fences.size()>=kBorgBattleAnchorSets||!split_fields(value,&a)||!id(a[0])||
               !number(a[1],1,120,&spec.radius)||!number(a[2],1,10000,&spec.damage)||!number(a[3],1,600,&spec.ticks))
                return fail("invalid anchor-fence binding");
            anchor_fences.push_back({line,std::string(a[0]),spec});
        }else if(key=="fence-recoil"){
            std::array<std::string_view,2> a;u16 amount=0;
            if(fence_recoils.size()>=kBorgBattleAnchorSets||!split_fields(value,&a)||!id(a[0])||
               !number(a[1],1,40,&amount))return fail("invalid fence-recoil binding");
            fence_recoils.emplace_back(std::string(a[0]),u8(amount));
        }else if(key=="place-anchor-offset"){
            std::array<std::string_view,4> a;AnchorOffsetRef r{line,{},0,0,0};
            if(anchor_offsets.size()>=kBorgBattleMoves*kBorgBattleAnchors||!split_fields(value,&a)||!id(a[0])||
               !number(a[1],1,599,&r.frame)||!scalar(a[2],-400,400,&r.forward)||!scalar(a[3],-120,400,&r.height))
                return fail("invalid place-anchor-offset binding");
            r.move=a[0];anchor_offsets.push_back(std::move(r));
        }else if(key=="anchor-set"){
            std::array<std::string_view,2> a;u16 cap=0;
            if(next.anchor_sets.size()>=kBorgBattleAnchorSets||!split_fields(value,&a)||!id(a[0])||
               index(next.anchor_sets,a[0])!=next.anchor_sets.size()||!number(a[1],1,kBorgBattleAnchors,&cap))
                return fail("invalid or duplicate anchor set");
            next.anchor_sets.push_back({std::string(a[0]),u8(cap)});
        }else if(key=="place-anchor"||key=="rewind"){
            std::array<std::string_view,5> place;std::array<std::string_view,4> rewind;
            BorgBattleAnchorMarker marker;u16 amount=0;
            if(anchors.size()>=kBorgBattleMoves*kBorgBattleAnchors)return fail("anchor marker budget");
            if(key=="place-anchor"&&split_fields(value,&place)&&id(place[0])&&number(place[1],1,599,&marker.frame)&&id(place[2])&&
               (id(place[3])||place[3]=="none")&&number(place[4],0,99,&amount)&&((place[3]=="none")==(amount==0))){
                marker.cost=u8(amount);
                anchors.push_back({line,std::string(place[0]),std::string(place[2]),std::string(place[3]),marker});
            }else if(key=="rewind"&&split_fields(value,&rewind)&&id(rewind[0])&&number(rewind[1],1,599,&marker.frame)&&
                     id(rewind[2])&&number(rewind[3],1,16,&amount)){
                marker.rewind_speed=u8(amount);
                anchors.push_back({line,std::string(rewind[0]),std::string(rewind[2]),"",marker});
            }else return fail("invalid anchor marker");
        }else if(key=="emitter"){
            std::array<std::string_view,7> a;BorgBattleEmitter spec;
            if(emitters.size()>=kBorgBattleEmitters||!split_fields(value,&a)||!id(a[0])||!id(a[1])||!id(a[2])||
               !scalar(a[3],-120,120,&spec.side)||!scalar(a[4],0,240,&spec.height)||
               !scalar(a[5],0,180,&spec.behind)||!number(a[6],1,600,&spec.lifetime))
                return fail("invalid emitter identity, anchor or lifetime");
            spec.id=a[0];emitters.push_back({line,std::string(a[1]),std::string(a[2]),std::move(spec)});
        }else if(key=="deploy-emitter"){
            std::array<std::string_view,3> a;u16 frame=0;
            if(deployments.size()>=kBorgBattleMoves*kBorgBattleEmitters||!split_fields(value,&a)||
               !id(a[0])||!number(a[1],1,599,&frame)||!id(a[2]))return fail("invalid emitter deployment marker");
            deployments.push_back({line,std::string(a[0]),std::string(a[2]),frame});
        }else if(key=="guard"){
            std::array<std::string_view,6> a;BorgBattleGuard spec;u16 contacts=0;
            if(guards.size()>=kBorgBattleMoves||!split_fields(value,&a)||!id(a[0])||
               !number(a[1],1,599,&spec.first)||!number(a[2],spec.first,599,&spec.last)||
               !scalar(a[3],.01f,3.141593f,&spec.half_angle)||!number(a[4],1,16,&contacts)||
               (a[5]!="shots"&&a[5]!="melee"&&a[5]!="direct"&&a[5]!="all"))
                return fail("invalid guard window, facing, contact budget or mask");
            spec.contacts=u8(contacts);spec.mask=a[5]=="shots"?1:a[5]=="melee"?2:a[5]=="direct"?3:7;
            guards.push_back({line,std::string(a[0]),spec});
        }else if(key=="range"){
            std::array<std::string_view,2> a;f32 distance=0;
            if(ranges.size()>=kBorgBattleMoves||!split_fields(value,&a)||!id(a[0])||!scalar(a[1],1,1200,&distance))
                return fail("invalid range gate");
            ranges.push_back({line,std::string(a[0]),distance});
        }else if(key=="activation"){
            std::array<std::string_view,4> a;u16 low=0,high=0;
            if(inputs.size()>=kBorgBattleMoves*2||!split_fields(value,&a)||!id(a[0]))return fail("invalid activation");
            if(a[1]=="release-window"&&number(a[2],0,kBorgBattleHoldTicks,&low)&&number(a[3],low,kBorgBattleHoldTicks,&high))
                inputs.push_back({line,std::string(a[0]),BorgBattleActivation::ReleaseWindow,low,high});
            else if((a[1]=="tap"||a[1]=="double-tap")&&number(a[2],1,60,&low)&&number(a[3],1,60,&high))
                inputs.push_back({line,std::string(a[0]),a[1]=="tap"?BorgBattleActivation::Tap:BorgBattleActivation::DoubleTap,low,high});
            else return fail("invalid activation window");
        }else if(key=="repeat"){
            std::array<std::string_view,2> a;u16 cycles=0;
            if(inputs.size()>=kBorgBattleMoves*2||!split_fields(value,&a)||!id(a[0])||!number(a[1],2,kBorgBattleRepeatCycles,&cycles))
                return fail("invalid held repeat cycle count");
            inputs.push_back({line,std::string(a[0]),BorgBattleActivation::Press,cycles,0});
        }else if(key=="held-fire"){
            std::array<std::string_view,8> a;BorgBattleHeldFire spec;u16 cost=0;
            if(held_fire.size()>=kBorgBattleMoves||!split_fields(value,&a)||!id(a[0])||
               (!id(a[1])&&a[1]!="none")||!number(a[2],0,99,&cost)||((a[1]=="none")!=(cost==0))||!id(a[3])||
               !number(a[4],1,600,&spec.warmup)||!number(a[5],1,600,&spec.slow_interval)||
               !number(a[6],1,spec.slow_interval,&spec.fast_interval)||!number(a[7],0,600,&spec.cooldown))
                return fail("invalid held-fire stock, cadence or cooldown");
            spec.cost=u8(cost);held_fire.push_back({line,std::string(a[0]),std::string(a[1]),std::string(a[3]),spec});
        }else if(key=="detonation-policy"){
            std::array<std::string_view,2> a;
            if(first_detonations.size()>=kBorgBattleMoves||!split_fields(value,&a)||!id(a[0])||a[1]!="first-eligible")
                return fail("invalid detonation-policy");
            first_detonations.emplace_back(line,std::string(a[0]));
        }else if(key=="detonate"){
            std::array<std::string_view,4> a;u16 frame=0;
            if(detonations.size()>=kBorgBattleMoves||!split_fields(value,&a)||!id(a[0])||!number(a[1],1,599,&frame)||
               !id(a[2])||(a[3]!="any"&&a[3]!="landed"))return fail("invalid detonation marker or selector");
            detonations.push_back({line,std::string(a[0]),std::string(a[2]),frame,a[3]=="landed"});
        }else if(key=="release-aim-gate"){
            std::array<std::string_view,4> a;BorgReleaseAimGate g;
            if(release_gates.size()>=kBorgBattleMoves||!split_fields(value,&a)||!id(a[0])||
               !number(a[1],1,599,&g.frame)||!scalar(a[2],.01f,1.5707964f,&g.half_angle)||
               !number(a[3],1,120,&g.timeout))return fail("invalid release-aim-gate");
            release_gates.push_back({line,std::string(a[0]),g});
        }else if(key=="entry-stock"){
            std::array<std::string_view,3> a;u16 minimum=0;
            if(entry_stocks.size()>=kBorgBattleMoves||!split_fields(value,&a)||!id(a[0])||!id(a[1])||
               !number(a[2],1,99,&minimum))return fail("invalid entry-stock binding");
            entry_stocks.push_back({line,std::string(a[0]),std::string(a[1]),minimum});
        }else if(key=="launch-heading"){
            std::array<std::string_view,2> a;
            if(body_launches.size()>=kBorgBattleAttacks||!split_fields(value,&a)||!id(a[0])||a[1]!="body")
                return fail("invalid launch-heading binding");
            body_launches.emplace_back(line,std::string(a[0]));
        }else if(key=="release-return"){
            std::array<std::string_view,3> a;u16 frame=0;
            if(release_pairs.size()>=kBorgBattleMoves*kBorgBattleReleases||!split_fields(value,&a)||
               !id(a[0])||!number(a[1],1,599,&frame)||a[2]!="all-retired")return fail("invalid release return binding");
            release_pairs.push_back({line,std::string(a[0]),frame});
        }else if(key=="release-extra"||key=="release-followup"){
            std::array<std::string_view,3> a;u16 frame=0;
            if(!split_fields(value,&a)||!id(a[0])||!id(a[2])||!number(a[1],1,599,&frame)||
               additional_releases.size()>=kBorgBattleMoves*kBorgBattleReleases*kBorgReleaseFollowups)
                return fail("invalid release follow-up binding");
            additional_releases.push_back({line,std::string(a[0]),std::string(a[2]),frame,key=="release-extra"});
        }else if(key=="release"){
            std::array<std::string_view,5> a;u16 frame=0,cost=0;
            if(!split_fields(value,&a)||!id(a[0])||(!id(a[2])&&a[2]!="none")||!id(a[4])||
               !number(a[1],1,599,&frame)||!number(a[3],0,99,&cost)||((a[2]=="none")!=(cost==0))||
               refs.size()>=kBorgBattleMoves*kBorgBattleReleases)return fail("invalid release binding");
            refs.push_back({line,std::string(a[0]),std::string(a[2]),std::string(a[4]),frame,cost});
        }else return fail("unsupported field");
    }
    if(next.variant.empty()||!has_movement||!next.health)return fail("variant, movement and health are required");
    const auto movement=next.movement;
    if(movement==BorgBattleMovement::NormalGRed){
        if(locomotion||!next.rig_prefix.empty()||!next.pose_profile.empty())return fail("G-Red adapter uses the qualified practice rig and locomotion");
    }else if(next.rig_prefix.empty()||next.pose_profile.empty()||
             locomotion!=((next.static_body()||next.cruise_entry)?0u:movement==BorgBattleMovement::Stationary?1u:next.dash_disabled?7u:15u))
        return fail("exact rig and locomotion clip bindings required");
    if(next.moves.empty()!=(movement==BorgBattleMovement::Stationary))return fail("only a stationary target has no moves");
    if(movement==BorgBattleMovement::Stationary&&(!next.stocks.empty()||!next.projectiles.empty()))return fail("stationary target cannot declare stock or attacks");
    for(const auto& [at,projectile]:body_launches){
        line=at;const auto pi=index(next.projectiles,projectile);
        if(pi==next.projectiles.size()||next.projectiles[pi].launch_body_heading)
            return fail("launch-heading needs one declared projectile");
        next.projectiles[pi].launch_body_heading=true;
    }
    for(const auto& [at,id]:add_on_debit_stocks){
        line=at;
        const auto stock=index(next.stocks,id);
        if(stock==next.stocks.size()||!next.stocks[stock].refill_ticks||next.stocks[stock].add_on_debit)
            return fail("stock-clock needs one timed stock pool");
        next.stocks[stock].add_on_debit=true;
    }
    for(const auto& h:hits){
        line=h.line;const auto pi=index(next.projectiles,h.projectile);
        if(pi==next.projectiles.size())return fail("unknown hit-effect projectile");
        auto& attack=next.projectiles[pi];
        if(h.pull){
            if(attack.pull_units||movement!=BorgBattleMovement::Grounded)return fail("duplicate pull or movement component does not support pulls");
            attack.pull_units=h.amount;
        }else{
            if(attack.walk_ticks)return fail("duplicate walk-speed effect");
            attack.walk_percent=h.amount;attack.walk_ticks=h.ticks;
        }
    }
    for(const auto& r:passes){
        line=r.line;const auto pi=index(next.projectiles,r.projectile);
        if(pi==next.projectiles.size()||next.projectiles[pi].ignore_actors||next.projectiles[pi].continue_after_actor_hit)
            return fail("unknown projectile or duplicate actor-contact");
        auto& a=next.projectiles[pi];
        if(r.continuing)a.continue_after_actor_hit=true;
        else{
            if(a.damage||a.pull_units||a.walk_ticks)return fail("ignoring actor contact cannot also apply direct-hit effects");
            a.ignore_actors=true;
        }
    }
    for(const auto& r:speed_ramps){
        line=r.line;const auto pi=index(next.projectiles,r.projectile);
        if(pi==next.projectiles.size()||next.projectiles[pi].speed_ramp)
            return fail("unknown projectile or duplicate projectile-speed-ramp");
        next.projectiles[pi].speed_ramp=r.spec;
    }
    for(const auto& link:rewind_hits){
        line=link.line;
        const auto pi=index(next.projectiles,link.projectile),si=index(next.anchor_sets,link.set);
        if(pi==next.projectiles.size()||si==next.anchor_sets.size())return fail("unresolved rewind carrier or anchor set");
        auto& shot=next.projectiles[pi];
        if(shot.rewind_hit||movement!=BorgBattleMovement::Grounded||shot.count!=1||shot.ignore_actors||shot.pull_units)
            return fail("recipient rewind needs one contacting carrier and no competing pull");
        shot.rewind_hit=BorgBattleRewindHit{u8(si),link.speed};
    }
    for(const auto& plant:stage_anchors){
        line=plant.line;
        const auto pi=index(next.projectiles,plant.projectile),si=index(next.anchor_sets,plant.set);
        if(pi==next.projectiles.size()||si==next.anchor_sets.size())return fail("unresolved anchor carrier or set");
        auto& shot=next.projectiles[pi];
        if(shot.plant_on_stage!=255||shot.floor!=BorgProjectileFloor::StageRetire||shot.count!=1||shot.rewind_hit)
            return fail("planting needs one separate stage-contact carrier");
        shot.plant_on_stage=u8(si);
    }
    for(const auto& f:anchor_fallbacks){
        line=f.line;
        const auto pi=index(next.projectiles,f.projectile),fi=index(next.projectiles,f.fallback);
        if(pi==next.projectiles.size()||fi==next.projectiles.size()||!next.projectiles[pi].rewind_hit||
           next.projectiles[pi].rewind_hit->fallback!=255||next.projectiles[fi].rewind_hit||next.projectiles[fi].plant_on_stage!=255)
            return fail("fallback must name a separate projectile without anchor effects");
        next.projectiles[pi].rewind_hit->fallback=u8(fi);
    }
    for(const auto& r:blasts){
        line=r.line;const auto pi=index(next.projectiles,r.projectile);
        if(pi==next.projectiles.size()||next.projectiles[pi].blast)return fail("unknown projectile or duplicate blast");
        const auto& a=next.projectiles[pi];
        if(!borg_blast_trigger_reachable(a,r.spec.trigger))
            return fail("blast trigger cannot occur under the projectile's contact policy");
        next.projectiles[pi].blast=r.spec;
    }
    for(const auto& r:proximities){
        line=r.line;const auto pi=index(next.projectiles,r.projectile);
        if(pi==next.projectiles.size()||next.projectiles[pi].proximity)return fail("unknown projectile or duplicate bomb proximity");
        next.projectiles[pi].proximity=r.spec;
    }
    for(const auto& r:attractions){
        line=r.line;const auto pi=index(next.projectiles,r.projectile);
        if(pi==next.projectiles.size()||next.projectiles[pi].attraction)return fail("unknown projectile or duplicate bomb attraction");
        next.projectiles[pi].attraction=r.spec;
    }
    for(const auto& r:fuse_tails){
        line=r.line;const auto pi=index(next.projectiles,r.projectile);
        if(pi==next.projectiles.size()||next.projectiles[pi].fuse_tail)return fail("unknown projectile or duplicate bomb fuse tail");
        next.projectiles[pi].fuse_tail=r.ticks;
    }
    for(const auto& r:blast_children){
        line=r.line;const auto pi=index(next.projectiles,r.projectile),ci=index(next.projectiles,r.child);
        if(pi==next.projectiles.size()||ci==next.projectiles.size()||next.projectiles[pi].blast_children!=255)
            return fail("unknown projectile or duplicate blast children");
        next.projectiles[pi].blast_children=u8(ci);
    }
    for(const auto& r:flight_children){
        line=r.line;const auto pi=index(next.projectiles,r.projectile),ci=index(next.projectiles,r.child);
        if(pi==next.projectiles.size()||ci==next.projectiles.size()||next.projectiles[pi].flight_children)
            return fail("unknown projectile or duplicate flight children");
        auto spec=r.spec;spec.projectile=u8(ci);next.projectiles[pi].flight_children=spec;
    }
    for(const auto& r:impact_children){
        line=r.line;const auto pi=index(next.projectiles,r.projectile),ci=index(next.projectiles,r.child);
        if(pi==next.projectiles.size()||ci==next.projectiles.size()||next.projectiles[pi].impact_children)
            return fail("unknown projectile or duplicate impact children");
        auto spec=r.spec;spec.projectile=u8(ci);next.projectiles[pi].impact_children=spec;
    }
    for(const auto& r:impact_trails){
        line=r.line;const auto pi=index(next.projectiles,r.projectile);
        if(pi==next.projectiles.size()||next.projectiles[pi].impact_trail)
            return fail("unknown projectile or duplicate impact trail");
        next.projectiles[pi].impact_trail=r.spec;
    }
    for(const auto& r:impact_hazards){
        line=r.line;const auto pi=index(next.projectiles,r.projectile);
        if(pi==next.projectiles.size()||next.projectiles[pi].impact_hazard)
            return fail("unknown projectile or duplicate impact hazard");
        next.projectiles[pi].impact_hazard=r.spec;
    }
    for(const auto& r:impact_areas){
        line=r.line;const auto pi=index(next.projectiles,r.projectile);
        if(pi==next.projectiles.size()||next.projectiles[pi].impact_area)
            return fail("unknown projectile or duplicate impact area");
        next.projectiles[pi].impact_area=r.spec;
    }
    for(const auto& r:impact_timings){
        line=r.line;const auto pi=index(next.projectiles,r.projectile);
        if(pi==next.projectiles.size())return fail("unknown projectile in impact-pattern-timing");
        auto& a=next.projectiles[pi];
        if((r.trail&&!a.impact_trail)||((r.hazard||r.query)&&!a.impact_hazard)||
           (a.impact_trail&&a.impact_trail->first_delay)||
           (a.impact_hazard&&(a.impact_hazard->first_delay||a.impact_hazard->first_query)))
            return fail("duplicate impact-pattern-timing or undeclared impact pattern");
        if(a.impact_trail)a.impact_trail->first_delay=r.trail;
        if(a.impact_hazard){a.impact_hazard->first_delay=r.hazard;a.impact_hazard->first_query=r.query;}
    }
    for(const auto& [at,projectile]:impact_world_space){
        line=at;const auto pi=index(next.projectiles,projectile);
        if(pi==next.projectiles.size()||next.projectiles[pi].impact_world_space||
           (!next.projectiles[pi].impact_trail&&!next.projectiles[pi].impact_hazard))
            return fail("impact-pattern-space needs one binding to a declared impact pattern");
        next.projectiles[pi].impact_world_space=true;
    }
    for(const auto& r:smoke){
        line=r.line;const auto pi=index(next.projectiles,r.projectile);
        if(pi==next.projectiles.size()||next.projectiles[pi].smoke)return fail("unknown projectile or duplicate blast smoke");
        next.projectiles[pi].smoke=r.spec;
    }
    for(const auto& r:ink){
        line=r.line;const auto pi=index(next.projectiles,r.projectile);
        if(pi==next.projectiles.size()||next.projectiles[pi].ink)return fail("unknown projectile or duplicate blast ink");
        next.projectiles[pi].ink=r.spec;
    }
    for(const auto& r:accuracy){
        line=r.line;const auto pi=index(next.projectiles,r.projectile);
        if(pi==next.projectiles.size()||next.projectiles[pi].accuracy)return fail("unknown projectile or duplicate blast accuracy");
        next.projectiles[pi].accuracy=r.spec;
    }
    for(const auto& [at,shot]:accuracy_sensitive){
        line=at;const auto pi=index(next.projectiles,shot);
        if(pi==next.projectiles.size()||next.projectiles[pi].accuracy_sensitive)return fail("unknown or duplicate accuracy-sensitive projectile");
        next.projectiles[pi].accuracy_sensitive=true;
    }
    for(const auto& r:power_fields){
        line=r.line;const auto mi=index(next.moves,r.move);
        if(mi==next.moves.size()||next.moves[mi].power_field)return fail("unknown move or duplicate power field");
        next.moves[mi].power_field=r.spec;
    }
    for(const auto& r:slow_fields){
        line=r.line;const auto mi=index(next.moves,r.move);
        if(mi==next.moves.size()||next.moves[mi].slow_field)return fail("unknown move or duplicate slow field");
        next.moves[mi].slow_field=r.spec;
    }
    for(auto& r:fields){
        line=r.line;const auto mi=index(next.moves,r.move);
        if(mi==next.moves.size()||next.moves[mi].field)return fail("unknown move or duplicate field");
        next.moves[mi].field=std::move(r.spec);
    }
    for(const auto& r:field_effects){
        line=r.line;const auto mi=index(next.moves,r.move);
        if(mi==next.moves.size()||!next.moves[mi].field||next.moves[mi].field->effects.size()>=kBorgFieldEffects)
            return fail("field-effect needs a field with available effect slots");
        next.moves[mi].field->effects.push_back(r.spec);
    }
    for(const auto& r:field_shot_pulls){
        line=r.line;const auto mi=index(next.moves,r.move);
        if(mi==next.moves.size()||!next.moves[mi].field||next.moves[mi].field->shot_pull)
            return fail("field-shot-pull needs one binding to a declared field");
        next.moves[mi].field->shot_pull=r.spec;
    }
    for(const auto& r:field_drifts){
        line=r.line;const auto mi=index(next.moves,r.move);
        if(mi==next.moves.size()||!next.moves[mi].field||next.moves[mi].field->drift)
            return fail("field-drift needs one declared field");
        next.moves[mi].field->drift=r.spec;
    }
    for(const auto& r:field_slots){
        line=r.line;const auto mi=index(next.moves,r.move);
        if(mi==next.moves.size()||!next.moves[mi].field||next.moves[mi].field->slot)
            return fail("field-slot needs one declared field");
        next.moves[mi].field->slot=r.spec;
    }
    for(const auto& r:field_actor_pulls){
        line=r.line;const auto mi=index(next.moves,r.move);
        if(mi==next.moves.size()||!next.moves[mi].field||next.moves[mi].field->actor_pull)
            return fail("field-actor-pull needs one binding to a declared field");
        next.moves[mi].field->actor_pull=r.spec;
    }
    for(const auto& r:field_pull_points){
        line=r.line;const auto mi=index(next.moves,r.move);
        if(mi==next.moves.size()||!next.moves[mi].field||!next.moves[mi].field->actor_pull||
           next.moves[mi].field->actor_pull->point)return fail("field-actor-pull-point needs one declared actor pull");
        next.moves[mi].field->actor_pull->point=r.spec;
    }
    for(const auto& [at,move]:field_repulsions){
        line=at;const auto mi=index(next.moves,move);
        if(mi==next.moves.size()||!next.moves[mi].field||!next.moves[mi].field->actor_pull||
           next.moves[mi].field->actor_pull->repel)return fail("field-actor-polarity needs one declared actor pull");
        next.moves[mi].field->actor_pull->repel=true;
    }
    for(const auto& r:field_core_damage){
        line=r.line;const auto mi=index(next.moves,r.move);
        if(mi==next.moves.size()||!next.moves[mi].field||next.moves[mi].field->core_damage)
            return fail("field-core-damage needs one binding to a declared field");
        next.moves[mi].field->core_damage=r.spec;
    }
    for(const auto& r:field_debit){
        line=r.line;const auto mi=index(next.moves,r.move);
        if(mi==next.moves.size()||!next.moves[mi].field||next.moves[mi].field->debit)
            return fail("field-debit needs one binding to a declared field");
        next.moves[mi].field->debit=r.spec;
    }
    for(const auto& r:move_charge){
        line=r.line;const auto mi=index(next.moves,r.move);
        if(mi==next.moves.size()||next.moves[mi].charge)
            return fail("move-charge needs one declared move");
        next.moves[mi].charge=r.spec;
    }
    for(const auto& r:reaction_offsets){
        line=r.line;const auto mi=index(next.moves,r.move);
        if(mi==next.moves.size()||next.moves[mi].reaction_offset_declared)
            return fail("reaction-offset needs one declared move");
        next.moves[mi].reaction_yaw_offset=r.yaw;
        next.moves[mi].reaction_pitch_offset=r.pitch;
        next.moves[mi].reaction_offset_declared=true;
    }
    for(const auto& r:free_release){
        line=r.line;const auto mi=index(next.moves,r.move);
        if(mi==next.moves.size()||next.moves[mi].free_release)
            return fail("free-release needs one declared move");
        next.moves[mi].free_release=true;
    }
    for(const auto& r:field_textures){
        line=r.line;const auto mi=index(next.moves,r.move);
        if(mi==next.moves.size()||!next.moves[mi].field||!next.moves[mi].field->texture.empty())
            return fail("field-texture needs one field and one local texture");
        next.moves[mi].field->texture=r.path;
    }
    for(const auto& [at,move]:field_follows){
        line=at;const auto mi=index(next.moves,move);
        if(mi==next.moves.size()||!next.moves[mi].field||next.moves[mi].field->follow)
            return fail("field-follow needs one explicitly declared field");
        next.moves[mi].field->follow=true;
    }
    for(const auto& r:field_sockets){
        line=r.line;const auto mi=index(next.moves,r.move);
        if(mi==next.moves.size()||!next.moves[mi].field||next.moves[mi].field->socket)
            return fail("field-socket needs one explicitly declared field");
        next.moves[mi].field->socket=r.socket;
    }
    for(const auto& r:returns){
        line=r.line;const auto pi=index(next.projectiles,r.projectile);
        if(pi==next.projectiles.size()||next.projectiles[pi].stock_return!=BorgProjectileReturn::None)
            return fail("unknown projectile or duplicate stock return");
        auto& shot=next.projectiles[pi];
        if(!borg_projectile_return_reachable(shot,r.reason))return fail("stock return cannot occur under the projectile's lifetime policy");
        shot.stock_return=r.reason;
    }
    for(const auto& r:sustained){
        line=r.line;const auto pi=index(next.projectiles,r.projectile);
        if(pi==next.projectiles.size()||next.projectiles[pi].sustained)return fail("unknown projectile or duplicate sustained beam");
        next.projectiles[pi].sustained=r.spec;
    }
    for(const auto& r:beam_sweeps){
        line=r.line;const auto pi=index(next.projectiles,r.projectile);
        if(pi==next.projectiles.size()||next.projectiles[pi].beam_sweep)return fail("unknown projectile or duplicate beam sweep");
        next.projectiles[pi].beam_sweep=r.spec;
    }
    for(const auto& r:guidance){
        line=r.line;const auto pi=index(next.projectiles,r.projectile);
        if(pi==next.projectiles.size()||next.projectiles[pi].guidance)return fail("unknown projectile or duplicate guidance");
        auto& shot=next.projectiles[pi];
        if(r.spec.ticks>shot.lifetime)return fail("guidance window exceeds projectile lifetime");
        shot.guidance=r.spec;
    }
    for(const auto& r:starts){
        line=r.line;const auto mi=index(next.moves,r.move);
        if(mi==next.moves.size()||next.moves[mi].pose_start)return fail("unknown move or duplicate pose start");
        next.moves[mi].pose_start=r.frame;
    }
    for(const auto& [source,move]:pose_loop_windows){
        line=source;const auto mi=index(next.moves,move);
        if(mi==next.moves.size()||next.moves[mi].pose_authored_loop_window)
            return fail("unknown move or duplicate pose loop window");
        next.moves[mi].pose_authored_loop_window=true;
    }
    for(const auto& r:aim_locks){
        line=r.line;const auto mi=index(next.moves,r.move);
        if(mi==next.moves.size()||next.moves[mi].aim_lock)return fail("unknown move or duplicate aim-lock marker");
        next.moves[mi].aim_lock=r.frame;
    }
    for(const auto& r:sockets){
        line=r.line;const auto pi=index(next.projectiles,r.projectile);
        if(pi==next.projectiles.size()||next.projectiles[pi].socket)return fail("unknown projectile or duplicate origin");
        next.projectiles[pi].socket=r.spec;
    }
    for(const auto& r:action_sounds){
        line=r.first;const auto mi=index(next.moves,r.second.first);
        if(mi==next.moves.size()||!next.moves[mi].action_sound.empty())
            return fail("unknown move or duplicate action sound");
        next.moves[mi].action_sound=r.second.second;
    }
    for(const auto& r:launch_sounds){
        line=r.line;const auto pi=index(next.projectiles,r.projectile);
        if(pi==next.projectiles.size()||!next.projectiles[pi].launch_sound.empty())
            return fail("unknown projectile or duplicate launch sound");
        next.projectiles[pi].launch_sound=r.path;
    }
    for(const auto& r:looks){
        line=r.line;const auto pi=index(next.projectiles,r.projectile);
        if(pi==next.projectiles.size()||next.projectiles[pi].look)return fail("unknown projectile or duplicate look");
        next.projectiles[pi].look=r.spec;
    }
    for(const auto& r:record_looks){
        line=r.line;const auto pi=index(next.projectiles,r.projectile);
        if(pi==next.projectiles.size()||next.projectiles[pi].record_look)return fail("unknown projectile or duplicate record");
        next.projectiles[pi].record_look=r.spec;
    }
    for(const auto& r:record_trails){
        line=r.line;const auto pi=index(next.projectiles,r.projectile);
        if(pi==next.projectiles.size()||next.projectiles[pi].record_trail)return fail("unknown projectile or duplicate record trail");
        next.projectiles[pi].record_trail=r.spec;
    }
    for(const auto& r:impact_records){
        line=r.line;const auto pi=index(next.projectiles,r.projectile);
        if(pi==next.projectiles.size()||next.projectiles[pi].impact_record)return fail("unknown projectile or duplicate impact record");
        next.projectiles[pi].impact_record=r.spec;
    }
    for(const auto& r:impact_record_growth){
        line=r.line;const auto pi=index(next.projectiles,r.projectile);
        if(pi==next.projectiles.size()||!next.projectiles[pi].impact_record||next.projectiles[pi].impact_record->end_scale||
           next.projectiles[pi].impact_record->ticks<2)return fail("impact record growth requires a unique visual lasting at least two ticks");
        next.projectiles[pi].impact_record->end_scale=r.end;
    }
    for(const auto& r:impact_sounds){
        line=r.line;const auto pi=index(next.projectiles,r.projectile);
        if(pi==next.projectiles.size()||next.projectiles[pi].impact_sound)return fail("unknown projectile or duplicate impact sound");
        next.projectiles[pi].impact_sound=r.spec;
    }
    for(const auto& r:animations){
        line=r.line;const auto pi=index(next.projectiles,r.projectile);
        if(pi==next.projectiles.size()||next.projectiles[pi].animation)return fail("unknown projectile or duplicate animation");
        const auto style=next.projectiles[pi].visual;
        if(r.spec.spin&&style!=BorgAttackVisual::Shuriken&&style!=BorgAttackVisual::Orb)
            return fail("projectile style has no visible rotation");
        next.projectiles[pi].animation=r.spec;
    }
    for(const auto& r:spins){
        line=r.line;const auto mi=index(next.moves,r.move);
        if(mi==next.moves.size()||next.moves[mi].part_spins.size()>=kBorgBattlePartAnimations)
            return fail("unknown move or part-spin budget");
        next.moves[mi].part_spins.push_back(r.spec);
    }
    for(const auto& r:part_motions){
        line=r.line;const auto mi=index(next.moves,r.move);
        if(mi==next.moves.size()||next.moves[mi].part_spins.size()+next.moves[mi].part_motions.size()>=kBorgBattlePartAnimations)
            return fail("unknown move or decorative animation budget");
        next.moves[mi].part_motions.push_back(r.spec);
    }
    for(const auto& r:scrolls){
        line=r.line;const auto mi=index(next.moves,r.move);
        if(mi==next.moves.size())return fail("unknown move for part-scroll");
        auto& move=next.moves[mi];
        if(move.part_spins.size()+move.part_motions.size()+move.part_scrolls.size()>=kBorgBattlePartAnimations)
            return fail("decorative animation budget");
        move.part_scrolls.push_back(r.spec);
    }
    for(const auto& r:swaps){
        line=r.line;const auto mi=index(next.moves,r.move),si=index(next.part_slots,r.slot);
        if(mi==next.moves.size()||si==next.part_slots.size())return fail("unknown move or part-swap slot");
        auto spec=r.spec;spec.slot=u8(si);next.moves[mi].part_swaps.push_back(spec);
    }
    for(const auto& r:part_poses){
        line=r.line;const auto si=index(next.part_slots,r.slot);
        if(si==next.part_slots.size())return fail("unknown part-pose slot");
        auto* target=&next.part_slots[si].pose;
        if(!r.move.empty()){
            const auto mi=index(next.moves,r.move);
            if(mi==next.moves.size())return fail("unknown part-swap-pose move");
            auto& entries=next.moves[mi].part_swaps;
            const auto bound=std::find_if(entries.begin(),entries.end(),[si](const auto& spec){return spec.slot==si;});
            if(bound==entries.end()||!bound->record)return fail("part-swap-pose requires a visible swap");
            target=&bound->pose;
        }
        if(*target)return fail("duplicate part pose");
        *target=r.spec;
    }
    for(const auto& r:part_turns){
        line=r.line;const auto si=index(next.part_slots,r.slot);
        if(si==next.part_slots.size()||next.part_slots[si].turn_step!=std::array<u16,3>{})
            return fail("unknown or duplicate part-turn slot");
        next.part_slots[si].turn_step=r.step;
    }
    for(const auto& r:part_bases){
        line=r.line;const auto si=index(next.part_slots,r.slot);
        if(si==next.part_slots.size()||next.part_slots[si].basis!=BorgPartBasis::Bone)
            return fail("unknown or duplicate part-basis slot");
        next.part_slots[si].basis=r.basis;
    }
    for(const auto& r:mount_spins){
        line=r.line;const auto si=index(next.part_slots,r.slot);
        if(si==next.part_slots.size()||next.part_slots[si].mount_spin||!next.part_slots[si].record)
            return fail("part mount spin needs one visible slot");
        next.part_slots[si].mount_spin=r.spec;
    }
    for(const auto& r:part_bones){
        line=r.line;const auto mi=index(next.moves,r.move),si=index(next.part_slots,r.slot);
        if(mi==next.moves.size()||si==next.part_slots.size())return fail("unknown part-swap-bone move or slot");
        auto& entries=next.moves[mi].part_swaps;
        const auto bound=std::find_if(entries.begin(),entries.end(),[si](const auto& spec){return spec.slot==si;});
        if(bound==entries.end()||!bound->record||bound->bone)return fail("part-swap-bone requires one visible swap");
        bound->bone=r.bone;
    }
    for(const auto& ref:refs){
        line=ref.line;
        const auto mi=index(next.moves,ref.move),pi=index(next.projectiles,ref.projectile),si=index(next.stocks,ref.stock);
        if(mi==next.moves.size()||pi==next.projectiles.size()||(ref.stock!="none"&&si==next.stocks.size()))return fail("unresolved move, stock or projectile");
        auto& m=next.moves[mi];
        if(ref.frame>=m.duration||m.releases.size()>=kBorgBattleReleases||
           (!m.releases.empty()&&m.releases.back().frame>=ref.frame)||
           (ref.cost&&ref.cost>next.stocks[si].capacity))return fail("release order, action window or stock cost");
        const BorgBattleRelease release{ref.frame,u8(ref.cost?si:255),u8(ref.cost),u8(pi)};
        m.releases.push_back(release);
    }
    for(const auto& ref:release_sounds){
        line=ref.line;const auto mi=index(next.moves,ref.move);
        if(mi==next.moves.size())return fail("unresolved move for release sound");
        auto& releases=next.moves[mi].releases;
        const auto r=std::find_if(releases.begin(),releases.end(),[&](const auto& release){return release.frame==ref.frame;});
        if(r==releases.end()||!r->cost||!r->debit_sound.empty())return fail("release sound requires one paid marker");
        r->debit_sound=ref.path;
    }
    std::array<unsigned,kBorgBattleMoves> legacy_followups{};
    for(const auto& ref:additional_releases){
        line=ref.line;const auto mi=index(next.moves,ref.move),pi=index(next.projectiles,ref.projectile);
        if(mi==next.moves.size()||pi==next.projectiles.size())return fail("unresolved move or projectile for release-extra");
        auto& releases=next.moves[mi].releases;
        const auto r=std::find_if(releases.begin(),releases.end(),[&](const auto& release){return release.frame==ref.frame;});
        if(r==releases.end()||!r->cost)return fail("release follow-up requires one paid marker");
        const auto mask=1u<<unsigned(r-releases.begin());
        if((legacy_followups[mi]&mask)||(ref.legacy&&r->count)||r->count==r->followups.size())
            return fail("mixed, duplicate or excessive release follow-ups");
        if(ref.legacy)legacy_followups[mi]|=mask;
        r->followups[r->count++]=u8(pi);
    }
    for(const auto& ref:release_pairs){
        line=ref.line;const auto mi=index(next.moves,ref.move);
        if(mi==next.moves.size())return fail("unknown move for release return");
        auto& releases=next.moves[mi].releases;
        const auto r=std::find_if(releases.begin(),releases.end(),[&](const auto& release){return release.frame==ref.frame;});
        if(r==releases.end()||r->return_pair)return fail("release return needs one declared marker");
        r->return_pair=true;
        if(!borg_battle_release_valid(next,*r))return fail("grouped return requires a one-unit return-only pool and two single projectiles");
    }
    for(const auto& r:entry_stocks){
        line=r.line;
        const auto mi=index(next.moves,r.move),si=index(next.stocks,r.stock);
        if(mi==next.moves.size()||si==next.stocks.size()||next.moves[mi].entry_stock||
           r.minimum>next.stocks[si].capacity)return fail("unknown, duplicate or unreachable entry-stock binding");
        next.moves[mi].entry_stock=BorgBattleEntryStock{u8(si),u8(r.minimum)};
    }
    for(const auto& r:release_gates){
        line=r.line;const auto mi=index(next.moves,r.move);
        if(mi==next.moves.size()||next.moves[mi].release_gate)return fail("release-aim-gate needs one declared move");
        next.moves[mi].release_gate=r.spec;
    }
    for(const auto& r:ranges){
        line=r.line;const auto move=index(next.moves,r.move);
        if(move==next.moves.size()||next.moves[move].entry_range)return fail("unknown or duplicate range move");
        next.moves[move].entry_range=r.distance;
    }
    for(const auto& m:melee){
        line=m.line;const auto move=index(next.moves,m.move);
        if(move==next.moves.size()||next.moves[move].melee)return fail("unknown or duplicate melee move");
        if(m.shape.last>=next.moves[move].duration)return fail("melee window extends outside move");
        next.moves[move].melee=m.shape;
    }
    for(const auto& i:inputs){
        line=i.line;const auto move=index(next.moves,i.move);
        if(move==next.moves.size())return fail("unknown activation or repeat move");
        auto& m=next.moves[move];
        if(m.activation!=BorgBattleActivation::Press||m.hold_cycles)return fail("duplicate or incompatible activation and repeat");
        if(i.activation!=BorgBattleActivation::Press){
            m.activation=i.activation;
            if(i.activation==BorgBattleActivation::ReleaseWindow){m.minimum_hold=i.first;m.maximum_hold=i.second;}
            else{m.maximum_hold=i.first;m.tap_window=i.second;}
        }else{
            if(m.melee||m.releases.size()!=1||!m.releases.front().cost)return fail("held repeat requires one paid projectile release and no melee window");
            m.hold_cycles=u8(i.first);
        }
    }
    for(const auto& h:held_fire){
        line=h.line;
        const auto mi=index(next.moves,h.move),si=index(next.stocks,h.stock),pi=index(next.projectiles,h.projectile);
        if(mi==next.moves.size()||pi==next.projectiles.size()||(h.spec.cost&&si==next.stocks.size()))
            return fail("unresolved held-fire move, stock or projectile");
        auto& m=next.moves[mi];
        if(!m.releases.empty()||m.melee||m.activation!=BorgBattleActivation::Press||m.hold_cycles||m.held_fire||
           (h.spec.cost&&h.spec.cost>next.stocks[si].capacity))return fail("held fire requires its own press-started move and valid cost");
        auto spec=h.spec;spec.stock=u8(spec.cost?si:255);spec.projectile=u8(pi);
        if(!borg_battle_emission_valid(next,spec.stock,spec.cost,spec.projectile))
            return fail("held-fire stock return requires one unit from return-only stock per carrier");
        m.held_fire=spec;
    }
    for(const auto& d:detonations){
        line=d.line;
        const auto mi=index(next.moves,d.move),pi=index(next.projectiles,d.projectile);
        if(mi==next.moves.size()||pi==next.projectiles.size()||!next.projectiles[pi].blast||
           next.projectiles[pi].blast->trigger!=BorgBlastTrigger::Remote)return fail("detonation must name a local remote blast projectile");
        auto& m=next.moves[mi];
        if(m.detonation||d.frame>=m.duration||m.hold_cycles||m.held_fire||
           (d.landed_only&&next.projectiles[pi].floor!=BorgProjectileFloor::Land))
            return fail("duplicate or incompatible detonation marker");
        m.detonation=BorgBattleDetonation{d.frame,u8(pi),d.landed_only};
    }
    for(const auto& [at,move]:first_detonations){
        line=at;const auto mi=index(next.moves,move);
        if(mi==next.moves.size()||!next.moves[mi].detonation||next.moves[mi].detonation->first_eligible)
            return fail("detonation-policy needs one declared detonation");
        next.moves[mi].detonation->first_eligible=true;
    }
    for(const auto& g:guards){
        line=g.line;const auto mi=index(next.moves,g.move);
        if(mi==next.moves.size())return fail("unknown guard move");
        auto& m=next.moves[mi];
        if(m.guard||m.hold_cycles||m.held_fire||g.spec.last>=m.duration)return fail("duplicate or incompatible guard window");
        m.guard=g.spec;
    }
    for(auto& e:emitters){
        line=e.line;
        const auto mi=index(next.moves,e.watches),pi=index(next.projectiles,e.projectile);
        if(index(next.emitters,e.spec.id)!=next.emitters.size()||mi==next.moves.size()||pi==next.projectiles.size()||
           (next.moves[mi].releases.empty()&&!next.moves[mi].held_fire)||
           next.projectiles[pi].stock_return!=BorgProjectileReturn::None||next.projectiles[pi].rewind_hit||
           next.projectiles[pi].plant_on_stage!=255||next.projectiles[pi].socket)
            return fail("duplicate emitter, missing firing action or child projectile needs owner stock/anchor authority");
        e.spec.watches=u8(mi);e.spec.projectile=u8(pi);next.emitters.push_back(std::move(e.spec));
    }
    for(const auto& d:deployments){
        line=d.line;
        const auto mi=index(next.moves,d.move),ei=index(next.emitters,d.emitter);
        if(mi==next.moves.size()||ei==next.emitters.size())return fail("unknown emitter or deployment move");
        auto& m=next.moves[mi];
        if(d.frame>=m.duration||m.hold_cycles||m.held_fire||
           std::any_of(m.emitters.begin(),m.emitters.end(),[&](const auto& r){return r.emitter==ei;}))
            return fail("duplicate or incompatible deployment marker");
        m.emitters.push_back({d.frame,u8(ei)});
    }
    for(const auto& [at,set]:anchor_returns){
        line=at;const auto si=index(next.anchor_sets,set);
        if(si==next.anchor_sets.size()||next.anchor_sets[si].return_on_reach)return fail("anchor-return needs one declared set");
        next.anchor_sets[si].return_on_reach=true;
    }
    for(auto& a:anchors){
        line=a.line;
        const auto mi=index(next.moves,a.move),si=index(next.anchor_sets,a.set),pi=index(next.stocks,a.stock);
        if(mi==next.moves.size()||si==next.anchor_sets.size()||(a.marker.cost&&(pi==next.stocks.size()||
           a.marker.cost>next.stocks[pi].capacity)))return fail("unresolved anchor marker or stock pool");
        auto& m=next.moves[mi];
        if(m.held_fire||m.hold_cycles||m.anchors.size()>=kBorgBattleAnchors||a.marker.frame>=m.duration||
           (!m.anchors.empty()&&a.marker.frame<=m.anchors.back().frame))return fail("duplicate or incompatible anchor marker");
        a.marker.set=u8(si);a.marker.stock=u8(a.marker.cost?pi:255);m.anchors.push_back(a.marker);
    }
    for(const auto& r:anchor_fences){
        line=r.line;const auto si=index(next.anchor_sets,r.set);
        if(si==next.anchor_sets.size()||next.anchor_sets[si].fence)return fail("anchor-fence needs one declared anchor set");
        next.anchor_sets[si].fence=r.spec;
    }
    for(const auto& [set,recoil]:fence_recoils){
        const auto si=index(next.anchor_sets,set);
        if(si==next.anchor_sets.size()||!next.anchor_sets[si].fence||next.anchor_sets[si].fence->recoil)
            return fail("fence-recoil needs one declared fence");
        next.anchor_sets[si].fence->recoil=recoil;
    }
    for(const auto& r:anchor_offsets){
        line=r.line;const auto mi=index(next.moves,r.move);
        if(mi==next.moves.size())return fail("unknown place-anchor-offset move");
        auto& a=next.moves[mi].anchors;
        const auto marker=std::find_if(a.begin(),a.end(),[&](const auto& m){return m.frame==r.frame;});
        if(marker==a.end()||marker->rewind_speed||marker->forward||marker->height||(!r.forward&&!r.height))
            return fail("place-anchor-offset needs one placement marker and nonzero offset");
        marker->forward=r.forward;marker->height=r.height;
    }
    for(const auto& m:next.moves)if(m.releases.empty()&&!m.melee&&!m.held_fire&&!m.detonation&&!m.guard&&!m.power_field&&
        !m.slow_field&&!m.field&&m.emitters.empty()&&m.anchors.empty())
        return fail("move needs an executable marker or behavior: "+m.id);
    for(const auto& r:pose_followups){
        line=r.line;const auto mi=index(next.moves,r.move);
        if(mi==next.moves.size()||next.moves[mi].pose_followup)
            return fail("pose-followup needs one declared move");
        next.moves[mi].pose_followup=r.spec;
        if(!borg_battle_pose_followup_valid(next,next.moves[mi]))
            return fail("pose-followup needs a grounded melee-only move and a switch no later than first contact");
    }
    for(const auto& r:contact_powers){
        line=r.line;const auto mi=index(next.moves,r.move);
        if(mi==next.moves.size()||next.moves[mi].contact_power)
            return fail("contact-power needs one declared move");
        next.moves[mi].contact_power=r.base;
        if(!borg_battle_contact_damage_valid(next,next.moves[mi]))
            return fail("contact-power needs a direct melee or projectile-only move");
    }
    for(const auto& [at,move]:contact_minimum_hp){
        line=at;const auto mi=index(next.moves,move);
        if(mi==next.moves.size()||next.moves[mi].contact_minimum_hp)
            return fail("contact-minimum needs one declared move");
        next.moves[mi].contact_minimum_hp=true;
        if(!borg_battle_contact_damage_valid(next,next.moves[mi]))
            return fail("contact-minimum needs a direct melee or projectile-only move");
    }
    for(const auto& r:reaction_mode){
        line=r.line;const auto mi=index(next.moves,r.move);
        if(mi==next.moves.size()||next.moves[mi].reaction_mode_declared)
            return fail("reaction-mode needs one declared move");
        auto& m=next.moves[mi];
        m.reaction_mode=r.mode;
        m.reaction_mode_declared=true;
    }
    for(const auto& [at,id]:reaction_roots){
        line=at;const auto mi=index(next.moves,id);
        if(mi==next.moves.size()||next.moves[mi].reaction_linked_owner)
            return fail("reaction-root needs one declared move");
        next.moves[mi].reaction_linked_owner=true;
        if(!borg_battle_reaction_scope_valid(next,next.moves[mi]))
            return fail("linked-owner reaction root requires mode 4 on a direct projectile-only move");
    }
    for(const auto& r:reaction_mode){
        line=r.line;
        if(!borg_battle_reaction_scope_valid(next,next.moves[index(next.moves,r.move)]))
            return fail("reaction mode 4 requires a melee-only contact or an explicit direct projectile reaction-root");
    }
    for(const auto& [at,move]:damage_credit_ladder_moves){
        line=at;const auto mi=index(next.moves,move);
        if(mi==next.moves.size()||next.moves[mi].damage_credit_ladders)
            return fail("damage-credit-ladders needs one declared move");
        next.moves[mi].damage_credit_ladders=true;
    }
    for(const auto& r:contact_tiers){
        line=r.line;const auto mi=index(next.moves,r.move);
        if(mi==next.moves.size()||next.moves[mi].contact_tier)
            return fail("contact-tier needs one declared move");
        next.moves[mi].contact_tier=r.tier;
    }
    for(const auto& r:contact_tier_reset_wear){
        line=r.line;const auto mi=index(next.moves,r.move);
        if(mi==next.moves.size()||next.moves[mi].contact_tier_reset_wear)
            return fail("contact-tier-reset needs one declared move");
        next.moves[mi].contact_tier_reset_wear=r.wear;
    }
    for(auto& r:contact_sounds){
        line=r.line;const auto mi=index(next.moves,r.move);
        if(mi==next.moves.size()||next.moves[mi].contact_sounds[r.band])
            return fail("contact-sound needs one declared move and unique band");
        next.moves[mi].contact_sounds[r.band]=std::move(r.sound);
        if(!borg_battle_contact_sound_valid(next,next.moves[mi]))
            return fail("contact-sound needs a direct move without actor-impact retirement audio");
    }
    for(const auto& r:contact_impacts){
        line=r.line;const auto mi=index(next.moves,r.move);
        if(mi==next.moves.size())return fail("contact-impact needs a declared move");
        auto& layers=next.moves[mi].contact_impacts[r.team];
        if(layers.size()>=kBorgContactImpactLayers)return fail("contact-impact team layer limit");
        layers.push_back(r.spec);
    }
    for(const auto& move:next.moves)if(!borg_battle_contact_impact_valid(next,move))
        return fail("contact-impact needs complete team rows on a direct move without actor-impact retirement art");
    for(const auto& r:contact_pose_holds){
        line=r.line;const auto mi=index(next.moves,r.move);
        if(mi==next.moves.size()||next.moves[mi].contact_pose_hold)
            return fail("contact-pose-hold needs one declared move");
        next.moves[mi].contact_pose_hold=r.spec;
        if(!borg_battle_contact_pose_hold_valid(next,next.moves[mi]))
            return fail("contact-pose-hold needs a native direct move; bodies requires melee");
    }
    if(!borg_damage_roles_valid(next))
        return fail("invalid damage roles or direct move scope; legacy attacker rows cannot mix with split roles or contact tier");
    if(!borg_gameplay_profile_valid(next))return fail("invalid executable definition");
    *out=std::move(next);if(error)error->clear();return true;
}
}
