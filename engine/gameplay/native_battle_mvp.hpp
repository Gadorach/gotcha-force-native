#pragma once
#include "engine/assets/battle_damage_indices.hpp"
#include "engine/assets/borg_ground_pace.hpp"
//Explicit native MVP rules, NOT source battle-callback authority. Two initialized
//gameplay actors own movement, health and attacks. Presentation binds their normal
//models through the shared animation/renderer systems; prepared art is not an actor.
#include <algorithm>
#include <array>
#include <limits>
#include <optional>
#include <memory>
#include <span>
#include <utility>
#include <vector>
#include "engine/input/input_map.hpp"
#include "engine/gameplay/battle_swept_circle.hpp"
#include "engine/gameplay/battle_pair_targeting.hpp"
#include "engine/gameplay/native_borg_movement.hpp"
#include "engine/gameplay/native_borg_contact.hpp"
#include "engine/gameplay/native_battle_reaction.hpp"
#include "engine/gameplay/character_air_resource.hpp"
#include "engine/gameplay/borgs/normal_gred_movement.hpp"
#include "engine/gameplay/borg_gameplay.hpp"
#include "engine/core/source_surface_push.hpp"
namespace gf {
using BattleMvpVec=std::array<f32,3>;
enum class BattleMvpAction:u8 {Idle,Move,Jump,Attack,Defeated,Land,GroundDash,AirDash};
enum class BattleMvpDashDirection:u8 {Forward,Backward,Left,Right};
enum class BattleMvpWeapon:u8 {EnergyBeam,Bullet,Shuriken,Bomb,Orb};
enum class BattleMvpResult:u8 {Fighting,GRedWins,SasukeWins,Draw};
struct BattleMvpHitRecord {u16 type=0;u16 slot=0;bool operator==(const BattleMvpHitRecord&)const=default;};
struct BattleMvpDamageTier {
    // A zero gauge is a lazy full value: the next qualified hit uses its
    // receiver's authored capacity.
    u16 remaining=0,reset_gauge=0,reset_remaining=0;
    u8 meter=0,level=0;
    bool reset_this_tick=false;
    void advance(){
        reset_this_tick=false;
        if(remaining&&!--remaining)meter=level=0;
        if(reset_remaining&&!--reset_remaining)reset_gauge=0;
    }
    void accept(u8 charge,u16 idle_ticks,u16 capacity=0,std::optional<u8> wear={}){
        remaining=idle_ticks;
        if(capacity&&wear){
            reset_remaining=idle_ticks;
            if(!reset_this_tick){
                if(!reset_gauge)reset_gauge=capacity;
                if(*wear>=reset_gauge){
                    reset_gauge=capacity;
                    meter=level=0;
                    reset_this_tick=true;
                }else reset_gauge-=*wear;
            }
        }
        const unsigned total=unsigned(meter)+charge;
        if(total>=100){
            meter=0; // The reviewed route discards overflow, promoting once.
            level=u8(std::min(unsigned(level)+1,kBorgDamageTierCells-1));
        }else meter=u8(total);
    }
};
struct BattleMvpActor {
    static constexpr unsigned takeoff_duration=6,landing_duration=6; //native MVP phase windows
    static constexpr unsigned lab_air_windup=5,lab_air_burst=8,lab_air_recovery=6;
    static constexpr unsigned lab_glide_entry=6;
    static constexpr f32 lab_glide_sink=1.5f;
    u16 model=0;u16 damage_model=0;u8 team=0;BattleMvpVec position{};BattlePoint facing{0,1};f32 vertical_speed=0;
    unsigned health=100;bool grounded=true;BattleMvpAction action=BattleMvpAction::Idle;
    std::array<unsigned,2> cooldown{};unsigned attack_ticks=0,hit_flash=0;
    BattlePoint velocity{};f32 movement_speed=0,move_forward=0,move_right=0;
    f64 travel_distance=0;unsigned landing_ticks=0;
    BattlePoint aim_facing{0,1};f32 upper_body_yaw=0;
    // Recovered 800300BC presentation state. The angles are relative to
    // `facing`; the decay window is authored (matches hit_flash).
    static constexpr unsigned reaction_duration=8;
    f32 reaction_yaw=0,reaction_pitch=0;unsigned reaction_ticks=0;
    u8 impact_state=0; // recovered +0x2AE: 4-tick impact-record cooldown
    std::optional<BattlePoint> package_aim_heading;
    f32 package_aim_slope=0;
    bool package_target_visible=true;
    unsigned takeoff_ticks=0,dash_ticks=0,dash_duration=0,dash_cooldown=0,air_dashes_used=0;
    unsigned lab_air_windup_ticks=0,lab_air_recovery_ticks=0;
    unsigned lab_glide_entry_ticks=0;
    bool lab_gliding=false;
    bool dash_airborne=false;BattlePoint dash_direction{};BattleMvpDashDirection dash_motion=BattleMvpDashDirection::Forward;
    bool source_ground_motion=false; //historical name: source-owned motion requires an explicit source-pose consumer
    u8 package_move=BorgBattleActionState::none;
    u16 package_frame=0;
    u32 package_action_born=0;
    u16 package_charge_total=0; // authored charge window for the active move, zero when idle
    u32 package_damage_serial=0;
    u32 package_last_hit_tick=0;
    u8 package_hit_chain=0;       // successive-hit index used by damage-chain
    u8 package_hit_chain_owner=255;
    BattleMvpDamageTier damage_tier{}; // receiver-owned, independent of the legacy per-owner chain
    u16 contact_sound_parity=0;
    u8 contact_sound_cooldown=0;
    u8 contact_pose_hold_ticks=0; // presentation only; never gates movement, stock, or hit resolution
    u8 damage_kind=0;             // declared 8002FD7C relation fields
    u16 damage_attack_key=0,damage_target_mask=0;
    std::vector<u16> damage_immunity;
    unsigned damage_taken=0,damage_credited=0,hits_taken=0,hits_dealt=0;
    std::array<BattleMvpHitRecord,8> hit_history{};
    u8 hit_history_index=0,hit_history_count=0;
    unsigned hit_history_total=0;
    u8 package_movement_cues=0;
    u8 package_air_points_used=0;
    u16 package_flight_used=0;
    CharacterAirResourceState package_boost_meter{};
    u16 package_boost_age=0;
    bool package_boosting=false;
};
class IBorgBattleOrigins {
public:
    virtual ~IBorgBattleOrigins()=default;
    virtual bool socket(unsigned owner,const BattleMvpActor& actor,u32 tick,u8 move,u16 frame,
                        const BorgBattleSocket& spec,BattleMvpVec* out)const=0;
};
struct BattleMvpAnchor {BattleMvpVec position{};u32 id=0,born=0;u8 return_stock=255;};
struct BattleMvpAnchorPath {std::array<BattleMvpAnchor,kBorgBattleAnchors> points{};u8 count=0;};
struct BattleMvpFenceLink {BattleMvpVec start{},end{};};
struct BattleMvpProjectile {
    bool active=false;u32 id=0;u8 owner=0;BattleMvpWeapon weapon=BattleMvpWeapon::Bullet;
    BattleMvpVec position{},previous{},velocity{};unsigned life=0;bool landed=false;
    // Optional API-authored payload used by the same battle object pool.
    u16 package_damage=0;f32 package_gravity=0,package_radius=0;
    u8 package_move=255;BorgProjectileFloor package_floor=BorgProjectileFloor::Ignore;
    u16 package_walk_percent=0,package_walk_ticks=0;
    u16 package_pull_units=0;
    bool from_package=false,package_ignore_actors=false;
    std::optional<BorgBattleBlast> package_blast{};
    u8 package_projectile=255;
    u8 package_return_stock=255; // paid channel captured only on successful allocation
    BattlePoint package_guide_point{};
    u8 package_guide_target=255;
    u16 package_guide_ticks=0;
    u32 package_emitter=0; // nonzero for a synchronized child volley; actor still owns credit
    BattleMvpAnchorPath package_rewind_path{};
    BorgProjectileLook package_look{};
    BorgProjectileAnimation package_animation{};
    u32 born_tick=0;
    bool sustained=false;
    BattleMvpVec beam_origin{}; // owner's translation offset, or world launch point for a sweep
    f32 beam_fraction=1;
    f32 beam_power_entry=1;
    u16 beam_power_visual=100;
    f32 bomb_travel_speed=0;
    u16 bomb_fuse_tail_left=0;
    u32 power_field=0;
    u16 power_visual_percent=100;
    u32 pulling_field=0;
    std::array<BattleMvpVec,kBorgProjectileTrailSamples> record_trail_positions{};
    u8 record_trail_count=0,record_trail_cursor=0;
    bool package_direct_release=false; // ordinary move emission; not a detached emitter or spawned child
    u16 package_base_damage=0; // snapshotted at allocation, before field damage changes
    u8 package_hit_recipients=0; // per-shot history; actors keep their round slots
    bool package_ramp_deflected=false; // a shot-pull field owns subsequent velocity
};
struct BorgProjectileVisualSample {f32 scale=1,angle=0;};
inline BorgProjectileVisualSample borg_projectile_visual_sample(const BattleMvpProjectile& shot,u32 tick){
    const auto age=tick>=shot.born_tick?tick-shot.born_tick:0;
    const auto& a=shot.package_animation;
    BorgProjectileVisualSample sample;
    sample.scale=f32(shot.power_visual_percent)*.01f;
    sample.angle=f32(std::fmod(f64(age)*a.spin,6.28318530717958647692));
    if(a.pulse_period){
        const f32 phase=f32(age%a.pulse_period)/a.pulse_period;
        sample.scale*=1.f+f32(a.pulse_percent)*.01f*(1.f-std::abs(2.f*phase-1.f));
    }
    return sample;
}
struct BattleMvpImpact {BattleMvpVec position{};unsigned ticks=0;bool explosion=false;f32 radius=0;unsigned total_ticks=18;};
struct BattleMvpImpactRecord {BattleMvpVec position{};u16 ticks=0;u8 owner=0,binding=0;};
struct BorgImpactRecordAppearance {f32 scale=1,alpha=1;};
inline BorgImpactRecordAppearance borg_impact_record_appearance(u16 start,u16 end,u16 duration,u16 remaining){
    if(!end||duration<2)return {f32(start)*.01f,1};
    const auto left=std::min(remaining,duration);
    const f32 elapsed=f32(duration-left)/f32(duration-1);
    return {std::lerp(f32(start),f32(end),std::min(elapsed,1.f))*.01f,f32(left)/duration};
}
struct BattleMvpWalkPulse {
    BattleMvpVec origin{};u32 id=0,born=0;u8 owner=0;bool resolved=false;BorgWalkPulse spec;
    f32 radius(u32 tick)const{return std::lerp(f32(spec.start),f32(spec.end),f32(tick-born)/spec.ticks);}
};
struct BattleMvpSmoke {BattleMvpVec centre{};u32 parent=0,born=0;BorgSmoke spec;};
struct BattleMvpInk {u32 object=0,born=0;BorgInkOverlay spec;};
struct BattleMvpAccuracy {u32 object=0,born=0;BorgBlastAccuracy spec;};
struct BattleMvpField {
    BattleMvpVec centre{},offset{};u32 id=0,born=0;u8 owner=0,move=0;BorgField spec;BattlePoint heading{};f32 speed=0;
    bool active()const{return id&&(!spec.drift||spec.drift->active_while_moving||speed==0);}
};
inline constexpr unsigned kBattleMvpFields=8;
struct BattleMvpEmitter {
    u32 id=0;BattleMvpVec position{};
    u16 remaining=0;u8 deployment_move=BorgBattleActionState::none;
};
constexpr unsigned kBattleMvpMaxActors=kPackageBattleActorCapacity;
static_assert(kBattleMvpMaxActors<=8, "projectile recipient history width");
struct BattleMvpForcePolicy {bool enabled=false;u8 mode=0;};
struct BattleMvpTeamLedgerFlags {bool flag108=false,flag109=false,flag_f6=false;u8 counter10a=0;};
struct BattleMvpConfig {
    f32 arena_half=1800,actor_radius=42,actor_height=100,move_speed=5,jump_speed=13,gravity=.55f,step_height=45;
    unsigned health=100;
    std::array<BattlePoint,kBattleMvpMaxActors> spawn{BattlePoint{-240,300},BattlePoint{240,-300}};
    f32 ground_acceleration=.9f,ground_braking=1.4f,air_acceleration=.25f,turn_per_tick=.18f;
    f32 dash_speed=12;unsigned dash_duration=12,dash_cooldown=30,air_dashes=1;
    f32 damage_scalar=1; // battle/team scalar from the recovered settings ladder
    std::optional<BattleMvpDamageIndices> damage_indices{}; // caller-selected at round start; no scheduler
    std::array<unsigned,2> team_force{}; // recovered ledger pool; zero = untracked
    // Opt-in recovery of the 8002F8DC force gate. Disabled keeps the simple
    // always-decrement pool used by the duel.
    std::array<BattleMvpForcePolicy,2> force_policy{};
    std::array<BattleMvpTeamLedgerFlags,2> team_ledger{};
    std::array<bool,kBattleMvpMaxActors> actor_flag3e6{},actor_flag1ea{};
    bool force_global1f=false;
    //Native-only static-floor query reference, NOT an air spawn or recovered
    //actor constructor. Default0 preserves the ordinary MVP's selection.
    std::array<f32,kBattleMvpMaxActors> spawn_query_y{};
    // Standalone control lab only. Zero keeps the existing duel unchanged.
    u16 solo_model=0;
    std::array<std::shared_ptr<const BorgGameplayProfile>,kBattleMvpMaxActors> packages{};
    unsigned declared_actors=2; // duel by default; 2..kBattleMvpMaxActors
    std::array<u8,kBattleMvpMaxActors> teams{};
    bool teams_declared=false;
    bool allow_multi_source=false; // host roster with per-actor source producers
    bool source_allow_active_opponent=false; // roster source with a moving/attacking opponent
    bool provisional_walk=false;
    bool provisional_jump=false; // opt-in lab jump, not a source actor's air-resource policy
    bool provisional_ground_dash=false;
    bool provisional_air_dash=false;
    bool provisional_glide=false;
    std::optional<BorgCruiseBoost> provisional_cruise;
    std::optional<u16> provisional_ground_pace;
    bool package_strafe=false; // opt-in host control; no automatic aim or camera-driven body turn
    bool local_aim_cycle=false; // standalone authored rosters
    u8 roster_local_players=2; // the first one or two actor slots
    bool solo()const{return solo_model!=0||provisional_walk;}
};
struct BattleMvpLocalRosterInput {
    const NativeBorgMovementFrame* peer_frame=nullptr;
    u8 cycle_aim=0; // one press edge per local slot, bits 0 and 1
    std::span<const u8> cpu_aim{}; // one entry per CPU slot, in order; 255 keeps nearest-enemy aim
};
inline bool battle_mvp_damage_indices_valid(const BattleMvpConfig& config,unsigned count){
    if(!config.damage_indices)return true;
    if(count<2||!config.damage_indices->valid(count))return false;
    bool has_role=false;
    for(unsigned i=0;i<count;++i){
        const auto& p=config.packages[i];
        if(!p||(p->damage_credit.declared&&!p->damage_credit_indexed.complete())||
           (p->damage_receive.declared&&!p->damage_receive_indexed.complete()))return false;
        has_role|=p->damage_credit.declared||p->damage_receive.declared;
    }
    return has_role;
}
class NativeBattleMvp;
class NativeBorgMotionScope {
    friend class NativeBattleMvp;
    explicit NativeBorgMotionScope(const NativeBattleMvp* owner):owner_(owner){}
    const NativeBattleMvp* owner_;
};
class NativeBorgMotionProducer {
    friend class NativeBattleMvp;
    explicit NativeBorgMotionProducer(std::weak_ptr<const NativeBorgMotionScope> world,NativeBorgMotionPolicy policy):world_(std::move(world)),policy_(policy){}
    std::weak_ptr<const NativeBorgMotionScope> world_;
    NativeBorgMotionPolicy policy_=NativeBorgMotionPolicy::GroundOnly;
};
struct NativeBorgMotionObservation {
    std::weak_ptr<const NativeBorgMotionScope> scope;u32 tick=0;u16 model=0;
    BattleMvpVec position{};BattlePoint facing{};bool grounded=false;
    f32 vertical_speed=0;
};
enum class NativeBorgMotionKind:u8 {Ground,VerticalTakeoff,VerticalAir,VerticalTouchdown,VerticalLanding,JumpTakeoff,JumpAir,JumpTouchdown,JumpLanding,
    AirDashWindupEntry,AirDashWindup,AirDashActiveEntry,AirDashActive,AirDashRecovery,RelaunchEntry,RelaunchWindup,RelaunchActive,StaticPush,AirDashRecoveryPush,AirDashWindupPush,AirDashActivePush,RelaunchWindupPush,RelaunchActivePush,RelaunchEntryPush,RelaunchEntryFloor,RelaunchWindupFloor,RelaunchActiveFloor,AirDashWindupFloor,AirDashActiveEntryFloor,AirDashActiveFloor,AirDashWindupEntryFloor,AirDashWindupPositiveEntryFloor,AirDashWindupEntryPush,GroundDeparture,GroundLossEntry,GroundLossAir,GroundLossTouchdown,GroundLossPush,AirDashRecoveryFloorComplete};
inline bool native_borg_motion_ground_loss(NativeBorgMotionKind kind){return kind>=NativeBorgMotionKind::GroundDeparture&&kind<=NativeBorgMotionKind::GroundLossPush;}
inline bool native_borg_motion_relaunch_entry(NativeBorgMotionKind kind){return kind==NativeBorgMotionKind::RelaunchEntry||kind==NativeBorgMotionKind::RelaunchEntryPush||kind==NativeBorgMotionKind::RelaunchEntryFloor;}
inline bool native_borg_motion_windup_floor_entry(NativeBorgMotionKind kind){return kind==NativeBorgMotionKind::AirDashWindupEntryFloor||kind==NativeBorgMotionKind::AirDashWindupPositiveEntryFloor;}
inline bool native_borg_motion_windup_entry(NativeBorgMotionKind kind){return kind==NativeBorgMotionKind::AirDashWindupEntry||kind==NativeBorgMotionKind::AirDashWindupEntryPush||native_borg_motion_windup_floor_entry(kind);}
inline bool native_borg_motion_windup_push(NativeBorgMotionKind kind){return kind==NativeBorgMotionKind::AirDashWindupPush||kind==NativeBorgMotionKind::AirDashWindupEntryPush;}
inline bool native_borg_motion_windup_update(NativeBorgMotionKind kind){return kind==NativeBorgMotionKind::AirDashWindup||kind==NativeBorgMotionKind::AirDashWindupPush||kind==NativeBorgMotionKind::AirDashWindupFloor;}
inline bool native_borg_motion_active_entry(NativeBorgMotionKind kind){return kind==NativeBorgMotionKind::AirDashActiveEntry||kind==NativeBorgMotionKind::AirDashActiveEntryFloor;}
inline bool native_borg_motion_active_update(NativeBorgMotionKind kind){return kind==NativeBorgMotionKind::AirDashActive||kind==NativeBorgMotionKind::AirDashActivePush||kind==NativeBorgMotionKind::AirDashActiveFloor;}
inline bool native_borg_motion_recovery(NativeBorgMotionKind kind){return kind==NativeBorgMotionKind::AirDashRecovery||kind==NativeBorgMotionKind::AirDashRecoveryPush;}
inline bool native_borg_motion_relaunch_windup(NativeBorgMotionKind kind){return kind==NativeBorgMotionKind::RelaunchWindup||kind==NativeBorgMotionKind::RelaunchWindupPush||kind==NativeBorgMotionKind::RelaunchWindupFloor;}
inline bool native_borg_motion_relaunch_active(NativeBorgMotionKind kind){return kind==NativeBorgMotionKind::RelaunchActive||kind==NativeBorgMotionKind::RelaunchActivePush||kind==NativeBorgMotionKind::RelaunchActiveFloor;}
inline bool native_borg_motion_relaunch_floor(NativeBorgMotionKind kind){return kind==NativeBorgMotionKind::RelaunchEntryFloor||kind==NativeBorgMotionKind::RelaunchWindupFloor||kind==NativeBorgMotionKind::RelaunchActiveFloor;}
inline bool native_borg_motion_air_dash_floor(NativeBorgMotionKind kind){return kind==NativeBorgMotionKind::AirDashWindupFloor||kind==NativeBorgMotionKind::AirDashActiveEntryFloor||kind==NativeBorgMotionKind::AirDashActiveFloor||native_borg_motion_windup_floor_entry(kind);}
inline bool native_borg_motion_relaunch_push(NativeBorgMotionKind kind){return kind==NativeBorgMotionKind::RelaunchWindupPush||kind==NativeBorgMotionKind::RelaunchActivePush||kind==NativeBorgMotionKind::RelaunchEntryPush;}
inline bool native_borg_motion_push(NativeBorgMotionKind kind){return kind==NativeBorgMotionKind::StaticPush||kind==NativeBorgMotionKind::AirDashRecoveryPush||native_borg_motion_windup_push(kind)||kind==NativeBorgMotionKind::AirDashActivePush||native_borg_motion_relaunch_push(kind)||kind==NativeBorgMotionKind::GroundLossPush;}
inline bool native_borg_motion_air_dash(NativeBorgMotionKind kind){return (unsigned(kind)>=9&&unsigned(kind)<=13)||kind==NativeBorgMotionKind::AirDashRecoveryPush||native_borg_motion_windup_push(kind)||kind==NativeBorgMotionKind::AirDashActivePush||native_borg_motion_air_dash_floor(kind)||kind==NativeBorgMotionKind::AirDashRecoveryFloorComplete;}
inline bool native_borg_motion_relaunch(NativeBorgMotionKind kind){return (unsigned(kind)>=14&&unsigned(kind)<=16)||native_borg_motion_relaunch_push(kind)||native_borg_motion_relaunch_floor(kind);}
inline bool native_gred_relaunch_acceleration_valid(f32 value){
    f32 cosine=0;source_heading_xz(8192,nullptr,&cosine);const f32 neutral=bits_to_f32(0x3f4ccccdu);
    return value==neutral||value==std::fma(neutral,cosine,0.f);
}
// Exact soft-Up branch of the qualified normal G-Red ascent turn (gain1).
// Recover the stored body angle without accepting a tolerance on travel:
// bounded atan2 can truncate one unit away from the exact forward vector.
// Charge-gauge progress for an active authored charge window. Elapsed is
// clamped to the total, so a move that outlives its window reports full
// rather than overflowing, and an idle actor reports zero.
// Filled cells for an HP bar of `cells` divisions. A zero maximum reports an
// empty bar rather than dividing by zero, and the result never exceeds the bar.
inline unsigned battle_health_cells(u32 current,u32 maximum,unsigned cells){
    if(!maximum||!cells)return 0;
    const u64 scaled=u64(current)*cells;
    return unsigned(std::min<u64>(cells,scaled/maximum));
}

struct BattleChargeGauge { u16 total=0, elapsed=0; };
inline BattleChargeGauge battle_charge_gauge(u16 total,u32 born,u32 tick){
    BattleChargeGauge gauge;
    if(!total)return gauge;
    gauge.total=total;
    const u32 spent=tick>=born?tick-born:0;
    gauge.elapsed=u16(spent<total?spent:total);
    return gauge;
}

inline bool native_gred_relaunch_up_turn(const NativeBorgMovementFrame& camera,BattlePoint before,s16 travel,BattlePoint after){
    CharacterMovementHeadingFrame frame;
    if(!native_borg_source_heading_frame(camera,before,&frame))return false;
    const auto desired=source_heading_wrap(s32(frame.camera5b0)-0x8000);
    for(int d=-1;d<=1;++d){
        const auto body=source_heading_wrap(s32(frame.body72)+d);
        BattlePoint facing;source_heading_xz(body,&facing.x,&facing.z);
        if(facing!=before)continue;
        const auto delta=source_heading_wrap(s32(desired)-body);
        const auto turned=source_heading_wrap(s32(body)+s32(f32(f32(delta)*bits_to_f32(0x3d4ccccdu))));
        source_heading_xz(turned,&facing.x,&facing.z);
        if(turned==travel&&facing==after)return true;
    }
    return false;
}
inline unsigned native_borg_motion_phase(NativeBorgMotionKind kind){
    const unsigned value=unsigned(kind);
    return value<=4?value:value<=8?value-4:(value<=35||kind==NativeBorgMotionKind::GroundLossPush||
        kind==NativeBorgMotionKind::AirDashRecoveryFloorComplete)?2:value==36?3:255;
}
struct NativeBorgStaticPush {
    BattleMvpVec integrated_position{};f32 vertical_speed=0;
    NativeBorgStaticSurface surface;s16 angle=0;f32 damping=0;
    std::optional<f32> relaunch_acceleration{}; //relaunch pushes only; absence required on every other push kind
    bool operator==(const NativeBorgStaticPush&)const=default;
};
struct NativeBorgRelaunchFloor {
    BattleMvpVec integrated_position{};f32 vertical_speed=0,acceleration=0,response_factor=0;
    NativeBorgStaticSurface surface;s16 angle=0;f32 timer698=0;
    bool operator==(const NativeBorgRelaunchFloor&)const=default;
};
struct NativeBorgAirDashFloor {
    BattleMvpVec integrated_position{};f32 vertical_speed=0,acceleration=0;
    NativeBorgStaticSurface surface;s16 angle=0;f32 timer698=0;
    bool operator==(const NativeBorgAirDashFloor&)const=default;
};
struct NativeBorgWalkLedge {
    BattleMvpVec frame_start{};NativeBorgStaticSurface surface;
    f32 timer698=0,speed44=0;s16 travel5ae=0;std::optional<s16> drift_heading{};
    bool walk_origin=false; // Only the scoped continued walking-fall push policy carries this lineage.
    bool operator==(const NativeBorgWalkLedge&)const=default;
};
//Sealed by the qualified movement owner AFTER the actual driver operation and
//shared root evaluation. Offers hold weak leases, not public float authority.
class NativeBorgOrderedRootScope {
    friend class NativeGRedGroundDashDemo;
    friend class NativeBattleMvp;
    NativeBorgOrderedRootScope()=default;
    NativeBorgOrderedRootScope(const NativeBorgOrderedRootScope&)=delete;
    NativeBorgOrderedRootScope& operator=(const NativeBorgOrderedRootScope&)=delete;
    std::weak_ptr<const NativeBorgMotionScope> world;
    std::weak_ptr<const NativeBorgMotionProducer> producer;
    u32 tick=0;u64 updates=0;f32 speed=0;bool evaluated=false,program_done=false;
    u8 phase=0,control_before=0,control_after=0;
};
struct NativeBorgOrderedGround {
    u8 phase=12,next_selector=13,coarse57d=0,control_before=0,control_after=0,recovery546=0,next546=0;
    f32 speed_before=0,move_speed=0,retained_speed=0,change4c=0,next_change4c=0,root_speed=0,timer568=0;
    s16 travel5ae=0;u32 held5b4=0;u64 root_updates=0;
    bool root_sampled=false,root_evaluated=false,program_done=false;
    std::weak_ptr<const NativeBorgOrderedRootScope> root_scope;
    bool operator==(const NativeBorgOrderedGround& b)const{
        return phase==b.phase&&next_selector==b.next_selector&&coarse57d==b.coarse57d&&
            control_before==b.control_before&&control_after==b.control_after&&recovery546==b.recovery546&&next546==b.next546&&
            speed_before==b.speed_before&&move_speed==b.move_speed&&retained_speed==b.retained_speed&&change4c==b.change4c&&
            next_change4c==b.next_change4c&&root_speed==b.root_speed&&timer568==b.timer568&&travel5ae==b.travel5ae&&
            held5b4==b.held5b4&&root_updates==b.root_updates&&root_sampled==b.root_sampled&&root_evaluated==b.root_evaluated&&program_done==b.program_done&&
            !root_scope.owner_before(b.root_scope)&&!b.root_scope.owner_before(root_scope);
    }
};
struct NativeBorgJumpMotion {
    u8 phase=4,next_selector=4,ascent_stage=0;
    bool entry=false,direction=false,ground_loss=false,lower_done=false,held_jump=false,air_dash_origin=false,relaunch_origin=false;
    u8 relaunch_coarse57d=0;
    f32 speed_before=0,move_speed=0,speed44=0,vertical_before=0,planar_accel=0,vertical_accel=0;
    s16 travel5ae=0,resource_before=0;std::optional<s16> drift_heading;
    bool timed_walk_relaunch_fall=false; //Accepted timed fall from a qualified tap, through its scoped contacts.
    bool second_timed_release_push=false; //One same-face successor to the accepted positive-timer push.
    bool third_timed_release_push=false; //Verified final successor on face 2021.
    bool down_timed_release_push=false; //Separate steered successor from face 2021 to face 2011.
    bool expiry_same_face_push=false; //Approach and projected exit stay above competing floors after the Down-drift receipt.
    bool operator==(const NativeBorgJumpMotion&)const=default;
};
struct NativeBorgAirMotion {
    u8 phase=15,next_selector=15,stage_before=0,next_stage=1,control=0;
    bool entry=false,callback=false,lower_done=false;
    f32 speed_before=0,move_speed=0,speed44=0,vertical_before=0,planar_accel=0,vertical_accel=0,timer_before=0,timer568=0;
    s16 travel5ae=0,resource_before=0,resource_after=0;std::optional<s16> drift_heading;
    bool walk_expiry_windup=false; //Scoped uninterrupted plain wind-up from an accepted walking expiry push.
    u8 expiring_walk_windup_step=0; //Ordered, accepted 1 -> 0 wind-up sequence; zero if not qualified.
    bool expiring_walk_windup_short=false; //The independently qualified five-tick variant.
    u8 expiring_snap_windup_step=0; //Sequence from the same-height snap, never a plain-entry history.
    bool timed_relaunch_floor_windup=false; //Uninterrupted flat-floor wind-up that entered directly from a timed descending relaunch snap.
    bool rising_relaunch_windup=false; //Qualified plain wind-up directly from neutral, powered ascent at zero698.
    bool operator==(const NativeBorgAirMotion&)const=default;
};
struct NativeBorgRelaunchMotion {
    u8 lower_before=0,upper_before=0,lower_after=1,upper_after=1,lower_program=14,upper_program=14,coarse57d=0;
    bool entry=false,direction=false,lower_done=false;
    f32 speed_before=0,move_speed=0,speed44=0,vertical_before=0,planar_accel=0,vertical_accel=0;
    s16 travel5ae=0,resource_before=0,resource_after=0;
    bool timed_walk_relaunch_tap=false; //Plain neutral entry from an accepted timed walking ledge fall.
    bool operator==(const NativeBorgRelaunchMotion&)const=default;
};
struct NativeBorgSurfaceLanding {
    BattleMvpVec integrated_position{};f32 vertical_speed=0,acceleration=0;
    NativeBorgStaticSurface surface{};s16 angle=0;f32 timer698=0;
    bool operator==(const NativeBorgSurfaceLanding&)const=default;
};
// The qualified producer seals the actual lower-lane result and, in the
// drift opt-in, any soft cardinal input used on either contact update.
class NativeBorgRecoveryCompletionScope {
    friend class NativeGRedGroundDashDemo;
    friend class NativeBattleMvp;
    NativeBorgRecoveryCompletionScope()=default;
    NativeBorgRecoveryCompletionScope(const NativeBorgRecoveryCompletionScope&)=delete;
    NativeBorgRecoveryCompletionScope& operator=(const NativeBorgRecoveryCompletionScope&)=delete;
    std::weak_ptr<const NativeBorgMotionScope> world;
    std::weak_ptr<const NativeBorgMotionProducer> producer;
    u32 tick=0;u64 serial=0;bool lower_done=false;
    NativeBorgMotionKind kind=NativeBorgMotionKind::Ground;
    std::optional<s16> drift_offset;
};
inline bool native_gred_walk_speed(f32 speed){return speed==0||speed==5||speed==7||speed==8||speed==9||speed==10||speed==12;}
inline bool native_borg_flat_surface(const NativeBorgStaticSurface& s){return std::isfinite(s.height)&&s.normal==BattleMvpVec{0,1,0};}
inline bool native_borg_static_surface_valid(const NativeBorgStaticSurface& s){
    if(!std::isfinite(s.height)||s.normal[1]<=0)return false;
    for(auto v:s.normal)if(!std::isfinite(v)||std::abs(v)>1e7f)return false;
    return true;
}
inline BattleMvpVec native_borg_ground_cross(const BattleMvpVec& a,const BattleMvpVec& b){
    return {f32(a[1]*b[2]-a[2]*b[1]),f32(a[2]*b[0]-a[0]*b[2]),f32(a[0]*b[1]-a[1]*b[0])};
}
inline bool native_gred_ground_position(const BattleMvpVec& position,f32 speed,s16 travel,const BattleMvpVec& normal,BattleMvpVec* out){
    if(!out||!std::isfinite(speed)||speed<=0||speed>1e5f)return false;
    f32 x=0,z=0;source_heading_xz(travel,&x,&z);const f32 vx=f32(speed*x),vz=f32(speed*z);
    const auto tangent=native_borg_ground_cross(native_borg_ground_cross(normal,{vx,0,vz}),normal);
    const f64 length=std::hypot(f64(tangent[0]),f64(tangent[1]),f64(tangent[2]));if(!std::isfinite(length)||!length)return false;
    *out={f32(position[0]+f32(f32(tangent[0]/length)*speed)),position[1],f32(position[2]+f32(f32(tangent[2]/length)*speed))};return true;
}
inline bool native_gred_walk_position(const BattleMvpVec& position,f32 speed,s16 travel,const BattleMvpVec& normal,BattleMvpVec* out){
    return native_gred_walk_speed(speed)&&native_gred_ground_position(position,speed,travel,normal,out);
}
struct NativeBorgMotionOffer {
    NativeBorgMotionObservation observed;BattleMvpVec proposed_position{};BattlePoint facing{};
    BattleMvpAction action=BattleMvpAction::Idle;u64 serial=0;
    std::weak_ptr<const NativeBorgMotionProducer> producer{};
    NativeBorgMotionKind kind=NativeBorgMotionKind::Ground;f32 vertical_speed=0;
    f32 entry_vertical_delta=0; //declared additive relaunch step only; every other kind requires0
    std::optional<NativeBorgStaticPush> static_push{};
    std::optional<NativeBorgRelaunchFloor> relaunch_floor{};
    std::optional<NativeBorgAirDashFloor> air_dash_floor{};
    std::optional<NativeBorgWalkLedge> walk_ledge{};
    std::optional<NativeBorgOrderedGround> ordered_ground{};
    std::optional<NativeBorgJumpMotion> jump_motion{};
    u8 landing_target=0; //4/12 only in the explicitly scoped accepted-landing-entry policy
    std::optional<NativeBorgAirMotion> air_motion{};
    std::optional<NativeBorgRelaunchMotion> relaunch_motion{};
    std::optional<NativeBorgSurfaceLanding> surface_landing{};
    std::optional<f32> controller_timer698{};
    std::weak_ptr<const NativeBorgRecoveryCompletionScope> recovery_completion{};
    std::optional<NativeBorgStaticSurface> recovery_first_seam_start{};
};
class NativeBorgMotionReceipt {
    friend class NativeBattleMvp;
    std::weak_ptr<const NativeBorgMotionScope> scope_;u32 tick_=0;u64 serial_=0;
    BattleMvpVec position_{};BattlePoint facing_{};NativeBorgContactResult contact_;
    std::weak_ptr<const NativeBorgMotionProducer> producer_;
    bool grounded_=false;f32 vertical_speed_=0;
public:
    const NativeBorgContactResult& contact()const{return contact_;}
    u32 tick()const{return tick_;}u64 serial()const{return serial_;}
};
class NativeBattleMvp {
public:
    static constexpr unsigned tick_rate=60,max_projectiles=128;
    bool start(const BattleMvpConfig& config={},const IBattleMvpTerrain* terrain=nullptr){
        if(config.solo_model&&config.solo_model!=0x0615&&config.solo_model!=0x000a)return false;
        for(f32 v:{config.arena_half,config.actor_radius,config.actor_height,config.move_speed,config.jump_speed,config.gravity,config.step_height})if(!std::isfinite(v)||v<=0)return false;
        if(config.arena_half>10000||config.actor_radius*4>=config.arena_half||config.actor_height>1000||config.move_speed>config.actor_radius||config.jump_speed>50||config.gravity<.05f||config.gravity>10||config.step_height>config.actor_height||!config.health||config.health>10000)return false;
        for(f32 v:{config.ground_acceleration,config.ground_braking,config.air_acceleration,config.turn_per_tick})if(!std::isfinite(v)||v<=0)return false;
        if(config.ground_acceleration>config.move_speed*2||config.ground_braking>config.move_speed*2||config.air_acceleration>config.ground_acceleration||config.turn_per_tick>3.141593f)return false;
        if(!std::isfinite(config.dash_speed)||config.dash_speed<config.move_speed||config.dash_speed>config.actor_radius||!config.dash_duration||config.dash_duration>60||config.dash_cooldown<config.dash_duration||config.dash_cooldown>600||config.air_dashes>4)return false;
        if(config.declared_actors<2||config.declared_actors>kBattleMvpMaxActors)return false;
        const bool package=bool(config.packages[0]);
        const unsigned count=config.solo()?1u:config.declared_actors;
        if(!battle_mvp_damage_indices_valid(config,count))return false;
        for(unsigned i=0;i<count;++i)if(bool(config.packages[i])!=package)return false;
        for(unsigned i=count;i<kBattleMvpMaxActors;++i)if(config.packages[i])return false;
        if((config.roster_local_players!=1&&config.roster_local_players!=2)||
           (config.roster_local_players==1&&!config.local_aim_cycle))return false;
        if(config.local_aim_cycle&&(!package||count<=2||config.allow_multi_source||
           config.packages[0]->movement==BorgBattleMovement::NormalGRed))return false;
        for(unsigned i=0;i<count;++i)if(config.teams_declared&&config.teams[i]>1)return false;
        if((package&&config.solo())||(config.provisional_walk&&config.solo_model)||
           ((config.provisional_jump||config.provisional_ground_dash)&&!config.provisional_walk)||
           ((config.provisional_air_dash||config.provisional_glide)&&!config.provisional_jump)||
            (config.provisional_glide&&config.provisional_air_dash)||
            (config.provisional_ground_pace&&(!config.provisional_walk||config.provisional_cruise||
              !borg_lab_ground_pace_valid(*config.provisional_ground_pace)||f32(*config.provisional_ground_pace)*.01f>config.actor_radius))||
             (config.package_strafe&&!package)||
             (config.provisional_cruise&&(!config.provisional_walk||config.provisional_jump||
             config.provisional_ground_dash||!borg_cruise_boost_valid(*config.provisional_cruise))))return false;
        if(package&&!config.allow_multi_source){
            // A single-source host keeps the stationary-opponent rule. A
            // roster host sets allow_multi_source and supplies one offer per
            // source actor.
            for(unsigned i=1;i<count;++i)if(config.packages[i]->movement==BorgBattleMovement::NormalGRed)return false;
            if(config.packages[0]->movement==BorgBattleMovement::NormalGRed)
                for(unsigned i=1;i<count;++i)if(config.packages[i]->movement!=BorgBattleMovement::Stationary)return false;
        }
        NativeBattleMvp next;next.config_=config;next.team_force_=config.team_force;
        next.force_global1f_=config.force_global1f;
        for(unsigned team=0;team<2;++team)next.force_counter10a_[team]=config.team_ledger[team].counter10a;
        if(package)for(unsigned i=0;i<count;++i){
            const auto& p=*config.packages[i];
            if(!borg_gameplay_profile_valid(p))return false;
            if(std::any_of(p.projectiles.begin(),p.projectiles.end(),[](const auto& shot){
                 return borg_projectile_uses_stage(shot.floor)||shot.attraction.has_value();
            })&&(!terrain||!terrain->supports_projectile_stage()))return false;
            next.package_actions_[i].reset(p);
        }
        for(unsigned i=0;i<count;++i){
            const auto p=config.spawn[i];if(!std::isfinite(p.x)||!std::isfinite(p.z)||std::abs(p.x)>config.arena_half-config.actor_radius||std::abs(p.z)>config.arena_half-config.actor_radius)return false;
            f32 clearance=0;
            if(!native_borg_partition_portion(terrain,p,{},config.actor_radius,&clearance)||clearance<1)return false;
            if(!std::isfinite(config.spawn_query_y[i])||std::abs(config.spawn_query_y[i])>20000)return false;
            auto& actor=next.actors_[i];actor.model=package?
                (config.packages[i]->movement==BorgBattleMovement::NormalGRed?0x0615:0):
                config.solo()?config.solo_model:i?0x000a:0x0615;
            actor.damage_model=package&&config.packages[i]->damage_model_declared?
                config.packages[i]->damage_model:actor.model;
            if(package){
                actor.damage_kind=config.packages[i]->damage_kind;
                actor.damage_attack_key=config.packages[i]->damage_attack_key;
                actor.damage_target_mask=config.packages[i]->damage_target_mask;
                actor.damage_immunity=config.packages[i]->damage_immunity;
            }
            actor.team=config.teams_declared?config.teams[i]:u8(i%2);
            actor.position={p.x,0,p.z};actor.health=package?config.packages[i]->health:config.health;
            if(!floor(terrain,p.x,p.z,config.spawn_query_y[i],&actor.position[1]))return false;
            const auto& cruise=package?config.packages[i]->cruise:config.provisional_cruise;
            if(cruise){
                actor.position[1]+=cruise->height;actor.grounded=false;actor.action=BattleMvpAction::Jump;
                actor.package_boost_meter.value736=s16(cruise->capacity);
            }
        }
        if(config.solo()){
            next.actors_[1].health=0;
            next.actors_[0].facing={0,-1};
        }else{
            for(unsigned i=0;i<count;++i)for(unsigned j=0;j<i;++j){
                const f32 dx=config.spawn[i].x-config.spawn[j].x,dz=config.spawn[i].z-config.spawn[j].z;
                if(dx*dx+dz*dz<=4*config.actor_radius*config.actor_radius)return false;
            }
            for(unsigned i=0;i<count;++i)next.face(i);
        }
        for(unsigned i=0;i<count;++i)next.actors_[i].aim_facing=next.actors_[i].facing;
        next.motion_scope_=std::shared_ptr<const NativeBorgMotionScope>(new NativeBorgMotionScope(this));
        next.ready_=true;*this=next;return true;
    }
    //One fixed60Hz tick. A/B/X = primary/jump/secondary, Y = restart,
    //L = ground/air dash, Start = pause. Second-pad control is available without AI.
    bool step(std::array<GamePadState,2> pads,bool opponent_ai=true,const IBattleMvpTerrain* terrain=nullptr,const std::array<std::optional<BattleMvpVec>,2>* muzzles=nullptr,const NativeBorgMovementFrame* player_frame=nullptr,const NativeBorgMotionOffer* source_offer=nullptr,
              const std::array<BorgBattleInputs,2>& package_input={},const IBorgBattleOrigins* origins=nullptr){
        std::array<BorgBattleInputs,kBattleMvpMaxActors> inputs{};
        inputs[0]=package_input[0];inputs[1]=package_input[1];
        std::array<std::optional<BattleMvpVec>,kBattleMvpMaxActors> muzzle_buffer{};
        if(muzzles){muzzle_buffer[0]=(*muzzles)[0];muzzle_buffer[1]=(*muzzles)[1];}
        const std::span<const std::optional<BattleMvpVec>> muzzle_span=
            muzzles?std::span<const std::optional<BattleMvpVec>>(muzzle_buffer.data(),2):std::span<const std::optional<BattleMvpVec>>{};
        return step_roster(pads,std::span<const BorgBattleInputs>(inputs.data(),2),opponent_ai,
                           terrain,muzzle_span,player_frame,source_offer,origins);
    }
    // Roster tick. `pads` and `package_input` must supply actor_count() entries.
    // Authored local rosters can supply a camera basis for pad 1 as well as
    // pad 0. CPU pads and borrowed source-motion offers keep their own inputs.
    bool step_roster(std::span<const GamePadState> pads,std::span<const BorgBattleInputs> package_input,
                      bool opponent_ai=true,const IBattleMvpTerrain* terrain=nullptr,
                      std::span<const std::optional<BattleMvpVec>> muzzles={},
                      const NativeBorgMovementFrame* player_frame=nullptr,
                      const NativeBorgMotionOffer* source_offer=nullptr,
                      const IBorgBattleOrigins* origins=nullptr,
                      std::span<const NativeBorgMotionOffer* const> source_offers={},
                      BattleMvpLocalRosterInput local_input={}){
        if(!ready_||tick_==std::numeric_limits<u32>::max())return false;
        const bool solo=config_.solo();
        const bool package=bool(config_.packages[0]);
        if(pads.size()<actor_count()||pads.size()>kBattleMvpMaxActors||package_input.size()<actor_count())return false;
        if(!muzzles.empty()&&muzzles.size()<actor_count())return false;
        std::array<GamePadState,kBattleMvpMaxActors> pad_store{};
        for(unsigned i=0;i<pads.size()&&i<kBattleMvpMaxActors;++i)pad_store[i]=pads[i];
        pads=std::span<const GamePadState>(pad_store.data(),pads.size());
        if(package&&opponent_ai)return false; // inputs supplied by Battle Test
        if(!package)for(unsigned i=0;i<actor_count();++i)if(package_input[i]!=BorgBattleInputs{})return false;
        for(unsigned i=0;i<actor_count();++i)if(!package_input[i].valid())return false;
        if(solo)for(unsigned i=1;i<pads.size();++i)if(pads[i]!=GamePadState{})return false;
        if(solo&&opponent_ai)return false;
        auto offered=[&](unsigned i)->const NativeBorgMotionOffer*{
            if(!source_offers.empty())return i<source_offers.size()?source_offers[i]:nullptr;
            return i==0?source_offer:nullptr;
        };
        if(!source_offers.empty()&&source_offers.size()<actor_count())return false;
        if(config_.provisional_walk){
            if(offered(0))return false;
            pad_store[0].a=pad_store[0].x=false;
            if(!config_.provisional_ground_dash&&!config_.provisional_air_dash)pad_store[0].l=pad_store[0].trigger_l_pressed=false;
            if(!config_.provisional_jump&&!config_.provisional_cruise)pad_store[0].b=false;
        }
        if(player_frame&&!native_borg_movement_frame_valid(*player_frame))return false;
        const unsigned local_count=config_.roster_local_players;
        const unsigned local_mask=(1u<<local_count)-1u;
        if((local_input.cycle_aim&~local_mask)||(local_input.cycle_aim&&!config_.local_aim_cycle))return false;
        if(!local_input.cpu_aim.empty()){
            if(!config_.local_aim_cycle||!terrain||terrain->body_partitions().empty()||
               source_offer||!source_offers.empty()||local_input.cpu_aim.size()!=actor_count()-local_count)return false;
            for(unsigned i=0;i<local_input.cpu_aim.size();++i){
                const u8 target=local_input.cpu_aim[i];
                if(target!=255&&(!actors_[i+local_count].health||!aim_eligible(i+local_count,target)))return false;
            }
        }
        const auto* peer_frame=local_input.peer_frame;
        if(peer_frame&&(local_count!=2||!package||actor_count()<=2||config_.allow_multi_source||
           config_.packages[0]->movement==BorgBattleMovement::NormalGRed||source_offer||!source_offers.empty()||
           !native_borg_movement_frame_valid(*peer_frame)))return false;
        for(unsigned i=0;i<muzzles.size();++i)if(muzzles[i])for(f32 v:*(muzzles[i]))if(!std::isfinite(v)||std::abs(v)>1e5f)return false;
        const auto human=pads[0];
        if(human.y&&!previous_[0].y){const auto config=config_;if(!start(config,terrain))return false;previous_[0]=human;return true;}
        if(human.start&&!previous_[0].start){set_paused(!paused_);remember_pads(pads);return true;}
        if(paused_||result_!=BattleMvpResult::Fighting){remember_pads(pads);return true;}
        for(unsigned i=0;i<actor_count();++i)
            if(!offered(i)&&(actors_[i].source_ground_motion||config_.solo_model==0x0615||
                (package&&config_.packages[i]->movement==BorgBattleMovement::NormalGRed)))return false; //no legacy fallback
        auto prepare_source = [&](unsigned source_index, const NativeBorgMotionOffer& o,
                                  BattleMvpActor& actor, const BattleMvpActor& other,
                                  const GamePadState& pad, const GamePadState& other_pad,
                                  NativeBorgContactResult* contact) -> bool {
            const auto producer=o.producer.lock();
            if(!producer||producer->world_.lock()!=motion_scope_||(actor.source_ground_motion&&producer!=motion_producers_[source_index].lock()))return false;
            const bool vertical=o.kind!=NativeBorgMotionKind::Ground;
            const bool moving=unsigned(o.kind)>=unsigned(NativeBorgMotionKind::JumpTakeoff);
            const bool air_dash=native_borg_motion_air_dash(o.kind),relaunch=native_borg_motion_relaunch(o.kind);
            const bool recovery_push=o.kind==NativeBorgMotionKind::AirDashRecoveryPush;
            const bool windup_entry_push=o.kind==NativeBorgMotionKind::AirDashWindupEntryPush;
            const bool windup_push=native_borg_motion_windup_push(o.kind);
            const bool active_push=o.kind==NativeBorgMotionKind::AirDashActivePush;
            const bool relaunch_entry_push=o.kind==NativeBorgMotionKind::RelaunchEntryPush;
            const bool relaunch_push=native_borg_motion_relaunch_push(o.kind);
            const bool relaunch_floor=native_borg_motion_relaunch_floor(o.kind);
            const bool air_floor=native_borg_motion_air_dash_floor(o.kind);
             const bool recovery_floor_complete=o.kind==NativeBorgMotionKind::AirDashRecoveryFloorComplete;
             const bool floor_snap=relaunch_floor||air_floor||recovery_floor_complete;
            const bool push=native_borg_motion_push(o.kind);
            const bool positive_windup_entry=o.kind==NativeBorgMotionKind::AirDashWindupPositiveEntryFloor;
            const bool windup_entry_floor=native_borg_motion_windup_floor_entry(o.kind);
            const bool windup_entry_push_policy=producer->policy_==NativeBorgMotionPolicy::WindupEntryPush;
            using Behavior=BorgMovementBehavior;
             const auto behaviors=NormalGRedMovementDefinition::behaviors(producer->policy_);
             const bool recovery_recontact_policy=behaviors.has(Behavior::RecoveryFloorRecontact);
             const bool recovery_drift_policy=behaviors.has(Behavior::RecoveryRecontactDrift);
             const bool recovery_seam_policy=behaviors.has(Behavior::RecoveryRecontactSeam);
             const bool recovery_first_seam_policy=behaviors.has(Behavior::RecoveryFirstContactSeam);
            constexpr const auto& meter=NormalGRedMovementDefinition::air_meter;
            const bool slope_seam_dash_policy=behaviors.has(Behavior::CoplanarSlopeDash);
            const bool slope_dash_policy=behaviors.has(Behavior::SlopeDash);
            const bool controller_windup_entry_push_policy=behaviors.has(Behavior::WindupEntryPush);
            const bool controller_relaunch_entry_push_policy=behaviors.has(Behavior::RelaunchEntryPush);
            const bool controller_relaunch_push_policy=behaviors.has(Behavior::RelaunchPush);
            const bool controller_active_push_policy=behaviors.has(Behavior::ActivePush);
            const bool controller_windup_push_policy=behaviors.has(Behavior::WindupPush);
            const bool controller_recovery_push_policy=behaviors.has(Behavior::RecoveryPush);
            const bool controller_relaunch_fall_push_policy=behaviors.has(Behavior::ReleasedRelaunchPush);
            const bool controller_air_fall_push_policy=behaviors.has(Behavior::PostAirFallPush);
            const bool controller_fall_push_policy=behaviors.has(Behavior::FallPush);
            const bool controller_ground_loss_relaunch_policy=behaviors.has(Behavior::RetainedTimerRelaunch);
            const bool controller_ground_loss_air_policy=behaviors.has(Behavior::RetainedTimerAirDash);
            const bool controller_landing_dash_policy=behaviors.has(Behavior::RetainedTimerLandingDash);
            const bool controller_landing_jump_policy=behaviors.has(Behavior::RetainedTimerLandingJump);
            const bool controller_ground_loss_surface_policy=behaviors.has(Behavior::GroundLossSurface);
            const bool controller_surface_policy=behaviors.has(Behavior::PairedSurfaceLanding);
            const bool controller_windup_entry_floor_policy=behaviors.has(Behavior::WindupEntryFloor);
            const bool controller_relaunch_floor_policy=behaviors.has(Behavior::RelaunchFloor);
            const bool controller_relaunch_policy=behaviors.has(Behavior::Relaunch);
            const bool controller_floor_policy=behaviors.has(Behavior::AirDashFloor);
            const bool positive_windup_policy=producer->policy_==NativeBorgMotionPolicy::WindupPositiveFloor||windup_entry_push_policy;
            const bool windup_entry_floor_policy=producer->policy_==NativeBorgMotionPolicy::WindupEntryFloor||positive_windup_policy;
            const bool air_floor_policy=producer->policy_==NativeBorgMotionPolicy::AirDashFloor||windup_entry_floor_policy;
            const bool floor_policy=producer->policy_==NativeBorgMotionPolicy::RelaunchFloor||air_floor_policy;
            const bool relaunch_entry_push_policy=producer->policy_==NativeBorgMotionPolicy::RelaunchEntryPush||floor_policy;
            const bool relaunch_push_policy=producer->policy_==NativeBorgMotionPolicy::RelaunchPush||relaunch_entry_push_policy;
            const bool active_push_policy=producer->policy_==NativeBorgMotionPolicy::ActivePush||relaunch_push_policy;
            const bool windup_push_policy=producer->policy_==NativeBorgMotionPolicy::WindupPush||active_push_policy;
            const bool recovery_push_policy=producer->policy_==NativeBorgMotionPolicy::RecoveryPush||windup_push_policy;
            const bool history_push_policy=producer->policy_==NativeBorgMotionPolicy::FallHistoryPush||recovery_push_policy;
            const bool push_policy=producer->policy_==NativeBorgMotionPolicy::StaticPush||history_push_policy;
            const bool controller_air_policy=behaviors.has(Behavior::AirDash);
            const bool landing_policy=behaviors.has(Behavior::LandingInterrupt);
            const bool jump_policy=behaviors.has(Behavior::Jump);
            const bool combined_ledge_policy=behaviors.has(Behavior::WalkLedge);
            const bool dash_ledge_policy=behaviors.has(Behavior::DashLedge);
            const bool ledge_policy=producer->policy_==NativeBorgMotionPolicy::WalkLedge||dash_ledge_policy;
             const bool ordered_policy=behaviors.has(Behavior::OrderedGround);
             const bool departure=o.kind==NativeBorgMotionKind::GroundDeparture;
             const bool ground_loss=native_borg_motion_ground_loss(o.kind);
             const bool ground_loss_push=o.kind==NativeBorgMotionKind::GroundLossPush;
             const bool walk_loss_push_policy=behaviors.has(Behavior::WalkLedgeFallPush);
             const bool controller_fall_push_frame=controller_fall_push_policy&&o.kind==NativeBorgMotionKind::StaticPush;
             if(o.jump_motion&&o.jump_motion->expiry_same_face_push&&!controller_fall_push_frame)return false;
            const bool controller_recovery_push_frame=controller_recovery_push_policy&&recovery_push;
            const bool controller_windup_push_frame=controller_windup_push_policy&&o.kind==NativeBorgMotionKind::AirDashWindupPush;
            const bool controller_windup_entry_push_frame=controller_windup_entry_push_policy&&windup_entry_push;
            const bool controller_active_push_frame=controller_active_push_policy&&active_push;
            const bool controller_air_push_frame=controller_recovery_push_frame||controller_windup_push_frame||
                controller_windup_entry_push_frame||controller_active_push_frame;
            const bool controller_relaunch_push_frame=controller_relaunch_push_policy&&relaunch_push&&
                (!relaunch_entry_push||controller_relaunch_entry_push_policy);
            const bool jump_kind=(unsigned(o.kind)>=5&&unsigned(o.kind)<=8)||controller_fall_push_frame;
            const bool controller_air_frame=controller_air_policy&&(air_dash||(o.kind==NativeBorgMotionKind::JumpTouchdown&&o.air_motion));
            const bool controller_relaunch_frame=controller_relaunch_policy&&relaunch;
            const bool jump_path=jump_policy&&jump_kind&&!controller_air_frame&&!controller_relaunch_frame&&
                (o.kind!=NativeBorgMotionKind::JumpLanding||!actor.source_ground_motion||!last_offers_[source_index].walk_ledge);
            const bool ledger_frame=ledge_policy&&!jump_path&&!controller_air_frame&&!controller_relaunch_frame;
            const unsigned phase=native_borg_motion_phase(o.kind);
             if(phase>4||(vertical&&producer->policy_==NativeBorgMotionPolicy::GroundOnly)||
                    (recovery_floor_complete&&!recovery_recontact_policy)||
                    (o.recovery_first_seam_start&&(!recovery_first_seam_policy||!recovery_floor_complete))||
                   (moving&&!ledge_policy&&!push_policy&&producer->policy_!=NativeBorgMotionPolicy::Jump&&producer->policy_!=NativeBorgMotionPolicy::AirDash&&producer->policy_!=NativeBorgMotionPolicy::Relaunch)||
                   (air_dash&&!controller_air_policy&&!push_policy&&producer->policy_!=NativeBorgMotionPolicy::AirDash&&producer->policy_!=NativeBorgMotionPolicy::Relaunch)||
                   (relaunch&&!controller_relaunch_policy&&!push_policy&&producer->policy_!=NativeBorgMotionPolicy::Relaunch)||
                      (push&&((!push_policy&&!controller_fall_push_frame&&!controller_air_push_frame&&!controller_relaunch_push_frame&&!ground_loss_push)||!o.static_push))||(!push&&o.static_push)||
                    (ground_loss_push&&!walk_loss_push_policy)||
                   (recovery_push&&!recovery_push_policy&&!controller_recovery_push_frame)||
                    (windup_push&&!windup_push_policy&&!controller_windup_push_frame&&!controller_windup_entry_push_frame)||
                   (windup_entry_push&&!windup_entry_push_policy&&!controller_windup_entry_push_frame)||
                   (active_push&&!active_push_policy&&!controller_active_push_frame)||
                   (relaunch_push&&!relaunch_push_policy&&!controller_relaunch_push_frame)||
                   (relaunch_entry_push&&!relaunch_entry_push_policy&&!controller_relaunch_push_frame)||
                    (relaunch_floor&&((!floor_policy&&!controller_relaunch_floor_policy)||!o.relaunch_floor))||(!relaunch_floor&&o.relaunch_floor)||
                    (air_floor&&((!air_floor_policy&&!controller_floor_policy)||!o.air_dash_floor))||(!air_floor&&o.air_dash_floor)||
                      (windup_entry_floor&&!windup_entry_floor_policy&&!controller_windup_entry_floor_policy)||
                       (positive_windup_entry&&!positive_windup_policy&&!controller_windup_entry_floor_policy)||
                   (ledger_frame&&(!o.walk_ledge||(!ground_loss&&o.kind!=NativeBorgMotionKind::Ground&&o.kind!=NativeBorgMotionKind::JumpLanding)))||
                     ((!ledge_policy||jump_path||controller_air_frame||controller_relaunch_frame)&&(ground_loss||o.walk_ledge))||
                     (ordered_policy&&!dash_ledge_policy&&o.kind!=NativeBorgMotionKind::Ground)||
                     (!ordered_policy&&o.ordered_ground)||
                      (dash_ledge_policy&&!combined_ledge_policy&&departure&&!o.ordered_ground)||
                      (o.ordered_ground&&o.kind!=NativeBorgMotionKind::Ground&&!departure)||
                     (jump_policy&&vertical&&!departure&&!controller_air_frame&&!controller_relaunch_frame&&!o.jump_motion)||(!jump_policy&&o.jump_motion)||
                     (o.jump_motion&&(!vertical||departure||controller_air_frame||controller_relaunch_frame))||
                     (controller_air_frame!=bool(o.air_motion))||
                      (controller_relaunch_frame!=bool(o.relaunch_motion))||
                       (bool(o.surface_landing)!=(controller_surface_policy&&(recovery_floor_complete||o.kind==NativeBorgMotionKind::JumpTouchdown||
                          (controller_ground_loss_surface_policy&&o.kind==NativeBorgMotionKind::GroundLossTouchdown))))||
                      (bool(o.controller_timer698)!=controller_landing_jump_policy)||
                !std::isfinite(o.vertical_speed)||std::abs(o.vertical_speed)>50||!std::isfinite(o.entry_vertical_delta)||
                (native_borg_motion_relaunch_entry(o.kind)?(o.entry_vertical_delta<=0||o.entry_vertical_delta>50):o.entry_vertical_delta!=0))return false;
            const bool active_opponent=config_.source_allow_active_opponent;
            if(opponent_ai||!motion_observation_current(o.observed,source_index)||!o.serial||
               (!vertical&&(!actor.grounded||actor.vertical_speed!=0||o.vertical_speed!=0))||
                actor.attack_ticks||actor.hit_flash||actor.dash_ticks||
                (!active_opponent&&!solo&&(other.attack_ticks||!other.grounded||other.vertical_speed!=0||other.movement_speed!=0||other.velocity!=BattlePoint{}))||
                (!active_opponent&&other_pad!=GamePadState{})||
               pad.a||pad.b||pad.x||(!active_opponent&&(other_pad.a||other_pad.b||other_pad.x))||
               (!vertical&&o.action!=BattleMvpAction::Idle&&o.action!=BattleMvpAction::Move&&o.action!=BattleMvpAction::GroundDash)||
               !std::isfinite(o.facing.x)||!std::isfinite(o.facing.z)||std::abs(std::hypot(o.facing.x,o.facing.z)-1)>.001f)return false;
             for(const auto& shot:projectiles_)if(shot.active&&(!package||!shot.from_package))return false;
            for(auto value:o.proposed_position)if(!std::isfinite(value)||std::abs(value)>1e5f)return false;
            const BattlePoint delta{o.proposed_position[0]-actor.position[0],o.proposed_position[2]-actor.position[2]};
            if(std::hypot(delta.x,delta.z)>config_.actor_radius)return false; //native endpoint-terrain budget
             const auto prior=actor.source_ground_motion?last_offers_[source_index].kind:NativeBorgMotionKind::Ground;
             const unsigned prior_phase=native_borg_motion_phase(prior);
             const bool timed_repeat_candidate=controller_fall_push_frame&&
                 o.controller_timer698>0.f&&last_offers_[source_index].controller_timer698>1.f&&
                 prior==NativeBorgMotionKind::StaticPush&&last_offers_[source_index].static_push&&last_offers_[source_index].jump_motion&&
                  last_offers_[source_index].jump_motion->timed_walk_relaunch_fall&&!last_offers_[source_index].jump_motion->third_timed_release_push&&
                  !last_offers_[source_index].jump_motion->down_timed_release_push;
             const bool timed_release_second=timed_repeat_candidate&&behaviors.has(Behavior::TimedWalkReleaseSecondPush)&&
                 !last_offers_[source_index].jump_motion->second_timed_release_push;
             const bool timed_release_third=timed_repeat_candidate&&behaviors.has(Behavior::TimedWalkReleaseThirdPush)&&
                 last_offers_[source_index].jump_motion->second_timed_release_push&&
                  NormalGRedMovementDefinition::third_timed_release_face(last_offers_[source_index].static_push->surface.triangle,last_offers_[source_index].static_push->angle);
              bool timed_release_down=false;
              if(behaviors.has(Behavior::TimedWalkReleaseDownPush)&&controller_fall_push_frame&&prior==NativeBorgMotionKind::StaticPush&&
                 last_offers_[source_index].controller_timer698==2.f&&o.controller_timer698==1.f&&last_offers_[source_index].jump_motion&&last_offers_[source_index].static_push&&
                 last_offers_[source_index].jump_motion->third_timed_release_push&&last_offers_[source_index].jump_motion->timed_walk_relaunch_fall&&
                 NormalGRedMovementDefinition::third_timed_release_face(last_offers_[source_index].static_push->surface.triangle,last_offers_[source_index].static_push->angle)&&
                 o.jump_motion&&o.jump_motion->drift_heading&&player_frame){
                  CharacterMovementHeadingFrame heading;
                  timed_release_down=native_borg_source_heading_frame(*player_frame,actor.facing,&heading)&&
                      *o.jump_motion->drift_heading==heading.camera5b0;
              }
             const bool timed_release_push=behaviors.has(Behavior::TimedWalkReleasePush)&&controller_fall_push_frame&&
                 o.controller_timer698>0.f&&o.jump_motion&&o.jump_motion->timed_walk_relaunch_fall&&
                 !o.jump_motion->entry&&actor.vertical_speed<0.f;
              const auto* walking_push=prior==NativeBorgMotionKind::GroundLossPush&&last_offers_[source_index].static_push&&
                  last_offers_[source_index].walk_ledge&&last_offers_[source_index].walk_ledge->walk_origin?&*last_offers_[source_index].walk_ledge:nullptr;
               const bool expiring_windup_origin=behaviors.has(Behavior::WalkExpiringAirDash)&&walking_push&&actor.vertical_speed<0.f&&
                   walking_push->timer698==1.f&&last_offers_[source_index].controller_timer698==1.f&&o.controller_timer698==0.f&&
                   NormalGRedMovementDefinition::walk_expiring_air_dash_face(last_offers_[source_index].static_push->surface.triangle,
                       last_offers_[source_index].static_push->angle);
               const bool expiring_push_windup=expiring_windup_origin&&o.kind==NativeBorgMotionKind::AirDashWindupEntry;
               const bool expiring_floor_windup=expiring_windup_origin&&behaviors.has(Behavior::WalkExpiringAirDashSnap)&&
                   o.kind==NativeBorgMotionKind::AirDashWindupEntryFloor&&o.air_dash_floor&&o.air_motion&&o.air_motion->timer568==6.f&&
                   o.proposed_position==actor.position&&o.air_dash_floor->surface.height==actor.position[1]&&
                   NormalGRedMovementDefinition::walk_expiring_air_dash_snap_face(o.air_dash_floor->surface.triangle,o.air_dash_floor->angle);
             const bool recovery_recontact=prior==NativeBorgMotionKind::AirDashRecoveryFloorComplete;
             // Only the next ordinary program13 update can confirm touchdown
             // at rest. A new action cannot borrow this contact.
             if(recovery_recontact&&(!recovery_recontact_policy||o.kind!=NativeBorgMotionKind::JumpTouchdown||
                !jump_path||!o.jump_motion||!o.jump_motion->entry||o.jump_motion->phase!=6||!o.jump_motion->air_dash_origin||
                o.jump_motion->held_jump||(!recovery_drift_policy&&o.jump_motion->drift_heading)||!last_offers_[source_index].air_motion||
                o.jump_motion->resource_before!=last_offers_[source_index].air_motion->resource_after||*o.controller_timer698!=0.f))return false;
             const bool recovery_program=(recovery_recontact_policy||controller_recovery_push_frame)&&
                 controller_air_frame&&o.air_motion->phase==17;
             const auto completion=o.recovery_completion.lock();
             if(recovery_program||(recovery_drift_policy&&recovery_recontact)){
                 const bool contact_update=recovery_floor_complete||recovery_recontact;
                 const auto drift=recovery_recontact?o.jump_motion->drift_heading:o.air_motion->drift_heading;
                 if(!completion||completion->world.lock()!=motion_scope_||completion->producer.lock()!=producer||
                    completion->tick!=tick_||completion->serial!=o.serial||completion->kind!=o.kind||
                    completion->lower_done!=(recovery_recontact?o.jump_motion->lower_done:o.air_motion->lower_done)||
                    bool(completion->drift_offset)!=(contact_update&&bool(drift)))return false;
                 if(completion->drift_offset){
                     CharacterMovementHeadingFrame frame;
                     const s16 offset=*completion->drift_offset;
                     if(!recovery_drift_policy||offset%0x4000||!player_frame||
                        !native_borg_source_heading_frame(*player_frame,actor.facing,&frame)||
                        *drift!=source_heading_wrap(s32(frame.camera5b0)+offset-0x8000))return false;
                 }
             }else if(completion)return false;
            if(controller_landing_jump_policy){
                const f32 previous=actor.source_ground_motion?last_offers_[source_index].controller_timer698.value_or(-1.f):0.f;
                const f32 expected=o.kind==NativeBorgMotionKind::GroundLossEntry?30.f:std::max(0.f,previous-1.f);
                const f32 timer=*o.controller_timer698;
                if(previous<0||!std::isfinite(timer)||timer<0||timer>30||timer!=expected||
                   (o.walk_ledge&&o.walk_ledge->timer698!=timer)||
                   (timer>0&&((native_borg_motion_windup_entry(o.kind)&&!controller_ground_loss_air_policy)||
                              (native_borg_motion_relaunch_entry(o.kind)&&!controller_ground_loss_relaunch_policy))))return false;
            }
            const bool active_landing=actor.source_ground_motion&&prior==NativeBorgMotionKind::JumpLanding&&
                last_offers_[source_index].jump_motion&&last_offers_[source_index].jump_motion->phase==7&&last_offers_[source_index].jump_motion->next_selector==7;
            if(o.landing_target){
                if(controller_ground_loss_surface_policy&&last_offers_[source_index].walk_ledge&&last_offers_[source_index].walk_ledge->timer698>1.f&&
                   (!controller_landing_jump_policy||(o.landing_target!=4&&!(controller_landing_dash_policy&&o.landing_target==12))))return false;
                if(controller_landing_jump_policy&&*o.controller_timer698>0&&o.landing_target!=4&&
                   !(controller_landing_dash_policy&&o.landing_target==12))return false;
                if(!landing_policy||!active_landing||!actor.grounded||actor.vertical_speed||
                   (o.landing_target==4?(o.kind!=NativeBorgMotionKind::JumpTakeoff||!o.jump_motion||
                                        o.jump_motion->phase!=4||!o.jump_motion->entry):
                    o.landing_target==12?(o.kind!=NativeBorgMotionKind::Ground||!o.ordered_ground||o.ordered_ground->phase!=12):true))return false;
            }
             auto surface_landing_height=[&](const BattleMvpVec& integrated,f32 velocity,f32 acceleration,f32* height){
                 if(!controller_surface_policy||!o.surface_landing||!height||!terrain||actor.grounded||
                    (recovery_recontact?actor.vertical_speed!=0:actor.vertical_speed>=0)||o.vertical_speed)return false;
                const auto& e=*o.surface_landing;NativeBorgStaticSurface surface;s16 angle=0;
                const bool ground_loss_landing=controller_ground_loss_surface_policy&&o.kind==NativeBorgMotionKind::GroundLossTouchdown;
                if(ground_loss_landing&&!o.walk_ledge)return false;
                const f32 timer=controller_landing_jump_policy?*o.controller_timer698:ground_loss_landing?o.walk_ledge->timer698:0.f;
                if(e.integrated_position!=integrated||e.vertical_speed!=velocity||e.acceleration!=acceleration||
                   e.timer698!=timer||
                   !terrain->static_surface(integrated[0],integrated[2],actor.position[1],&surface)||
                   surface!=e.surface||!native_borg_static_surface_valid(surface)||integrated[1]>surface.height||
                   actor.position[1]<surface.height-.01f||!source_surface_angle(surface.normal,&angle)||
                    angle!=e.angle||angle<0||angle>(timer>0?6144:10240))return false;
                 if(recovery_floor_complete||recovery_recontact){
                     NativeBorgStaticSurface start;
                     if(timer!=0.f||!native_borg_flat_surface(surface)||
                         !terrain->static_surface(actor.position[0],actor.position[2],actor.position[1],&start)||
                         (recovery_recontact&&(!last_offers_[source_index].surface_landing||last_offers_[source_index].surface_landing->surface!=start||
                            actor.position[1]!=surface.height||integrated[1]!=surface.height)))return false;
                     if(start!=surface){
                         const bool scoped_seam=recovery_recontact?
                             recovery_seam_policy&&bool(o.jump_motion->drift_heading):
                             recovery_first_seam_policy&&o.recovery_first_seam_start==start&&
                                 bool(o.air_motion->drift_heading)&&actor.position[1]>start.height;
                         if(!scoped_seam||!native_borg_flat_surface(start)||start.height!=surface.height||
                             !terrain->coplanar_crossing(start.triangle,surface.triangle,actor.position,integrated))
                             return false;
                     }else if(o.recovery_first_seam_start)return false;
                 }
                //Qualified normal G-Red timer-bound gates/epsilon. Match the shared
                //source contact's bounded host normalization, not an UP floor.
                BattleMvpVec movement{};for(unsigned k=0;k<3;++k)movement[k]=f32(integrated[k]-actor.position[k]);
                const f32 square=f32(f32(f32(movement[0]*movement[0])+f32(movement[1]*movement[1]))+f32(movement[2]*movement[2]));
                if(!std::isfinite(square))return false;
                if(square>bits_to_f32(0x2edbe6feu)){
                    const f64 length=std::sqrt(f64(square));for(auto& v:movement)v=f32(v/length);
                    const f32 dot=f32(f32(f32(movement[0]*surface.normal[0])+f32(movement[1]*surface.normal[1]))+f32(movement[2]*surface.normal[2]));
                    if(dot>0)return false;
                }
                const BattlePoint planar{integrated[0]-actor.position[0],integrated[2]-actor.position[2]};
                NativeBorgContactRequest pre{actor.position,other.position,planar,config_.arena_half,config_.actor_radius,config_.actor_height,config_.step_height,false,other.health!=0};
                pre.vertical_end_y=integrated[1];NativeBorgContactResult checked;
                if(!native_borg_resolve_contact(pre,terrain,&checked)||checked.body_hit||checked.bounds_hit||checked.terrain_blocked||
                   checked.position[0]!=integrated[0]||checked.position[2]!=integrated[2])return false;
                *height=surface.height;return true;
            };
            const f32 push_damping=bits_to_f32(0x3f666666u);
            auto project_integrated_push=[&](BattleMvpVec& integrated,f32 velocity){
                const auto& p=*o.static_push;
                return p.integrated_position==integrated&&p.vertical_speed==velocity&&p.damping==push_damping&&
                    o.vertical_speed==f32(velocity*push_damping)&&
                    source_surface_push_position(integrated,p.surface.normal,p.surface.height,p.angle,&integrated);
            };
            if(controller_air_frame){
                const auto& e=*o.air_motion;
                const auto* previous=actor.source_ground_motion&&last_offers_[source_index].air_motion?&*last_offers_[source_index].air_motion:nullptr;
                const bool expiry_lineage=behaviors.has(Behavior::ExpiryPlainWindupRelaunch)&&e.phase==15&&
                    o.controller_timer698&&*o.controller_timer698==0.f&&
                    (e.entry?(o.kind==NativeBorgMotionKind::AirDashWindupEntry&&walking_push&&walking_push->timer698==0.f):
                        (o.kind==NativeBorgMotionKind::AirDashWindup&&previous&&previous->phase==15&&previous->walk_expiry_windup));
                 if(e.walk_expiry_windup!=expiry_lineage)return false;
                 u8 expiring_step=0;bool expiring_short=false;
                 if(behaviors.has(Behavior::WalkExpiringAirDashRelaunch)&&e.phase==15&&o.controller_timer698==0.f){
                     if(e.entry){
                         expiring_short=expiring_push_windup&&e.timer568==5.f&&behaviors.has(Behavior::WalkExpiringShortAirDashRelaunch);
                         expiring_step=expiring_push_windup&&(e.timer568==6.f||expiring_short)?1:0;
                     }else if(previous&&(o.kind==NativeBorgMotionKind::AirDashWindup||controller_windup_push_frame)){
                         expiring_step=NormalGRedMovementDefinition::expiring_walk_windup_next(previous->expiring_walk_windup_step,
                             controller_windup_push_frame,o.static_push?o.static_push->surface.triangle:0,
                             o.static_push?o.static_push->angle:0,previous->expiring_walk_windup_short);
                         expiring_short=expiring_step&&previous->expiring_walk_windup_short;
                     }
                 }
                 if(e.expiring_walk_windup_step!=expiring_step||e.expiring_walk_windup_short!=expiring_short)return false;
                 u8 expiring_snap_step=0;
                 if(behaviors.has(Behavior::WalkExpiringAirDashSnapRelaunch)&&e.phase==15&&o.controller_timer698==0.f){
                     if(e.entry)expiring_snap_step=expiring_floor_windup?1:0;
                     else if(previous&&o.kind==NativeBorgMotionKind::AirDashWindup)
                         expiring_snap_step=NormalGRedMovementDefinition::expiring_snap_windup_next(previous->expiring_snap_windup_step);
                 }
                 if(e.expiring_snap_windup_step!=expiring_snap_step)return false;
                 bool rising_windup=false;
                 if(behaviors.has(Behavior::RisingPlainWindupRelaunch)&&e.phase==15&&
                    last_offers_[source_index].controller_timer698==0.f&&o.controller_timer698==0.f&&actor.vertical_speed>0.f&&o.vertical_speed>0.f){
                     if(e.entry){
                         const auto& lift=last_offers_[source_index].relaunch_motion;
                         rising_windup=o.kind==NativeBorgMotionKind::AirDashWindupEntry&&
                             NormalGRedMovementDefinition::rising_relaunch_windup_length(behaviors,e.timer568)&&
                             prior==NativeBorgMotionKind::RelaunchActive&&lift&&!lift->direction&&
                             lift->lower_after==3&&lift->upper_after==3;
                     }else rising_windup=o.kind==NativeBorgMotionKind::AirDashWindup&&previous&&previous->rising_relaunch_windup;
                 }
                 if(e.rising_relaunch_windup!=rising_windup)return false;
                bool timed_floor_lineage=false;
                if(behaviors.has(Behavior::TimedSnapWindupRelaunch)&&e.phase==15&&o.air_dash_floor&&
                    native_borg_flat_surface(o.air_dash_floor->surface)&&o.controller_timer698){
                    const auto& selected=o.air_dash_floor->surface;
                    if(e.entry){
                        timed_floor_lineage=o.kind==NativeBorgMotionKind::AirDashWindupEntryFloor&&
                            native_borg_motion_relaunch_floor(prior)&&last_offers_[source_index].relaunch_floor&&
                            last_offers_[source_index].relaunch_floor->surface==selected&&actor.vertical_speed<0.f&&
                            last_offers_[source_index].controller_timer698>1.f&&*o.controller_timer698>0.f;
                    }else if(o.kind==NativeBorgMotionKind::AirDashWindupFloor&&previous&&previous->phase==15&&
                        previous->timed_relaunch_floor_windup&&last_offers_[source_index].air_dash_floor){
                        const auto& start=last_offers_[source_index].air_dash_floor->surface;
                        timed_floor_lineage=selected.height==start.height&&actor.position[1]==start.height&&
                            (selected==start||(terrain&&terrain->coplanar_crossing(start.triangle,selected.triangle,
                                actor.position,o.proposed_position)));
                    }
                }
                if(e.timed_relaunch_floor_windup!=timed_floor_lineage)return false;
                const auto* jump=actor.source_ground_motion&&last_offers_[source_index].jump_motion?&*last_offers_[source_index].jump_motion:nullptr;
                const auto* relaunched=controller_relaunch_policy&&last_offers_[source_index].relaunch_motion?&*last_offers_[source_index].relaunch_motion:nullptr;
                const f32 prior_speed=previous?previous->speed44:jump?jump->speed44:relaunched?relaunched->speed44:0;
                s16 prior_resource=previous?previous->resource_after:jump?jump->resource_before:relaunched?relaunched->resource_after:0;
                if(jump&&jump->phase==5)prior_resource=s16(std::max(0,int(prior_resource)-meter.lift_cost(jump->entry)));
                if(e.phase<15||e.phase>17||e.stage_before>2||e.next_stage>2||e.resource_before<0||e.resource_before>meter.limit||
                   e.resource_after<0||e.resource_before!=prior_resource||e.speed_before!=prior_speed||
                   !std::isfinite(e.timer_before)||!std::isfinite(e.timer568)||std::abs(e.timer_before)>1e5f||std::abs(e.timer568)>1e5f)return false;
                for(f32 value:{e.speed_before,e.move_speed,e.speed44})if(!std::isfinite(value)||value<0||value>30)return false;
                 const bool touchdown=o.kind==NativeBorgMotionKind::JumpTouchdown||recovery_floor_complete;
                 if((controller_air_push_frame&&*o.controller_timer698!=0.f)||
                    (controller_recovery_push_frame&&e.phase!=17)||
                     (controller_windup_push_frame&&(e.phase!=15||e.entry))||
                     (controller_windup_entry_push_frame&&(e.phase!=15||!e.entry))||
                    (controller_active_push_frame&&(e.phase!=16||e.entry)))return false;
                 //The offer records the acceleration used before contact. A
                 //steep push clears that acceleration for the next update.
                 const bool prior_air_push=prior==NativeBorgMotionKind::AirDashActivePush||
                     prior==NativeBorgMotionKind::AirDashRecoveryPush;
                auto expected=actor.position;f32 speed=prior_speed,vertical=actor.vertical_speed,change=0,acceleration=bits_to_f32(0xbd8f5c29u);
                f32 timer=e.timer_before;u8 stage=1,next=e.phase;bool callback=false;int cost=0;
                if(e.phase==15){
                    if(e.entry){
                        if((!jump&&!relaunched&&(!previous||previous->phase!=17))||(jump&&(jump->phase!=5&&jump->phase!=6))||
                            (jump&&jump->ground_loss&&last_offers_[source_index].walk_ledge&&last_offers_[source_index].walk_ledge->timer698>0&&!controller_ground_loss_air_policy)||
                           e.stage_before||timer<5||timer>9||std::floor(timer)!=timer)return false;
                        speed=f32(speed*.5f);vertical=f32(vertical*.5f);
                         if((o.kind!=NativeBorgMotionKind::AirDashWindupEntry&&!windup_entry_floor&&!controller_windup_entry_push_frame)||touchdown||
                            (!air_floor&&!controller_windup_entry_push_frame&&o.proposed_position!=actor.position))return false;
                    }else{
                        if(!previous||previous->next_selector!=15||e.stage_before!=1||timer!=previous->timer568)return false;
                        timer=f32(timer-1);if(timer<=0){next=16;stage=0;cost=meter.cost(AirMeterCharge::DashHandoff);}
                         if(o.kind!=NativeBorgMotionKind::AirDashWindup&&o.kind!=NativeBorgMotionKind::AirDashWindupFloor&&!controller_windup_push_frame)return false;
                    }
                }else if(e.phase==16){
                    if(e.entry){
                        if(!previous||previous->next_selector!=16||previous->phase!=15||e.stage_before||timer!=previous->timer568)return false;
                        speed=30;vertical=0;change=bits_to_f32(0xbd4ccccdu);timer=30;
                        if(o.kind!=NativeBorgMotionKind::AirDashActiveEntry&&o.kind!=NativeBorgMotionKind::AirDashActiveEntryFloor)return false;
                    }else{
                        if(!previous||previous->next_selector!=16||e.stage_before!=previous->next_stage||timer!=previous->timer568)return false;
                         stage=e.stage_before;change=prior_air_push?0.f:previous->planar_accel;
                        if(stage==1){timer=f32(timer-1);if(timer<=0){stage=2;change=0;}}
                         if(o.kind!=NativeBorgMotionKind::AirDashActive&&o.kind!=NativeBorgMotionKind::AirDashActiveFloor&&
                            !touchdown&&!controller_active_push_frame)return false;
                    }
                    cost=meter.cost(AirMeterCharge::DashActive);callback=stage==2;
                    if(e.resource_before<=cost){next=17;stage=0;callback=false;}
                }else{
                    if(!previous||(e.entry?(previous->next_selector!=17&&!(previous->phase==16&&previous->next_stage==2)):
                        previous->next_selector!=17)||timer!=previous->timer568||e.control)return false;
                    if(controller_ground_loss_air_policy&&*o.controller_timer698>0&&e.drift_heading)return false;
                     change=prior_air_push?0.f:previous->planar_accel;
                     acceleration=-1;callback=true;
                    speed=f32(speed*bits_to_f32(0x3f7d70a4u));
                     if(o.kind!=NativeBorgMotionKind::AirDashRecovery&&!touchdown&&!controller_recovery_push_frame)return false;
                    if(e.drift_heading){f32 x=0,z=0;source_heading_xz(*e.drift_heading,&x,&z);
                        expected[0]=std::fma(4.f,x,expected[0]);expected[2]=std::fma(4.f,z,expected[2]);}
                    if(!touchdown&&e.lower_done)next=6;
                }
                if(e.phase!=17&&e.drift_heading)return false;
                const bool moves=!(e.phase==15&&e.entry);
                const f32 retained=moves?std::max(0.f,std::fma(change,1.f,speed)):speed;
                 if(e.move_speed!=speed||e.speed44!=(controller_air_push_frame?f32(retained*push_damping):retained)||e.vertical_before!=vertical||e.planar_accel!=change||
                   e.vertical_accel!=acceleration||e.timer568!=timer||e.next_stage!=stage||e.callback!=callback||
                   e.resource_after!=std::max(0,int(e.resource_before)-cost))return false;
                if(moves){f32 x=0,z=0;source_heading_xz(e.travel5ae,&x,&z);
                    expected[0]=std::fma(1.f,f32(speed*x),expected[0]);expected[1]=std::fma(vertical,1.f,expected[1]);
                    expected[2]=std::fma(1.f,f32(speed*z),expected[2]);}
                if(touchdown){
                    if(!callback||recovery_floor_complete!=(e.phase==17&&e.lower_done)||
                       (recovery_floor_complete&&(e.entry||prior!=NativeBorgMotionKind::AirDashRecovery||
                          !previous||e.travel5ae!=previous->travel5ae||o.facing!=actor.facing)))return false;
                    if(controller_surface_policy){
                        if(!surface_landing_height(expected,std::max(-30.f,std::fma(acceleration,1.f,vertical)),acceleration,&expected[1]))return false;
                    }else{
                        NativeBorgStaticSurface current,surface;
                        if(!terrain||
                           !terrain->static_surface(actor.position[0],actor.position[2],actor.position[1],&current)||
                           !terrain->static_surface(expected[0],expected[2],actor.position[1],&surface)||
                           !native_borg_flat_surface(current)||!native_borg_flat_surface(surface)||current.height!=surface.height||
                           actor.vertical_speed>=0||expected[1]>surface.height||o.vertical_speed)return false;
                        expected[1]=surface.height;
                    }
                     next=recovery_floor_complete?6:7;
                 }else if(controller_air_push_frame){
                     const f32 integrated=moves?std::max(-30.f,std::fma(acceleration,1.f,vertical)):vertical;
                     if(!project_integrated_push(expected,integrated))return false;
                 }else if(air_floor){
                    const auto& floor=*o.air_dash_floor;
                    const f32 integrated=moves?std::max(-30.f,std::fma(1.f,acceleration,vertical)):vertical;
                    if(!controller_floor_policy||callback||floor.integrated_position!=expected||floor.vertical_speed!=integrated||
                       o.vertical_speed!=(e.phase==15?0.f:integrated))return false;
                    expected[1]=floor.surface.height;
                }else if(o.vertical_speed!=(moves?std::max(-30.f,std::fma(1.f,acceleration,vertical)):vertical))return false;
                if(e.next_selector!=next||o.proposed_position!=expected)return false;
            }
             if(controller_relaunch_frame){
                 const auto& e=*o.relaunch_motion;
                 const bool timed_walk_tap=behaviors.has(Behavior::TimedWalkReleasePush)&&e.entry&&!e.direction&&
                     o.kind==NativeBorgMotionKind::RelaunchEntry&&o.controller_timer698>0.f&&
                     prior==NativeBorgMotionKind::GroundLossAir&&last_offers_[source_index].walk_ledge&&last_offers_[source_index].walk_ledge->walk_origin;
                 if(e.timed_walk_relaunch_tap!=timed_walk_tap)return false;
                if(controller_relaunch_push_frame&&(e.entry!=relaunch_entry_push||*o.controller_timer698!=0.f))return false;
                const auto* previous=actor.source_ground_motion&&last_offers_[source_index].relaunch_motion?&*last_offers_[source_index].relaunch_motion:nullptr;
                const auto* air=actor.source_ground_motion&&last_offers_[source_index].air_motion?&*last_offers_[source_index].air_motion:nullptr;
                const auto* jump=actor.source_ground_motion&&last_offers_[source_index].jump_motion?&*last_offers_[source_index].jump_motion:nullptr;
                const f32 speed=previous?previous->speed44:air?air->speed44:jump?jump->speed44:0;
                const s16 resource=previous?previous->resource_after:air?air->resource_after:jump?jump->resource_before:0;
                if(!std::isfinite(e.speed_before)||e.speed_before<0||e.speed_before>30||e.speed_before!=speed||
                   e.resource_before!=resource||resource<=0||e.coarse57d>3||e.lower_before>3||e.upper_before>3||
                   e.upper_before>e.lower_before||e.entry!=(e.lower_before==0))return false;
                  if(e.entry){
                      const bool completed_windup=air&&air->phase==15&&air->next_selector==16&&air->next_stage==0&&air->timer568==0;
                      const bool windup_push_handoff=completed_windup&&controller_windup_push_policy&&
                          prior==NativeBorgMotionKind::AirDashWindupPush;
                      const bool plain_handoff=completed_windup&&
                          prior==NativeBorgMotionKind::AirDashWindup&&o.kind==NativeBorgMotionKind::RelaunchEntry&&
                          last_offers_[source_index].controller_timer698&&o.controller_timer698;
                      const bool timed_plain_handoff=plain_handoff&&behaviors.has(Behavior::TimedPlainWindupRelaunch)&&
                          *last_offers_[source_index].controller_timer698>1.f&&
                          *o.controller_timer698==*last_offers_[source_index].controller_timer698-1.f;
                       const bool expiry_plain_handoff=plain_handoff&&behaviors.has(Behavior::ExpiryPlainWindupRelaunch)&&
                           air->walk_expiry_windup&&*last_offers_[source_index].controller_timer698==0.f&&*o.controller_timer698==0.f;
                       bool expiring_walk_handoff=false;
                       if(plain_handoff&&*last_offers_[source_index].controller_timer698==0.f&&*o.controller_timer698==0.f&&
                           ((behaviors.has(Behavior::WalkExpiringAirDashRelaunch)&&
                              air->expiring_walk_windup_step==NormalGRedMovementDefinition::expiring_walk_windup_end(air->expiring_walk_windup_short))||
                            (behaviors.has(Behavior::WalkExpiringAirDashSnapRelaunch)&&
                              air->expiring_snap_windup_step==NormalGRedMovementDefinition::expiring_snap_windup_complete))){
                           BattlePoint travel;source_heading_xz(e.travel5ae,&travel.x,&travel.z);
                           expiring_walk_handoff=!e.direction&&travel==actor.facing&&o.facing==actor.facing&&actor.vertical_speed<0.f&&
                               e.vertical_before>0.f&&o.vertical_speed>0.f;
                       }
                      const bool floor_handoff_clock=last_offers_[source_index].controller_timer698&&o.controller_timer698&&
                          NormalGRedMovementDefinition::floor_windup_handoff_clock(behaviors,*last_offers_[source_index].controller_timer698,*o.controller_timer698);
                      const bool timed_floor_handoff=completed_windup&&behaviors.has(Behavior::TimedSnapWindupRelaunch)&&
                          prior==NativeBorgMotionKind::AirDashWindupFloor&&air->timed_relaunch_floor_windup&&
                          last_offers_[source_index].air_dash_floor&&floor_handoff_clock&&
                          o.kind==NativeBorgMotionKind::RelaunchEntry;
                      if(timed_floor_handoff){
                          const auto& floor=last_offers_[source_index].air_dash_floor->surface;
                          NativeBorgStaticSurface start,end;
                          BattlePoint facing;source_heading_xz(e.travel5ae,&facing.x,&facing.z);
                          if(!terrain||actor.vertical_speed||e.direction||facing!=actor.facing||o.facing!=actor.facing||
                              !native_borg_flat_surface(floor)||actor.position[1]!=floor.height||
                              !terrain->static_surface(actor.position[0],actor.position[2],actor.position[1],&start)||start!=floor||
                              !terrain->static_surface(o.proposed_position[0],o.proposed_position[2],actor.position[1],&end)||end!=floor)
                              return false;
                      }
                       const auto entry_kind=relaunch_entry_push?NativeBorgMotionKind::RelaunchEntryPush:
                           relaunch_floor?NativeBorgMotionKind::RelaunchEntryFloor:NativeBorgMotionKind::RelaunchEntry;
                        const bool rising_handoff=plain_handoff&&behaviors.has(Behavior::RisingPlainWindupRelaunch)&&
                            air->rising_relaunch_windup&&air->resource_before-air->resource_after==meter.cost(AirMeterCharge::DashHandoff)&&
                            last_offers_[source_index].controller_timer698==0.f&&o.controller_timer698==0.f&&actor.vertical_speed>0.f&&o.vertical_speed>0.f&&
                            !e.direction&&o.facing==actor.facing;
                        if(previous||(!air&&!jump)||(air&&air->phase!=16&&air->phase!=17&&
                             !windup_push_handoff&&!timed_plain_handoff&&!expiry_plain_handoff&&!timed_floor_handoff&&!expiring_walk_handoff&&!rising_handoff)||
                        (jump&&(jump->phase!=6||(jump->ground_loss&&!controller_ground_loss_relaunch_policy)))||e.upper_before||
                        o.kind!=entry_kind||o.entry_vertical_delta!=15)return false;
                }else if(!previous||e.lower_before!=previous->lower_after||e.upper_before!=previous->upper_after||
                         e.coarse57d!=previous->coarse57d||o.entry_vertical_delta)return false;
                 using Programs=NormalGRedMovementDefinition::Programs;
                 const u8 program=e.lower_before==2||e.lower_before==3?Programs::relaunch_active:Programs::relaunch_windup;
                u8 lower=e.entry?1:e.lower_before==2?3:e.lower_before;
                if(!e.entry&&lower==1&&e.lower_done)lower=2;
                 u8 upper=e.upper_before;const u8 upper_program=upper>=2?Programs::relaunch_active:Programs::relaunch_windup;
                 if(!upper)upper=1;else if(upper==1&&lower>1)upper=2;else if(upper==2)upper=3;
                 const bool windup=program==Programs::relaunch_windup;
                  const auto continued_kind=controller_relaunch_push_frame?
                      (windup?NativeBorgMotionKind::RelaunchWindupPush:NativeBorgMotionKind::RelaunchActivePush):
                      (relaunch_floor?(windup?NativeBorgMotionKind::RelaunchWindupFloor:NativeBorgMotionKind::RelaunchActiveFloor):
                          (windup?NativeBorgMotionKind::RelaunchWindup:NativeBorgMotionKind::RelaunchActive));
                 if(e.lower_program!=program||e.upper_program!=upper_program||e.lower_after!=lower||e.upper_after!=upper||
                    (!e.entry&&o.kind!=continued_kind))return false;
                f32 sine=0,cosine=1;source_heading_xz(e.direction?8192:0,&sine,&cosine);
                const f32 change=f32(bits_to_f32(0x3f4ccccdu)*sine);
                const f32 acceleration=std::fma(bits_to_f32(0x3f4ccccdu),cosine,0.f);
                const f32 moving=e.direction?speed:f32(speed*bits_to_f32(0x3f7ae148u));
                const f32 vertical=e.entry?f32(actor.vertical_speed+15.f):actor.vertical_speed;
                const f32 accelerated=std::max(0.f,std::fma(change,1.f,moving));
                const f32 retained=change>0||!controller_ground_loss_relaunch_policy?std::min(8.f,accelerated):accelerated;
                 if(e.move_speed!=moving||e.speed44!=(controller_relaunch_push_frame?f32(retained*push_damping):retained)||e.vertical_before!=vertical||e.planar_accel!=change||
                   e.vertical_accel!=acceleration||e.resource_after!=std::max(0,int(resource)-meter.lift_cost(e.entry)))return false;
                auto expected=actor.position;f32 x=0,z=0;source_heading_xz(e.travel5ae,&x,&z);
                expected[0]=std::fma(1.f,f32(moving*x),expected[0]);expected[1]=std::fma(vertical,1.f,expected[1]);
                expected[2]=std::fma(1.f,f32(moving*z),expected[2]);
                const f32 integrated=std::clamp(std::fma(acceleration,1.f,vertical),-30.f,18.f);
                 if(controller_relaunch_push_frame){
                     if(o.static_push->relaunch_acceleration!=acceleration||!project_integrated_push(expected,integrated))return false;
                 }else if(relaunch_floor){
                    const auto& floor=*o.relaunch_floor;
                    if(!controller_relaunch_floor_policy||floor.integrated_position!=expected||floor.vertical_speed!=integrated||
                       floor.acceleration!=acceleration||floor.response_factor!=(integrated<0?bits_to_f32(0x3f4ccccdu):1.f)||
                       o.vertical_speed!=f32(integrated*floor.response_factor))return false;
                    expected[1]=floor.surface.height;
                }else if(o.vertical_speed!=integrated)return false;
                if(o.proposed_position!=expected)return false;
            }
            if(jump_policy){
                const auto* previous=actor.source_ground_motion&&last_offers_[source_index].jump_motion?&*last_offers_[source_index].jump_motion:nullptr;
                if(o.kind==NativeBorgMotionKind::Ground&&previous&&previous->next_selector&&o.landing_target!=12)return false;
                if(o.jump_motion){
                     const auto& e=*o.jump_motion;
                     const bool timed_walk_fall=behaviors.has(Behavior::TimedWalkReleasePush)&&e.phase==6&&e.relaunch_origin&&
                          o.controller_timer698>0.f&&!e.held_jump&&(!e.drift_heading||timed_release_down)&&
                         (o.kind==NativeBorgMotionKind::JumpAir||controller_fall_push_frame)&&
                         (e.entry?(prior==NativeBorgMotionKind::RelaunchEntry&&last_offers_[source_index].relaunch_motion&&
                             last_offers_[source_index].relaunch_motion->timed_walk_relaunch_tap):
                             ((prior==NativeBorgMotionKind::JumpAir&&previous&&previous->timed_walk_relaunch_fall)||
                                timed_release_second||timed_release_third||timed_release_down));
                      if(e.timed_walk_relaunch_fall!=timed_walk_fall||
                          e.second_timed_release_push!=(timed_walk_fall&&timed_release_second)||
                           e.third_timed_release_push!=(timed_walk_fall&&timed_release_third)||
                           e.down_timed_release_push!=(timed_walk_fall&&timed_release_down))return false;
                    if(controller_fall_push_frame&&(e.phase!=6||e.ground_loss||(e.air_dash_origin&&!controller_air_fall_push_policy)||
                        (e.relaunch_origin&&!controller_relaunch_fall_push_policy)||(*o.controller_timer698>0&&!timed_release_push)))return false;
                    const f32 speed_limit=e.air_dash_origin||(controller_ground_loss_relaunch_policy&&e.relaunch_origin)?30.f:25.f;
                    if(e.phase<4||e.phase>7||e.resource_before<0||e.resource_before>meter.limit||
                       e.ground_loss!=(!jump_path)||!std::isfinite(e.speed_before)||e.speed_before<0||e.speed_before>speed_limit||
                       !std::isfinite(e.move_speed)||e.move_speed<0||e.move_speed>speed_limit||!std::isfinite(e.speed44)||e.speed44<0||e.speed44>speed_limit||
                       !std::isfinite(e.vertical_before)||!std::isfinite(e.planar_accel)||!std::isfinite(e.vertical_accel))return false;
                    if(e.air_dash_origin&&(!controller_air_policy||e.ground_loss||e.relaunch_origin))return false;
                    if(e.relaunch_coarse57d>3||(!e.relaunch_origin&&e.relaunch_coarse57d)||
                       (e.relaunch_origin&&(!controller_relaunch_policy||e.ground_loss)))return false;
                    const f32 previous_speed=previous?previous->speed44:
                        controller_air_policy&&last_offers_[source_index].air_motion?last_offers_[source_index].air_motion->speed44:
                        controller_relaunch_policy&&last_offers_[source_index].relaunch_motion?last_offers_[source_index].relaunch_motion->speed44:
                        actor.source_ground_motion&&last_offers_[source_index].walk_ledge?last_offers_[source_index].walk_ledge->speed44:0.f;
                    if(e.speed_before!=previous_speed)return false;
                    if(e.phase==4){
                        const bool first=prior==NativeBorgMotionKind::Ground||
                            (prior==NativeBorgMotionKind::JumpLanding&&previous&&!previous->next_selector)||o.landing_target==4;
                        if(o.kind!=NativeBorgMotionKind::JumpTakeoff||e.ground_loss||e.air_dash_origin||e.relaunch_origin||e.entry!=first||
                           (!first&&(prior!=NativeBorgMotionKind::JumpTakeoff||!previous||previous->next_selector!=4))||
                           e.next_selector!=(!e.entry&&e.lower_done?5:4)||e.move_speed||e.speed44||e.vertical_before||
                           e.planar_accel||e.vertical_accel||e.drift_heading||o.proposed_position!=actor.position||o.vertical_speed)return false;
                    }else if(e.phase==7){
                        const bool first=prior==NativeBorgMotionKind::JumpTouchdown||prior==NativeBorgMotionKind::GroundLossTouchdown;
                        if(o.kind!=NativeBorgMotionKind::JumpLanding||e.entry!=first||
                            e.air_dash_origin!=(previous?previous->air_dash_origin:bool(controller_air_policy&&last_offers_[source_index].air_motion))||
                            e.relaunch_origin!=(previous&&previous->relaunch_origin)||
                            e.relaunch_coarse57d!=(previous?previous->relaunch_coarse57d:0)||
                           (!first&&(prior!=NativeBorgMotionKind::JumpLanding||!previous||previous->next_selector!=7))||
                           e.next_selector!=(!e.entry&&e.lower_done?0:7)||e.move_speed||e.speed44||e.vertical_before||
                           e.planar_accel||e.vertical_accel||e.drift_heading||o.proposed_position!=actor.position||o.vertical_speed)return false;
                     }else if(e.ground_loss){
                         if(e.phase!=6||!ground_loss||e.entry!=(o.kind==NativeBorgMotionKind::GroundLossEntry)||
                            e.next_selector!=(o.kind==NativeBorgMotionKind::GroundLossTouchdown?7:6)||
                            e.planar_accel!=0||e.vertical_accel!=-1||e.vertical_before!=actor.vertical_speed||
                            e.speed44!=o.walk_ledge->speed44||e.travel5ae!=o.walk_ledge->travel5ae||
                            e.drift_heading!=o.walk_ledge->drift_heading)return false;
                         if(ground_loss_push){
                             if(!previous||e.held_jump||e.resource_before!=previous->resource_before||
                                e.speed44!=f32(e.move_speed*push_damping))return false;
                         }else if(e.move_speed!=e.speed44)return false;
                    }else{
                        const auto* air_previous=controller_air_policy&&last_offers_[source_index].air_motion?&*last_offers_[source_index].air_motion:nullptr;
                        const auto* relaunch_previous=controller_relaunch_policy&&last_offers_[source_index].relaunch_motion?&*last_offers_[source_index].relaunch_motion:nullptr;
                        if((!previous&&!air_previous&&!relaunch_previous)||(previous&&previous->ground_loss)||o.ordered_ground||o.walk_ledge||
                            (o.kind!=NativeBorgMotionKind::JumpAir&&o.kind!=NativeBorgMotionKind::JumpTouchdown&&!controller_fall_push_frame))return false;
                        auto expected=actor.position;f32 x=0,z=0,speed=e.speed_before,vertical=actor.vertical_speed,change=0,acceleration=0;
                        if(e.phase==5){
                            if(!previous||e.air_dash_origin||e.relaunch_origin)return false;
                            const bool first=prior==NativeBorgMotionKind::JumpTakeoff&&previous->next_selector==5;
                            if(e.entry!=first||(!first&&(previous->phase!=5||previous->next_selector!=5))||
                               e.next_selector!=5||e.ascent_stage<1||e.ascent_stage>2||e.drift_heading||
                               o.kind!=NativeBorgMotionKind::JumpAir)return false;
                            if(!first&&previous->ascent_stage==1&&
                               (e.direction!=previous->direction||e.travel5ae!=previous->travel5ae))return false;
                            if(first){vertical=15;if(e.direction)speed=std::max(speed,6.f);}
                            f32 sine=0,cosine=1;source_heading_xz(e.direction?8192:0,&sine,&cosine);
                            change=f32(bits_to_f32(0x3f4ccccdu)*sine);
                            acceleration=std::fma(bits_to_f32(0x3f4ccccdu),cosine,0.f);
                            if(!e.direction)speed=f32(speed*bits_to_f32(0x3f7ae148u));
                        }else{
                            const bool from_air=air_previous&&air_previous->phase==17&&air_previous->next_selector==6;
                            const bool from_relaunch=bool(relaunch_previous);
                            const bool first=from_air||from_relaunch||(previous&&previous->phase==5&&previous->next_selector==5);
                            if(e.entry!=first||(!first&&(!previous||previous->phase!=6||previous->next_selector!=6))||
                               (!from_air&&first&&e.held_jump&&e.resource_before>0)||
                                e.air_dash_origin!=(from_air||(previous&&previous->air_dash_origin))||
                                e.relaunch_origin!=(from_relaunch||(previous&&previous->relaunch_origin))||
                                e.relaunch_coarse57d!=(from_relaunch?relaunch_previous->coarse57d:previous?previous->relaunch_coarse57d:0)||
                                e.travel5ae!=(from_air?air_previous->travel5ae:from_relaunch?relaunch_previous->travel5ae:previous->travel5ae)||o.facing!=actor.facing||
                                e.next_selector!=(o.kind==NativeBorgMotionKind::JumpTouchdown?7:6))return false;
                             //Seal release/depletion even on a non-push frame, so later
                             //contact cannot inherit a fabricated falling predecessor.
                             if(controller_relaunch_fall_push_policy&&e.relaunch_origin&&
                                e.resource_before!=(from_relaunch?relaunch_previous->resource_after:previous->resource_before))return false;
                            speed=f32(speed*bits_to_f32(0x3f7d70a4u));acceleration=-1;
                            if(e.drift_heading){
                                source_heading_xz(*e.drift_heading,&x,&z);
                                expected[0]=std::fma(4.f,x,expected[0]);expected[2]=std::fma(4.f,z,expected[2]);
                            }
                        }
                        const f32 final_speed=e.phase==5?std::min(8.f,std::max(0.f,std::fma(change,1.f,speed))):speed;
                         if(e.move_speed!=speed||e.speed44!=(controller_fall_push_frame?f32(final_speed*push_damping):final_speed)||e.vertical_before!=vertical||
                           e.planar_accel!=change||e.vertical_accel!=acceleration)return false;
                        source_heading_xz(e.travel5ae,&x,&z);
                        expected[0]=std::fma(1.f,f32(speed*x),expected[0]);expected[1]=std::fma(vertical,1.f,expected[1]);
                        expected[2]=std::fma(1.f,f32(speed*z),expected[2]);
                         if(controller_fall_push_frame){
                             if(!project_integrated_push(expected,std::max(-30.f,std::fma(acceleration,1.f,vertical))))return false;
                        }else if(o.kind==NativeBorgMotionKind::JumpTouchdown){
                            if(controller_surface_policy){
                                if(e.phase!=6||!surface_landing_height(expected,std::max(-30.f,std::fma(acceleration,1.f,vertical)),acceleration,&expected[1]))return false;
                            }else{
                                NativeBorgStaticSurface current,surface;
                                if(e.phase!=6||!terrain||!terrain->static_surface(actor.position[0],actor.position[2],actor.position[1],&current)||
                                   !terrain->static_surface(expected[0],expected[2],actor.position[1],&surface)||
                                   !native_borg_flat_surface(current)||!native_borg_flat_surface(surface)||current.height!=surface.height||
                                   actor.vertical_speed>=0||expected[1]>surface.height||o.vertical_speed)return false;
                                expected[1]=surface.height;
                            }
                        }else{
                            f32 velocity=std::max(-30.f,std::fma(1.f,acceleration,vertical));
                            if(e.phase==5)velocity=std::min(18.f,velocity);
                            if(o.vertical_speed!=velocity)return false;
                        }
                        if(o.proposed_position!=expected)return false;
                    }
                }
            }
             bool inclined_departure=false;
             if(ordered_policy&&(o.kind==NativeBorgMotionKind::Ground||departure)){
                const auto* previous=actor.source_ground_motion&&last_offers_[source_index].ordered_ground?&*last_offers_[source_index].ordered_ground:nullptr;
                if(!o.ordered_ground){
                    if(o.action==BattleMvpAction::GroundDash||(previous&&previous->next_selector))return false;
                }else{
                    const auto& e=*o.ordered_ground;
                    for(f32 v:{e.speed_before,e.move_speed,e.retained_speed,e.root_speed})if(!std::isfinite(v)||v<0||v>25)return false;
                    if(!std::isfinite(e.timer568)||std::abs(e.timer568)>1e5f||!std::isfinite(e.change4c)||!std::isfinite(e.next_change4c)||
                       e.coarse57d>3||e.recovery546>1||e.next546>1||
                       o.action!=(departure?BattleMvpAction::Jump:BattleMvpAction::GroundDash)||
                       (e.held5b4&~0xf0000u)||!e.root_updates)return false;
                    if(e.root_sampled){
                        const auto root=e.root_scope.lock();
                        if(!root||root->world.lock()!=motion_scope_||root->producer.lock()!=producer||root->tick!=tick_||
                           root->updates!=e.root_updates||root->speed!=e.root_speed||root->evaluated!=e.root_evaluated||
                           root->phase!=e.phase||root->control_before!=e.control_before||root->control_after!=e.control_after||
                           root->program_done!=e.program_done)return false;
                    }else{
                        const std::weak_ptr<const NativeBorgOrderedRootScope> empty;
                        if(e.root_scope.owner_before(empty)||empty.owner_before(e.root_scope)||e.root_evaluated)return false;
                    }
                    if(e.phase==12){
                        if((previous&&previous->next_selector)||!e.root_sampled||!e.root_evaluated||e.root_speed!=0||e.move_speed!=25||
                           e.change4c!=-.5f||e.retained_speed!=24.5f||e.next_change4c!=-.5f||e.timer568!=30||
                           e.next_selector!=13||e.recovery546||e.next546)return false;
                    }else{
                        if(!previous||previous->next_selector!=e.phase||e.speed_before!=previous->retained_speed||
                           e.coarse57d!=previous->coarse57d||
                           e.root_updates!=previous->root_updates+u64(e.root_evaluated)||
                           (!e.root_evaluated&&e.root_speed!=previous->root_speed))return false;
                        f32 speed=e.speed_before,change=previous->next_change4c;u8 stage=0,next=0;
                        if(e.phase==13){
                            if(e.root_sampled!=!(e.control_before&1u)||e.root_evaluated!=e.root_sampled)return false;
                            if(!e.root_sampled&&(e.control_before!=previous->control_after||e.control_after!=e.control_before||
                               e.program_done!=previous->program_done))return false;
                            if(speed<22){speed=22;change=0;}
                            const f32 timer=e.control_after?f32(previous->timer568-1):previous->timer568;
                            constexpr std::array<u32,4> masks={8u<<16,1u<<16,4u<<16,2u<<16};
                            next=e.control_after&&(timer<=0||!(e.held5b4&masks[e.coarse57d]))?14:13;
                            if(e.timer568!=timer||e.recovery546||e.next546)return false;
                        }else if(e.phase==14){
                            if(!e.root_sampled||(!e.root_evaluated&&!e.program_done)||e.timer568!=previous->timer568||
                               (previous->phase!=14&&(e.control_before&2u)))return false;
                            stage=previous->next546;
                            if(!stage){speed=f32(speed*bits_to_f32(0x3f666666u));if(speed<e.root_speed){speed=e.root_speed;stage=1;}}
                            else speed=e.root_speed;
                            next=e.program_done?0:14;
                            if(e.recovery546!=stage||e.next546!=(next?stage:0))return false;
                        }else return false;
                        const bool reset=e.phase==14&&e.program_done;
                        if(e.move_speed!=speed||e.change4c!=change||e.next_selector!=next||
                           e.retained_speed!=(reset?0.f:std::max(0.f,std::fma(change,1.f,speed)))||
                           e.next_change4c!=(reset?0.f:change))return false;
                    }
                     NativeBorgStaticSurface current,selected;
                     if(!terrain||!terrain->static_surface(actor.position[0],actor.position[2],actor.position[1],&current)||
                        !terrain->static_surface(o.proposed_position[0],o.proposed_position[2],actor.position[1],&selected)||
                        current.height!=actor.position[1])return false;
                      const bool start_flat=native_borg_flat_surface(current),end_flat=native_borg_flat_surface(selected);
                      if(departure&&dash_ledge_policy&&behaviors.has(Behavior::InclinedDashLedge)&&
                         !start_flat&&end_flat&&e.move_speed&&native_borg_static_surface_valid(current)&&native_borg_static_surface_valid(selected)&&
                         selected.height<f32(current.height-std::max(60.f,config_.step_height)))
                          inclined_departure=terrain->inclined_ledge_crossing(current.triangle,selected.triangle,actor.position,
                              o.proposed_position,std::max(60.f,config_.step_height));
                      const bool supported_slope=slope_dash_policy&&!departure&&
                          native_borg_static_surface_valid(current)&&native_borg_static_surface_valid(selected)&&
                          ((!start_flat&&current.normal==selected.normal&&(current.triangle==selected.triangle||
                              (slope_seam_dash_policy&&terrain->coplanar_crossing(current.triangle,selected.triangle,
                                  actor.position,o.proposed_position))))||
                           (behaviors.allows_shallow_dash_join(start_flat,end_flat)&&
                            terrain->gentle_slope_crossing(current.triangle,selected.triangle,actor.position,o.proposed_position)))&&
                          std::abs(selected.height-current.height)<=std::min(60.f,config_.step_height);
                      if(supported_slope){
                          if(o.proposed_position[1]!=selected.height)return false;
                       }else if((!inclined_departure&&(!start_flat||!end_flat))||
                         o.proposed_position[1]!=current.height||
                         (departure?(!dash_ledge_policy||selected.height>=f32(current.height-60.f)||!e.move_speed):
                          selected.height!=current.height))return false;
                     auto expected=actor.position;
                     if(e.move_speed&&!native_gred_ground_position(actor.position,e.move_speed,e.travel5ae,current.normal,&expected))return false;
                      if(supported_slope)expected[1]=selected.height;
                     if(o.proposed_position!=expected)return false;
                }
            }
               const bool expiring_walking_push=behaviors.has(Behavior::WalkExpiringDirectRelaunch)&&walking_push&&
                   walking_push->timer698==1.f&&last_offers_[source_index].controller_timer698==1.f;
               const bool descending_up_entry=walking_push&&o.controller_timer698==0.f&&o.relaunch_motion&&o.relaunch_motion->entry&&o.relaunch_motion->direction&&
                   o.relaunch_motion->vertical_before<0.f&&o.vertical_speed<0.f&&player_frame&&
                   native_gred_relaunch_up_turn(*player_frame,actor.facing,o.relaunch_motion->travel5ae,o.facing);
               const bool direct_expiry_relaunch=behaviors.has(Behavior::WalkExpiryDirectRelaunch)&&walking_push&&
                   (walking_push->timer698==0.f||expiring_walking_push)&&o.kind==NativeBorgMotionKind::RelaunchEntry&&descending_up_entry;
               const bool direct_expiring_entry_push=behaviors.has(Behavior::WalkExpiringDirectRelaunchPush)&&expiring_walking_push&&
                   controller_relaunch_push_frame&&relaunch_entry_push&&o.static_push&&descending_up_entry&&
                   NormalGRedMovementDefinition::expiry_relaunch_push_faces(last_offers_[source_index].static_push->surface.triangle,
                       last_offers_[source_index].static_push->angle,o.static_push->surface.triangle,o.static_push->angle,
                       behaviors.has(Behavior::WalkExpiringAdjacentRelaunchPush));
              bool direct_timed_relaunch=false;
              if(behaviors.has(Behavior::WalkTimedDirectRelaunch)&&walking_push&&walking_push->timer698>1.f&&
                  o.controller_timer698&&*o.controller_timer698==walking_push->timer698-1.f&&
                  o.kind==NativeBorgMotionKind::RelaunchEntry&&o.relaunch_motion&&o.relaunch_motion->entry&&
                  actor.vertical_speed<0.f&&o.relaunch_motion->vertical_before>0.f&&o.vertical_speed>0.f){
                  if(o.relaunch_motion->direction){
                      direct_timed_relaunch=behaviors.has(Behavior::WalkTimedDirectUpRelaunch)&&player_frame&&
                          native_gred_relaunch_up_turn(*player_frame,actor.facing,o.relaunch_motion->travel5ae,o.facing);
                  }else{
                      BattlePoint travel;source_heading_xz(o.relaunch_motion->travel5ae,&travel.x,&travel.z);
                      direct_timed_relaunch=travel==actor.facing&&o.facing==actor.facing;
                  }
              }
               const bool relaunch_entry_prior=direct_expiry_relaunch||direct_timed_relaunch||direct_expiring_entry_push||
                 prior==NativeBorgMotionKind::JumpAir||prior==NativeBorgMotionKind::StaticPush||
                native_borg_motion_windup_update(prior)||native_borg_motion_active_entry(prior)||
                native_borg_motion_active_update(prior)||native_borg_motion_recovery(prior)||
                 (controller_ground_loss_relaunch_policy&&(prior==NativeBorgMotionKind::GroundLossEntry||prior==NativeBorgMotionKind::GroundLossAir));
               // Three distinct clocks for a plain wind-up. The 1 -> 0 case
               // does not count as an accepted zero-timer push or inherit its relaunch lineage.
              const bool push_windup=walking_push&&o.kind==NativeBorgMotionKind::AirDashWindupEntry&&o.controller_timer698;
              const bool expiry_windup=behaviors.has(Behavior::WalkLedgeExpiryAirDash)&&push_windup&&
                  walking_push->timer698==0.f&&*o.controller_timer698==0.f;
               const bool timed_push_windup=behaviors.has(Behavior::WalkLedgeDirectTimedAirDash)&&push_windup&&
                   walking_push->timer698>1.f&&*o.controller_timer698==walking_push->timer698-1.f;
             const bool windup_entry_prior=prior==NativeBorgMotionKind::JumpAir||prior==NativeBorgMotionKind::StaticPush||
                 native_borg_motion_recovery(prior)||native_borg_motion_relaunch(prior)||
                  (controller_air_policy&&(prior==NativeBorgMotionKind::GroundLossEntry||prior==NativeBorgMotionKind::GroundLossAir))||
                   expiry_windup||timed_push_windup||expiring_push_windup||expiring_floor_windup;
            if(ledger_frame){
                //Normal G-Red, rank1/fixed60Hz, separately scoped ground loss.
                //numeric validation; no original history/actor callback ACK.
                const auto& e=*o.walk_ledge;
                if(!terrain||e.frame_start!=actor.position||!std::isfinite(e.timer698)||e.timer698<0||e.timer698>30||
                   !std::isfinite(e.speed44)||e.speed44<0||e.speed44>(dash_ledge_policy?25.f:12.f))return false;
                if(actor.source_ground_motion&&!last_offers_[source_index].walk_ledge&&
                   (!jump_policy||o.kind!=NativeBorgMotionKind::Ground||!last_offers_[source_index].jump_motion||
                    (last_offers_[source_index].jump_motion->next_selector&&o.landing_target!=12)))return false;
                const auto previous_timer=controller_landing_jump_policy&&actor.source_ground_motion?*last_offers_[source_index].controller_timer698:
                    actor.source_ground_motion&&last_offers_[source_index].walk_ledge?last_offers_[source_index].walk_ledge->timer698:0.f;
                const f32 timer=o.kind==NativeBorgMotionKind::GroundLossEntry?30.f:std::max(0.f,previous_timer-1.f);
                const bool walked=walk_loss_push_policy&&
                    (departure?!o.ordered_ground:ground_loss&&last_offers_[source_index].walk_ledge&&last_offers_[source_index].walk_ledge->walk_origin);
                if(e.walk_origin!=walked)return false;
                NativeBorgStaticSurface selected;
                if(e.timer698!=timer||!terrain->static_surface(o.proposed_position[0],o.proposed_position[2],actor.position[1],&selected)||
                   selected!=e.surface||!native_borg_static_surface_valid(selected))return false;
                if(o.kind==NativeBorgMotionKind::Ground||departure){
                    NativeBorgStaticSurface current;
                    if(!actor.grounded||actor.vertical_speed||o.vertical_speed||e.drift_heading||
                       !terrain->static_surface(actor.position[0],actor.position[2],actor.position[1],&current)||
                       !native_borg_static_surface_valid(current)||current.height!=actor.position[1])return false;
                    const bool walking_contract=!dash_ledge_policy||(combined_ledge_policy&&!o.ordered_ground);
                    if(!walking_contract){
                        if(o.ordered_ground&&(e.speed44!=o.ordered_ground->retained_speed||e.travel5ae!=o.ordered_ground->travel5ae))return false;
                         if(departure&&(!o.ordered_ground||(!inclined_departure&&
                             (!native_borg_flat_surface(current)||!native_borg_flat_surface(selected)))))return false;
                    }else if(!native_gred_walk_speed(e.speed44)||o.action==BattleMvpAction::GroundDash)return false;
                    if(walking_contract&&delta!=BattlePoint{}){
                        BattleMvpVec expected;
                        if(!native_gred_walk_position(actor.position,e.speed44,e.travel5ae,current.normal,&expected)||
                           expected[0]!=o.proposed_position[0]||expected[2]!=o.proposed_position[2])return false;
                    }
                    if(departure){
                        if(!actor.source_ground_motion||prior!=NativeBorgMotionKind::Ground||delta==BattlePoint{}||
                           o.proposed_position[1]!=actor.position[1]||selected.height>=f32(actor.position[1]-60.f))return false;
                    }else if(o.proposed_position[1]!=selected.height)return false;
                }else if(o.kind==NativeBorgMotionKind::JumpLanding){
                    if((prior!=NativeBorgMotionKind::GroundLossTouchdown&&prior!=NativeBorgMotionKind::JumpLanding)||
                       e.speed44!=0||e.drift_heading)return false;
                    if(controller_ground_loss_surface_policy){
                        s16 angle=0;if(!source_surface_angle(selected.normal,&angle)||angle<0||angle>(e.timer698>0?6144:10240))return false;
                    }else if(!native_borg_flat_surface(selected))return false;
                }else{
                    const bool entry=o.kind==NativeBorgMotionKind::GroundLossEntry;
                    if(actor.grounded||!actor.source_ground_motion||
                       (entry?prior!=NativeBorgMotionKind::GroundDeparture:
                         prior!=NativeBorgMotionKind::GroundLossEntry&&prior!=NativeBorgMotionKind::GroundLossAir&&
                         prior!=NativeBorgMotionKind::GroundLossPush)||
                       (entry&&actor.vertical_speed!=0)||e.travel5ae!=last_offers_[source_index].walk_ledge->travel5ae||o.facing!=actor.facing||
                        (e.timer698>0&&e.drift_heading))return false;
                    const f32 travel_speed=f32(last_offers_[source_index].walk_ledge->speed44*bits_to_f32(0x3f7d70a4u));
                    if(ground_loss_push){
                        // One witnessed boundary only: a neutral update
                        // immediately after the accepted positive-timer push.
                        const bool on_expiry=behaviors.has(Behavior::WalkLedgeExpiryPush)&&timer==0&&previous_timer==1&&
                            prior==NativeBorgMotionKind::GroundLossPush&&!e.drift_heading;
                        if(!walked||(timer<=0&&!on_expiry)||entry||actor.vertical_speed>=0||!o.jump_motion||
                           o.jump_motion->move_speed!=travel_speed||e.speed44!=f32(travel_speed*push_damping))return false;
                    }else if(e.speed44!=travel_speed)return false;
                    auto expected=actor.position;f32 x=0,z=0;
                    if(e.drift_heading){
                        source_heading_xz(*e.drift_heading,&x,&z);
                        expected[0]=std::fma(4.f,x,expected[0]);expected[2]=std::fma(4.f,z,expected[2]);
                    }
                    source_heading_xz(e.travel5ae,&x,&z);
                    expected[0]=std::fma(1.f,f32(travel_speed*x),expected[0]);
                    expected[1]=std::fma(actor.vertical_speed,1.f,expected[1]);
                    expected[2]=std::fma(1.f,f32(travel_speed*z),expected[2]);
                    if(ground_loss_push&&(!project_integrated_push(expected,std::max(-30.f,f32(actor.vertical_speed-1.f)))||
                        o.proposed_position!=expected))return false;
                    if(o.proposed_position[0]!=expected[0]||o.proposed_position[2]!=expected[2]||actor.position[1]<selected.height-.01f)return false;
                    if(o.kind==NativeBorgMotionKind::GroundLossTouchdown){
                        if(controller_ground_loss_surface_policy){
                            if(!surface_landing_height(expected,std::max(-30.f,f32(actor.vertical_speed-1.f)),-1.f,&expected[1])||
                               expected[1]!=selected.height||o.proposed_position!=expected)return false;
                        }else if(!native_borg_flat_surface(selected)||actor.vertical_speed>=0||expected[1]>selected.height||o.proposed_position[1]!=selected.height||o.vertical_speed!=0)return false;
                    }else if(!ground_loss_push&&(o.proposed_position[1]!=expected[1]||expected[1]<=selected.height||
                             o.vertical_speed!=std::max(-30.f,f32(actor.vertical_speed-1.f))))return false;
                }
            }
            if(air_floor){
                //61338 zeros48 on ret1;61560 has no caller tail. Both null754
                //contracts preserve50/history and remain phase2, not65E64.
                const auto& e=*o.air_dash_floor;const bool windup=windup_entry_floor||o.kind==NativeBorgMotionKind::AirDashWindupFloor;
                const f32 contact_timer=controller_ground_loss_air_policy?*o.controller_timer698:0.f;
                const bool entry=o.kind==NativeBorgMotionKind::AirDashActiveEntryFloor;
                const bool phase_prior=windup_entry_floor?windup_entry_prior:
                    windup?(native_borg_motion_windup_entry(prior)||native_borg_motion_windup_update(prior)):
                    entry?native_borg_motion_windup_update(prior):
                    (native_borg_motion_active_entry(prior)||native_borg_motion_active_update(prior));
                //61338 entry halves48 but does NOT translate or apply50.
                //Its ret1 tail then zeros48; the -.07 operand is retained for
                //future updates, not added to this pre-response half-speed.
                const f32 speed=entry||windup_entry_floor?0.f:actor.vertical_speed;
                const f32 acceleration=bits_to_f32(0xbd8f5c29u);
                const f32 pre=windup_entry_floor?f32(actor.vertical_speed*.5f):std::max(-30.f,f32(speed+acceleration));
                if(!terrain||actor.grounded||(positive_windup_entry?actor.vertical_speed<=0:actor.vertical_speed>0)||prior_phase!=2||!phase_prior||
                   e.timer698!=contact_timer||e.acceleration!=acceleration||e.vertical_speed!=pre||o.vertical_speed!=(windup?0.f:pre))return false;
                if(windup_entry_floor&&(delta!=BattlePoint{}||e.integrated_position!=actor.position))return false;
                if(positive_windup_entry&&o.proposed_position!=actor.position)return false;
                for(auto v:e.integrated_position)if(!std::isfinite(v)||std::abs(v)>1e5f)return false;
                NativeBorgStaticSurface surface;s16 angle=0;
                if(e.integrated_position[1]!=f32(actor.position[1]+speed)||std::abs(e.integrated_position[1]-actor.position[1])>50||
                   !terrain->static_surface(e.integrated_position[0],e.integrated_position[2],actor.position[1],&surface)||surface!=e.surface||
                   !std::isfinite(surface.height)||e.integrated_position[1]>surface.height||actor.position[1]<surface.height-.01f||
                    !source_surface_angle(surface.normal,&angle)||angle!=e.angle||angle<0||angle>(contact_timer>0?6144:10240)||
                   o.proposed_position!=BattleMvpVec{e.integrated_position[0],surface.height,e.integrated_position[2]})return false;
                f32 dot=0;for(unsigned k=0;k<3;++k)dot=f32(dot+f32(f32(e.integrated_position[k]-actor.position[k])*surface.normal[k]));
                if(dot>0)return false;
                const BattlePoint movement{e.integrated_position[0]-actor.position[0],e.integrated_position[2]-actor.position[2]};
                if(std::hypot(movement.x,movement.z)>config_.actor_radius)return false;
                NativeBorgContactRequest pre_request{actor.position,other.position,movement,config_.arena_half,config_.actor_radius,config_.actor_height,config_.step_height,false,other.health!=0};
                pre_request.vertical_end_y=e.integrated_position[1];NativeBorgContactResult checked;
                if(!native_borg_resolve_contact(pre_request,terrain,&checked)||checked.body_hit||checked.bounds_hit||checked.terrain_blocked||
                   checked.position[0]!=e.integrated_position[0]||checked.position[2]!=e.integrated_position[2])return false;
            }
            if(relaunch_floor){
                //Null754 ret1: snap height and preserve airborne source history.
                //This is not65E64/landing and must not weaken ordinary air tests.
                const auto& e=*o.relaunch_floor;const bool entry=o.kind==NativeBorgMotionKind::RelaunchEntryFloor;
                const f32 contact_timer=controller_ground_loss_relaunch_policy?*o.controller_timer698:0.f;
                const bool phase_prior=entry?relaunch_entry_prior:
                    o.kind==NativeBorgMotionKind::RelaunchWindupFloor?
                        (native_borg_motion_relaunch_entry(prior)||native_borg_motion_relaunch_windup(prior)):
                        (native_borg_motion_relaunch_windup(prior)||native_borg_motion_relaunch_active(prior));
                const f32 speed=entry?f32(actor.vertical_speed+o.entry_vertical_delta):actor.vertical_speed;
                if(!terrain||actor.grounded||actor.vertical_speed>0||speed>0||prior_phase!=2||!phase_prior||
                    (entry&&o.entry_vertical_delta!=15.f)||e.timer698!=contact_timer||!native_gred_relaunch_acceleration_valid(e.acceleration))return false;
                const f32 pre=std::clamp(f32(speed+e.acceleration),-30.f,18.f);
                if(e.vertical_speed!=pre||e.response_factor!=(pre<0?bits_to_f32(0x3f4ccccdu):1.f)||
                   o.vertical_speed!=f32(pre*e.response_factor))return false;
                for(auto v:e.integrated_position)if(!std::isfinite(v)||std::abs(v)>1e5f)return false;
                NativeBorgStaticSurface surface;s16 angle=0;
                if(e.integrated_position[1]!=f32(actor.position[1]+speed)||std::abs(e.integrated_position[1]-actor.position[1])>50||
                   !terrain->static_surface(e.integrated_position[0],e.integrated_position[2],actor.position[1],&surface)||surface!=e.surface||
                   !std::isfinite(surface.height)||e.integrated_position[1]>surface.height||actor.position[1]<surface.height-.01f||
                    !source_surface_angle(surface.normal,&angle)||angle!=e.angle||angle<0||angle>(contact_timer>0?6144:10240)||
                   o.proposed_position!=BattleMvpVec{e.integrated_position[0],surface.height,e.integrated_position[2]})return false;
                f32 dot=0;for(unsigned k=0;k<3;++k)dot=f32(dot+f32(f32(e.integrated_position[k]-actor.position[k])*surface.normal[k]));
                if(dot>0)return false;
                const BattlePoint movement{e.integrated_position[0]-actor.position[0],e.integrated_position[2]-actor.position[2]};
                if(std::hypot(movement.x,movement.z)>config_.actor_radius)return false;
                NativeBorgContactRequest pre_request{actor.position,other.position,movement,config_.arena_half,config_.actor_radius,config_.actor_height,config_.step_height,false,other.health!=0};
                pre_request.vertical_end_y=e.integrated_position[1];NativeBorgContactResult checked;
                if(!native_borg_resolve_contact(pre_request,terrain,&checked)||checked.body_hit||checked.bounds_hit||checked.terrain_blocked||
                   checked.position[0]!=e.integrated_position[0]||checked.position[2]!=e.integrated_position[2])return false;
            }
            if(push){
                //Narrow native admission for explicitly tagged source push paths. Independently
                //query the PRE-projection plane; source does not re-query after
                //projection, which may finish above a different adjacent floor.
                //Normal G-Red, rank1/fixed60Hz: fall/recovery -1, continued
                //windup/active -.07, lower speed limit -30, zero698 gate10240;
                //wind-up entry only halves48, without integration/clamp.
                //Continued relaunch
                //has separately reviewed positive acceleration/upper18 clamp.
                //These are not universal Borg physics.
                const auto& e=*o.static_push;NativeBorgStaticSurface surface;
                //Source selector6 can be entered from finished dash recovery
                //or released relaunch. It resets50 before this integration;
                //the old policy deliberately cannot borrow this entry contract.
                const bool history_entry=(history_push_policy&&
                    (native_borg_motion_recovery(prior)||native_borg_motion_relaunch(prior)))||
                     (controller_air_fall_push_policy&&native_borg_motion_recovery(prior)&&last_offers_[source_index].air_motion&&
                     last_offers_[source_index].air_motion->phase==17&&last_offers_[source_index].air_motion->next_selector==6&&last_offers_[source_index].air_motion->lower_done)||
                    (controller_relaunch_fall_push_policy&&native_borg_motion_relaunch(prior)&&last_offers_[source_index].relaunch_motion);
                const bool active_prior=native_borg_motion_active_entry(prior)||native_borg_motion_active_update(prior);
                const bool recovery_prior=active_prior||
                    native_borg_motion_recovery(prior);
                const bool windup_prior=windup_entry_push?windup_entry_prior:
                    (native_borg_motion_windup_entry(prior)||native_borg_motion_windup_update(prior));
                const bool relaunch_windup_prior=native_borg_motion_relaunch_entry(prior)||native_borg_motion_relaunch_windup(prior);
                const bool relaunch_active_prior=native_borg_motion_relaunch_windup(prior)||native_borg_motion_relaunch_active(prior);
                const bool relaunch_prior=relaunch_entry_push?relaunch_entry_prior:
                    o.kind==NativeBorgMotionKind::RelaunchWindupPush?relaunch_windup_prior:relaunch_active_prior;
                f32 acceleration=windup_push||active_push?bits_to_f32(0xbd8f5c29u):-1.f;
                if(relaunch_push){
                    if(!e.relaunch_acceleration||!native_gred_relaunch_acceleration_valid(*e.relaunch_acceleration))return false;
                    acceleration=*e.relaunch_acceleration;
                }else if(e.relaunch_acceleration)return false;
                //Relaunch entry adds data4C BEFORE displacement/acceleration.
                //Wind-up entry does not displace. Continued pushes must not
                //borrow either entry contract; old policies refuse them.
                const f32 integrated_vertical=windup_entry_push?0.f:relaunch_entry_push?f32(actor.vertical_speed+o.entry_vertical_delta):actor.vertical_speed;
                if(relaunch_entry_push&&(o.entry_vertical_delta!=15.f||integrated_vertical>0))return false;
                //61338 entry halves old48, but does not translate or apply50.
                //Steep677B0 ret0 then applies .9, NOT the ret1 vertical zero.
                f32 expected_vertical=windup_entry_push?f32(actor.vertical_speed*.5f):std::max(-30.f,f32(integrated_vertical+acceleration));
                if(relaunch_push)expected_vertical=std::min(18.f,expected_vertical);
                if(!terrain||actor.grounded||actor.vertical_speed>0||prior_phase!=2||
                    (!ground_loss_push&&(relaunch_push?!relaunch_prior:active_push?!active_prior:windup_push?!windup_prior:recovery_push?!recovery_prior:(prior!=NativeBorgMotionKind::JumpAir&&prior!=NativeBorgMotionKind::StaticPush&&!history_entry)))||
                   e.damping!=bits_to_f32(0x3f666666u)||e.vertical_speed!=expected_vertical||
                    o.vertical_speed!=f32(e.vertical_speed*e.damping))return false;
                if(windup_entry_push&&e.integrated_position!=actor.position)return false;
                for(auto v:e.integrated_position)if(!std::isfinite(v)||std::abs(v)>1e5f)return false;
                const BattlePoint pre_delta{e.integrated_position[0]-actor.position[0],e.integrated_position[2]-actor.position[2]};
                if(std::hypot(pre_delta.x,pre_delta.z)>config_.actor_radius||
                     e.integrated_position[1]!=f32(actor.position[1]+integrated_vertical)||
                   std::abs(e.integrated_position[1]-actor.position[1])>50||
                   !terrain->static_surface(e.integrated_position[0],e.integrated_position[2],actor.position[1],&surface)||
                   surface!=e.surface||!std::isfinite(surface.height)||e.integrated_position[1]>surface.height)return false;
                 if((timed_release_second||timed_release_third)&&(surface.triangle!=last_offers_[source_index].static_push->surface.triangle||
                    surface.normal!=last_offers_[source_index].static_push->surface.normal))return false;
                 s16 angle=0;BattleMvpVec projected;
                 if(!source_surface_angle(surface.normal,&angle)||angle!=e.angle||
                    (ground_loss_push?(angle<=(o.walk_ledge->timer698>0?6144:10240)||
                         angle>NormalGRedMovementDefinition::walk_ledge_push_angle_cap(behaviors)):
                          timed_release_down?!NormalGRedMovementDefinition::down_timed_release_face(surface.triangle,angle):
                          timed_release_push?(angle<=6144||angle>10240):angle<=10240)||
                   !source_surface_push_position(e.integrated_position,surface.normal,surface.height,angle,&projected)||
                   projected!=o.proposed_position)return false;
                f32 dot=0;for(unsigned k=0;k<3;++k)dot=f32(dot+f32(f32(e.integrated_position[k]-actor.position[k])*surface.normal[k]));
                if(dot>0)return false;
                NativeBorgContactRequest pre{actor.position,other.position,pre_delta,config_.arena_half,config_.actor_radius,config_.actor_height,config_.step_height,false,other.health!=0};
                pre.vertical_end_y=e.integrated_position[1];NativeBorgContactResult checked;
                 if(!native_borg_resolve_contact(pre,terrain,&checked))return false;
                 bool expiry_same_face=false;
                 const auto* jump=o.jump_motion?&*o.jump_motion:nullptr;
                 if(checked.terrain_blocked&&behaviors.has(Behavior::WalkReleaseExpirySameFacePush)&&controller_fall_push_frame&&
                    prior==NativeBorgMotionKind::StaticPush&&last_offers_[source_index].jump_motion&&last_offers_[source_index].jump_motion->down_timed_release_push&&
                    last_offers_[source_index].static_push&&last_offers_[source_index].controller_timer698==1.f&&o.controller_timer698==0.f&&
                    jump&&jump->phase==6&&jump->relaunch_origin&&!jump->entry&&!jump->held_jump&&jump->drift_heading&&
                    player_frame&&actor.vertical_speed<0.f&&NormalGRedMovementDefinition::down_timed_release_face(surface.triangle,angle)&&
                    last_offers_[source_index].static_push->surface.triangle==surface.triangle&&last_offers_[source_index].static_push->surface.normal==surface.normal&&
                    last_offers_[source_index].static_push->angle==angle){
                     CharacterMovementHeadingFrame heading;
                     NativeBorgStaticSurface start;
                     expiry_same_face=native_borg_source_heading_frame(*player_frame,actor.facing,&heading)&&
                         *jump->drift_heading==heading.camera5b0&&
                         terrain->static_surface(actor.position[0],actor.position[2],actor.position[1],&start)&&
                         start.triangle==surface.triangle&&start.normal==surface.normal&&
                         terrain->same_face_push_exit(surface.triangle,actor.position,e.integrated_position,o.proposed_position);
                 }
                 if(jump&&jump->expiry_same_face_push!=expiry_same_face)return false;
                 if(expiry_same_face){
                     // Recheck the full, nonsliding approach for body and bounds.
                     // The footprint proof replaces only this endpoint height test.
                     if(!native_borg_resolve_contact(pre,nullptr,&checked))return false;
                 }
                 if(checked.body_hit||checked.bounds_hit||checked.terrain_blocked||
                    checked.position[0]!=e.integrated_position[0]||checked.position[2]!=e.integrated_position[2])return false;
            }
            if(!vertical&&(prior_phase==1||prior_phase==2||prior_phase==3))return false;
            if(vertical&&prior_phase&&moving!=(unsigned(prior)>=unsigned(NativeBorgMotionKind::JumpTakeoff)))return false;
            const bool air=phase==2||phase==3;
            NativeBorgContactRequest request{actor.position,other.position,delta,config_.arena_half,config_.actor_radius,config_.actor_height,config_.step_height,air?false:actor.grounded,other.health!=0};
            if(air)request.vertical_end_y=o.proposed_position[1];
            if(!native_borg_resolve_contact(request,terrain,contact))return false;
            if((push||floor_snap||ledge_policy||ordered_policy)&&(contact->body_hit||contact->bounds_hit||contact->terrain_blocked||
               contact->position[0]!=o.proposed_position[0]||contact->position[2]!=o.proposed_position[2]))return false;
            if(vertical){
                f32 y=0;
                if((!moving&&(delta!=BattlePoint{}||o.facing!=actor.facing))||
                   !floor(terrain,contact->position[0],contact->position[2],actor.position[1],&y)||
                   std::abs(o.proposed_position[1]-actor.position[1])>50||
                   (o.action!=(air_dash?BattleMvpAction::AirDash:phase==4?BattleMvpAction::Land:BattleMvpAction::Jump)))return false;
                if(phase==1){
                    if(delta!=BattlePoint{}||!actor.grounded||actor.vertical_speed||o.vertical_speed||(!moving&&actor.movement_speed)||
                       (prior_phase!=0&&prior_phase!=1&&prior_phase!=4)||
                       std::abs(o.proposed_position[1]-y)>.01f)return false;
                }else if(phase==2){
                    if(!actor.source_ground_motion||(prior_phase!=1&&prior_phase!=2&&!departure)||
                        (floor_snap?std::abs(o.proposed_position[1]-y)>.01f:push?o.proposed_position[1]<y-.01f:o.proposed_position[1]<=y)||
                        (!push&&!air_dash&&!relaunch&&prior_phase==2&&std::abs(o.proposed_position[1]-actor.position[1]-actor.vertical_speed)>.01f)||
                       (prior_phase==1&&(o.proposed_position[1]<=actor.position[1]||o.vertical_speed<=0)))return false;
                    if(air_dash){
                        if(actor.grounded||prior_phase!=2)return false;
                        const bool entry=native_borg_motion_windup_entry(o.kind)||native_borg_motion_active_entry(o.kind);
                        if(!push&&!floor_snap&&std::abs(o.proposed_position[1]-actor.position[1]-(entry?0.f:actor.vertical_speed))>.01f)return false;
                        if(native_borg_motion_windup_entry(o.kind)){
                              if(!windup_entry_prior)return false;
                            if(!windup_entry_push&&delta!=BattlePoint{})return false;
                        }else if(native_borg_motion_windup_update(o.kind)){
                              if(!native_borg_motion_windup_entry(prior)&&!native_borg_motion_windup_update(prior))return false;
                        }else if(native_borg_motion_active_entry(o.kind)){
                             if(!native_borg_motion_windup_update(prior))return false;
                        }else if(native_borg_motion_active_update(o.kind)){
                            if(!native_borg_motion_active_entry(prior)&&!native_borg_motion_active_update(prior))return false;
                        }else if(!native_borg_motion_active_entry(prior)&&!native_borg_motion_active_update(prior)&&!native_borg_motion_recovery(prior))return false;
                    }else if(relaunch){
                        if(actor.grounded||prior_phase!=2||
                            (!push&&!floor_snap&&std::abs(o.proposed_position[1]-actor.position[1]-actor.vertical_speed-o.entry_vertical_delta)>.01f))return false;
                        if(native_borg_motion_relaunch_entry(o.kind)){
                             if(!relaunch_entry_prior)return false;
                        }else if(native_borg_motion_relaunch_windup(o.kind)){
                            if(!native_borg_motion_relaunch_entry(prior)&&!native_borg_motion_relaunch_windup(prior))return false;
                        }else if(!native_borg_motion_relaunch_windup(prior)&&!native_borg_motion_relaunch_active(prior))return false;
                    }else if(native_borg_motion_air_dash(prior)&&!native_borg_motion_recovery(prior))return false;
                }else if(phase==3){
                    if(prior_phase!=2||(recovery_recontact?actor.vertical_speed!=0:actor.vertical_speed>=0)||o.vertical_speed||
                       actor.position[1]+actor.vertical_speed>y+.01f||std::abs(o.proposed_position[1]-y)>.01f)return false;
                }else if(delta!=BattlePoint{}||(prior_phase!=3&&prior_phase!=4))return false;
                if(!air&&(!actor.grounded||o.vertical_speed||std::abs(o.proposed_position[1]-y)>.01f))return false;
                //No original standing-on-Borg/vertical-solid response is known.
                //Refuse an overlap reached from above rather than tunnelling or
                //inventing a floor on the opponent. Shared planar sweep handles
                //the ordinary inward horizontal contact before this check.
                if(moving&&air&&other.health&&std::min(actor.position[1],o.proposed_position[1])<other.position[1]+config_.actor_height&&
                   std::max(actor.position[1],o.proposed_position[1])>other.position[1]-config_.actor_height&&
                   std::hypot(contact->position[0]-other.position[0],contact->position[2]-other.position[2])<2*config_.actor_radius-.001f)return false;
                contact->position[1]=o.proposed_position[1];contact->grounded=phase!=2;
            }else if(!contact->grounded||std::abs(contact->position[1]-o.proposed_position[1])>.01f)return false;
            return true;
        };
        std::array<NativeBorgContactResult,kBattleMvpMaxActors> source_contacts{};
        std::array<bool,kBattleMvpMaxActors> source_ready{};
        for(unsigned i=0;i<actor_count();++i){
            const auto* offer=offered(i);
            if(!offer)continue;
            const u8 target=actor_target(i);
            const unsigned other_index=target==255?(i==0?1:0):target;
            const GamePadState no_pad{};
            const auto& other_pad=other_index<pads.size()?pads[other_index]:no_pad;
            if(!prepare_source(i,*offer,actors_[i],actors_[other_index],pads[i],other_pad,&source_contacts[i]))return false;
            source_ready[i]=true;
        }
        if(opponent_ai)for(unsigned i=1;i<actor_count();++i)pad_store[i]=ai_input(i);
        //Preflight terrain before mutating a tick. A missing grid is not floor0.
        std::array<f32,kBattleMvpMaxActors> ground{};
        for(unsigned i=0;i<actor_count();++i)if(!floor(terrain,actors_[i].position[0],actors_[i].position[2],actors_[i].position[1],&ground[i]))return false;
        if(config_.local_aim_cycle)for(unsigned i=0;i<local_count;++i){
            if(!actors_[i].health||!aim_eligible(i,local_aim_[i]))local_aim_[i]=255;
            if(actors_[i].health&&(local_input.cycle_aim&(1u<<i)))cycle_package_aim(i);
        }
        cpu_aim_.fill(255);
        std::copy(local_input.cpu_aim.begin(),local_input.cpu_aim.end(),cpu_aim_.begin());
        ++tick_;
        package_event_count_=0;
        for(auto& field:fields_)if(field.id&&tick_-field.born>=field.spec.ticks)field={};
        for(auto& field:fields_)if(field.id&&field.spec.drift&&field.speed>0){
            field.centre[0]+=field.heading.x*field.speed;field.centre[2]+=field.heading.z*field.speed;
            field.speed=std::max(0.f,field.spec.drift->speed-f32(tick_-field.born)*field.spec.drift->deceleration);
            if(std::abs(field.centre[0])>config_.arena_half||std::abs(field.centre[2])>config_.arena_half)field={};
            else if(field.speed==0&&field.spec.slot&&field.spec.slot->on_ready)replace_package_field_slot(field);
        }
        for(auto& cloud:smoke_clouds_)if(cloud.parent&&tick_-cloud.born>cloud.spec.ticks)cloud={};
        for(auto& ink:ink_)if(ink.object&&tick_-ink.born>=ink.spec.ticks)ink={};
        for(auto& effect:accuracy_)if(effect.object&&tick_-effect.born>effect.spec.ticks)effect={};
        child_request_count_=0;
        std::array<bool,kBattleMvpMaxActors> previously_grounded{},airborne_jump{},package_interrupted{},pulled{};
        std::array<f32,kBattleMvpMaxActors> walk_distance{};
        std::array<BattleMvpVec,kBattleMvpMaxActors> before{};
        for(unsigned i=0;i<actor_count();++i){
            previously_grounded[i]=actors_[i].grounded;
            package_interrupted[i]=actors_[i].hit_flash!=0;
            before[i]=actors_[i].position;
        }
        if(package)expire_package_fence_anchors();
        if(package)for(unsigned i=0;i<actor_count();++i){
            const bool jump=pads[i].b&&!previous_[i].b;
            const bool pulled_now=consume_package_pull(i,terrain,jump);
            const bool rewound=consume_package_rewind(i,terrain,jump||pulled_now);
            const bool dash=pads[i].l||pads[i].trigger_l_pressed;
            const bool recoil=consume_package_fence_recoil(i,terrain,jump||dash||pulled_now||rewound);
            pulled[i]=pulled_now||rewound||recoil||consume_package_field_pull(i,terrain,jump||dash);
        }
        for(auto& effect:impacts_)if(effect.ticks)--effect.ticks;
        for(auto& effect:impact_records_)if(effect.ticks)--effect.ticks;
        for(unsigned i=0;i<actor_count();++i){
            auto& actor=actors_[i];
            if(actor.reaction_ticks){
                if(!--actor.reaction_ticks){actor.reaction_yaw=actor.reaction_pitch=0;}
                else{actor.reaction_yaw*=.75f;actor.reaction_pitch*=.75f;}
            }
            if(actor.impact_state)--actor.impact_state;
            if(actor.contact_sound_cooldown)--actor.contact_sound_cooldown;
            if(actor.contact_pose_hold_ticks)--actor.contact_pose_hold_ticks;
            if(!actor.health){actor.action=BattleMvpAction::Defeated;continue;}
            for(auto& cooldown:actor.cooldown)if(cooldown)--cooldown;
            if(actor.attack_ticks)--actor.attack_ticks;
            if(actor.hit_flash)--actor.hit_flash;
            actor.damage_tier.advance();
            if(pulled[i])continue; // one accepted movement for this actor visit
            if(source_ready[i]){
                const auto& contact=source_contacts[i];const auto* offer=offered(i);
                actor.position=contact.position;actor.velocity=contact.actual_delta;actor.facing=offer->facing;
                actor.movement_speed=std::hypot(actor.velocity.x,actor.velocity.z);actor.travel_distance+=actor.movement_speed;
                actor.grounded=contact.grounded;actor.vertical_speed=offer->vertical_speed;actor.action=offer->action;actor.source_ground_motion=true;
                actor.takeoff_ticks=actor.landing_ticks=actor.dash_ticks=actor.dash_cooldown=actor.air_dashes_used=0;actor.dash_airborne=false;
                actor.move_forward=actor.movement_speed?(actor.velocity.x*actor.facing.x+actor.velocity.z*actor.facing.z)/actor.movement_speed:0;
                actor.move_right=actor.movement_speed?(-actor.velocity.x*actor.facing.z+actor.velocity.z*actor.facing.x)/actor.movement_speed:0;
                const u8 aim_target=actor_target(i);
                actor.aim_facing=solo||aim_target==255?actor.facing:native_borg_direction({actor.position[0],actor.position[2]},
                    {actors_[aim_target].position[0],actors_[aim_target].position[2]},actor.facing);actor.upper_body_yaw=0;
                continue;
            }
            actor.source_ground_motion=false;
            const auto* definition=config_.packages[i].get();
            if(definition&&definition->movement==BorgBattleMovement::Stationary)continue;
            const auto* input_frame=i==0?player_frame:i==1?peer_frame:nullptr;
            const auto& cruise=definition?definition->cruise:config_.provisional_cruise;
            if(cruise){
                const auto multiplier=definition?package_actor_slow(i):1.f;
                const auto base=definition?definition->walk:config_.move_speed;
                const auto boost=definition?definition->dash:config_.dash_speed;
                if(!step_cruise(i,*cruise,base,boost,multiplier,pads[i],input_frame,terrain,package_interrupted[i])){
                    ready_=false;return false;
                }
                continue;
            }
            const auto& modifier=walk_modifiers_[i];
            const f32 slo=definition?package_actor_slow(i):1.f;
            const f32 walk=definition?definition->walk*(modifier.ticks?f32(modifier.percent)/100.f:1.f)*slo:config_.move_speed;
            const f32 jump=definition?definition->jump:config_.jump_speed;
            const f32 dash_speed=definition?definition->dash*slo:config_.dash_speed;
            if(actor.landing_ticks)--actor.landing_ticks;
            if(actor.takeoff_ticks)--actor.takeoff_ticks;
            if(actor.dash_cooldown)--actor.dash_cooldown;
            const bool lab_air=config_.provisional_air_dash;
            const bool start_air_burst=actor.lab_air_windup_ticks&&!--actor.lab_air_windup_ticks;
            if(actor.lab_air_recovery_ticks)--actor.lab_air_recovery_ticks;
            if(lab_air&&actor.dash_airborne&&actor.dash_ticks==1)
                actor.lab_air_recovery_ticks=BattleMvpActor::lab_air_recovery;
            if(actor.dash_ticks)--actor.dash_ticks;
            if(pads[i].b&&!previous_[i].b){
                if(actor.grounded){
                    actor.grounded=false;actor.vertical_speed=jump;actor.takeoff_ticks=BattleMvpActor::takeoff_duration;
                    actor.landing_ticks=actor.dash_ticks=actor.air_dashes_used=0;
                    actor.package_air_points_used=definition&&(definition->air_jump_points||definition->air_action_budget)?1:0;
                }else if(definition&&actor.package_air_points_used<
                         (definition->air_action_budget?definition->air_action_budget:definition->air_jump_points)){
                    ++actor.package_air_points_used;actor.vertical_speed=jump;actor.dash_ticks=0;
                    airborne_jump[i]=true;
                }
            }
            const bool was_gliding=actor.lab_gliding;
            actor.lab_gliding=config_.provisional_glide&&!actor.grounded&&!actor.takeoff_ticks&&
                pads[i].b&&actor.vertical_speed<=0;
            if(!actor.lab_gliding)actor.lab_glide_entry_ticks=0;
            else if(!was_gliding)actor.lab_glide_entry_ticks=BattleMvpActor::lab_glide_entry;
            else if(actor.lab_glide_entry_ticks)--actor.lab_glide_entry_ticks;
            if(!actor.grounded){
                if(actor.lab_gliding)actor.vertical_speed=std::max(-BattleMvpActor::lab_glide_sink,actor.vertical_speed);
                if(definition&&pads[i].b&&actor.package_flight_used<definition->flight_ticks&&!actor.dash_ticks){
                    actor.vertical_speed=std::max(actor.vertical_speed,definition->flight_lift);
                    ++actor.package_flight_used;
                }
                if(lab_air||!(actor.dash_ticks&&actor.dash_airborne)){actor.position[1]+=actor.vertical_speed;actor.vertical_speed-=config_.gravity;}
                if(actor.position[1]<=ground[i]){
                    actor.position[1]=ground[i];actor.vertical_speed=0;actor.grounded=true;
                    actor.landing_ticks=BattleMvpActor::landing_duration;actor.takeoff_ticks=actor.dash_ticks=0;
                    actor.air_dashes_used=actor.package_air_points_used=actor.package_flight_used=0;
                    actor.lab_air_windup_ticks=actor.lab_air_recovery_ticks=actor.lab_glide_entry_ticks=0;actor.lab_gliding=false;
                }
            }
            f32 x=axis(pads[i].stick_x),z=axis(pads[i].stick_y);
            x+=f32(pads[i].dpad_right)-f32(pads[i].dpad_left);z+=f32(pads[i].dpad_down)-f32(pads[i].dpad_up);unit(x,z,false);
            const auto direction=native_borg_move_direction(x,z,input_frame?*input_frame:NativeBorgMovementFrame{});
            const bool has_input=direction.x!=0||direction.z!=0;
            const bool dash_held=pads[i].l||pads[i].trigger_l_pressed,previous_dash=previous_[i].l||previous_[i].trigger_l_pressed;
            const bool dash_allowed=config_.provisional_walk?
                (actor.grounded?config_.provisional_ground_dash&&previously_grounded[i]:
                 lab_air&&!previously_grounded[i]&&!actor.takeoff_ticks&&!actor.air_dashes_used):
                 ((!definition||!definition->dash_disabled)&&(actor.grounded||
                   ((!definition||!definition->air_dash_disabled)&&actor.air_dashes_used<config_.air_dashes&&
                    (!definition||!definition->air_action_budget||
                     actor.package_air_points_used<definition->air_action_budget))));
            if(start_air_burst&&!actor.grounded){
                actor.dash_airborne=true;actor.dash_ticks=actor.dash_duration=BattleMvpActor::lab_air_burst;
            }
            const bool dash_cooldown_clear=(lab_air&&!actor.grounded)||!actor.dash_cooldown;
            if(dash_held&&!previous_dash&&!actor.dash_ticks&&!actor.lab_air_windup_ticks&&
               !actor.lab_air_recovery_ticks&&dash_cooldown_clear&&dash_allowed){
                auto dash=has_input?direction:actor.facing;unit(dash.x,dash.z,true);
                const f32 front=dash.x*actor.facing.x+dash.z*actor.facing.z,side=-dash.x*actor.facing.z+dash.z*actor.facing.x;
                actor.dash_motion=std::abs(side)>std::abs(front)?(side<0?BattleMvpDashDirection::Left:BattleMvpDashDirection::Right):(front<0?BattleMvpDashDirection::Backward:BattleMvpDashDirection::Forward);
                actor.dash_direction=dash;
                if(lab_air&&!actor.grounded){
                    actor.lab_air_windup_ticks=BattleMvpActor::lab_air_windup;++actor.air_dashes_used;
                }else{
                    actor.dash_ticks=actor.dash_duration=config_.dash_duration;actor.dash_cooldown=config_.dash_cooldown;actor.dash_airborne=!actor.grounded;
                    if(actor.dash_airborne){++actor.air_dashes_used;actor.vertical_speed=0;actor.takeoff_ticks=0;actor.facing=dash;
                        if(definition&&definition->air_action_budget)++actor.package_air_points_used;} //classified air dash always faces its travel direction
                }
            }
            if(actor.dash_ticks){
                if(has_input&&!config_.provisional_walk){actor.dash_direction=direction;unit(actor.dash_direction.x,actor.dash_direction.z,true);}
                if(actor.dash_airborne)actor.facing=actor.dash_direction;
                else{
                    const auto dash=actor.dash_direction;const f32 front=dash.x*actor.facing.x+dash.z*actor.facing.z,side=-dash.x*actor.facing.z+dash.z*actor.facing.x;
                    actor.dash_motion=std::abs(side)>std::abs(front)?(side<0?BattleMvpDashDirection::Left:BattleMvpDashDirection::Right):(front<0?BattleMvpDashDirection::Backward:BattleMvpDashDirection::Forward);
                }
                actor.velocity={actor.dash_direction.x*dash_speed,actor.dash_direction.z*dash_speed};
            }
            else{
                //All burst exits (expiry, jump, landing, collision or ledge) return
                //to the same bounded controller, not a flight of leftover dash speed.
                const f32 pace=actor.grounded&&config_.provisional_ground_pace?f32(*config_.provisional_ground_pace)*.01f:walk;
                const f32 speed=std::hypot(actor.velocity.x,actor.velocity.z);
                if(speed>pace){actor.velocity.x*=pace/speed;actor.velocity.z*=pace/speed;}
                actor.velocity=native_borg_velocity(actor.velocity,{direction.x*pace,direction.z*pace},actor.grounded?(has_input?config_.ground_acceleration:config_.ground_braking):config_.air_acceleration);
            }
            const bool was_dashing=actor.dash_ticks!=0;
            const u8 contact_slot=actor_target(i);
            NativeBorgContactResult contact;
            if(!resolve_native_body_motion(i,contact_slot==255?(i==0?1:0):contact_slot,
                                          actor.velocity,actor.grounded,terrain,&contact)){ready_=false;return false;}
            if(config_.provisional_walk&&!config_.provisional_jump&&!contact.grounded){
                contact.position=actor.position;contact.actual_delta={};contact.grounded=true;
            }
            actor.position=contact.position;actor.grounded=contact.grounded;actor.velocity=contact.actual_delta;
            actor.movement_speed=std::hypot(actor.velocity.x,actor.velocity.z);actor.travel_distance+=actor.movement_speed;
            if(actor.dash_ticks&&(contact.terrain_blocked||contact.body_hit||actor.movement_speed<dash_speed*.99f||(!actor.grounded&&!actor.dash_airborne))){
                actor.dash_ticks=0;
                if(lab_air&&actor.dash_airborne&&!actor.grounded)actor.lab_air_recovery_ticks=BattleMvpActor::lab_air_recovery;
            }
            if(lab_air&&!actor.dash_ticks)actor.dash_airborne=false;
            const bool moving=actor.movement_speed>.05f;
            if(has_input&&!was_dashing&&actor.grounded&&previously_grounded[i]&&!actor.landing_ticks&&!package_interrupted[i])
                walk_distance[i]=actor.movement_speed;
            const bool strafe=config_.package_strafe&&actor.grounded&&(pads[i].r||pads[i].trigger_r_pressed);
            if(has_input&&!was_dashing&&!strafe){
                if(moving)actor.facing=native_borg_turn(actor.facing,actor.velocity,config_.turn_per_tick);
                // A grounded roster CPU can steer even if a solid peer
                // canceled translation. Use its stick direction, not its
                // aim target, and keep the normal angular limit.
                else if(config_.local_aim_cycle&&i>=local_count&&actor.grounded&&previously_grounded[i]&&
                        !actor.landing_ticks&&!package_interrupted[i]&&!pulled[i]&&contact.body_hit&&!contact.terrain_blocked)
                    actor.facing=native_borg_turn(actor.facing,direction,config_.turn_per_tick);
            }
            if(solo)actor.aim_facing=actor.facing;
            else{
                const u8 aim_slot=actor_target(i);
                actor.aim_facing=aim_slot==255?actor.facing:native_borg_direction({actor.position[0],actor.position[2]},
                    {actors_[aim_slot].position[0],actors_[aim_slot].position[2]},actor.facing);
            }
            const f32 speed=std::max(actor.movement_speed,.0001f);
            actor.move_forward=(actor.velocity.x*actor.facing.x+actor.velocity.z*actor.facing.z)/speed;
            actor.move_right=(-actor.velocity.x*actor.facing.z+actor.velocity.z*actor.facing.x)/speed;
            if(!solo&&!package&&pads[i].a&&!actor.cooldown[0])fire(i,false,muzzles.empty()?nullptr:&muzzles[i]);
            if(!solo&&!package&&pads[i].x&&!actor.cooldown[1])fire(i,true,muzzles.empty()?nullptr:&muzzles[i]);
            if(actor.attack_ticks)actor.upper_body_yaw=native_borg_aim_delta(actor.facing,actor.aim_facing);
            else actor.upper_body_yaw+=std::clamp(-actor.upper_body_yaw,-.2f,.2f);
            actor.action=actor.dash_ticks?(actor.dash_airborne?BattleMvpAction::AirDash:BattleMvpAction::GroundDash):!actor.grounded?BattleMvpAction::Jump:actor.landing_ticks?BattleMvpAction::Land:actor.attack_ticks?BattleMvpAction::Attack:moving?BattleMvpAction::Move:BattleMvpAction::Idle;
        }
        if(config_.provisional_jump){
            auto& actor=actors_[0];
            // The solo mixer only consumes an accepted native ground-contact
            // edge. No retail gain guard, rise loop, or air-jump is inferred.
            actor.package_movement_cues=!previously_grounded[0]&&actor.grounded&&actor.landing_ticks?
                u8(BorgMovementCue::Landing):0;
        }
        if(package)for(unsigned i=0;i<actor_count();++i){
            auto& actor=actors_[i];
            u8 cues=0;
            if(previously_grounded[i]&&!actor.grounded&&actor.vertical_speed>0)cues|=u8(BorgMovementCue::Takeoff);
            if(!previously_grounded[i]&&actor.grounded)cues|=u8(BorgMovementCue::Landing);
            if(!actor.grounded&&actor.vertical_speed>0)cues|=u8(BorgMovementCue::Rise);
            if((airborne_jump[i]&&!actor.grounded)||(source_ready[i]&&native_borg_motion_relaunch_entry(offered(i)->kind)))
                cues|=u8(BorgMovementCue::AirJump);
            actor.package_movement_cues=cues;
        }
        if(package)for(unsigned i=0;i<actor_count();++i)step_package_emitters(i);
        if(package)for(unsigned i=0;i<actor_count();++i){
            auto& actor=actors_[i];auto& action=package_actions_[i];
            if(!actor.health)continue;
            settle_package_pairs(i);
            auto& modifier=walk_modifiers_[i];
            if(modifier.ticks&&!--modifier.ticks){
                record_package(BorgBattleEventKind::ModifierExpired,modifier.owner,modifier.move,0,modifier.object,0,u8(i));
                modifier={};
            }
            const u8 package_target=package_aim_target(i);
            const bool has_target=package_target!=255;
            const auto& other=actors_[has_target?package_target:(i==0?1:0)];
            actor.package_target_visible=has_target&&other.health&&!package_smoke_blocks_aim(i,package_target);
            const auto tracking=actor.package_target_visible?native_borg_direction({actor.position[0],actor.position[2]},
                {other.position[0],other.position[2]},actor.facing):actor.facing;
            actor.aim_facing=actor.package_aim_heading.value_or(tracking);
            const auto range=has_target&&other.health?std::optional{std::hypot(other.position[0]-actor.position[0],
                                                                    other.position[2]-actor.position[2])}:std::nullopt;
            u8 available_anchors=0;
            for(unsigned set=0;set<kBorgBattleAnchorSets;++set)
                if(anchor_paths_[i][set].count)available_anchors|=u8(1u<<set);
            action.visit(*config_.packages[i],package_input[i],actor.grounded,package_interrupted[i],
                [&](const auto& shot,u8 move,u8 return_stock){return emit_package(i,move,shot,return_stock,origins);},
                [&](auto kind,u8 move,u16 frame,u32 object,u16 value){
                    if(kind==BorgBattleEventKind::Started){
                        actor.package_action_born=tick_;
                        const auto& package=*config_.packages[i];
                        if(move<package.moves.size()&&!package.moves[move].action_sound.empty())
                            record_package(BorgBattleEventKind::ActionSound,i,move,0,move,0);
                        if(move<package.moves.size()&&package.moves[move].charge){
                            // The source movement program owns the burst; the
                            // native controller reuses its existing dash state.
                            const auto ticks=package.moves[move].charge->ticks;
                            actor.package_charge_total=u16(ticks);
                            actor.dash_ticks=actor.dash_duration=ticks;
                            actor.dash_direction=actor.facing;
                            actor.dash_motion=BattleMvpDashDirection::Forward;
                            actor.dash_airborne=!actor.grounded;
                            if(actor.dash_airborne){++actor.air_dashes_used;actor.vertical_speed=0;actor.takeoff_ticks=0;}
                        }
                    }
                    if(kind==BorgBattleEventKind::Completed||kind==BorgBattleEventKind::Interrupted)
                        actor.package_charge_total=0;
                    if(kind==BorgBattleEventKind::Started||kind==BorgBattleEventKind::Repeated||
                       kind==BorgBattleEventKind::Completed||kind==BorgBattleEventKind::Interrupted){
                         actor.package_aim_heading.reset();actor.package_aim_slope=0;actor.aim_facing=tracking;
                    }
                    if(kind==BorgBattleEventKind::AimCommitted){
                         actor.aim_facing=tracking;
                         actor.package_aim_heading=actor.aim_facing;
                        // A frozen direction, not a pointer or a launch-time
                        // re-aim. Pitch is a centre-height proxy for this API.
                        actor.package_aim_slope=actor.package_target_visible&&range?
                            std::clamp((other.position[1]-actor.position[1])/std::max(1.f,*range),-1.f,1.f):0;
                    }
                    record_package(kind,i,move,frame,object,value);
                    if(kind==BorgBattleEventKind::Emitted)emit_package_followers(i,move,frame);
                },range,available_anchors,[&](u8 move,u8 stock,u32 first,u32 second){
                    pair_returns_[i][stock]={true,{first,second},move};
                },actor.package_target_visible?std::optional{native_borg_aim_delta(actor.facing,tracking)}:std::nullopt);
            actor.package_move=action.move;actor.package_frame=action.frame;
            if(action.move!=BorgBattleActionState::none)actor.upper_body_yaw=native_borg_aim_delta(actor.facing,actor.aim_facing);
        }
        if(package){
            for(auto& field:fields_)if(field.id&&field.spec.follow){
                const auto& source=actors_[field.owner];
                if(!source.health){field={};continue;}
                if(field.spec.socket){
                    if(!sample_field_socket(field.owner,source,*field.spec.socket,origins,&field.centre)){
                        record_package(BorgBattleEventKind::FieldAnchorLost,field.owner,field.move,0,field.id,field.spec.socket->bone);
                        field={};continue;
                    }
                }else for(unsigned a=0;a<3;++a)field.centre[a]=source.position[a]+field.offset[a];
                if(std::abs(field.centre[0])>config_.arena_half||std::abs(field.centre[2])>config_.arena_half)field={};
            }
            for(unsigned i=0;i<actor_count();++i){deploy_package_emitters(i);place_package_field(i,origins);}
        }
        if(package)for(unsigned i=0;i<actor_count();++i)visit_package_anchor_markers(i);
        if(package)for(unsigned i=0;i<actor_count();++i)step_package_guard(i);
        if(package)for(unsigned i=0;i<actor_count();++i)step_package_melee(i,terrain);
        if(package)step_package_fences(before);
        if(package){step_package_walk_pulses(walk_distance);step_package_field_cores();}
        if(package)for(unsigned i=0;i<actor_count();++i)step_package_detonation(i,terrain);
        for(auto& shot:projectiles_)if(shot.active){
            shot.previous=shot.position;
            if(shot.from_package){
                if(shot.sustained){
                    const auto& pad=pads[shot.owner];
                     step_sustained_beam(shot,std::clamp(axis(pad.stick_x)+f32(pad.dpad_right)-f32(pad.dpad_left),-1.f,1.f),terrain);
                }else if(!consume_package_remote(shot,terrain))step_package_shot(shot,terrain);
                if(shot.active&&!shot.landed){
                    const auto& t=config_.packages[shot.owner]->projectiles[shot.package_projectile].record_trail;
                    const auto age=tick_-shot.born_tick;
                    if(t&&age&&age%t->interval==0&&shot.position!=shot.previous){
                        shot.record_trail_positions[shot.record_trail_cursor]=shot.previous;
                        shot.record_trail_cursor=u8((shot.record_trail_cursor+1)%t->count);
                        shot.record_trail_count=std::min(t->count,u8(shot.record_trail_count+1));
                    }
                }
                continue;
            }
            if(shot.weapon==BattleMvpWeapon::Bomb){
                if(!shot.landed){for(unsigned k=0;k<3;++k)shot.position[k]+=shot.velocity[k];shot.velocity[1]-=config_.gravity*.65f;
                    f32 y=0;if(!floor(terrain,shot.position[0],shot.position[2],shot.position[1],&y)){ready_=false;return false;}
                    if(shot.position[1]<=y){shot.position[1]=y;shot.landed=true;}}
                if(--shot.life==0){impact(shot.position,true);for(unsigned i=0;i<actor_count();++i)if(i!=shot.owner&&actors_[i].health){f32 squared=0;for(unsigned k=0;k<3;++k){const f32 d=actors_[i].position[k]+(k==1?config_.actor_height*.5f:0)-shot.position[k];squared+=d*d;}if(squared<=180.f*180.f)damage(i,35,shot.owner);}shot.active=false;}
            }else{
                const u8 target=actor_target(shot.owner);f32 t=0;bool hit=false;
                if(target==255){shot.active=false;continue;}
                if(actors_[target].health&&battle_swept_hit({shot.position[0],shot.position[2]},{shot.velocity[0],shot.velocity[2]},{actors_[target].position[0],actors_[target].position[2]},config_.actor_radius+8,&t)){
                    const f32 y=shot.position[1]+shot.velocity[1]*t;hit=y>=actors_[target].position[1]-8&&y<=actors_[target].position[1]+config_.actor_height+8;
                }
                if(hit){for(unsigned k=0;k<3;++k)shot.position[k]+=shot.velocity[k]*t;damage(target,shot.weapon==BattleMvpWeapon::EnergyBeam?12:shot.weapon==BattleMvpWeapon::Shuriken?4:6,shot.owner);hit_impact(target,shot.position);shot.active=false;}
                else{for(unsigned k=0;k<3;++k)shot.position[k]+=shot.velocity[k];if(--shot.life==0)shot.active=false;}
            }
            if(std::abs(shot.position[0])>config_.arena_half+300||std::abs(shot.position[2])>config_.arena_half+300||shot.position[1]<-10000)shot.active=false;
        }
        if(package){
            for(unsigned i=0;i<remote_requests_.size();++i)if(remote_requests_[i].active){
                const auto request=std::exchange(remote_requests_[i],{});
                record_package(BorgBattleEventKind::Detonation,i,request.move,request.marker.frame,0,0);
            }
            flush_package_children();step_package_impact_hazards();step_package_impact_trails();
        }
        if(!solo){
            for(unsigned i=0;i<actor_count();++i)if(auto& actor=actors_[i];!actor.health){
                actor.action=BattleMvpAction::Defeated;
                walk_modifiers_[i]={};
                for(auto& field:fields_)if(field.id&&field.spec.follow&&field.owner==i)field={};
                if(package&&actor.package_move!=BorgBattleActionState::none){
                    record_package(BorgBattleEventKind::Interrupted,i,actor.package_move,actor.package_frame,0,0);
                     package_actions_[i].cancel_action();
                     actor.package_move=BorgBattleActionState::none;actor.package_frame=0;
                     actor.package_aim_heading.reset();actor.package_aim_slope=0;actor.upper_body_yaw=0;
                }
            }
            bool team0=false,team1=false;
            for(unsigned i=0;i<actor_count();++i)if(actors_[i].health){if(actors_[i].team==0)team0=true;else if(actors_[i].team==1)team1=true;}
            if(!team0||!team1){
                result_=!team0?(!team1?BattleMvpResult::Draw:BattleMvpResult::SasukeWins):BattleMvpResult::GRedWins;
                for(unsigned i=0;i<actor_count();++i)if(const auto request=std::exchange(pull_requests_[i],{});request.object)
                    record_package(BorgBattleEventKind::PullCancelled,i,request.move,0,request.object,
                        u16(BorgBattlePullRefusal::RoundEnded),request.recipient);
                for(unsigned i=0;i<actor_count();++i)retire_package_melee(i);
                 for(unsigned i=0;i<actor_count();++i)retire_package_guard(i);
                for(auto& shot:projectiles_)if(shot.active&&shot.sustained)retire_package_shot(shot,PackageRetirement::OwnerEnded);
                for(unsigned i=0;i<actor_count();++i)for(auto& emitter:emitters_[i])retire_package_emitter(i,emitter,2);
                for(unsigned i=0;i<actor_count();++i)if(rewinds_[i].id)finish_package_rewind(i,BorgBattlePullRefusal::RoundEnded);
            }
        }
        for(unsigned i=0;i<actor_count();++i){
            const auto* offer=offered(i);
            if(!offer){last_receipts_[i]={};continue;}
            last_offers_[i]=*offer;
            auto& receipt=last_receipts_[i];
            receipt.scope_=motion_scope_;receipt.tick_=tick_;receipt.serial_=offer->serial;
            receipt.position_=actors_[i].position;receipt.facing_=actors_[i].facing;receipt.contact_=source_contacts[i];
            motion_producers_[i]=offer->producer;receipt.producer_=offer->producer;
            receipt.grounded_=actors_[i].grounded;receipt.vertical_speed_=actors_[i].vertical_speed;
        }
        remember_pads(pads);return true;
    }
    void remember_pads(std::span<const GamePadState> pads){
        for(unsigned i=0;i<kBattleMvpMaxActors;++i)previous_[i]=i<pads.size()?pads[i]:GamePadState{};
    }
    bool ready()const{return ready_;}bool paused()const{return paused_;}u32 tick()const{return tick_;}
    unsigned target_count()const{return config_.solo()?1u:std::min(config_.declared_actors,kBattleMvpMaxActors);}
    unsigned actor_count()const{return !ready_?0u:target_count();}
    bool set_paused(bool paused){
        if(!ready_)return false;
        if(paused!=paused_)for(auto& action:package_actions_)action.discard_input();
        paused_=paused;return true;
    }
    const auto& actors()const{return actors_;}const auto& projectiles()const{return projectiles_;}const auto& impacts()const{return impacts_;}
    u8 target_of(unsigned actor)const{return actor<actor_count()?actor_target(actor):u8(255);}
    u8 aim_target_of(unsigned actor)const{
        return actor<actor_count()&&actors_[actor].health?package_aim_target(actor):u8(255);
    }
    const auto& team_force()const{return team_force_;}const auto& team_tally()const{return team_tally_;}
    const auto& force_counter10a()const{return force_counter10a_;}
    u8 last_attacker()const{return last_attacker_;}u8 last_receiver()const{return last_receiver_;}
    const auto& impact_records()const{return impact_records_;}
    const auto& walk_pulses()const{return walk_pulses_;}
    const auto& smoke_clouds()const{return smoke_clouds_;}
    const auto& fields()const{return fields_;}
    const BattleMvpInk* ink_overlay(unsigned viewer)const{
        return viewer<actor_count()&&actors_[viewer].health&&ink_[viewer].object?&ink_[viewer]:nullptr;
    }
    const auto& emitters()const{return emitters_;}
    const auto& anchor_paths()const{return anchor_paths_;}
    std::span<const BattleMvpFenceLink> fence_links()const{return {fence_links_.data(),fence_link_count_};}
    const BattleMvpConfig& config()const{return config_;}BattleMvpResult result()const{return result_;}
    GamePadState duel_ai_pad()const{return actor_count()==2&&result_==BattleMvpResult::Fighting?ai_input():GamePadState{};}
    GamePadState roster_ai_pad(unsigned slot)const{
        return slot>=config_.roster_local_players&&slot<actor_count()&&!paused_&&result_==BattleMvpResult::Fighting?
            ai_input(slot):GamePadState{};
    }
    u32 shots_spawned()const{return shots_;}u32 hits()const{return hits_;}
    std::span<const BorgBattleEvent> package_events()const{return {package_events_.data(),package_event_count_};}
    const BorgBattleActionState& package_action(unsigned slot)const{return package_actions_.at(slot);}
    unsigned maximum_health(unsigned slot)const{return config_.packages.at(slot)?config_.packages[slot]->health:config_.health;}
    bool observe_motion(NativeBorgMotionObservation* out)const{return observe_motion(0,out);}
    bool observe_motion(unsigned actor,NativeBorgMotionObservation* out)const{
        if(!out||actor>=actor_count()||!ready_||paused_||result_!=BattleMvpResult::Fighting||!motion_scope_||motion_scope_->owner_!=this||
           actors_[actor].model!=NormalGRedMovementDefinition::model||!actors_[actor].health)return false;
        *out={motion_scope_,tick_,actors_[actor].model,actors_[actor].position,actors_[actor].facing,actors_[actor].grounded,actors_[actor].vertical_speed};return true;
    }
    std::shared_ptr<const NativeBorgMotionProducer> new_motion_producer(bool vertical_jump=false)const{
        return new_motion_producer(vertical_jump?NativeBorgMotionPolicy::VerticalJump:NativeBorgMotionPolicy::GroundOnly);
    }
    std::shared_ptr<const NativeBorgMotionProducer> new_motion_producer(NativeBorgMotionPolicy policy)const{
        NativeBorgMotionObservation observed;
        const auto& actor=actors_[0];
           if(unsigned(policy)>unsigned(NativeBorgMotionPolicy::GroundWalkExpirySameFacePush)||!observe_motion(&observed)||actor.source_ground_motion||!actor.grounded||actor.vertical_speed||
           actor.attack_ticks||actor.hit_flash||actor.dash_ticks||actor.upper_body_yaw)return {};
        for(const auto& shot:projectiles_)if(shot.active)return {};
        return std::shared_ptr<const NativeBorgMotionProducer>(new NativeBorgMotionProducer(motion_scope_,policy));
    }
    std::weak_ptr<const NativeBorgMotionScope> presentation_scope()const{
        return ready_&&motion_scope_&&motion_scope_->owner_==this?motion_scope_:std::weak_ptr<const NativeBorgMotionScope>{};
    }
    bool owns_motion_scope(const std::weak_ptr<const NativeBorgMotionScope>& scope)const{
        return ready_&&motion_scope_&&motion_scope_->owner_==this&&scope.lock()==motion_scope_;
    }
    bool motion_observation_current(const NativeBorgMotionObservation& observed,unsigned actor=0)const{
        if(actor>=actor_count())return false;
        return ready_&&!paused_&&result_==BattleMvpResult::Fighting&&motion_scope_&&motion_scope_->owner_==this&&
            observed.scope.lock()==motion_scope_&&observed.tick==tick_&&observed.model==actors_[actor].model&&
            observed.position==actors_[actor].position&&observed.facing==actors_[actor].facing&&observed.grounded==actors_[actor].grounded&&observed.vertical_speed==actors_[actor].vertical_speed;
    }
    bool motion_receipt(const NativeBorgMotionOffer& offer,NativeBorgMotionReceipt* out,unsigned actor=0)const{
        if(actor>=actor_count()||!out||!motion_receipt_current(last_receipts_[actor],actor)||offer.observed.scope.lock()!=motion_scope_||offer.producer.lock()!=last_offers_[actor].producer.lock()||offer.serial!=last_offers_[actor].serial||
           offer.observed.tick!=last_offers_[actor].observed.tick||offer.observed.position!=last_offers_[actor].observed.position||offer.observed.facing!=last_offers_[actor].observed.facing||
           offer.observed.model!=last_offers_[actor].observed.model||offer.observed.grounded!=last_offers_[actor].observed.grounded||
           offer.proposed_position!=last_offers_[actor].proposed_position||offer.facing!=last_offers_[actor].facing||offer.action!=last_offers_[actor].action||
           offer.kind!=last_offers_[actor].kind||offer.vertical_speed!=last_offers_[actor].vertical_speed||offer.observed.vertical_speed!=last_offers_[actor].observed.vertical_speed||
            offer.entry_vertical_delta!=last_offers_[actor].entry_vertical_delta||offer.static_push!=last_offers_[actor].static_push||offer.relaunch_floor!=last_offers_[actor].relaunch_floor||
            offer.air_dash_floor!=last_offers_[actor].air_dash_floor||offer.walk_ledge!=last_offers_[actor].walk_ledge||
            offer.ordered_ground!=last_offers_[actor].ordered_ground||offer.jump_motion!=last_offers_[actor].jump_motion||
            offer.landing_target!=last_offers_[actor].landing_target||offer.air_motion!=last_offers_[actor].air_motion||
            offer.relaunch_motion!=last_offers_[actor].relaunch_motion||offer.surface_landing!=last_offers_[actor].surface_landing||
             offer.controller_timer698!=last_offers_[actor].controller_timer698||
             offer.recovery_first_seam_start!=last_offers_[actor].recovery_first_seam_start||
             offer.recovery_completion.owner_before(last_offers_[actor].recovery_completion)||
             last_offers_[actor].recovery_completion.owner_before(offer.recovery_completion))return false;
        *out=last_receipts_[actor];return true;
    }
    bool motion_receipt_current(const NativeBorgMotionReceipt& receipt,unsigned actor=0)const{
        if(actor>=actor_count())return false;
        return ready_&&motion_scope_&&motion_scope_->owner_==this&&receipt.scope_.lock()==motion_scope_&&!receipt.producer_.expired()&&receipt.producer_.lock()==motion_producers_[actor].lock()&&receipt.serial_&&receipt.tick_==tick_&&
            actors_[actor].source_ground_motion&&receipt.position_==actors_[actor].position&&receipt.facing_==actors_[actor].facing&&
            receipt.grounded_==actors_[actor].grounded&&receipt.vertical_speed_==actors_[actor].vertical_speed;
    }
private:
    bool resolve_native_body_motion(unsigned owner,unsigned legacy_peer,BattlePoint delta,bool grounded,
                                    const IBattleMvpTerrain* terrain,NativeBorgContactResult* out)const{
        const auto& actor=actors_[owner];const auto& other=actors_[legacy_peer];
        const NativeBorgContactRequest request{actor.position,other.position,delta,config_.arena_half,config_.actor_radius,
            config_.actor_height,config_.step_height,grounded,other.health!=0};
        if(actor_count()<=2||!config_.packages[owner])return native_borg_resolve_contact(request,terrain,out);

        // Physical peers are not attack targets. Allies and immune actors
        // are solid too; use positions already accepted in this actor visit.
        static_assert(kBattleMvpMaxActors-1<=kNativeBorgContactPeerLimit);
        std::array<BattleMvpVec,kBattleMvpMaxActors-1> bodies;
        unsigned count=0;
        for(unsigned i=0;i<actor_count();++i)if(i!=owner&&actors_[i].health)bodies[count++]=actors_[i].position;
        return native_borg_resolve_contact(request,{bodies.data(),count},terrain,out);
    }
    bool step_cruise(unsigned owner,const BorgCruiseBoost& spec,f32 base,f32 boost,f32 multiplier,
                     const GamePadState& pad,const NativeBorgMovementFrame* frame,
                     const IBattleMvpTerrain* terrain,bool interrupted){
        auto& actor=actors_[owner];
        const u8 other_slot=actor_target(owner);
        const auto& other=actors_[other_slot==255?owner:other_slot];
        auto& meter=actor.package_boost_meter;
        const AirMeterRules rules{AirMeterRecoveryGate::Behavior80Clear,s16(spec.capacity),s16(spec.delay),
            s16(spec.refill),{0,0,0,s16(spec.cost)}};
        character_air_resource_recover(meter,rules,actor.package_boosting?0x80u:0,0,&meter);
        if(actor.package_boosting){
            if(interrupted||(actor.package_boost_age>=spec.minimum&&(!pad.b||!meter.value736))){
                actor.package_boosting=false;actor.package_boost_age=0;
            }
        }else if(!interrupted&&pad.b&&!previous_[owner].b&&meter.value736>0)actor.package_boosting=true;
        const bool thrust=actor.package_boosting&&meter.value736>0;
        if(actor.package_boosting){
            if(actor.package_boost_age<spec.minimum)++actor.package_boost_age;
            character_air_resource_consume(meter,s16(spec.cost),s16(spec.delay),1.f,&meter);
        }
        f32 x=axis(pad.stick_x)+f32(pad.dpad_right)-f32(pad.dpad_left),
            z=axis(pad.stick_y)+f32(pad.dpad_down)-f32(pad.dpad_up);
        unit(x,z,false);
        const auto move=native_borg_move_direction(x,z,frame?*frame:NativeBorgMovementFrame{});
        if(move!=BattlePoint{})actor.facing=native_borg_turn(actor.facing,move,config_.turn_per_tick);
        const f32 speed=(thrust?boost:base)*multiplier;
        const BattlePoint delta{actor.facing.x*speed,actor.facing.z*speed};
        NativeBorgContactResult contact;
        if(!resolve_native_body_motion(owner,other_slot==255?owner:other_slot,delta,false,terrain,&contact))return false;
        f32 support=0;
        if(!floor(terrain,contact.position[0],contact.position[2],contact.position[1],&support))return false;
        actor.position=contact.position;actor.position[1]=support+spec.height;
        actor.grounded=false;actor.vertical_speed=0;actor.velocity=contact.actual_delta;
        actor.movement_speed=std::hypot(actor.velocity.x,actor.velocity.z);actor.travel_distance+=actor.movement_speed;
        actor.move_forward=actor.movement_speed?(actor.velocity.x*actor.facing.x+actor.velocity.z*actor.facing.z)/actor.movement_speed:0;
        actor.move_right=actor.movement_speed?(-actor.velocity.x*actor.facing.z+actor.velocity.z*actor.facing.x)/actor.movement_speed:0;
        actor.aim_facing=config_.solo()?actor.facing:
            native_borg_direction({actor.position[0],actor.position[2]},{other.position[0],other.position[2]},actor.facing);
        actor.action=actor.package_boosting?BattleMvpAction::AirDash:BattleMvpAction::Jump;
        return true;
    }
    template<class Spec> struct ImpactPattern {
        BattleMvpVec centre;BattlePoint heading;u32 parent=0,born=0;u8 owner=0,move=0;bool world_space=false;Spec spec;
    };
    bool sample_field_socket(unsigned owner,const BattleMvpActor& actor,const BorgBattleSocket& socket,
                             const IBorgBattleOrigins* origins,BattleMvpVec* centre)const{
        return origins&&origins->socket(owner,actor,tick_,actor.package_move,actor.package_frame,socket,centre)&&
            std::all_of(centre->begin(),centre->end(),[](f32 v){return std::isfinite(v)&&std::abs(v)<=1e5f;});
    }
    static bool same_package_field_slot(const BattleMvpField& field,unsigned owner,const BorgField& spec){
        return field.id&&field.owner==owner&&field.spec.slot&&spec.slot&&field.spec.slot->id==spec.slot->id;
    }
    void replace_package_field_slot(const BattleMvpField& next){
        for(auto& field:fields_)if(&field!=&next&&same_package_field_slot(field,next.owner,next.spec)){
            record_package(BorgBattleEventKind::FieldReplaced,field.owner,field.move,0,field.id,0);
            field={};
        }
    }
    void place_package_field(unsigned owner,const IBorgBattleOrigins* origins){
        using K=BorgFieldEffectKind;using S=BorgFieldScope;
        const auto& actor=actors_[owner];const auto& action=package_actions_[owner];
        if(!actor.health||action.move==BorgBattleActionState::none)return;
        const auto& move=config_.packages[owner]->moves[action.move];
        BorgField spec;
        auto placed=BorgBattleEventKind::FieldPlaced,refused=BorgBattleEventKind::FieldRefused;
        u16 value=0;
        if(move.field&&action.frame==move.field->frame)spec=*move.field;
        else if(move.slow_field&&action.frame==move.slow_field->frame){
            const auto& s=*move.slow_field;
            spec={s.frame,s.ticks,s.forward,s.height,BorgFieldShape::Sphere,{s.radius,s.radius,s.radius},{72,190,208,55},{},{},false,{},{},{},{},{}};
            if(s.actor_percent!=100)spec.effects.push_back({K::ActorSpeed,S::Enemies,s.actor_percent});
            if(s.shot_percent!=100)spec.effects.push_back({K::ShotSpeed,S::Enemies,s.shot_percent});
            if(s.break_guidance)spec.effects.push_back({K::BreakGuidance,S::Enemies,0});
            placed=BorgBattleEventKind::SlowFieldPlaced;refused=BorgBattleEventKind::SlowFieldRefused;value=s.actor_percent;
        }else if(move.power_field&&action.frame==move.power_field->frame){
            const auto& s=*move.power_field;
            spec={s.frame,s.ticks,s.forward,s.height,BorgFieldShape::Sphere,{s.radius,s.radius,s.radius},{223,75,55,48},{},{},false,{},{},{},{},{}};
            if(s.damage_percent!=100)spec.effects.push_back({K::ShotDamage,S::All,s.damage_percent});
            if(s.visual_percent!=100)spec.effects.push_back({K::ShotSize,S::All,s.visual_percent});
            placed=BorgBattleEventKind::PowerFieldPlaced;refused=BorgBattleEventKind::PowerFieldRefused;value=s.damage_percent;
        }else return;
        if(const auto& debit=spec.debit){
            const auto& profile=*config_.packages[owner];
            const auto found=std::find_if(profile.stocks.begin(),profile.stocks.end(),
                [&](const auto& stock){return stock.id==debit->stock;});
            if(found==profile.stocks.end()){
                record_package(refused,owner,action.move,action.frame,0,0);return;
            }
            auto& stock=package_actions_[owner].stocks[unsigned(found-profile.stocks.begin())];
            WeaponStockPlan plan;
            if(!weapon_stock_plan(stock,BorgBattleActionState::stock_mode(*found),WeaponStockOperation::Debit,
                                  debit->cost,&plan)||!plan.accepted){
                record_package(BorgBattleEventKind::StockRefused,owner,action.move,action.frame,0,u16(stock.current));
                record_package(refused,owner,action.move,action.frame,0,0);return;
            }
            stock=plan.state;
            record_package(BorgBattleEventKind::StockSpent,owner,action.move,action.frame,0,u16(stock.current));
        }
        auto free=std::find_if(fields_.begin(),fields_.end(),[](const auto& field){return !field.id;});
        const auto same=[&](const auto& field){return same_package_field_slot(field,owner,spec);};
        const bool wait=spec.slot&&spec.slot->on_ready;
        const bool pending=wait&&std::any_of(fields_.begin(),fields_.end(),[&](const auto& field){
            return same(field)&&field.spec.slot->on_ready&&!field.active();
        });
        if(free==fields_.end()&&spec.slot&&!wait)free=std::find_if(fields_.begin(),fields_.end(),same);
        BattleMvpVec centre{actor.position[0]+actor.facing.x*spec.forward,actor.position[1]+spec.height,
                             actor.position[2]+actor.facing.z*spec.forward};
        if(pending||free==fields_.end()||next_id_==std::numeric_limits<u32>::max()||
           (spec.socket&&!sample_field_socket(owner,actor,*spec.socket,origins,&centre))||
           std::abs(centre[0])>config_.arena_half||std::abs(centre[2])>config_.arena_half){
            record_package(refused,owner,action.move,action.frame,0,0);return;
        }
        const BattleMvpVec offset{centre[0]-actor.position[0],centre[1]-actor.position[1],centre[2]-actor.position[2]};
        const auto speed=spec.drift?spec.drift->speed:0.f;
        if(free->id)record_package(BorgBattleEventKind::FieldReplaced,free->owner,free->move,0,free->id,0);
        *free={centre,offset,++next_id_,tick_,u8(owner),action.move,std::move(spec),actor.facing,speed};
        if(free->spec.slot&&!wait)replace_package_field_slot(*free);
        record_package(placed,owner,action.move,action.frame,free->id,value);
    }
    static bool field_scope_matches(BorgFieldScope scope,unsigned owner,unsigned subject){
        return scope==BorgFieldScope::All||(scope==BorgFieldScope::Enemies?owner!=subject:owner==subject);
    }
    static u16 field_effect(const BattleMvpField& field,BorgFieldEffectKind kind,unsigned subject){
        u16 value=100;
        if(!field.active())return value;
        for(const auto& e:field.spec.effects)if(e.kind==kind&&field_scope_matches(e.scope,field.owner,subject))
            value=e.value<100||value<100?std::min(value,e.value):std::max(value,e.value);
        return value;
    }
    static bool package_field_interval(const BattleMvpVec& start,const BattleMvpVec& delta,const BattleMvpField& field,
                                       f64* first,f64* last){
        if(field.spec.shape==BorgFieldShape::Box){
            *first=0;*last=1;
            for(unsigned k=0;k<3;++k){
                const f64 d=f64(start[k])-field.centre[k],r=field.spec.extent[k],v=delta[k];
                if(!v){if(std::abs(d)>r)return false;continue;}
                const f64 a=(-r-d)/v,b=(r-d)/v;
                *first=std::max(*first,std::min(a,b));*last=std::min(*last,std::max(a,b));
                if(*first>*last)return false;
            }
            return true;
        }
        const auto radius=field.spec.extent[0];const auto& centre=field.centre;
        f64 vv=0,dv=0,cc=-f64(radius)*radius;
        for(unsigned k=0;k<3;++k){
            const auto d=f64(start[k])-centre[k];
            dv+=d*delta[k];vv+=f64(delta[k])*delta[k];cc+=d*d;
        }
        if(!vv){*first=0;*last=1;return cc<=0;}
        const auto determinant=dv*dv-vv*cc;
        if(determinant<0)return false;
        const auto root=std::sqrt(determinant);
        *first=std::max(0.,(-dv-root)/vv);*last=std::min(1.,(-dv+root)/vv);
        return *first<=*last;
    }
    void pull_package_shot(BattleMvpProjectile& shot){
        const BattleMvpField* selected=nullptr;f32 nearest=0;BattleMvpVec inward{};
        for(const auto& field:fields_)if(field.active()&&field.spec.shot_pull&&
            field_scope_matches(field.spec.shot_pull->scope,field.owner,shot.owner)){
            f64 enter=0,exit=0;
            if(!package_field_interval(shot.position,{},field,&enter,&exit))continue;
            const BattleMvpVec delta{field.centre[0]-shot.position[0],field.centre[1]-shot.position[1],field.centre[2]-shot.position[2]};
            const f32 distance=std::hypot(delta[0],delta[1],delta[2]);
            if(!selected||distance<nearest||(distance==nearest&&field.id<selected->id)){
                selected=&field;nearest=distance;inward=delta;
            }
        }
        if(!selected||nearest<.001f){shot.pulling_field=0;return;}
        if(shot.pulling_field!=selected->id)
            record_package(BorgBattleEventKind::FieldShotPulled,shot.owner,shot.package_move,0,shot.id,0);
        shot.pulling_field=selected->id;
        shot.package_ramp_deflected=true;
        const f32 step=std::min(selected->spec.shot_pull->acceleration,nearest)/nearest;
        const auto speed=[](const auto& v){return std::hypot(v[0],v[1],v[2]);};
        const f32 cap=std::max(200.f,speed(shot.velocity));
        for(unsigned k=0;k<3;++k)shot.velocity[k]+=step*inward[k];
        const f32 new_speed=speed(shot.velocity);
        if(new_speed>cap)for(auto& v:shot.velocity)v*=cap/new_speed;
    }
    void step_package_field_cores(){
        for(const auto& field:fields_)if(field.active()&&field.spec.core_damage){
            const auto& core=*field.spec.core_damage;
            const auto age=tick_-field.born;
            if(!age||age%core.interval)continue;
            for(u8 i=0;i<actor_count();++i){
                const auto& actor=actors_[i];
                if(!actor.health||!field_scope_matches(core.scope,field.owner,i))continue;
                const f32 x=field.centre[0]-actor.position[0],y=field.centre[1]-actor.position[1]-config_.actor_height*.5f,
                          z=field.centre[2]-actor.position[2];
                if(x*x+y*y+z*z>f32(core.radius)*core.radius||
                   try_package_guard(i,field.owner,BorgBattleAttackKind::Blast,{x,z},field.id))continue;
                accept_package_hit(field.owner,i,field.move,0,field.id,core.damage,field.centre);
            }
        }
    }
    void step_package_walk_pulses(std::span<const f32> distance){
        for(auto& wave:walk_pulses_)if(wave.id){
            const auto age=tick_-wave.born;
            if(actor_target(wave.owner)==255){wave={};continue;}
            if(!wave.resolved){
                const f32 inner=wave.radius(tick_-1),outer=wave.radius(tick_);
                const auto contact=package_body_contact(wave.owner,actor_count()>2,[&](u8 candidate,f32* entry){
                    const auto& actor=actors_[candidate];
                    if(std::abs(actor.position[1]-wave.origin[1])>wave.spec.height)return false;
                    const f32 r=std::hypot(wave.origin[0]-actor.position[0],wave.origin[2]-actor.position[2]);
                    const f32 near=r-config_.actor_radius;
                    if(r+config_.actor_radius<inner||near>outer)return false;
                    // First radial touch within this slice of expansion,
                    // using the same current body samples as the duel.
                    *entry=std::clamp((near-inner)/(outer-inner),0.f,1.f);
                    return true;
                });
                if(contact){
                    const auto target=contact.recipient;
                    const auto& actor=actors_[target];
                    const BattlePoint inbound{wave.origin[0]-actor.position[0],wave.origin[2]-actor.position[2]};
                    wave.resolved=true;
                    if(!try_package_guard(target,wave.owner,BorgBattleAttackKind::Blast,inbound,wave.id))
                        accept_package_hit(wave.owner,target,BorgBattleActionState::none,0,wave.id,wave.spec.damage,wave.origin);
                }
            }
            if(age>=wave.spec.ticks)wave={};
        }
        for(u8 owner=0;owner<actor_count();++owner){
            const auto& spec=config_.packages[owner]->walk_pulse;
            auto& carry=walk_pulse_distance_[owner];
            if(!spec||!actors_[owner].health||distance[owner]<=.05f){carry=0;continue;}
            carry+=distance[owner];
            if(carry<spec->stride)continue;
            carry=std::fmod(carry,f32(spec->stride)); // one request per accepted walk visit, no backlog
            const auto free=std::find_if(walk_pulses_.begin(),walk_pulses_.end(),[](const auto& wave){return !wave.id;});
            const bool accepted=free!=walk_pulses_.end()&&next_id_!=std::numeric_limits<u32>::max();
            if(accepted)*free={actors_[owner].position,++next_id_,tick_,owner,false,*spec};
            record_package(accepted?BorgBattleEventKind::WalkPulseCreated:BorgBattleEventKind::WalkPulseRefused,
                           owner,BorgBattleActionState::none,0,accepted?free->id:0,spec->end);
        }
    }
    f32 package_actor_slow(unsigned actor)const{
        auto centre=actors_[actor].position;centre[1]+=config_.actor_height*.5f;
        u16 percent=100;
        for(const auto& field:fields_)if(field.id){
            f64 first=0,last=0;
            const auto candidate=field_effect(field,BorgFieldEffectKind::ActorSpeed,actor);
            if(candidate!=100&&package_field_interval(centre,{},field,&first,&last))
                percent=candidate<100||percent<100?std::min(percent,candidate):std::max(percent,candidate);
        }
        return f32(percent)*.01f;
    }
    f32 package_slow_fraction(const BattleMvpProjectile& shot)const{
        struct Span {f64 first,last;u16 percent;};
        std::array<Span,kBattleMvpFields> spans{};
        std::array<f64,2+2*kBattleMvpFields> boundaries{0,1};
        unsigned count=0,points=2;
        for(const auto& field:fields_)if(field.id){
            f64 first=0,last=0;
            const auto speed=field_effect(field,BorgFieldEffectKind::ShotSpeed,shot.owner);
            if(speed==100||!package_field_interval(shot.position,shot.velocity,field,&first,&last)||first==last)continue;
            spans[count++]={first,last,speed};
            boundaries[points++]=first;boundaries[points++]=last;
        }
        if(!count)return 1;
        std::sort(boundaries.begin(),boundaries.begin()+points);
        f64 remaining=1;
        for(unsigned i=1;i<points;++i){
            const f64 from=boundaries[i-1],to=boundaries[i];
            if(from==to)continue;
            u16 percent=100;
            for(unsigned j=0;j<count;++j)if(spans[j].first<=from&&spans[j].last>=to)
                percent=std::min(percent,spans[j].percent);
            const f64 scale=percent*.01,time=(to-from)/scale;
            if(time>=remaining)return f32(std::min(to,from+remaining*scale));
            remaining-=time;
        }
        return 1;
    }
    void break_package_guidance_in_fields(BattleMvpProjectile& shot,const BattleMvpVec& delta,f32 limit,bool terminal){
        if(!shot.package_guide_ticks)return;
        for(const auto& field:fields_)if(field.id&&!field_effect(field,BorgFieldEffectKind::BreakGuidance,shot.owner)){
            f64 enter=0,exit=0;
            if(package_field_interval(shot.position,delta,field,&enter,&exit)&&
               enter<=limit&&(!terminal||enter<limit)){
                shot.package_guide_ticks=0;
                record_package(BorgBattleEventKind::FieldGuidanceBroken,shot.owner,shot.package_move,0,shot.id,0);
                return;
            }
        }
    }
    const BattleMvpField* first_package_power_field(const BattleMvpProjectile& shot,const BattleMvpVec& delta,
                                                       f32 limit,bool terminal,f64* entry)const{
        const BattleMvpField* first=nullptr;
        f64 nearest=limit;
        for(const auto& field:fields_)if(field.id&&(field_effect(field,BorgFieldEffectKind::ShotDamage,shot.owner)!=100||
                                                    field_effect(field,BorgFieldEffectKind::ShotSize,shot.owner)!=100)){
            f64 t=0,exit=0;
            if(!package_field_interval(shot.position,delta,field,&t,&exit)||
               t>limit||(terminal&&t>=limit)||t>nearest||(first&&t==nearest&&field.id>=first->id))continue;
            first=&field;nearest=t;
        }
        if(first&&entry)*entry=nearest;
        return first;
    }
    static u16 powered_damage(const BattleMvpProjectile& shot,const BattleMvpField& field){
        return u16(std::min<u32>(10000,u32(shot.package_damage)*field_effect(field,BorgFieldEffectKind::ShotDamage,shot.owner)/100));
    }
    void power_package_shot(BattleMvpProjectile& shot,const BattleMvpVec& delta,f32 limit,bool terminal){
        // Travelling direct shots keep their first accepted boost.
        if(shot.power_field||!shot.package_damage||shot.package_blast||shot.package_rewind_path.count)return;
        const auto* first=first_package_power_field(shot,delta,limit,terminal,nullptr);
        if(!first)return;
        shot.power_field=first->id;shot.power_visual_percent=field_effect(*first,BorgFieldEffectKind::ShotSize,shot.owner);
        shot.package_damage=powered_damage(shot,*first);
        record_package(BorgBattleEventKind::ShotPowered,shot.owner,shot.package_move,0,shot.id,shot.package_damage);
    }
    void record_package(BorgBattleEventKind kind,unsigned actor,u8 move,u16 frame,u32 object,u16 value,u8 recipient=255){
        if(package_event_count_<package_events_.size())
            package_events_[package_event_count_++]={kind,tick_,object,u8(actor),move,recipient,frame,value};
    }
    // Recovered 8002FD7C at the MVP hit boundary. The duel has distinct owners
    // and teams, so only declared immunity keys and the target mask can refuse.
    bool package_hit_relation(unsigned owner,u8 recipient)const{
        if(owner>=actors_.size()||recipient>=actors_.size())return false;
        BattlePairFields a{},b{};
        a.declared=a.exists=a.active=true;
        a.actor_key=u32(owner)+1u;a.owner_key=u32(owner)+1u;a.team=actors_[owner].team;
        a.kind=actors_[owner].damage_kind;
        a.attack_key=actors_[owner].damage_attack_key;
        b.declared=b.exists=b.active=true;
        b.actor_key=u32(recipient)+1u;b.owner_key=u32(recipient)+1u;
        b.team=actors_[recipient].team;b.kind=actors_[recipient].damage_kind;
        b.target_mask=actors_[recipient].damage_target_mask;
        b.immunity_count=u8(std::min<std::size_t>(48,actors_[recipient].damage_immunity.size()));
        for(unsigned i=0;i<b.immunity_count;++i)b.immunity[i]=actors_[recipient].damage_immunity[i];
        bool ok=false;std::string error;
        return battle_pair_relation_ok(a,b,2,&ok,&error)&&ok;
    }
    struct PackageBodyContact {
        u8 recipient=255;
        f32 entry=0;
        explicit operator bool()const{return recipient!=255;}
    };
    template<class Probe> PackageBodyContact package_body_contact(unsigned owner,bool roster,Probe probe)const{
        if(!roster){
            // Duels and effects with indirect ownership still use their
            // existing aim-selected body and late relation filter.
            const u8 target=actor_target(owner);f32 entry=0;
            if(target!=255&&probe(target,&entry))return {target,entry};
            return {};
        }
        PackageBodyContact best;
        for(u8 i=0;i<actor_count();++i){
            if(i==owner||!actors_[i].health||actors_[i].team==actors_[owner].team||!package_hit_relation(owner,i))
                continue;
            f32 entry=0;
            // Native sweeps use first touch, not distance to the aiming
            // target. Keep the later physical slot on an exact tie.
            if(probe(i,&entry)&&(!best||entry<=best.entry))best={i,entry};
        }
        return best;
    }
    // 8002F8DC force pool. The opt-in policy reproduces the mode-dependent
    // decrement and the last-attacker/last-receiver writes.
    void ledger_force(unsigned owner,u8 recipient,unsigned amount){
        auto& target=actors_[recipient];
        const u8 team=target.team;
        bool decrement=true,b=false;
        if(config_.force_policy[team].enabled){
            const auto& policy=config_.force_policy[team];
            const auto& flags=config_.team_ledger[team];
            const bool flag3e6=config_.actor_flag3e6[recipient];
            if(policy.mode==0){
                decrement=flags.flag108&&(team!=0||!flag3e6);
                if(flags.flag109&&config_.actor_flag1ea[recipient]&&force_counter10a_[team]>0){
                    --force_counter10a_[team];
                    if(!force_global1f_){
                        if(actors_[owner].damage_kind==0)last_attacker_=u8(owner);
                        b=true;last_receiver_=u8(recipient);
                    }
                }
            }else if(policy.mode==1){
                decrement=team!=0||!flag3e6;
            }else{
                decrement=flags.flag_f6?!flag3e6:flag3e6;
            }
        }
        if(decrement)team_force_[team]=team_force_[team]>=amount?team_force_[team]-amount:0u;
        if(team_force_[team]<1){
            team_force_[team]=0;
            if(!force_global1f_&&!b){
                if(actors_[owner].damage_kind==0)last_attacker_=u8(owner);
                last_receiver_=u8(recipient);
            }
        }
    }
    enum class DamageRoleScope {Unbound,DirectMove};
    struct TierHit {const BorgGameplayProfile* receiver=nullptr;const BorgBattleMove* move=nullptr;};
    TierHit package_tier_hit(unsigned owner,u8 recipient,u8 move,DamageRoleScope scope)const{
        if(scope!=DamageRoleScope::DirectMove)return {};
        const auto& attacker=config_.packages[owner];
        const auto& receiver=config_.packages[recipient];
        if(!attacker||!receiver||move>=attacker->moves.size()||!attacker->moves[move].contact_tier||
           !receiver->damage_tier_receive||!attacker->damage_credit.find(actors_[owner].team)||
           !receiver->damage_receive.find(actors_[recipient].team))return {};
        return {receiver.get(),&attacker->moves[move]};
    }
    u16 package_damage(unsigned owner,u8 recipient,u16 base,DamageRoleScope scope,u16 raw_base,u8 move,f32 tier){
        if(owner>=config_.packages.size()||!config_.packages[owner])return base;
        const auto& profile=*config_.packages[owner];
        const auto& recipient_profile=config_.packages[recipient];
        const auto* receive=scope==DamageRoleScope::DirectMove&&recipient_profile?
            recipient_profile->damage_receive.find(actors_[recipient].team):nullptr;
        const bool split=profile.damage_credit.find(actors_[owner].team)&&receive;
        if(!profile.damage_factors_declared&&!profile.damage_chain_count&&
           profile.damage_rows.empty()&&profile.damage_side_rows.empty()&&!split)return base;
        auto& target=actors_[recipient];
        if(target.package_hit_chain_owner!=u8(owner)||tick_-target.package_last_hit_tick>30){
            target.package_hit_chain=0;
            target.package_hit_chain_owner=u8(owner);
        }
        // `damage-side` rows are paired with the attacker's team, not its
        // actor index, so rosters with more than two actors keep the side.
        const f32 matchup=split&&actors_[owner].health&&target.health?
            borg_damage_matchup(profile,*recipient_profile):1.f;
        BorgDamageLadderRates response,outgoing;
        std::array<f32,2> indexed_credit{},indexed_receive{};
        const std::array<f32,2>* selected_credit=nullptr;
        if(split){
            const auto& attacker=actors_[owner];
            u8 credit_bank=attacker.team,receive_bank=target.team;
            if(const auto& indices=config_.damage_indices;indices){
                credit_bank=indices->bank(owner,attacker.team);
                receive_bank=indices->bank(recipient,target.team);
                indexed_credit=profile.damage_credit_indexed.select(
                    credit_bank,indices->settings[attacker.team],indices->ranks[owner]);
                indexed_receive=recipient_profile->damage_receive_indexed.select(
                    receive_bank,indices->settings[target.team],indices->ranks[recipient]);
                selected_credit=&indexed_credit;receive=&indexed_receive;
            }
            response.hp=borg_damage_ladder_factor(recipient_profile->damage_receive_hp,receive_bank,
                borg_damage_hp_cell(target.health,recipient_profile->health,raw_base));
            response.force=borg_damage_ladder_factor(recipient_profile->damage_receive_force,receive_bank,
                borg_damage_force_cell(team_force_[target.team],config_.team_force[target.team]));
            // Require the recorded launch/contact move. A detached hit
            // must not inherit whichever attack is currently active.
            if(attacker.health&&move<profile.moves.size()&&profile.moves[move].damage_credit_ladders){
                outgoing.hp=borg_damage_ladder_factor(profile.damage_credit_hp,credit_bank,
                    borg_damage_hp_cell(attacker.health,profile.health));
                outgoing.force=borg_damage_ladder_factor(profile.damage_credit_force,credit_bank,
                    borg_damage_force_cell(team_force_[attacker.team],config_.team_force[attacker.team]));
            }
        }
        const bool minimum_hp=scope==DamageRoleScope::DirectMove&&raw_base&&move<profile.moves.size()&&
            profile.moves[move].contact_minimum_hp;
        const u16 value=borg_damage_amount(profile,base,target.package_hit_chain,
                                            actors_[recipient].damage_model,
                                            actors_[owner].team,
                                             config_.damage_scalar,receive,matchup,response,outgoing,tier,selected_credit,minimum_hp);
        if(profile.damage_chain_count&&target.package_hit_chain<255)++target.package_hit_chain;
        return value;
    }
    void accept_package_hit(unsigned owner,u8 recipient,u8 move,u16 frame,u32 object,u16 damage_value,
                            const BattleMvpVec& position,const NativeBattleReactionContact& contact={},
                            DamageRoleScope scope=DamageRoleScope::Unbound,u16 raw_base=0){
        if(!package_hit_relation(owner,recipient))return;
        const auto tier=package_tier_hit(owner,recipient,move,scope);
        auto& target=actors_[recipient];
        const f32 tier_factor=tier.move&&tier.move->contact_tier->scale?
            tier.receiver->damage_tier_receive->factors[target.damage_tier.level]:1.f;
        const auto amount=u16(std::min(actors_[recipient].health,
            unsigned(package_damage(owner,recipient,damage_value,scope,raw_base?raw_base:damage_value,move,tier_factor))));
        damage(recipient,amount,owner,move,contact);
        hit_impact(recipient,position,scope==DamageRoleScope::DirectMove?owner:actor_count(),move);
        if(scope==DamageRoleScope::DirectMove){
            const auto& profile=config_.packages[owner];
            if(profile&&move<profile->moves.size())if(const auto& hold=profile->moves[move].contact_pose_hold;hold){
                extend_contact_pose_hold(recipient,hold->ticks);
                if(hold->both_bodies)extend_contact_pose_hold(owner,hold->ticks);
            }
        }
        if(tier.move&&target.health)
            target.damage_tier.accept(tier.move->contact_tier->charge,tier.receiver->damage_tier_receive->idle_ticks,
                tier.receiver->damage_tier_reset_capacity,tier.move->contact_tier_reset_wear);
        if(scope==DamageRoleScope::DirectMove&&!target.contact_sound_cooldown&&target.damage_kind==0){
            const auto& attacker=config_.packages[owner];
            const auto& receiver=config_.packages[recipient];
            if(attacker&&receiver&&move<attacker->moves.size()&&receiver->contact_sound_band){
                const unsigned band=*receiver->contact_sound_band;
                if(const auto& cue=attacker->moves[move].contact_sounds[band];cue){
                    const unsigned bit=1u<<cue->lane;
                    const unsigned alternate=bool(target.contact_sound_parity&bit);
                    target.contact_sound_parity^=u16(bit);
                    target.contact_sound_cooldown=4;
                    record_package(BorgBattleEventKind::ContactSound,owner,move,frame,object,
                                   u16(2*band+alternate),recipient);
                }
            }
        }
        if(target.damage_kind==0){ // 8002F8DC returns before any ledger write
            ++target.hits_taken;target.damage_taken+=amount;
            if(owner!=recipient){
                auto& attacker=actors_[owner];++attacker.hits_dealt;attacker.damage_credited+=amount;
                if(attacker.damage_kind==0)team_tally_[attacker.team]+=100u;
            }
            ledger_force(owner,recipient,amount);
            // Each admitted body owns a ring until restart, even if defeated.
            // It never observes its own hit.
            for(unsigned p=0;p<actor_count();++p){
                if(p==recipient)continue;
                auto& slot=actors_[p];const u16 type=slot.team==target.team?4:3;
                slot.hit_history[slot.hit_history_index]={type,u16(recipient)};
                slot.hit_history_index=u8((slot.hit_history_index+1)%slot.hit_history.size());
                if(type==4){++slot.hit_history_count;slot.hit_history_total+=amount;}
            }
        }
        record_package(BorgBattleEventKind::Hit,owner,move,frame,object,amount,recipient);
    }
    void extend_contact_pose_hold(unsigned slot,u8 ticks){
        auto& actor=actors_[slot];const auto& profile=config_.packages[slot];
        if(actor.health&&actor.damage_kind==0&&!actor.source_ground_motion&&profile&&profile->contact_pose_hold_receive)
            actor.contact_pose_hold_ticks=std::max(actor.contact_pose_hold_ticks,ticks);
    }
    void retire_package_guard(unsigned defender){
        const auto window=std::exchange(guard_windows_[defender],{});
        if(window.move!=BorgBattleActionState::none)
            record_package(BorgBattleEventKind::GuardRetired,defender,window.move,window.frame,0,window.remaining);
    }
    void step_package_guard(unsigned defender){
        const auto& action=package_actions_[defender];
        const auto* spec=action.move==BorgBattleActionState::none?nullptr:&config_.packages[defender]->moves[action.move].guard;
        if(!actors_[defender].health||!spec||!*spec||action.frame<(*spec)->first||action.frame>(*spec)->last){
            retire_package_guard(defender);return;
        }
        auto& window=guard_windows_[defender];
        if(action.frame==(*spec)->first){
            window={action.move,(*spec)->contacts,action.frame};
            record_package(BorgBattleEventKind::GuardArmed,defender,action.move,action.frame,0,window.remaining);
        }else window.frame=action.frame;
    }
    bool try_package_guard(u8 defender,u8 attacker,BorgBattleAttackKind kind,BattlePoint toward_source,u32 object){
        auto& window=guard_windows_[defender];
        if(!window.remaining||!actors_[defender].health)return false;
        const auto& spec=*config_.packages[defender]->moves[window.move].guard;
        if(!(spec.mask&u8(kind)))return false;
        const auto heading=actors_[defender].facing;
        const f32 length=std::hypot(toward_source.x,toward_source.z);
        if(length<=.0001f||heading.x*toward_source.x+heading.z*toward_source.z<std::cos(spec.half_angle)*length)
            return false;
        --window.remaining;
        record_package(BorgBattleEventKind::GuardBlocked,defender,window.move,window.frame,object,window.remaining,attacker);
        if(!window.remaining)
            record_package(BorgBattleEventKind::GuardBroken,defender,window.move,window.frame,object,0,attacker);
        return true;
    }
    void retire_package_melee(unsigned owner){
        const auto window=std::exchange(melee_windows_[owner],{});
        if(window.object)record_package(BorgBattleEventKind::MeleeRetired,owner,window.move,window.frame,window.object,0);
    }
    void step_package_melee(unsigned owner,const IBattleMvpTerrain* terrain){
        const auto& action=package_actions_[owner];
        const auto& actor=actors_[owner];
        auto& window=melee_windows_[owner];
        const auto* move=action.move==BorgBattleActionState::none?nullptr:&config_.packages[owner]->moves[action.move];
        if(!actor.health||!move||!move->melee||action.frame<move->melee->first||action.frame>move->melee->last){
            retire_package_melee(owner);return;
        }
        if(actor_target(owner)==255){retire_package_melee(owner);return;}
        const auto& shape=*move->melee;
        if(action.frame==shape.first){
            if(next_id_==std::numeric_limits<u32>::max()){
                record_package(BorgBattleEventKind::MeleeRefused,owner,action.move,action.frame,0,0);return;
            }
            window={++next_id_,action.move,action.frame,false};
            record_package(BorgBattleEventKind::MeleeArmed,owner,action.move,action.frame,window.object,0);
        }
        if(!window.object||window.move!=action.move)return;
        window.frame=action.frame;
        if(window.hit)return;
        // Actor-bound planar capsule in the current body basis, checked
        // against the same destination body/height proxy as a projectile.
        // No aim ray, animation mesh, target lock or inventory write.
        const f32 height=actor.position[1]+shape.height;
        const bool direct=borg_battle_direct_melee(*move);
        const bool roster=actor_count()>2&&direct;
        const auto contact=package_body_contact(owner,roster,[&](u8 candidate,f32* entry){
            const auto& other=actors_[candidate];
            if(!other.health||!native_borg_melee_height_overlap(height,shape.half_height,other.position[1],
                                                               config_.actor_height))return false;
            const f32 x=other.position[0]-actor.position[0],z=other.position[2]-actor.position[2];
            if(x*actor.facing.x+z*actor.facing.z<0)return false;
            const BattlePoint from{actor.position[0],actor.position[2]},to{other.position[0],other.position[2]};
            return battle_swept_hit(from,{actor.facing.x*shape.reach,actor.facing.z*shape.reach},to,
                                    config_.actor_radius+shape.radius,entry)&&
                   (!direct||native_borg_melee_path_clear(terrain,from,to));
        });
        if(!contact)return;
        const auto recipient=contact.recipient;
        const auto& other=actors_[recipient];
        const f32 x=other.position[0]-actor.position[0],z=other.position[2]-actor.position[2];
        const f32 t=contact.entry;
        window.hit=true;
        if(try_package_guard(recipient,u8(owner),BorgBattleAttackKind::Melee,{-x,-z},window.object))return;
        NativeBattleReactionContact reaction;
        reaction.body_forward=actor.facing;
        // Mode 3 samples the centres of the actual native proxies, not
        // the endpoint of the capsule or the point of first overlap.
        const f32 centre=shape.reach*.5f;
        reaction.centres=NativeBattleReactionCentres{
            {actor.position[0]+actor.facing.x*centre,height,actor.position[2]+actor.facing.z*centre},
            {other.position[0],other.position[1]+config_.actor_height*.5f,other.position[2]}};
        accept_package_hit(owner,recipient,action.move,action.frame,window.object,
            move->contact_power?move->contact_power:shape.damage,
            {actor.position[0]+actor.facing.x*shape.reach*t,height,actor.position[2]+actor.facing.z*shape.reach*t},
            reaction,DamageRoleScope::DirectMove);
    }
    void queue_package_pull(const BattleMvpProjectile& shot,u8 recipient,const BattleMvpVec& observed_position){
        auto& next=pull_requests_[shot.owner];
        if(next.object){
            record_package(BorgBattleEventKind::PullRefused,shot.owner,shot.package_move,0,shot.id,
                u16(BorgBattlePullRefusal::AlreadyQueued),recipient);
            return;
        }
        next={{observed_position[0],observed_position[2]},shot.id,shot.package_pull_units,shot.package_move,recipient};
        record_package(BorgBattleEventKind::PullQueued,shot.owner,shot.package_move,0,shot.id,shot.package_pull_units,recipient);
    }
    bool consume_package_pull(unsigned owner,const IBattleMvpTerrain* terrain,bool jump){
        // The round owns both actor slots until restart. This pending request
        // is never exported or resolvable against a replacement world.
        const auto request=std::exchange(pull_requests_[owner],{});
        if(!request.object)return false;
        auto refuse=[&](BorgBattlePullRefusal why){
            record_package(BorgBattleEventKind::PullRefused,owner,request.move,0,request.object,u16(why),request.recipient);
            return false;
        };
        auto& actor=actors_[owner];
        if(config_.packages[owner]->movement!=BorgBattleMovement::Grounded||!actor.health||!actor.grounded||
           actor.vertical_speed||actor.dash_ticks||actor.landing_ticks||actor.hit_flash||jump)
            return refuse(BorgBattlePullRefusal::OwnerBusy);
        const f32 x=request.destination.x-actor.position[0],z=request.destination.z-actor.position[2],distance=std::hypot(x,z);
        if(distance<=.01f)return refuse(BorgBattlePullRefusal::NoTravel);
        const auto scale=std::min(1.f,f32(request.units)/distance);
        const BattlePoint delta{x*scale,z*scale};
        bool accepted=false;
        const auto reason=apply_package_flat_step(owner,delta,terrain,&accepted);
        if(!accepted)return refuse(reason);
        record_package(BorgBattleEventKind::PullApplied,owner,request.move,0,request.object,
            u16(std::lround(actor.movement_speed)),request.recipient);
        return true;
    }
    bool consume_package_field_pull(unsigned recipient,const IBattleMvpTerrain* terrain,bool input_priority){
        const auto& actor=actors_[recipient];
        if(input_priority||config_.packages[recipient]->movement!=BorgBattleMovement::Grounded||!actor.health||
           !actor.grounded||actor.vertical_speed||actor.dash_ticks||actor.landing_ticks||actor.hit_flash)return false;
        BattleMvpVec point=actor.position;point[1]+=config_.actor_height*.5f;
        const BattleMvpField* selected=nullptr;
        f32 nearest=0;
        BattlePoint toward{};
        for(const auto& field:fields_)if(field.active()&&field.spec.actor_pull&&
          (!field.spec.follow||actors_[field.owner].health)&&
          field_scope_matches(field.spec.actor_pull->scope,field.owner,recipient)){
            f64 enter=0,exit=0;
            if(!package_field_interval(point,{},field,&enter,&exit))continue;
            const auto& pull=*field.spec.actor_pull;
            auto centre=field.centre;
            if(pull.point){
                centre[0]+=field.heading.x*pull.point->forward;centre[1]+=pull.point->height;
                centre[2]+=field.heading.z*pull.point->forward;
            }
            const f32 x=centre[0]-point[0],z=centre[2]-point[2],distance=std::hypot(x,z);
            if(distance<=.05f)continue;
            if(pull.point&&std::hypot(distance,centre[1]-point[1])<pull.point->minimum)continue;
            const f32 angle=pull.half_angle;
            if(angle&&-(x*field.heading.x+z*field.heading.z)<std::cos(angle)*distance)continue;
            if(!selected||distance<nearest||(distance==nearest&&field.id<selected->id)){
                selected=&field;nearest=distance;toward={x,z};
            }
        }
        if(!selected)return false;
        const auto& pull=*selected->spec.actor_pull;
        const f32 scale=(pull.repel?-pull.step:std::min(nearest,pull.step))/nearest;
        const BattlePoint delta{toward.x*scale,toward.z*scale};
        auto record=[&](BorgBattleEventKind event,u16 value){
            record_package(event,selected->owner,selected->move,0,selected->id,value,u8(recipient));
        };
        if(!package_forced_sweep_clear(recipient,delta,terrain)){
            record(BorgBattleEventKind::FieldActorPullRefused,u16(BorgBattlePullRefusal::TerrainOrBody));
            return false;
        }
        bool accepted=false;
        const auto reason=apply_package_flat_step(recipient,delta,terrain,&accepted);
        record(accepted?BorgBattleEventKind::FieldActorPulled:BorgBattleEventKind::FieldActorPullRefused,
               accepted?u16(std::lround(actor.movement_speed)):u16(reason));
        return accepted;
    }
    bool package_forced_sweep_clear(unsigned recipient,BattlePoint delta,const IBattleMvpTerrain* terrain)const{
        if(!terrain||!terrain->supports_projectile_stage())return true;
        const f32 len=std::hypot(delta.x,delta.z);
        if(len<=.0001f)return false;
        auto point=actors_[recipient].position;point[1]+=config_.actor_height*.5f;
        for(f32 sample:{-config_.actor_radius,0.f,config_.actor_radius}){
            const BattleMvpVec start{point[0]-delta.z/len*sample,point[1],point[2]+delta.x/len*sample};
            const BattleMvpVec end{start[0]+delta.x,start[1],start[2]+delta.z};
            NativeBorgStageProjectileHit wall{};
            if(!terrain->projectile_stage(start,end,&wall)||wall.hit)return false;
        }
        return true;
    }
    BorgBattlePullRefusal apply_package_flat_step(unsigned owner,BattlePoint delta,const IBattleMvpTerrain* terrain,bool* accepted){
        auto& actor=actors_[owner];*accepted=false;
        // Shared grounded motion acceptance for short pulled/rewound steps.
        // Do not authorize a slope, ledge drop or unknown support here.
        for(f32 at:{.5f,1.f}){
            f32 y=0;
            if(!floor(terrain,actor.position[0]+delta.x*at,actor.position[2]+delta.z*at,actor.position[1],&y))
                return BorgBattlePullRefusal::SupportUnavailable;
            if(std::abs(y-actor.position[1])>.01f)return BorgBattlePullRefusal::TerrainOrBody;
        }
        const u8 pull_target=actor_target(owner);
        if(pull_target==255)return BorgBattlePullRefusal::TerrainOrBody;
        NativeBorgContactResult result;
        if(!resolve_native_body_motion(owner,pull_target,delta,true,terrain,&result))
            return BorgBattlePullRefusal::SupportUnavailable;
        if(!result.grounded||result.terrain_blocked||result.body_hit||result.bounds_hit||result.slid)
            return BorgBattlePullRefusal::TerrainOrBody;
        const f32 speed=std::hypot(result.actual_delta.x,result.actual_delta.z);
        if(speed<=.0001f)return BorgBattlePullRefusal::NoTravel;
        actor.position=result.position;
        actor.velocity={}; //do not carry the forced step as momentum into later input
        actor.movement_speed=speed;
        actor.travel_distance+=actor.movement_speed;
        actor.move_forward=(result.actual_delta.x*actor.facing.x+result.actual_delta.z*actor.facing.z)/actor.movement_speed;
        actor.move_right=(-result.actual_delta.x*actor.facing.z+result.actual_delta.z*actor.facing.x)/actor.movement_speed;
        actor.action=BattleMvpAction::Move;
        *accepted=true;return BorgBattlePullRefusal::NoTravel; // reason is used only on refusal
    }
    void visit_package_anchor_markers(unsigned owner){
        const auto& action=package_actions_[owner];const auto& actor=actors_[owner];
        if(!actor.health||action.move==BorgBattleActionState::none)return;
        const auto& profile=*config_.packages[owner];
        for(const auto& marker:profile.moves[action.move].anchors)if(marker.frame==action.frame){
            auto& path=anchor_paths_[owner][marker.set];auto& winding=rewinds_[owner];
            const bool rewind=marker.rewind_speed!=0;
            const BattleMvpVec point{actor.position[0]+actor.facing.x*marker.forward,actor.position[1]+marker.height,
                                     actor.position[2]+actor.facing.z*marker.forward};
            if(winding.id||next_id_==std::numeric_limits<u32>::max()||
               (rewind?!path.count:path.count>=profile.anchor_sets[marker.set].capacity)||
               std::abs(point[0])>config_.arena_half||std::abs(point[2])>config_.arena_half){
                record_package(BorgBattleEventKind::AnchorRefused,owner,action.move,action.frame,0,marker.set);continue;
            }
            if(rewind){
                winding={++next_id_,path,marker.rewind_speed,action.move,u8(owner),actor.package_damage_serial};
                path={}; // the exact accepted set moves into the independent traversal
                record_package(BorgBattleEventKind::RewindQueued,owner,action.move,action.frame,winding.id,winding.path.count);
                continue;
            }
            if(marker.cost){
                auto& stock=package_actions_[owner].stocks[marker.stock];WeaponStockPlan debit;
                if(!weapon_stock_plan(stock,BorgBattleActionState::stock_mode(profile.stocks[marker.stock]),
                   WeaponStockOperation::Debit,marker.cost,&debit)||!debit.accepted){
                    record_package(BorgBattleEventKind::StockRefused,owner,action.move,action.frame,0,u16(stock.current));continue;
                }
                stock=debit.state;record_package(BorgBattleEventKind::StockSpent,owner,action.move,action.frame,0,u16(stock.current));
            }
            append_package_anchor(owner,action.move,action.frame,path,point,
                marker.cost&&profile.anchor_sets[marker.set].return_on_reach?marker.stock:255);
        }
    }
    void append_package_anchor(unsigned owner,u8 move,u16 frame,BattleMvpAnchorPath& path,const BattleMvpVec& position,u8 return_stock=255){
        const auto id=++next_id_;
        path.points[path.count++]={position,id,tick_,return_stock};
        record_package(BorgBattleEventKind::AnchorPlaced,owner,move,frame,id,path.count);
    }
    void plant_package_anchor(const BattleMvpProjectile& shot){
        const auto& profile=*config_.packages[shot.owner];
        const auto set=profile.projectiles[shot.package_projectile].plant_on_stage;
        if(set==255)return;
        auto& path=anchor_paths_[shot.owner][set];
        const auto& point=shot.position;
        if(!actors_[shot.owner].health||rewinds_[shot.owner].id||next_id_==std::numeric_limits<u32>::max()||
           path.count>=profile.anchor_sets[set].capacity||std::abs(point[0])>config_.arena_half||
           std::abs(point[2])>config_.arena_half){
            record_package(BorgBattleEventKind::AnchorRefused,shot.owner,shot.package_move,0,shot.id,set);
            return;
        }
        append_package_anchor(shot.owner,shot.package_move,0,path,point);
        record_package(BorgBattleEventKind::AnchorPlanted,shot.owner,shot.package_move,0,shot.id,path.count);
    }
    void expire_package_fence_anchors(){
        for(unsigned owner=0;owner<actor_count();++owner){
            const auto& sets=config_.packages[owner]->anchor_sets;
            for(unsigned i=0;i<sets.size();++i)if(const auto& spec=sets[i].fence){
                auto& path=anchor_paths_[owner][i];
                auto& latches=fence_inside_[owner][i];
                if(!actors_[owner].health){path={};latches={};continue;}
                const auto first=std::find_if(path.points.begin(),path.points.begin()+path.count,
                    [&](const auto& p){return tick_-p.born<spec->ticks;});
                const auto last=path.points.begin()+path.count;
                if(first!=path.points.begin()){
                    std::move(first,last,path.points.begin());
                    path.count=u8(last-first);
                }
                if(path.count<2)latches={};
            }
        }
    }
    // Closest points between two finite 3D segments; either segment can be
    // a point. The returned point on the fence also defines recoil direction.
    static f64 fence_distance_sq(const BattleMvpVec& a,const BattleMvpVec& b,
                                 const BattleMvpVec& p,const BattleMvpVec& q,BattleMvpVec* nearest){
        using V=std::array<f64,3>;
        auto diff=[](const auto& x,const auto& y){return V{f64(x[0])-y[0],f64(x[1])-y[1],f64(x[2])-y[2]};};
        auto dot=[](const V& x,const V& y){return x[0]*y[0]+x[1]*y[1]+x[2]*y[2];};
        const auto d=diff(b,a),e=diff(q,p),r=diff(a,p);
        const f64 dd=dot(d,d),ee=dot(e,e),de=dot(d,e),dr=dot(d,r),er=dot(e,r);
        constexpr f64 tiny=1e-10;
        f64 s=0,t=0;
        if(dd<=tiny){if(ee>tiny)t=std::clamp(er/ee,0.,1.);}
        else if(ee<=tiny)s=std::clamp(-dr/dd,0.,1.);
        else{
            const f64 denom=dd*ee-de*de;
            if(denom>tiny)s=std::clamp((de*er-dr*ee)/denom,0.,1.);
            t=(de*s+er)/ee;
            if(t<0){t=0;s=std::clamp(-dr/dd,0.,1.);}
            else if(t>1){t=1;s=std::clamp((de-dr)/dd,0.,1.);}
        }
        V delta;
        for(unsigned k=0;k<3;++k){
            if(nearest)(*nearest)[k]=f32(a[k]+s*d[k]);
            delta[k]=r[k]+s*d[k]-t*e[k];
        }
        return dot(delta,delta);
    }
    void step_package_fences(const std::array<BattleMvpVec,kBattleMvpMaxActors>& before){
        fence_link_count_=0;
        const f32 body=std::min(config_.actor_height*.5f,config_.actor_radius);
        for(unsigned owner=0;owner<actor_count();++owner){
            const auto& sets=config_.packages[owner]->anchor_sets;
            for(unsigned set=0;set<sets.size();++set)if(const auto& spec=sets[set].fence){
                auto& latches=fence_inside_[owner][set];
                const auto& path=anchor_paths_[owner][set];
                if(!actors_[owner].health||path.count<2){latches={};continue;}
                const auto each_link=[&](auto visit){
                    for(unsigned i=1;i<path.count;++i){
                        const auto& a=path.points[i-1];const auto& b=path.points[i];
                        if(a.position!=b.position)visit(a,b);
                    }
                };
                bool linked=false;
                each_link([&](const auto& a,const auto& b){
                    linked=true;
                    if(fence_link_count_<fence_links_.size())fence_links_[fence_link_count_++]={a.position,b.position};
                });
                if(!linked){latches={};continue;}

                const f64 limit=f64(spec->radius+body)*(spec->radius+body);
                for(u8 target=0;target<actor_count();++target){
                    auto& actor=actors_[target];auto& latched=latches[target];
                    if(!actor.health||actor.team==actors_[owner].team||!package_hit_relation(owner,target)){
                        latched=false;continue;
                    }
                    auto previous=before[target],now=actor.position;
                    previous[1]+=config_.actor_height*.5f;now[1]+=config_.actor_height*.5f;
                    bool inside=false;u32 hit=0;BattleMvpVec contact{};
                    each_link([&](const auto& a,const auto& b){
                        inside|=fence_distance_sq(a.position,b.position,now,now,nullptr)<=limit;
                        if(!hit&&!latched&&fence_distance_sq(a.position,b.position,previous,now,&contact)<=limit)hit=b.id;
                    });
                    latched=inside;
                    if(!hit)continue;
                    const BattlePoint inbound{contact[0]-previous[0],contact[2]-previous[2]};
                    if(try_package_guard(target,u8(owner),BorgBattleAttackKind::Blast,inbound,hit))continue;
                    accept_package_hit(owner,target,BorgBattleActionState::none,0,hit,spec->damage,contact);
                    if(spec->recoil&&actor.health&&!fence_recoil_[target].object){
                        const auto heading=native_borg_direction({0,0},{-inbound.x,-inbound.z},
                            native_borg_direction({now[0],now[2]},{previous[0],previous[2]},{-actor.facing.x,-actor.facing.z}));
                        fence_recoil_[target]={{heading.x*spec->recoil,heading.z*spec->recoil},hit,u8(owner)};
                    }
                }
            }
        }
    }
    bool consume_package_fence_recoil(unsigned recipient,const IBattleMvpTerrain* terrain,bool input_priority){
        const auto push=std::exchange(fence_recoil_[recipient],{});
        if(!push.object)return false;
        const auto& actor=actors_[recipient];
        bool accepted=false;
        if(!input_priority&&actor.health&&actor.grounded&&!actor.vertical_speed&&!actor.dash_ticks&&
           config_.packages[recipient]->movement==BorgBattleMovement::Grounded&&package_forced_sweep_clear(recipient,push.step,terrain))
            apply_package_flat_step(recipient,push.step,terrain,&accepted);
        record_package(accepted?BorgBattleEventKind::FenceRecoilApplied:BorgBattleEventKind::FenceRecoilRefused,
            push.owner,BorgBattleActionState::none,0,push.object,0,u8(recipient));
        return accepted;
    }
    void finish_package_rewind(unsigned owner,std::optional<BorgBattlePullRefusal> reason={}){
        const auto current=std::exchange(rewinds_[owner],{});
        record_package(reason?BorgBattleEventKind::RewindStopped:BorgBattleEventKind::RewindCompleted,
            current.issuer,current.move,0,current.id,reason?u16(*reason):0,u8(owner));
    }
    bool consume_package_rewind(unsigned owner,const IBattleMvpTerrain* terrain,bool busy){
        auto& current=rewinds_[owner];if(!current.id)return false;
        auto& actor=actors_[owner];
        if(busy||!actor.health||!actor.grounded||actor.vertical_speed||actor.dash_ticks||actor.landing_ticks||
           (current.issuer==owner?actor.hit_flash!=0:actor.package_damage_serial!=current.damage_serial)){
            finish_package_rewind(owner,BorgBattlePullRefusal::OwnerBusy);return false;
        }
        while(current.path.count){
            const auto& point=current.path.points[current.path.count-1];
            const auto reached=[&](){
                --current.path.count;
                record_package(BorgBattleEventKind::RewindReached,current.issuer,current.move,0,point.id,current.path.count,u8(owner));
                const auto channel=point.return_stock;
                if(current.issuer!=owner||channel==255)return;
                auto& stock=package_actions_[owner].stocks[channel];
                const auto old=stock.current;
                if(weapon_stock_return_one(stock,s16(config_.packages[owner]->stocks[channel].capacity),&stock)&&stock.current!=old)
                    record_package(BorgBattleEventKind::StockReturned,owner,current.move,0,point.id,u16(stock.current));
            };
            if(std::abs(point.position[1]-actor.position[1])>.01f){
                finish_package_rewind(owner,BorgBattlePullRefusal::TerrainOrBody);return false;
            }
            const BattlePoint delta{point.position[0]-actor.position[0],point.position[2]-actor.position[2]};
            const auto length=std::hypot(delta.x,delta.z);
            if(length>.001f){
                const f32 scale=std::min(1.f,f32(current.speed)/length);
                bool accepted=false;
                const BattlePoint step{delta.x*scale,delta.z*scale};
                if(point.return_stock!=255&&!package_forced_sweep_clear(owner,step,terrain)){
                    finish_package_rewind(owner,BorgBattlePullRefusal::TerrainOrBody);return false;
                }
                const auto reason=apply_package_flat_step(owner,step,terrain,&accepted);
                if(!accepted){finish_package_rewind(owner,reason);return false;}
                if(scale==1){
                    reached();
                    if(!current.path.count)finish_package_rewind(owner);
                }
                return true;
            }
            reached();
        }
        finish_package_rewind(owner);return false;
    }
    u32 emit_package(unsigned owner,u8 move,const BorgBattleProjectileDefinition& definition,u8 return_stock,const IBorgBattleOrigins* origins){
        const auto& actor=actors_[owner];auto aim=definition.launch_body_heading?actor.facing:actor.aim_facing;
        BattleMvpVec muzzle{actor.position[0]+aim.x*(config_.actor_radius+12),
            actor.position[1]+config_.actor_height*.6f,actor.position[2]+aim.z*(config_.actor_radius+12)};
        f32 additional_rise=0;
        const auto frame=package_actions_[owner].frame;
        if(definition.socket){
            const auto& spec=*definition.socket;
            if(!origins||!origins->socket(owner,actor,tick_,move,frame,spec,&muzzle)||
               !std::all_of(muzzle.begin(),muzzle.end(),[](f32 x){return std::isfinite(x)&&std::abs(x)<=1e5f;})){
                record_package(BorgBattleEventKind::SocketRefused,owner,move,frame,0,spec.bone);return 0;
            }
            const u8 socket_target=package_aim_target(owner);
            const auto& target=actors_[socket_target==255?owner:socket_target];
            // A body-heading launch samples the socket only for position.
            if(!definition.launch_body_heading&&actor.package_aim_heading){
                if(spec.aim_height)additional_rise=definition.speed*actor.package_aim_slope;
            }else if(!definition.launch_body_heading&&actor.package_target_visible){
                aim=native_borg_direction({muzzle[0],muzzle[2]},{target.position[0],target.position[2]},aim);
                if(spec.aim_height){
                    const f32 distance=std::max(1.f,std::hypot(target.position[0]-muzzle[0],target.position[2]-muzzle[2]));
                    additional_rise=std::clamp(definition.speed*(target.position[1]+config_.actor_height*.5f-muzzle[1])/distance,
                                               -definition.speed,definition.speed);
                }
            }
        }
        auto* path=definition.rewind_hit?&anchor_paths_[owner][definition.rewind_hit->set]:nullptr;
        if(path&&!path->count)return 0;
        const auto& accuracy=accuracy_[owner];
        const bool affected=definition.accuracy_sensitive&&accuracy.object&&accuracy.born!=tick_;
        if(affected){
            // Mix the accepted object sequence with the status source. No
            // draw-, clock-, or global input-driven random state is consumed.
            u32 hash=(next_id_+1u)^accuracy.object*0x9e3779b9u;
            hash^=hash>>16;hash*=0x7feb352du;hash^=hash>>15;hash*=0x846ca68bu;hash^=hash>>16;
            const f32 yaw=(f32(hash&65535u)*(2.f/65535.f)-1.f)*accuracy.spec.radians;
            const f32 c=std::cos(yaw),s=std::sin(yaw);
            aim={aim.x*c-aim.z*s,aim.z*c+aim.x*s};
        }
        const u32 first=allocate_package_volley(owner,move,definition,muzzle,aim,return_stock,0,path,additional_rise,
                                                /*radial=*/false,/*direct_release=*/true);
        if(first&&!definition.launch_sound.empty()){
            const auto& definitions=config_.packages[owner]->projectiles;
            record_package(BorgBattleEventKind::LaunchSound,owner,move,frame,first,u16(&definition-definitions.data()));
        }
        if(first&&affected)
            record_package(BorgBattleEventKind::AccuracyAffected,owner,move,frame,first,0);
        if(first&&definition.socket)
            record_package(BorgBattleEventKind::SocketLaunch,owner,move,frame,first,definition.socket->bone);
        if(first&&path){
            record_package(BorgBattleEventKind::AnchorPathSealed,owner,move,package_actions_[owner].frame,first,path->count);
            *path={};
        }
        return first;
    }
    void settle_package_pairs(unsigned owner){
        auto& action=package_actions_[owner];
        auto& pending=pair_returns_[owner];
        for(unsigned channel=0;channel<pending.size();++channel){
            auto& pair=pending[channel];
            if(!pair.pending||std::any_of(projectiles_.begin(),projectiles_.end(),[&](const auto& shot){
                return shot.active&&(shot.id==pair.ids[0]||shot.id==pair.ids[1]);
            }))continue;
            auto& stock=action.stocks[channel];
            const auto old=stock.current;
            if(weapon_stock_return_one(stock,1,&stock)&&stock.current!=old)
                record_package(BorgBattleEventKind::StockReturned,owner,pair.move,0,
                               pair.ids[0]?pair.ids[0]:pair.ids[1],u16(stock.current));
            pair={};
        }
    }
    u32 allocate_package_volley(unsigned owner,u8 move,const BorgBattleProjectileDefinition& definition,
                                const BattleMvpVec& muzzle,BattlePoint aim,u8 return_stock,u32 emitter=0,
                                 const BattleMvpAnchorPath* rewind_path=nullptr,f32 additional_rise=0,
                                 bool radial=false,bool direct_release=false){
        unsigned free=0;for(const auto& shot:projectiles_)free+=!shot.active;
        if(!definition.count||free<definition.count||next_id_>std::numeric_limits<u32>::max()-definition.count)return 0;
        const u8 target=direct_release?package_aim_target(owner):actor_target(owner);
        const auto& observed=actors_[target==255?owner:target];
        const bool guide=definition.guidance&&observed.health&&(emitter||actors_[owner].package_target_visible);
        const u32 first=next_id_+1;unsigned n=0;
        u16 damage=definition.damage;
        const auto& moves=config_.packages[owner]->moves;
        if(direct_release&&move<moves.size()&&moves[move].contact_power)damage=moves[move].contact_power;
        for(auto& shot:projectiles_)if(!shot.active){
            const f32 angle=radial?f32(n)*6.28318530718f/definition.count:
                (f32(n)-f32(definition.count-1)*.5f)*definition.spread;
            const f32 c=std::cos(angle),s=std::sin(angle);
            shot={true,++next_id_,u8(owner),definition.visual==BorgAttackVisual::Beam?BattleMvpWeapon::EnergyBeam:
                  definition.visual==BorgAttackVisual::Shuriken?BattleMvpWeapon::Shuriken:
                  definition.visual==BorgAttackVisual::Orb?BattleMvpWeapon::Orb:BattleMvpWeapon::Bullet,
                  muzzle,muzzle,{definition.speed*(aim.x*c-aim.z*s),definition.rise+additional_rise,definition.speed*(aim.z*c+aim.x*s)},definition.lifetime,false,
                    damage,definition.gravity,definition.radius,move,definition.floor,
                   definition.walk_percent,definition.walk_ticks,definition.pull_units,true,definition.ignore_actors,definition.blast,
                   u8(&definition-config_.packages[owner]->projectiles.data()),return_stock};
            shot.package_emitter=emitter;
            shot.package_direct_release=direct_release;
            shot.package_base_damage=damage;
            shot.package_look=definition.look.value_or(BorgProjectileLook{});shot.born_tick=tick_;
            shot.package_animation=definition.animation.value_or(BorgProjectileAnimation{
                definition.visual==BorgAttackVisual::Shuriken?.36f:0.f,0,0});
            if(definition.sustained){
                shot.sustained=true;
                const auto length=std::hypot(definition.speed,shot.velocity[1]);
                for(unsigned k=0;k<3;++k){
                    shot.beam_origin[k]=definition.beam_sweep?muzzle[k]:muzzle[k]-actors_[owner].position[k];
                    shot.velocity[k]*=definition.sustained->length/length; // segment delta, not travel velocity
                }
                if(const auto& sweep=definition.beam_sweep){
                    const f32 reach=std::sqrt(f32(definition.sustained->length)*definition.sustained->length-f32(sweep->drop)*sweep->drop);
                    shot.velocity={aim.x*reach,-f32(sweep->drop),aim.z*reach};
                }
            }
            if(rewind_path)shot.package_rewind_path=*rewind_path;
            if(guide){
                shot.package_guide_point={observed.position[0],observed.position[2]};
                shot.package_guide_target=target;shot.package_guide_ticks=definition.guidance->ticks;
            }
            ++shots_;if(++n==definition.count)break;
        }
        return first;
    }
    static BattleMvpVec emitter_position(const BattleMvpActor& actor,const BorgBattleEmitter& spec){
        const auto& h=actor.facing;
        return {actor.position[0]+h.z*spec.side-h.x*spec.behind,actor.position[1]+spec.height,
                actor.position[2]-h.x*spec.side-h.z*spec.behind};
    }
    void retire_package_emitter(unsigned owner,BattleMvpEmitter& slot,u16 reason){
        if(slot.id)record_package(BorgBattleEventKind::EmitterRetired,owner,slot.deployment_move,0,slot.id,reason);
        slot={};
    }
    void step_package_emitters(unsigned owner){
        const auto& actor=actors_[owner];const auto& rules=config_.packages[owner]->emitters;
        for(unsigned i=0;i<rules.size();++i)if(auto& slot=emitters_[owner][i];slot.id){
            if(!actor.health)retire_package_emitter(owner,slot,2);
            else if(!--slot.remaining)retire_package_emitter(owner,slot,1);
            else slot.position=emitter_position(actor,rules[i]);
        }
    }
    void deploy_package_emitters(unsigned owner){
        const auto& action=package_actions_[owner];const auto& actor=actors_[owner];
        if(!actor.health||action.move==BorgBattleActionState::none)return;
        const auto& profile=*config_.packages[owner];
        for(const auto& marker:profile.moves[action.move].emitters)if(marker.frame==action.frame){
            auto& slot=emitters_[owner][marker.emitter];
            if(slot.id||next_id_==std::numeric_limits<u32>::max()){
                record_package(BorgBattleEventKind::EmitterDeployRefused,owner,action.move,action.frame,slot.id,marker.emitter);
                continue;
            }
            const auto& rule=profile.emitters[marker.emitter];
            slot={++next_id_,emitter_position(actor,rule),rule.lifetime,action.move};
            record_package(BorgBattleEventKind::EmitterDeployed,owner,action.move,action.frame,slot.id,marker.emitter);
        }
    }
    void emit_package_followers(unsigned owner,u8 move,u16 frame){
        const auto& profile=*config_.packages[owner];const auto& actor=actors_[owner];
        const u8 follower_target=actor_target(owner);
        const auto& other=actors_[follower_target==255?owner:follower_target];
        // Follows an accepted allocation, never raw input or another child's
        // emission; allocation may independently refuse with no stock changes.
        for(unsigned i=0;i<profile.emitters.size();++i){
            const auto& spec=profile.emitters[i];const auto& slot=emitters_[owner][i];
            if(!slot.id||spec.watches!=move)continue;
            const auto& attack=profile.projectiles[spec.projectile];
            const auto aim=other.health?native_borg_direction({slot.position[0],slot.position[2]},
                {other.position[0],other.position[2]},actor.aim_facing):actor.aim_facing;
            const auto first=allocate_package_volley(owner,move,attack,slot.position,aim,255,slot.id);
            record_package(first?BorgBattleEventKind::EmitterFired:BorgBattleEventKind::EmitterFireRefused,
                owner,move,frame,first?first:slot.id,first?attack.count:0);
        }
    }
    enum class PackageRetirement:u16 {None,Actor,Floor,UnknownFloor,Expired,Outside,Remote,Guard,Stage,UnknownStage,OwnerEnded,Proximity,UnlandedFuse};
    bool package_smoke_blocks_aim(unsigned owner,u8 smoke_target)const{
        const auto& start=actors_[owner].position;const auto& end=actors_[smoke_target].position;
        const BattleMvpVec delta{end[0]-start[0],end[1]-start[1],end[2]-start[2]};
        const f32 length=delta[0]*delta[0]+delta[1]*delta[1]+delta[2]*delta[2];
        for(const auto& cloud:smoke_clouds_)if(cloud.parent&&cloud.born!=tick_&&cloud.spec.block_aim){
            const BattleMvpVec relative{cloud.centre[0]-start[0],cloud.centre[1]-start[1]-config_.actor_height*.5f,cloud.centre[2]-start[2]};
            const f32 t=length?std::clamp((relative[0]*delta[0]+relative[1]*delta[1]+relative[2]*delta[2])/length,0.f,1.f):0;
            f32 distance=0;
            for(unsigned k=0;k<3;++k){const auto d=relative[k]-t*delta[k];distance+=d*d;}
            if(distance<=f32(cloud.spec.radius)*cloud.spec.radius)return true;
        }
        return false;
    }
    void trigger_package_blast(const BattleMvpProjectile& shot,PackageRetirement why){
        if(!shot.package_blast)return;
        const auto& blast=*shot.package_blast;
        const auto& definition=config_.packages[shot.owner]->projectiles[shot.package_projectile];
        if(!((why==PackageRetirement::Actor&&(blast.trigger==BorgBlastTrigger::Actor||blast.trigger==BorgBlastTrigger::Impact))||
             (why==PackageRetirement::Floor&&(blast.trigger==BorgBlastTrigger::Floor||blast.trigger==BorgBlastTrigger::Impact))||
             (why==PackageRetirement::Stage&&(blast.trigger==BorgBlastTrigger::Stage||blast.trigger==BorgBlastTrigger::Impact))||
             (why==PackageRetirement::Expired&&blast.trigger==BorgBlastTrigger::Expiry)||
             (why==PackageRetirement::Proximity&&definition.proximity&&definition.proximity->blast)||
             (why==PackageRetirement::Remote&&blast.trigger==BorgBlastTrigger::Remote)))return;
        // One immediate query, not a resident field, and no damage is attached
        // to the presentation ring. Sample the current body centres in order.
        record_package(BorgBattleEventKind::BlastTriggered,shot.owner,shot.package_move,0,shot.id,u16(why));
        if(blast.damage)impact(shot.position,true,f32(blast.radius));
        if(definition.smoke){
            const auto available=std::find_if(smoke_clouds_.begin(),smoke_clouds_.end(),[](const auto& cloud){return !cloud.parent;});
            if(available==smoke_clouds_.end())
                record_package(BorgBattleEventKind::SmokeRefused,shot.owner,shot.package_move,0,shot.id,0);
            else{
                *available={shot.position,shot.id,tick_,*definition.smoke};
                available->centre[1]+=definition.smoke->radius*.5f;
                record_package(BorgBattleEventKind::SmokeCreated,shot.owner,shot.package_move,0,shot.id,definition.smoke->radius);
            }
        }
        const f32 radius_squared=f32(blast.radius)*f32(blast.radius);
        const auto owner_team=actors_[shot.owner].team;
        for(u8 i=0;blast.damage&&i<actor_count();++i){
            const auto& actor=actors_[i];
            if(!actor.health||(!blast.include_owner&&actor.team==owner_team))continue;
            const f32 x=actor.position[0]-shot.position[0],y=actor.position[1]+config_.actor_height*.5f-shot.position[1],
                      z=actor.position[2]-shot.position[2];
            if(x*x+y*y+z*z>radius_squared)continue;
            if(try_package_guard(i,shot.owner,BorgBattleAttackKind::Blast,{-x,-z},shot.id))continue;
            const auto amount=u16(std::min(actor.health,unsigned(blast.damage)));
            damage(i,amount,shot.owner,shot.package_move);
            record_package(BorgBattleEventKind::BlastHit,shot.owner,shot.package_move,0,shot.id,amount,i);
            if(definition.ink&&actor.health){
                ink_[i]={shot.id,tick_,*definition.ink};
                record_package(BorgBattleEventKind::InkApplied,shot.owner,shot.package_move,0,shot.id,definition.ink->ticks,i);
            }
            if(definition.accuracy&&actor.health){
                accuracy_[i]={shot.id,tick_,*definition.accuracy};
                record_package(BorgBattleEventKind::AccuracyApplied,shot.owner,shot.package_move,0,shot.id,
                               definition.accuracy->ticks,i);
            }
        }
        if(definition.blast_children!=255)queue_package_children(shot,definition.blast_children,{0,1},true,ChildTrigger::Blast);
    }
    enum class ChildTrigger:u8 {Blast,Flight,Impact};
    static BorgBattleEventKind child_event(ChildTrigger trigger,bool success){
        switch(trigger){
        case ChildTrigger::Blast:return success?BorgBattleEventKind::BlastChildren:BorgBattleEventKind::BlastChildrenRefused;
        case ChildTrigger::Flight:return success?BorgBattleEventKind::FlightChildren:BorgBattleEventKind::FlightChildrenRefused;
        case ChildTrigger::Impact:return success?BorgBattleEventKind::ImpactChildren:BorgBattleEventKind::ImpactChildrenRefused;
        }
        return BorgBattleEventKind::ImpactChildrenRefused;
    }
    void queue_package_children(const BattleMvpProjectile& shot,u8 projectile,BattlePoint aim,bool radial,ChildTrigger trigger){
        if(child_request_count_==child_requests_.size()){
            record_package(child_event(trigger,false),shot.owner,shot.package_move,0,shot.id,0);
            return;
        }
        child_requests_[child_request_count_++]={shot.position,aim,shot.id,shot.owner,shot.package_move,projectile,radial,trigger};
    }
    void flush_package_children(){
        // Visit only after the existing object pass: newly allocated slots
        // must not receive a same-frame update based on their pool index.
        for(unsigned i=0;i<child_request_count_;++i){
            const auto& request=child_requests_[i];
            const auto& attack=config_.packages[request.owner]->projectiles[request.child];
            const auto first=allocate_package_volley(request.owner,request.move,attack,request.position,request.aim,
                                                     255,0,nullptr,0,request.radial);
            record_package(child_event(request.trigger,first!=0),
                           request.owner,request.move,0,request.parent,first?attack.count:0);
        }
        child_request_count_=0;
    }
    static bool matches_impact_lane(PackageRetirement reason,BorgImpactLane lane){
        return (reason==PackageRetirement::Actor&&lane==BorgImpactLane::Actor)||
               (reason==PackageRetirement::Floor&&lane==BorgImpactLane::Floor)||
               (reason==PackageRetirement::Stage&&lane==BorgImpactLane::Stage);
    }
    template<class Spec,std::size_t N>
    void start_package_impact_pattern(const BattleMvpProjectile& shot,const Spec& spec,
        bool world_space,std::array<ImpactPattern<Spec>,N>& pool,BorgBattleEventKind started,BorgBattleEventKind refused){
        const auto slot=std::find_if(pool.begin(),pool.end(),[](const auto& pattern){return !pattern.parent;});
        const bool accepted=slot!=pool.end();
        if(accepted){
            const auto heading=native_borg_direction({0,0},{shot.velocity[0],shot.velocity[2]},actors_[shot.owner].facing);
            *slot={shot.position,heading,shot.id,tick_,shot.owner,shot.package_move,world_space,spec};
        }
        record_package(accepted?started:refused,shot.owner,shot.package_move,0,shot.id,accepted?spec.count:0);
    }
    template<class Spec,std::size_t N,class Place>
    void visit_package_impact_patterns(std::array<ImpactPattern<Spec>,N>& pool,Place place){
        for(auto& pattern:pool)if(pattern.parent){
            const u32 age=tick_-pattern.born;
            const auto& spec=pattern.spec;
            const u32 first=spec.first_delay?spec.first_delay:spec.interval;
            if(age<first||(age-first)%spec.interval)continue;
            const auto n=1+(age-first)/spec.interval;
            constexpr f32 radians=.01745329252f;
            const f32 angle=radians*(n*spec.degrees+360.f*((n-1)%spec.arms)/spec.arms);
            const f32 c=std::cos(angle),s=std::sin(angle),r=f32(spec.radius+n*spec.advance);
            const auto h=pattern.heading;
            auto point=pattern.centre;
            point[0]+=r*(pattern.world_space?s:h.x*c-h.z*s);
            point[2]+=r*(pattern.world_space?c:h.z*c+h.x*s);
            place(pattern,point);
            if(n==spec.count)pattern={};
        }
    }
    void step_package_impact_trails(){
        visit_package_impact_patterns(impact_trails_,[&](const auto& trail,const auto& point){
            impact(point,false,trail.spec.mark_size,trail.spec.mark_ticks);
        });
    }
    void step_package_impact_hazards(){
        // The room has two opposing actors. A resolved contact (including
        // a guard) is terminal for this patch's only eligible recipient.
        for(auto& patch:impact_hazards_)if(patch.id){
            if(patch.warmup){--patch.warmup;continue;}
            const auto target=u8(1-patch.owner);
            const auto& actor=actors_[target];
            if(actor.health){
                const f32 x=patch.centre[0]-actor.position[0],z=patch.centre[2]-actor.position[2];
                const f32 inset=std::min(config_.actor_radius,config_.actor_height*.5f);
                const f32 actor_low=actor.position[1]+inset,actor_high=actor.position[1]+config_.actor_height-inset;
                const f32 y=std::max({0.f,patch.centre[1]-actor_high,actor_low-(patch.centre[1]+patch.height)});
                const f32 r=f32(patch.radius)+inset,squared=x*x+y*y+z*z;
                const bool overlaps=patch.height?squared<r*r:squared<=r*r;
                if(overlaps){
                    if(!try_package_guard(target,patch.owner,BorgBattleAttackKind::Blast,{x,z},patch.id))
                        accept_package_hit(patch.owner,target,patch.move,0,patch.id,patch.damage,patch.centre);
                    patch={};
                    continue;
                }
            }
            if(!--patch.remaining)patch={};
        }
        visit_package_impact_patterns(hazard_patterns_,[&](const auto& pattern,const auto& point){
            const auto slot=std::find_if(impact_hazards_.begin(),impact_hazards_.end(),[](const auto& patch){return !patch.id;});
            if(slot==impact_hazards_.end()||next_id_==std::numeric_limits<u32>::max()){
                record_package(BorgBattleEventKind::ImpactHazardRefused,pattern.owner,pattern.move,0,pattern.parent,0);
                return;
            }
            const auto& spec=pattern.spec;
            *slot={point,++next_id_,pattern.owner,pattern.move,spec.ticks,spec.hit_radius,spec.height,spec.damage,
                   u16(spec.first_query?spec.first_query-1:0)};
            record_package(BorgBattleEventKind::ImpactHazardPlaced,pattern.owner,pattern.move,0,slot->id,spec.ticks);
        });
    }
    void retire_package_shot(BattleMvpProjectile& shot,PackageRetirement reason){
        trigger_package_blast(shot,reason);
        const auto& profile=*config_.packages[shot.owner];
        const auto& definition=profile.projectiles[shot.package_projectile];
        if(const auto& effect=definition.impact_record;effect&&matches_impact_lane(reason,effect->lane))
            for(auto& slot:impact_records_)if(!slot.ticks){
                slot={shot.position,effect->ticks,shot.owner,shot.package_projectile};
                break;
            }
        if(const auto& sound=definition.impact_sound;sound&&matches_impact_lane(reason,sound->lane))
            record_package(BorgBattleEventKind::ImpactSound,shot.owner,shot.package_move,0,shot.id,shot.package_projectile);
        if(const auto& spec=definition.impact_children;spec&&matches_impact_lane(reason,spec->lane)){
            const auto aim=spec->radial?BattlePoint{0,1}:
                native_borg_direction({0,0},{shot.velocity[0],shot.velocity[2]},actors_[shot.owner].facing);
            queue_package_children(shot,spec->projectile,aim,spec->radial,ChildTrigger::Impact);
        }
        if(const auto& trail=definition.impact_trail;trail&&matches_impact_lane(reason,trail->lane))
            start_package_impact_pattern(shot,*trail,definition.impact_world_space,impact_trails_,BorgBattleEventKind::ImpactTrailStarted,
                                         BorgBattleEventKind::ImpactTrailRefused);
        if(const auto& hazard=definition.impact_hazard;hazard&&matches_impact_lane(reason,hazard->lane))
            start_package_impact_pattern(shot,*hazard,definition.impact_world_space,hazard_patterns_,BorgBattleEventKind::ImpactHazardStarted,
                                         BorgBattleEventKind::ImpactHazardRefused);
        shot.active=false;
        shot.package_rewind_path={};
        record_package(BorgBattleEventKind::Retired,shot.owner,shot.package_move,0,shot.id,u16(reason));
        const auto channel=std::exchange(shot.package_return_stock,u8(255));
        if(channel==255||!actors_[shot.owner].health)return;
        const auto disposition=definition.stock_return;
        if(!((disposition==BorgProjectileReturn::Actor&&reason==PackageRetirement::Actor)||
             (disposition==BorgProjectileReturn::Floor&&reason==PackageRetirement::Floor)||
             (disposition==BorgProjectileReturn::Expiry&&reason==PackageRetirement::Expired)||
             (disposition==BorgProjectileReturn::Stage&&reason==PackageRetirement::Stage)||
             (disposition==BorgProjectileReturn::ExpiryOrProximity&&(reason==PackageRetirement::Expired||reason==PackageRetirement::Proximity))||
             (disposition==BorgProjectileReturn::Remote&&reason==PackageRetirement::Remote)))return;
        auto& stock=package_actions_[shot.owner].stocks[channel];
        const auto previous=stock.current;
        if(weapon_stock_return_one(stock,s16(profile.stocks[channel].capacity),&stock)&&stock.current!=previous)
            record_package(BorgBattleEventKind::StockReturned,shot.owner,shot.package_move,0,shot.id,u16(stock.current));
    }
    static bool package_shot_supported(const BattleMvpProjectile& shot,const IBattleMvpTerrain* terrain){
        f32 support=0;
        return floor(terrain,shot.position[0],shot.position[2],shot.position[1],&support)&&std::abs(support-shot.position[1])<=.01f;
    }
    struct PackageChargeTarget {
        u8 slot=255;
        f32 distance_squared=std::numeric_limits<f32>::max();
        BattlePoint toward{};
    };
    PackageChargeTarget package_charge_target(const BattleMvpProjectile& shot)const{
        PackageChargeTarget result;
        const auto owner_team=actors_[shot.owner].team;
        for(u8 i=0;i<actor_count();++i){
            const auto& actor=actors_[i];
            if(!actor.health||actor.team==owner_team)continue;
            const f32 x=actor.position[0]-shot.position[0],y=actor.position[1]+config_.actor_height*.5f-shot.position[1],
                      z=actor.position[2]-shot.position[2];
            const f32 distance=x*x+y*y+z*z;
            if(distance<=result.distance_squared)result={i,distance,{x,z}};
        }
        return result;
    }
    void step_bomb_attraction(BattleMvpProjectile& shot,const BorgBombAttraction& spec,
                              f32 dx,f32 dz,const IBattleMvpTerrain* terrain){
        const f32 span=std::hypot(dx,dz);
        if(!(span>0)){shot.bomb_travel_speed=0;return;}
        const auto speed=std::min(spec.cap,shot.bomb_travel_speed+spec.acceleration);
        const auto fraction=std::min(speed/span,1.f);
        auto end=shot.position;
        end[0]+=dx*fraction;end[2]+=dz*fraction;
        // Bounded centreline above the support plane. The collision radius
        // remains a separate gameplay operand, not a swept wall radius.
        auto from=shot.position,to=end;from[1]+=1;to[1]+=1;
        NativeBorgStageProjectileHit wall;
        f32 middle=0,support=0;
        if(std::abs(end[0])>config_.arena_half||std::abs(end[2])>config_.arena_half||
           !terrain||!terrain->supports_projectile_stage()||!terrain->projectile_stage(from,to,&wall)||wall.hit||
           !floor(terrain,(end[0]+shot.position[0])*.5f,(end[2]+shot.position[2])*.5f,shot.position[1],&middle)||
           !floor(terrain,end[0],end[2],shot.position[1],&support)||
           std::abs(middle-shot.position[1])>.01f||std::abs(support-shot.position[1])>.01f){
            shot.bomb_travel_speed=0;return;
        }
        shot.position=end;shot.bomb_travel_speed=speed;
    }
    static bool matches_package_detonation(const BattleMvpProjectile& shot,unsigned owner,
                                           const BorgBattleDetonation& marker,const IBattleMvpTerrain* terrain){
        return shot.active&&shot.from_package&&shot.owner==owner&&shot.package_projectile==marker.projectile&&
               shot.package_blast&&shot.package_blast->trigger==BorgBlastTrigger::Remote&&
               (!marker.landed_only||shot.landed)&&(!shot.landed||package_shot_supported(shot,terrain));
    }
    bool consume_package_remote(BattleMvpProjectile& shot,const IBattleMvpTerrain* terrain){
        auto& pending=remote_requests_[shot.owner];
        if(!pending.active||!matches_package_detonation(shot,shot.owner,pending.marker,terrain))return false;
        const auto request=std::exchange(pending,{});
        record_package(BorgBattleEventKind::Detonation,shot.owner,request.move,request.marker.frame,shot.id,1);
        retire_package_shot(shot,PackageRetirement::Remote);
        return true;
    }
    void step_package_detonation(unsigned owner,const IBattleMvpTerrain* terrain){
        const auto& action=package_actions_[owner];
        if(!actors_[owner].health||action.move==BorgBattleActionState::none)return;
        const auto& marker=config_.packages[owner]->moves[action.move].detonation;
        if(!marker||marker->frame!=action.frame)return;
        if(marker->first_eligible){remote_requests_[owner]={true,*marker,action.move};return;}
        const auto matches=[&](const BattleMvpProjectile& shot){
            return matches_package_detonation(shot,owner,*marker,terrain);
        };
        const auto count=std::count_if(projectiles_.begin(),projectiles_.end(),matches);
        record_package(BorgBattleEventKind::Detonation,owner,action.move,action.frame,0,u16(count));
        // Accepted set belongs to this marker; a blast may defeat its owner
        // without cancelling the other charges already selected by the same marker.
        for(auto& shot:projectiles_)if(matches(shot))retire_package_shot(shot,PackageRetirement::Remote);
    }
    // Shared native segment versus finite padded body cylinder. Intersect
    // the vertical interval before probing the horizontal sweep.
    bool package_segment_contact(const BattleMvpProjectile& shot,const BattleMvpVec& step,u8 recipient,f32* entry)const{
        const auto& actor=actors_[recipient];
        if(!actor.health||shot.package_ignore_actors)return false;
        f32 t=0,leave=1;
        const f32 low=actor.position[1]-shot.package_radius,high=actor.position[1]+config_.actor_height+shot.package_radius;
        if(step[1]!=0){
            const auto a=(low-shot.position[1])/step[1],b=(high-shot.position[1])/step[1];
            t=std::max(0.f,std::min(a,b));leave=std::min(1.f,std::max(a,b));
            if(t>leave)return false;
        }else if(shot.position[1]<low||shot.position[1]>high)return false;
        f32 fraction=0;
        const BattlePoint start{shot.position[0]+step[0]*t,shot.position[2]+step[2]*t};
        const BattlePoint delta{step[0]*(leave-t),step[2]*(leave-t)};
        if(!battle_swept_hit(start,delta,{actor.position[0],actor.position[2]},config_.actor_radius+shot.package_radius,&fraction))
            return false;
        *entry=std::lerp(t,leave,fraction);return true;
    }
    static bool package_stage_crossing(const BattleMvpProjectile& shot,const BattleMvpVec& delta,const IBattleMvpTerrain* terrain,NativeBorgStageProjectileHit* out){
        BattleMvpVec end=shot.position;for(unsigned k=0;k<3;++k)end[k]+=delta[k];
        return terrain&&terrain->supports_projectile_stage()&&terrain->projectile_stage(shot.position,end,out)&&
            (!out->hit||(std::isfinite(out->fraction)&&out->fraction>=0&&out->fraction<=1&&out->layer&&out->layer<=3));
    }
    void step_sustained_beam(BattleMvpProjectile& shot,f32 steering,const IBattleMvpTerrain* terrain){
        const auto& owner=actors_[shot.owner];
        if(!owner.health||owner.package_move!=shot.package_move||owner.hit_flash){
            retire_package_shot(shot,PackageRetirement::OwnerEnded);return;
        }
        const auto& definition=config_.packages[shot.owner]->projectiles[shot.package_projectile];
        const auto& spec=*definition.sustained;
        const u32 age=tick_-shot.born_tick;
        if(age>=shot.life){retire_package_shot(shot,PackageRetirement::Expired);return;}
        if(age&&steering&&spec.turn){
            const f32 a=steering*spec.turn,c=std::cos(a),s=std::sin(a),x=shot.velocity[0],z=shot.velocity[2];
            shot.velocity[0]=x*c+z*s;shot.velocity[2]=z*c-x*s;
        }
        if(const auto& sweep=definition.beam_sweep){
            const auto advance=f32(sweep->travel)*(f32(age)/(shot.life-1))/std::hypot(shot.velocity[0],shot.velocity[2]);
            shot.position=shot.beam_origin;
            shot.position[0]+=shot.velocity[0]*advance;shot.position[2]+=shot.velocity[2]*advance;
        }else for(unsigned k=0;k<3;++k)shot.position[k]=owner.position[k]+shot.beam_origin[k];
        const f32 reach=package_slow_fraction(shot);
        shot.beam_fraction=reach;
        NativeBorgStageProjectileHit obstruction;
        if(shot.package_floor==BorgProjectileFloor::StageClip){
            auto segment=shot.velocity;for(auto& axis:segment)axis*=reach;
            if(!package_stage_crossing(shot,segment,terrain,&obstruction)){
                retire_package_shot(shot,PackageRetirement::UnknownStage);return;
            }
            if(obstruction.hit)shot.beam_fraction*=obstruction.fraction;
        }
        f64 power_entry=1;
        const auto* field=first_package_power_field(shot,shot.velocity,shot.beam_fraction,true,&power_entry);
        shot.beam_power_entry=f32(power_entry);
        shot.beam_power_visual=field?field_effect(*field,BorgFieldEffectKind::ShotSize,shot.owner):100;
        if(field&&shot.power_field!=field->id)
            record_package(BorgBattleEventKind::ShotPowered,shot.owner,shot.package_move,owner.package_frame,
                           shot.id,powered_damage(shot,*field));
        shot.power_field=field?field->id:0;
        if(age%spec.interval||actor_target(shot.owner)==255)return;
        if(definition.beam_sweep&&obstruction.hit){
            BattleMvpVec point=shot.position;
            for(unsigned k=0;k<3;++k)point[k]+=shot.velocity[k]*shot.beam_fraction;
            impact(point,true,definition.beam_sweep->impact_radius);
            record_package(BorgBattleEventKind::StageImpact,shot.owner,shot.package_move,owner.package_frame,shot.id,obstruction.layer);
        }
        const bool roster=actor_count()>2&&shot.package_direct_release;
        const auto contact=package_body_contact(shot.owner,roster,[&](u8 candidate,f32* entry){
            return package_segment_contact(shot,shot.velocity,candidate,entry)&&*entry<=shot.beam_fraction&&
                !(obstruction.hit&&*entry==shot.beam_fraction);
        });
        if(!contact)return;
        const auto recipient=contact.recipient;
        const auto& target=actors_[recipient];
        const auto recovery=config_.packages[recipient]->beam_recovery;
        // Protection belongs to the contacted body. A recovering or
        // blocking body consumes this pulse rather than exposing the next.
        if(recovery&&target.package_damage_serial&&tick_-target.package_last_hit_tick<=recovery)return;
        if(try_package_guard(recipient,shot.owner,BorgBattleAttackKind::Shot,{-shot.velocity[0],-shot.velocity[2]},shot.id))return;
        const f32 entry=contact.entry;
        BattleMvpVec point=shot.position;
        for(unsigned k=0;k<3;++k)point[k]+=entry*shot.velocity[k];
        const auto damage=field&&power_entry<entry?powered_damage(shot,*field):shot.package_damage;
        accept_package_hit(shot.owner,recipient,shot.package_move,owner.package_frame,shot.id,damage,point);
    }
    void step_package_shot(BattleMvpProjectile& shot,const IBattleMvpTerrain* terrain){
        const auto& definition=config_.packages[shot.owner]->projectiles[shot.package_projectile];
        if(shot.landed){
            if(!package_shot_supported(shot,terrain))
                retire_package_shot(shot,PackageRetirement::UnknownFloor);
            else if(shot.bomb_fuse_tail_left){
                if(!--shot.bomb_fuse_tail_left)retire_package_shot(shot,PackageRetirement::Expired);
            }
            else{
                bool attraction_step=false;
                if(definition.proximity&&tick_-shot.born_tick>=definition.proximity->arming){
                    // The armed device senses current bodies from its own
                    // origin. This is independent of aim and damage masks.
                    const auto target=package_charge_target(shot);
                    if(target.slot!=255&&target.distance_squared<=f32(definition.proximity->radius)*definition.proximity->radius){
                        record_package(BorgBattleEventKind::BombProximity,shot.owner,shot.package_move,0,shot.id,
                                       definition.proximity->radius,target.slot);
                        retire_package_shot(shot,PackageRetirement::Proximity);return;
                    }
                    if(target.slot!=255&&definition.attraction&&shot.life>1&&
                       target.distance_squared<=f32(definition.attraction->radius)*definition.attraction->radius){
                        step_bomb_attraction(shot,*definition.attraction,target.toward.x,target.toward.z,terrain);
                        attraction_step=true;
                    }
                }
                if(!attraction_step)shot.bomb_travel_speed=0;
                if(!--shot.life)expire_package_fuse(shot,definition);
            }
            return;
        }
        break_package_guidance_in_fields(shot,{},1,false);
        step_package_guidance(shot);
        pull_package_shot(shot);
        const f32 travel=package_slow_fraction(shot);
        const BattleMvpVec step{shot.velocity[0]*travel,shot.velocity[1]*travel,shot.velocity[2]*travel};
        const auto& profile=*config_.packages[shot.owner];
        const bool continuing=definition.continue_after_actor_hit&&shot.package_direct_release;
        const bool roster=actor_count()>2&&shot.package_direct_release&&shot.package_damage&&
            shot.package_move<profile.moves.size()&&borg_battle_direct_releases(profile,profile.moves[shot.package_move]);
        const auto contact=package_body_contact(shot.owner,roster||continuing,[&](u8 candidate,f32* entry){
            if(continuing&&(shot.package_hit_recipients&(1u<<candidate)))return false;
            return package_segment_contact(shot,step,candidate,entry);
        });
        const u8 recipient=contact.recipient;
        f32 t=contact.entry;
        bool hit=bool(contact);
        f32 endpoint_y=shot.position[1]+step[1],surface=0;
        auto retire=PackageRetirement::None;
        u8 stage_layer=0;
        if(shot.package_floor==BorgProjectileFloor::StageRetire){
            NativeBorgStageProjectileHit surface_hit;
            if(!package_stage_crossing(shot,step,terrain,&surface_hit)){
                retire_package_shot(shot,PackageRetirement::UnknownStage);
                return;
            }
            if(surface_hit.hit&&(!hit||surface_hit.fraction<=t)){
                t=surface_hit.fraction;hit=false;retire=PackageRetirement::Stage;stage_layer=surface_hit.layer;
            }
        }else if(shot.package_floor!=BorgProjectileFloor::Ignore){
            if(!floor(terrain,shot.position[0]+step[0],shot.position[2]+step[2],shot.position[1],&surface)){
                retire=PackageRetirement::UnknownFloor;hit=false; // unknown support; no fabricated floor
            }else if(endpoint_y<=surface){
                const f32 floor_t=step[1]<0?std::clamp((surface-shot.position[1])/step[1],0.f,1.f):0.f;
                if(!hit||t>=floor_t){t=floor_t;retire=PackageRetirement::Floor;hit=false;}
            }
        }
        // A guard resolves only the accepted body contact. A planar miss
        // or an earlier (including tied) surface can't spend guard budget.
        if(hit&&try_package_guard(recipient,shot.owner,BorgBattleAttackKind::Shot,{-shot.velocity[0],-shot.velocity[2]},shot.id)){
            for(unsigned k=0;k<3;++k)shot.position[k]+=step[k]*t;
            impact(shot.position,false);
            retire_package_shot(shot,PackageRetirement::Guard);
            return;
        }
        if(retire!=PackageRetirement::UnknownFloor){
            const bool terminal=hit||retire!=PackageRetirement::None;
            const f32 limit=terminal?t:1.f;
            power_package_shot(shot,step,limit,terminal);
            break_package_guidance_in_fields(shot,step,limit,terminal);
            for(unsigned k=0;k<3;++k)shot.position[k]+=step[k]*limit;
        }
        shot.velocity[1]-=shot.package_gravity;
        if(hit){
            const auto& actor=actors_[recipient];
            if(shot.package_damage){
                // The shot keeps its original round slot and move. A
                // qualified link samples that living owner's body now, not
                // the launch pose, velocity, current move, or target.
                NativeBattleReactionContact reaction;
                if(shot.package_direct_release){
                    const auto& owner=actors_[shot.owner];
                    const auto& moves=profile.moves;
                    if(owner.health&&shot.package_move<moves.size()&&moves[shot.package_move].reaction_linked_owner)
                        reaction.body_forward=owner.facing;
                    // The object stops at the native contact fraction. The
                    // pair's two submitted centres span the full tested step.
                    auto& motion=reaction.motion.emplace();
                    for(unsigned k=0;k<3;++k)motion.object_delta[k]=shot.position[k]-shot.previous[k];
                    motion.pair_delta=step;
                    motion.object_vector=shot.velocity;
                    reaction.object_origin=shot.position;
                    reaction.centres=NativeBattleReactionCentres{shot.position,
                        {actor.position[0],actor.position[1]+config_.actor_height*.5f,actor.position[2]}};
                }
                accept_package_hit(shot.owner,recipient,shot.package_move,0,shot.id,
                    shot.package_damage,shot.position,reaction,
                    shot.package_direct_release?DamageRoleScope::DirectMove:DamageRoleScope::Unbound,
                    shot.package_base_damage);
            }
            else record_package(BorgBattleEventKind::ProjectileContact,shot.owner,shot.package_move,0,shot.id,0,recipient);
            if(shot.package_walk_ticks){
                const bool apply=actor.health&&config_.packages[recipient]->movement==BorgBattleMovement::Grounded;
                if(apply)walk_modifiers_[recipient]={shot.package_walk_ticks,shot.package_walk_percent,shot.owner,shot.package_move,shot.id};
                record_package(apply?BorgBattleEventKind::ModifierApplied:BorgBattleEventKind::ModifierRefused,
                    shot.owner,shot.package_move,0,shot.id,shot.package_walk_percent,recipient);
            }
            if(shot.package_pull_units)queue_package_pull(shot,recipient,actor.position);
            if(shot.package_rewind_path.count)queue_package_recipient_rewind(shot,recipient);
            // Resume on the next visit from the accepted contact point,
            // not from the untested remainder of this step.
            if(continuing)shot.package_hit_recipients|=u8(1u<<recipient);
            else retire=PackageRetirement::Actor;
        }else if(retire==PackageRetirement::Stage){
            record_package(BorgBattleEventKind::StageImpact,shot.owner,shot.package_move,0,shot.id,stage_layer);
            plant_package_anchor(shot);
            impact(shot.position,false);
        }else if(retire==PackageRetirement::Floor){
            if(shot.package_floor==BorgProjectileFloor::Land){
                f32 support=0;
                if(!floor(terrain,shot.position[0],shot.position[2],shot.previous[1],&support)||std::abs(support-surface)>.01f)
                    retire=PackageRetirement::UnknownFloor;
                else{
                    shot.position[1]=support;shot.velocity={};shot.landed=true;retire=PackageRetirement::None;
                    record_package(BorgBattleEventKind::ProjectileLanded,shot.owner,shot.package_move,0,shot.id,0);
                }
            }else impact(shot.position,false);
        }
        if(retire==PackageRetirement::None){
            if(std::abs(shot.position[0])>config_.arena_half+300||std::abs(shot.position[2])>config_.arena_half+300||shot.position[1]<-10000)
                retire=PackageRetirement::Outside;
            else if(!--shot.life){
                expire_package_fuse(shot,definition);return;
            }
        }
        if(retire!=PackageRetirement::None)retire_package_shot(shot,retire);
        else if(!shot.landed){
            if(const auto& ramp=definition.speed_ramp;ramp&&!shot.package_ramp_deflected){
                // Use the current speed for the entire tested step. A
                // surviving shot gains speed only for its next visit.
                const f32 speed=std::hypot(shot.velocity[0],shot.velocity[2]);
                if(speed>0&&speed<ramp->cap){
                    const f32 scale=std::min(ramp->cap,speed+ramp->gain)/speed;
                    for(auto& v:shot.velocity)v*=scale;
                }
            }
            if(definition.flight_children){
                const auto& spec=*definition.flight_children;
                const u32 age=tick_-shot.born_tick;
                if(age>=spec.first&&(age-spec.first)%spec.interval==0&&(age-spec.first)/spec.interval<spec.count){
                    const auto aim=native_borg_direction({0,0},{shot.velocity[0],shot.velocity[2]},actors_[shot.owner].facing);
                    queue_package_children(shot,spec.projectile,aim,false,ChildTrigger::Flight);
                }
            }
        }
    }
    void expire_package_fuse(BattleMvpProjectile& shot,const BorgBattleProjectileDefinition& definition){
        if(!definition.fuse_tail){retire_package_shot(shot,PackageRetirement::Expired);return;}
        if(!shot.landed){retire_package_shot(shot,PackageRetirement::UnlandedFuse);return;}
        shot.bomb_fuse_tail_left=definition.fuse_tail;shot.bomb_travel_speed=0;
        record_package(BorgBattleEventKind::BombFuseCommitted,shot.owner,shot.package_move,0,shot.id,definition.fuse_tail);
    }
    void step_package_guidance(BattleMvpProjectile& shot){
        if(!shot.package_guide_ticks)return;
        const auto& spec=*config_.packages[shot.owner]->projectiles[shot.package_projectile].guidance;
        const auto recipient=shot.package_guide_target;
        const auto& target=actors_[recipient];
        // Slots are stable within this two-actor round; restart discards the
        // entire projectile pool. This is an aim observation, not a hit.
        if(spec.target==BorgGuidanceTarget::Tracking&&!target.health){
            shot.package_guide_ticks=0;
            record_package(BorgBattleEventKind::GuidanceEnded,shot.owner,shot.package_move,0,shot.id,2,recipient);
            return;
        }
        if(shot.package_guide_ticks==spec.ticks)
            record_package(BorgBattleEventKind::GuidanceBound,shot.owner,shot.package_move,0,shot.id,unsigned(spec.target),recipient);
        const BattlePoint goal=spec.target==BorgGuidanceTarget::Tracking?
            BattlePoint{target.position[0],target.position[2]}:shot.package_guide_point;
        const auto toward=native_borg_direction({shot.position[0],shot.position[2]},goal,{0,0});
        if(toward!=BattlePoint{}){
            const auto direction=native_borg_turn({shot.velocity[0],shot.velocity[2]},toward,spec.turn);
            const auto speed=std::hypot(shot.velocity[0],shot.velocity[2]);
            shot.velocity[0]=direction.x*speed;shot.velocity[2]=direction.z*speed;
        }
        if(!--shot.package_guide_ticks)
            record_package(BorgBattleEventKind::GuidanceEnded,shot.owner,shot.package_move,0,shot.id,1,recipient);
    }
    void queue_package_recipient_rewind(const BattleMvpProjectile& shot,u8 recipient){
        const auto& actor=actors_[recipient];auto& current=rewinds_[recipient];
        if(!actor.health||current.id||config_.packages[recipient]->movement!=BorgBattleMovement::Grounded){
            record_package(BorgBattleEventKind::RewindRefused,shot.owner,shot.package_move,0,shot.id,
                u16(BorgBattlePullRefusal::OwnerBusy),recipient);
            return;
        }
        const auto& policy=*config_.packages[shot.owner]->projectiles[shot.package_projectile].rewind_hit;
        current={shot.id,shot.package_rewind_path,policy.speed,shot.package_move,shot.owner,actor.package_damage_serial};
        record_package(BorgBattleEventKind::RewindQueued,shot.owner,shot.package_move,0,shot.id,current.path.count,recipient);
    }
    static f32 axis(s32 value){return native_borg_stick_axis(value);}
    static void unit(f32& x,f32& z,bool always){const f32 length=std::sqrt(x*x+z*z);if(length>0&&(always||length>1)){x/=length;z/=length;}}
    static bool floor(const IBattleMvpTerrain* terrain,f32 x,f32 z,f32 current,f32* out){if(!terrain){*out=0;return true;}return terrain->height(x,z,current,out)&&std::isfinite(*out)&&std::abs(*out)<=20000;}
    u8 actor_target(unsigned owner)const{
        const auto& self=actors_[owner];u8 best=255;f32 best_squared=std::numeric_limits<f32>::max();
        for(unsigned i=0;i<target_count();++i){
            if(i==owner||!actors_[i].health||actors_[i].team==self.team)continue;
            const f32 dx=actors_[i].position[0]-self.position[0],dz=actors_[i].position[2]-self.position[2];
            const f32 squared=dx*dx+dz*dz;
            if(squared<=best_squared){best_squared=squared;best=u8(i);}
        }
        return best;
    }
    bool aim_eligible(unsigned owner,u8 candidate)const{
        return candidate<actor_count()&&actors_[candidate].health&&actors_[candidate].team!=actors_[owner].team;
    }
    u8 package_aim_target(unsigned owner)const{
        const unsigned locals=config_.roster_local_players;
        if(config_.local_aim_cycle&&owner<locals&&aim_eligible(owner,local_aim_[owner]))return local_aim_[owner];
        if(owner>=locals&&owner-locals<cpu_aim_.size()&&aim_eligible(owner,cpu_aim_[owner-locals]))
            return cpu_aim_[owner-locals];
        return actor_target(owner);
    }
    void cycle_package_aim(unsigned owner){
        const u8 current=package_aim_target(owner);
        if(current==255)return;
        for(unsigned n=1;n<=actor_count();++n){
            const u8 candidate=u8((current+n)%actor_count());
            if(aim_eligible(owner,candidate)){local_aim_[owner]=candidate;return;}
        }
    }
    void face(unsigned i){
        const u8 target=actor_target(i);if(target==255){actors_[i].facing={0,1};return;}
        f32 x=actors_[target].position[0]-actors_[i].position[0],z=actors_[target].position[2]-actors_[i].position[2];
        unit(x,z,true);if(x||z)actors_[i].facing={x,z};
    }
    GamePadState ai_input()const{return ai_input(1);}
    GamePadState ai_input(unsigned actor)const{
        GamePadState pad;if(actor>=actor_count()||!actors_[actor].health)return pad;
        const u8 target=actor_target(actor);if(target==255)return pad;
        f32 x=actors_[target].position[0]-actors_[actor].position[0],z=actors_[target].position[2]-actors_[actor].position[2];const f32 distance=std::sqrt(x*x+z*z);unit(x,z,true);
        const f32 side=(tick_/120)%2?1.f:-1.f;
        const f32 approach=distance>700?1.f:distance<300?-.7f:0.f;
        f32 mx=x*approach-z*side*.5f,mz=z*approach+x*side*.5f;unit(mx,mz,false);
        pad.stick_x=s8(std::clamp(int(mx*127),-127,127));pad.stick_y=s8(std::clamp(int(mz*127),-127,127));pad.a=true;pad.b=tick_%240==100;return pad;
    }
    void fire(unsigned owner,bool secondary,const std::optional<BattleMvpVec>* muzzle_override){
        const u8 target_slot=actor_target(owner);if(target_slot==255)return;
        const auto weapon=owner?(secondary?BattleMvpWeapon::Bullet:BattleMvpWeapon::Shuriken):(secondary?BattleMvpWeapon::Bomb:BattleMvpWeapon::EnergyBeam);
        const unsigned count=weapon==BattleMvpWeapon::Shuriken?3:1;unsigned free=0;for(const auto& shot:projectiles_)free+=!shot.active;
        if(free<count||next_id_>std::numeric_limits<u32>::max()-count)return;
        auto& actor=actors_[owner];const auto aim=actor.aim_facing;BattleMvpVec muzzle{actor.position[0]+aim.x*(config_.actor_radius+12),actor.position[1]+config_.actor_height*.6f,actor.position[2]+aim.z*(config_.actor_radius+12)};
        if(muzzle_override)muzzle=**muzzle_override;
        const auto& target=actors_[target_slot];const f32 dx=target.position[0]-muzzle[0],dy=target.position[1]+config_.actor_height*.6f-muzzle[1],dz=target.position[2]-muzzle[2],horizontal=std::sqrt(dx*dx+dz*dz);
        const f32 speed=weapon==BattleMvpWeapon::EnergyBeam?60:weapon==BattleMvpWeapon::Bullet?34:weapon==BattleMvpWeapon::Shuriken?24:10;
        const f32 pitch=weapon==BattleMvpWeapon::Bomb?10:std::clamp(speed*dy/std::max(horizontal,1.f),-speed,speed);
        for(unsigned n=0;n<count;++n){const f32 spread=count==3?(f32(n)-1)*.11f:0;const f32 c=std::cos(spread),s=std::sin(spread);
            for(auto& shot:projectiles_)if(!shot.active){shot={true,++next_id_,u8(owner),weapon,muzzle,muzzle,{speed*(aim.x*c-aim.z*s),pitch,speed*(aim.z*c+aim.x*s)},weapon==BattleMvpWeapon::Bomb?70u:120u,false};++shots_;break;}}
        actor.cooldown[secondary?1:0]=weapon==BattleMvpWeapon::Bomb?120:weapon==BattleMvpWeapon::EnergyBeam?18:24;actor.attack_ticks=10;
    }
    const BorgBattleMove* reaction_definition(unsigned attacker,u8 move,bool body_contact)const{
        const auto& package=config_.packages[attacker];
        if(!package)return nullptr;
        // Unlabelled native field/wave hits may inherit an active move, but
        // never a scoped body-root mode or its offsets without its basis.
        if(move==BorgBattleActionState::none)move=actors_[attacker].package_move;
        if(move>=package->moves.size())return nullptr;
        const auto& spec=package->moves[move];
        return spec.reaction_mode==kBorgReactionBodyRoot&&!body_contact?nullptr:&spec;
    }
    void damage(unsigned target,unsigned amount,unsigned attacker,u8 move=BorgBattleActionState::none,
                const NativeBattleReactionContact& contact={}){
        auto& actor=actors_[target];
        actor.health-=std::min(actor.health,amount);actor.hit_flash=8;
        if(!actor.health){actor.damage_tier={};actor.contact_pose_hold_ticks=0;}
        actor.package_damage_serial=++hits_;actor.package_last_hit_tick=tick_;
        // Mode 0 is a real mode, not a sentinel for missing or self credit.
        if(attacker>=actor_count()||attacker==target)return;
        const auto& source=actors_[attacker];
        const auto* spec=reaction_definition(attacker,move,contact.body_forward.has_value());
        const u8 mode=spec?spec->reaction_mode:u8(NativeBattleReactionMode::AttackerToReceiver);
        // The undeclared authored default stays caster-to-receiver. An
        // explicit mode 1 may opt into the colliding carrier's actual origin.
        const auto* origin=mode==u8(NativeBattleReactionMode::AttackerToReceiver)&&
            spec&&spec->reaction_mode_declared&&contact.object_origin?
            contact.object_origin->data():source.position.data();
        f32 direction[3];
        if(!native_battle_reaction_direction(mode,origin,
            source.velocity,actor.position.data(),direction,contact))return;
        auto reaction=native_battle_reaction_angles(direction,actor.facing);
        if(spec){
            reaction.yaw=native_battle_reaction_wrap(
                reaction.yaw-f32(spec->reaction_yaw_offset)*kNativeBattleReactionUnit);
            reaction.pitch=native_battle_reaction_wrap(
                reaction.pitch-f32(spec->reaction_pitch_offset)*kNativeBattleReactionUnit);
        }
        actor.reaction_yaw=reaction.yaw;actor.reaction_pitch=reaction.pitch;
        actor.reaction_ticks=BattleMvpActor::reaction_duration;
    }
    // The native hit ring and contact records share a four-tick visual gate
    // analogous to receiver+0x2AF, separate from contact audio. Explosions
    // and terrain marks do not use it.
    void hit_impact(unsigned target,const BattleMvpVec& position,
                    unsigned owner=kBattleMvpMaxActors,u8 move=BorgBattleActionState::none){
        auto& actor=actors_[target];
        if(actor.impact_state)return;
        actor.impact_state=4;
        if(owner<actor_count()&&actor.damage_kind==0&&config_.packages[target]&&
           config_.packages[target]->contact_impact_receive){
            const auto& profile=config_.packages[owner];
            if(profile&&move<profile->moves.size()){
                const auto team=actors_[owner].team;
                const auto& layers=profile->moves[move].contact_impacts[team];
                const auto free=std::count_if(impact_records_.begin(),impact_records_.end(),[](const auto& r){return !r.ticks;});
                if(!layers.empty()&&layers.size()<=std::size_t(free)){
                    unsigned layer=0;
                    for(auto& slot:impact_records_)if(!slot.ticks){
                        slot={position,layers[layer].ticks,u8(owner),borg_contact_impact_slot(move,team,layer)};
                        if(++layer==layers.size())return;
                    }
                }
            }
        }
        impact(position,false);
    }
    void impact(const BattleMvpVec& position,bool explosion,f32 radius=0,unsigned ticks=18){
        impacts_[impact_cursor_]={position,ticks,explosion,radius,ticks};impact_cursor_=(impact_cursor_+1)%impacts_.size();
    }
    BattleMvpConfig config_;std::array<BattleMvpActor,kBattleMvpMaxActors> actors_{};std::array<BattleMvpProjectile,max_projectiles> projectiles_{};
    std::array<u8,2> local_aim_{255,255};
    std::array<u8,kBattleMvpMaxActors-1> cpu_aim_{255,255,255,255,255};
    std::array<unsigned,2> team_force_{};std::array<u32,2> team_tally_{};
    std::array<u8,2> force_counter10a_{};bool force_global1f_=false;u8 last_attacker_=255,last_receiver_=255;
    std::array<BattleMvpImpact,16> impacts_{};std::array<GamePadState,kBattleMvpMaxActors> previous_{};unsigned impact_cursor_=0;u32 tick_=0,next_id_=0,shots_=0,hits_=0;
    std::array<BattleMvpImpactRecord,16> impact_records_{};
    struct ImpactHazard {BattleMvpVec centre;u32 id=0;u8 owner=0,move=0;u16 remaining=0,radius=0,height=0,damage=0,warmup=0;};
    std::array<ImpactPattern<BorgImpactTrail>,8> impact_trails_{};
    std::array<ImpactPattern<BorgImpactHazard>,8> hazard_patterns_{};
    std::array<ImpactHazard,32> impact_hazards_{};
    std::array<BattleMvpWalkPulse,16> walk_pulses_{};
    std::array<f32,kBattleMvpMaxActors> walk_pulse_distance_{};
    BattleMvpResult result_=BattleMvpResult::Fighting;bool ready_=false,paused_=false;
    std::shared_ptr<const NativeBorgMotionScope> motion_scope_;
    std::array<NativeBorgMotionOffer,kBattleMvpMaxActors> last_offers_{};
    std::array<NativeBorgMotionReceipt,kBattleMvpMaxActors> last_receipts_{};
    std::array<std::weak_ptr<const NativeBorgMotionProducer>,kBattleMvpMaxActors> motion_producers_{};
    std::array<BorgBattleActionState,kBattleMvpMaxActors> package_actions_{};
    struct WalkModifier {u16 ticks=0,percent=0;u8 owner=0,move=255;u32 object=0;};
    std::array<WalkModifier,kBattleMvpMaxActors> walk_modifiers_{};
    struct PendingPull {BattlePoint destination;u32 object=0;u16 units=0;u8 move=255,recipient=255;};
    std::array<PendingPull,kBattleMvpMaxActors> pull_requests_{};
    struct MeleeWindow {u32 object=0;u8 move=255;u16 frame=0;bool hit=false;};
    std::array<MeleeWindow,kBattleMvpMaxActors> melee_windows_{};
    struct GuardWindow {u8 move=BorgBattleActionState::none,remaining=0;u16 frame=0;};
    std::array<GuardWindow,kBattleMvpMaxActors> guard_windows_{};
    std::array<std::array<BattleMvpEmitter,kBorgBattleEmitters>,kBattleMvpMaxActors> emitters_{};
    struct Rewind {u32 id=0;BattleMvpAnchorPath path;u8 speed=0,move=255,issuer=255;u32 damage_serial=0;};
    std::array<Rewind,kBattleMvpMaxActors> rewinds_{};
    std::array<std::array<BattleMvpAnchorPath,kBorgBattleAnchorSets>,kBattleMvpMaxActors> anchor_paths_{};
    struct PairReturn {bool pending=false;std::array<u32,2> ids{};u8 move=255;};
    std::array<std::array<PairReturn,kBorgBattleStocks>,kBattleMvpMaxActors> pair_returns_{};
    struct RemoteRequest {bool active=false;BorgBattleDetonation marker;u8 move=255;};
    std::array<RemoteRequest,kBattleMvpMaxActors> remote_requests_{};
    std::array<std::array<std::array<bool,kBattleMvpMaxActors>,kBorgBattleAnchorSets>,kBattleMvpMaxActors> fence_inside_{};
    std::array<BattleMvpFenceLink,kBattleMvpMaxActors*kBorgBattleAnchorSets*(kBorgBattleAnchors-1)> fence_links_{};
    unsigned fence_link_count_=0;
    struct FenceRecoil {BattlePoint step;u32 object=0;u8 owner=0;};
    std::array<FenceRecoil,kBattleMvpMaxActors> fence_recoil_{};
    std::array<BorgBattleEvent,max_projectiles*9+64> package_events_{};std::size_t package_event_count_=0;
    struct ChildRequest {BattleMvpVec position;BattlePoint aim;u32 parent;u8 owner,move,child;bool radial;ChildTrigger trigger;};
    // One carrier may contribute both terminal trigger families in one visit.
    std::array<ChildRequest,2*max_projectiles> child_requests_{};
    unsigned child_request_count_=0;
    std::array<BattleMvpSmoke,8> smoke_clouds_{};
    std::array<BattleMvpInk,kBattleMvpMaxActors> ink_{};
    std::array<BattleMvpAccuracy,kBattleMvpMaxActors> accuracy_{};
    std::array<BattleMvpField,kBattleMvpFields> fields_{};
};
}
