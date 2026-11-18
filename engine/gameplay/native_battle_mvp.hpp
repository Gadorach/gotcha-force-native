#pragma once
//Explicit native MVP rules, NOT source battle-callback authority. Two initialized
//gameplay actors own movement, health and attacks. Presentation binds their normal
//models through the shared animation/renderer systems; prepared art is not an actor.
#include <algorithm>
#include <array>
#include <limits>
#include <optional>
#include <memory>
#include "engine/input/input_map.hpp"
#include "engine/gameplay/battle_swept_circle.hpp"
#include "engine/gameplay/native_borg_movement.hpp"
#include "engine/gameplay/native_borg_contact.hpp"
#include "engine/core/source_surface_push.hpp"
namespace gf {
using BattleMvpVec=std::array<f32,3>;
enum class BattleMvpAction:u8 {Idle,Move,Jump,Attack,Defeated,Land,GroundDash,AirDash};
enum class BattleMvpDashDirection:u8 {Forward,Backward,Left,Right};
enum class BattleMvpWeapon:u8 {EnergyBeam,Bullet,Shuriken,Bomb};
enum class BattleMvpResult:u8 {Fighting,GRedWins,SasukeWins,Draw};
struct BattleMvpActor {
    static constexpr unsigned takeoff_duration=6,landing_duration=6; //native MVP phase windows
    u16 model=0;BattleMvpVec position{};BattlePoint facing{0,1};f32 vertical_speed=0;
    unsigned health=100;bool grounded=true;BattleMvpAction action=BattleMvpAction::Idle;
    std::array<unsigned,2> cooldown{};unsigned attack_ticks=0,hit_flash=0;
    BattlePoint velocity{};f32 movement_speed=0,move_forward=0,move_right=0;
    f64 travel_distance=0;unsigned landing_ticks=0;
    BattlePoint aim_facing{0,1};f32 upper_body_yaw=0;
    unsigned takeoff_ticks=0,dash_ticks=0,dash_duration=0,dash_cooldown=0,air_dashes_used=0;
    bool dash_airborne=false;BattlePoint dash_direction{};BattleMvpDashDirection dash_motion=BattleMvpDashDirection::Forward;
    bool source_ground_motion=false; //historical name: source-owned motion requires an explicit source-pose consumer
};
struct BattleMvpProjectile {
    bool active=false;u32 id=0;u8 owner=0;BattleMvpWeapon weapon=BattleMvpWeapon::Bullet;
    BattleMvpVec position{},previous{},velocity{};unsigned life=0;bool landed=false;
};
struct BattleMvpImpact {BattleMvpVec position{};unsigned ticks=0;bool explosion=false;};
struct BattleMvpConfig {
    f32 arena_half=1800,actor_radius=42,actor_height=100,move_speed=5,jump_speed=13,gravity=.55f,step_height=45;
    unsigned health=100;
    std::array<BattlePoint,2> spawn{BattlePoint{-240,300},BattlePoint{240,-300}};
    f32 ground_acceleration=.9f,ground_braking=1.4f,air_acceleration=.25f,turn_per_tick=.18f;
    f32 dash_speed=12;unsigned dash_duration=12,dash_cooldown=30,air_dashes=1;
    //Native-only static-floor query reference, NOT an air spawn or recovered
    //actor constructor. Default0 preserves the ordinary MVP's selection.
    std::array<f32,2> spawn_query_y{};
};
class NativeBattleMvp;
enum class NativeBorgMotionPolicy:u8 {GroundOnly,VerticalJump,Jump,AirDash,Relaunch,StaticPush,FallHistoryPush,RecoveryPush,WindupPush,ActivePush,RelaunchPush,RelaunchEntryPush,RelaunchFloor,AirDashFloor,WindupEntryFloor,WindupPositiveFloor,WindupEntryPush,WalkLedge,OrderedGround,DashLedge,GroundLedge,GroundJump,GroundLanding,GroundAir,GroundAirFloor,GroundRelaunch,GroundRelaunchFloor,GroundWindupEntryFloor,GroundSurface,GroundLossSurface,GroundLossJump,GroundLossLanding,GroundLossAir,GroundLossRelaunch,GroundFallPush,GroundAirFallPush,GroundRelaunchFallPush,GroundRecoveryPush};
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
    AirDashWindupEntry,AirDashWindup,AirDashActiveEntry,AirDashActive,AirDashRecovery,RelaunchEntry,RelaunchWindup,RelaunchActive,StaticPush,AirDashRecoveryPush,AirDashWindupPush,AirDashActivePush,RelaunchWindupPush,RelaunchActivePush,RelaunchEntryPush,RelaunchEntryFloor,RelaunchWindupFloor,RelaunchActiveFloor,AirDashWindupFloor,AirDashActiveEntryFloor,AirDashActiveFloor,AirDashWindupEntryFloor,AirDashWindupPositiveEntryFloor,AirDashWindupEntryPush,GroundDeparture,GroundLossEntry,GroundLossAir,GroundLossTouchdown};
inline bool native_borg_motion_ground_loss(NativeBorgMotionKind kind){return unsigned(kind)>=33&&unsigned(kind)<=36;}
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
inline bool native_borg_motion_push(NativeBorgMotionKind kind){return kind==NativeBorgMotionKind::StaticPush||kind==NativeBorgMotionKind::AirDashRecoveryPush||native_borg_motion_windup_push(kind)||kind==NativeBorgMotionKind::AirDashActivePush||native_borg_motion_relaunch_push(kind);}
inline bool native_borg_motion_air_dash(NativeBorgMotionKind kind){return (unsigned(kind)>=9&&unsigned(kind)<=13)||kind==NativeBorgMotionKind::AirDashRecoveryPush||native_borg_motion_windup_push(kind)||kind==NativeBorgMotionKind::AirDashActivePush||native_borg_motion_air_dash_floor(kind);}
inline bool native_borg_motion_relaunch(NativeBorgMotionKind kind){return (unsigned(kind)>=14&&unsigned(kind)<=16)||native_borg_motion_relaunch_push(kind)||native_borg_motion_relaunch_floor(kind);}
inline bool native_gred_relaunch_acceleration_valid(f32 value){
    f32 cosine=0;source_heading_xz(8192,nullptr,&cosine);const f32 neutral=bits_to_f32(0x3f4ccccdu);
    return value==neutral||value==std::fma(neutral,cosine,0.f);
}
inline unsigned native_borg_motion_phase(NativeBorgMotionKind kind){
    const unsigned value=unsigned(kind);return value<=4?value:value<=8?value-4:value<=35?2:value==36?3:255;
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
    bool operator==(const NativeBorgJumpMotion&)const=default;
};
struct NativeBorgAirMotion {
    u8 phase=15,next_selector=15,stage_before=0,next_stage=1,control=0;
    bool entry=false,callback=false,lower_done=false;
    f32 speed_before=0,move_speed=0,speed44=0,vertical_before=0,planar_accel=0,vertical_accel=0,timer_before=0,timer568=0;
    s16 travel5ae=0,resource_before=0,resource_after=0;std::optional<s16> drift_heading;
    bool operator==(const NativeBorgAirMotion&)const=default;
};
struct NativeBorgRelaunchMotion {
    u8 lower_before=0,upper_before=0,lower_after=1,upper_after=1,lower_program=14,upper_program=14,coarse57d=0;
    bool entry=false,direction=false,lower_done=false;
    f32 speed_before=0,move_speed=0,speed44=0,vertical_before=0,planar_accel=0,vertical_accel=0;
    s16 travel5ae=0,resource_before=0,resource_after=0;
    bool operator==(const NativeBorgRelaunchMotion&)const=default;
};
struct NativeBorgSurfaceLanding {
    BattleMvpVec integrated_position{};f32 vertical_speed=0,acceleration=0;
    NativeBorgStaticSurface surface{};s16 angle=0;f32 timer698=0;
    bool operator==(const NativeBorgSurfaceLanding&)const=default;
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
        for(f32 v:{config.arena_half,config.actor_radius,config.actor_height,config.move_speed,config.jump_speed,config.gravity,config.step_height})if(!std::isfinite(v)||v<=0)return false;
        if(config.arena_half>10000||config.actor_radius*4>=config.arena_half||config.actor_height>1000||config.move_speed>config.actor_radius||config.jump_speed>50||config.gravity<.05f||config.gravity>10||config.step_height>config.actor_height||!config.health||config.health>10000)return false;
        for(f32 v:{config.ground_acceleration,config.ground_braking,config.air_acceleration,config.turn_per_tick})if(!std::isfinite(v)||v<=0)return false;
        if(config.ground_acceleration>config.move_speed*2||config.ground_braking>config.move_speed*2||config.air_acceleration>config.ground_acceleration||config.turn_per_tick>3.141593f)return false;
        if(!std::isfinite(config.dash_speed)||config.dash_speed<config.move_speed||config.dash_speed>config.actor_radius||!config.dash_duration||config.dash_duration>60||config.dash_cooldown<config.dash_duration||config.dash_cooldown>600||config.air_dashes>4)return false;
        NativeBattleMvp next;next.config_=config;
        for(unsigned i=0;i<2;++i){
            const auto p=config.spawn[i];if(!std::isfinite(p.x)||!std::isfinite(p.z)||std::abs(p.x)>config.arena_half-config.actor_radius||std::abs(p.z)>config.arena_half-config.actor_radius)return false;
            if(!std::isfinite(config.spawn_query_y[i])||std::abs(config.spawn_query_y[i])>20000)return false;
            auto& actor=next.actors_[i];actor.model=i?0x000a:0x0615;actor.position={p.x,0,p.z};actor.health=config.health;
            if(!floor(terrain,p.x,p.z,config.spawn_query_y[i],&actor.position[1]))return false;
        }
        const f32 dx=config.spawn[0].x-config.spawn[1].x,dz=config.spawn[0].z-config.spawn[1].z;
        if(dx*dx+dz*dz<=4*config.actor_radius*config.actor_radius)return false;
        next.face(0);next.face(1);for(auto& actor:next.actors_)actor.aim_facing=actor.facing;
        next.motion_scope_=std::shared_ptr<const NativeBorgMotionScope>(new NativeBorgMotionScope(this));
        next.ready_=true;*this=next;return true;
    }
    //One fixed60Hz tick. A/B/X = primary/jump/secondary, Y = restart,
    //L = ground/air dash, Start = pause. Second-pad control is available without AI.
    bool step(std::array<GamePadState,2> pads,bool opponent_ai=true,const IBattleMvpTerrain* terrain=nullptr,const std::array<std::optional<BattleMvpVec>,2>* muzzles=nullptr,const NativeBorgMovementFrame* player_frame=nullptr,const NativeBorgMotionOffer* source_offer=nullptr){
        if(!ready_||tick_==std::numeric_limits<u32>::max())return false;
        if(player_frame&&!native_borg_movement_frame_valid(*player_frame))return false;
        if(muzzles)for(unsigned i=0;i<2;++i)if((*muzzles)[i])for(f32 v:*(*muzzles)[i])if(!std::isfinite(v)||std::abs(v)>1e5f)return false;
        const auto human=pads[0];
        if(human.y&&!previous_[0].y){const auto config=config_;if(!start(config,terrain))return false;previous_[0]=human;return true;}
        if(human.start&&!previous_[0].start){paused_=!paused_;previous_=pads;return true;}
        if(paused_||result_!=BattleMvpResult::Fighting){previous_=pads;return true;}
        if(actors_[0].source_ground_motion&&!source_offer)return false; //no implicit legacy-controller fallback
        NativeBorgContactResult source_contact;
        if(source_offer){
            const auto& o=*source_offer;const auto& actor=actors_[0];const auto& other=actors_[1];
            const auto producer=o.producer.lock();
            if(!producer||producer->world_.lock()!=motion_scope_||(actor.source_ground_motion&&producer!=motion_producer_.lock()))return false;
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
            const bool floor_snap=relaunch_floor||air_floor;
            const bool push=native_borg_motion_push(o.kind);
            const bool positive_windup_entry=o.kind==NativeBorgMotionKind::AirDashWindupPositiveEntryFloor;
            const bool windup_entry_floor=native_borg_motion_windup_floor_entry(o.kind);
            const bool windup_entry_push_policy=producer->policy_==NativeBorgMotionPolicy::WindupEntryPush;
            const bool controller_recovery_push_policy=producer->policy_==NativeBorgMotionPolicy::GroundRecoveryPush;
            const bool controller_relaunch_fall_push_policy=producer->policy_==NativeBorgMotionPolicy::GroundRelaunchFallPush||controller_recovery_push_policy;
            const bool controller_air_fall_push_policy=producer->policy_==NativeBorgMotionPolicy::GroundAirFallPush||controller_relaunch_fall_push_policy;
            const bool controller_fall_push_policy=producer->policy_==NativeBorgMotionPolicy::GroundFallPush||controller_air_fall_push_policy;
            const bool controller_ground_loss_relaunch_policy=producer->policy_==NativeBorgMotionPolicy::GroundLossRelaunch||controller_fall_push_policy;
            const bool controller_ground_loss_air_policy=producer->policy_==NativeBorgMotionPolicy::GroundLossAir||controller_ground_loss_relaunch_policy;
            const bool controller_landing_dash_policy=producer->policy_==NativeBorgMotionPolicy::GroundLossLanding||controller_ground_loss_air_policy;
            const bool controller_landing_jump_policy=producer->policy_==NativeBorgMotionPolicy::GroundLossJump||controller_landing_dash_policy;
            const bool controller_ground_loss_surface_policy=producer->policy_==NativeBorgMotionPolicy::GroundLossSurface||controller_landing_jump_policy;
            const bool controller_surface_policy=producer->policy_==NativeBorgMotionPolicy::GroundSurface||controller_ground_loss_surface_policy;
            const bool controller_windup_entry_floor_policy=producer->policy_==NativeBorgMotionPolicy::GroundWindupEntryFloor||controller_surface_policy;
            const bool controller_relaunch_floor_policy=producer->policy_==NativeBorgMotionPolicy::GroundRelaunchFloor||controller_windup_entry_floor_policy;
            const bool controller_relaunch_policy=producer->policy_==NativeBorgMotionPolicy::GroundRelaunch||controller_relaunch_floor_policy;
            const bool controller_floor_policy=producer->policy_==NativeBorgMotionPolicy::GroundAirFloor||controller_relaunch_policy;
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
            const bool controller_air_policy=producer->policy_==NativeBorgMotionPolicy::GroundAir||controller_floor_policy;
            const bool landing_policy=producer->policy_==NativeBorgMotionPolicy::GroundLanding||controller_air_policy;
            const bool jump_policy=producer->policy_==NativeBorgMotionPolicy::GroundJump||landing_policy;
            const bool combined_ledge_policy=producer->policy_==NativeBorgMotionPolicy::GroundLedge||jump_policy;
            const bool dash_ledge_policy=producer->policy_==NativeBorgMotionPolicy::DashLedge||combined_ledge_policy;
            const bool ledge_policy=producer->policy_==NativeBorgMotionPolicy::WalkLedge||dash_ledge_policy;
            const bool ordered_policy=producer->policy_==NativeBorgMotionPolicy::OrderedGround||dash_ledge_policy;
            const bool departure=o.kind==NativeBorgMotionKind::GroundDeparture;
            const bool ground_loss=native_borg_motion_ground_loss(o.kind);
            const bool controller_fall_push_frame=controller_fall_push_policy&&o.kind==NativeBorgMotionKind::StaticPush;
            const bool controller_recovery_push_frame=controller_recovery_push_policy&&recovery_push;
            const bool jump_kind=(unsigned(o.kind)>=5&&unsigned(o.kind)<=8)||controller_fall_push_frame;
            const bool controller_air_frame=controller_air_policy&&(air_dash||(o.kind==NativeBorgMotionKind::JumpTouchdown&&o.air_motion));
            const bool controller_relaunch_frame=controller_relaunch_policy&&relaunch;
            const bool jump_path=jump_policy&&jump_kind&&!controller_air_frame&&!controller_relaunch_frame&&
                (o.kind!=NativeBorgMotionKind::JumpLanding||!actor.source_ground_motion||!last_offer_.walk_ledge);
            const bool ledger_frame=ledge_policy&&!jump_path&&!controller_air_frame&&!controller_relaunch_frame;
            const unsigned phase=native_borg_motion_phase(o.kind);
            if(phase>4||(vertical&&producer->policy_==NativeBorgMotionPolicy::GroundOnly)||
                   (moving&&!ledge_policy&&!push_policy&&producer->policy_!=NativeBorgMotionPolicy::Jump&&producer->policy_!=NativeBorgMotionPolicy::AirDash&&producer->policy_!=NativeBorgMotionPolicy::Relaunch)||
                   (air_dash&&!controller_air_policy&&!push_policy&&producer->policy_!=NativeBorgMotionPolicy::AirDash&&producer->policy_!=NativeBorgMotionPolicy::Relaunch)||
                   (relaunch&&!controller_relaunch_policy&&!push_policy&&producer->policy_!=NativeBorgMotionPolicy::Relaunch)||
                    (push&&((!push_policy&&!controller_fall_push_frame&&!controller_recovery_push_frame)||!o.static_push))||(!push&&o.static_push)||
                   (recovery_push&&!recovery_push_policy&&!controller_recovery_push_frame)||
                   (windup_push&&!windup_push_policy)||
                   (windup_entry_push&&!windup_entry_push_policy)||
                  (active_push&&!active_push_policy)||
                  (relaunch_push&&!relaunch_push_policy)||
                  (relaunch_entry_push&&!relaunch_entry_push_policy)||
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
                      (bool(o.surface_landing)!=(controller_surface_policy&&(o.kind==NativeBorgMotionKind::JumpTouchdown||
                          (controller_ground_loss_surface_policy&&o.kind==NativeBorgMotionKind::GroundLossTouchdown))))||
                      (bool(o.controller_timer698)!=controller_landing_jump_policy)||
                !std::isfinite(o.vertical_speed)||std::abs(o.vertical_speed)>50||!std::isfinite(o.entry_vertical_delta)||
                (native_borg_motion_relaunch_entry(o.kind)?(o.entry_vertical_delta<=0||o.entry_vertical_delta>50):o.entry_vertical_delta!=0))return false;
            if(opponent_ai||!motion_observation_current(o.observed)||!o.serial||
               (!vertical&&(!actor.grounded||actor.vertical_speed!=0||o.vertical_speed!=0))||
               actor.attack_ticks||actor.hit_flash||actor.dash_ticks||other.attack_ticks||
               !other.grounded||other.vertical_speed!=0||other.movement_speed!=0||other.velocity!=BattlePoint{}||pads[1]!=GamePadState{}||
               pads[0].a||pads[0].b||pads[0].x||pads[1].a||pads[1].b||pads[1].x||
               (!vertical&&o.action!=BattleMvpAction::Idle&&o.action!=BattleMvpAction::Move&&o.action!=BattleMvpAction::GroundDash)||
               !std::isfinite(o.facing.x)||!std::isfinite(o.facing.z)||std::abs(std::hypot(o.facing.x,o.facing.z)-1)>.001f)return false;
            for(const auto& shot:projectiles_)if(shot.active)return false;
            for(auto value:o.proposed_position)if(!std::isfinite(value)||std::abs(value)>1e5f)return false;
            const BattlePoint delta{o.proposed_position[0]-actor.position[0],o.proposed_position[2]-actor.position[2]};
            if(std::hypot(delta.x,delta.z)>config_.actor_radius)return false; //native endpoint-terrain budget
            const auto prior=actor.source_ground_motion?last_offer_.kind:NativeBorgMotionKind::Ground;
            const unsigned prior_phase=native_borg_motion_phase(prior);
            if(controller_landing_jump_policy){
                const f32 previous=actor.source_ground_motion?last_offer_.controller_timer698.value_or(-1.f):0.f;
                const f32 expected=o.kind==NativeBorgMotionKind::GroundLossEntry?30.f:std::max(0.f,previous-1.f);
                const f32 timer=*o.controller_timer698;
                if(previous<0||!std::isfinite(timer)||timer<0||timer>30||timer!=expected||
                   (o.walk_ledge&&o.walk_ledge->timer698!=timer)||
                   (timer>0&&((native_borg_motion_windup_entry(o.kind)&&!controller_ground_loss_air_policy)||
                              (native_borg_motion_relaunch_entry(o.kind)&&!controller_ground_loss_relaunch_policy))))return false;
            }
            const bool active_landing=actor.source_ground_motion&&prior==NativeBorgMotionKind::JumpLanding&&
                last_offer_.jump_motion&&last_offer_.jump_motion->phase==7&&last_offer_.jump_motion->next_selector==7;
            if(o.landing_target){
                if(controller_ground_loss_surface_policy&&last_offer_.walk_ledge&&last_offer_.walk_ledge->timer698>1.f&&
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
                   actor.vertical_speed>=0||o.vertical_speed)return false;
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
            auto project_integrated_push=[&](BattleMvpVec& integrated,f32 vertical,f32 acceleration){
                const auto& p=*o.static_push;
                const f32 velocity=std::max(-30.f,std::fma(acceleration,1.f,vertical));
                return p.integrated_position==integrated&&p.vertical_speed==velocity&&p.damping==push_damping&&
                    o.vertical_speed==f32(velocity*push_damping)&&
                    source_surface_push_position(integrated,p.surface.normal,p.surface.height,p.angle,&integrated);
            };
            if(controller_air_frame){
                const auto& e=*o.air_motion;
                const auto* previous=actor.source_ground_motion&&last_offer_.air_motion?&*last_offer_.air_motion:nullptr;
                const auto* jump=actor.source_ground_motion&&last_offer_.jump_motion?&*last_offer_.jump_motion:nullptr;
                const auto* relaunched=controller_relaunch_policy&&last_offer_.relaunch_motion?&*last_offer_.relaunch_motion:nullptr;
                const f32 prior_speed=previous?previous->speed44:jump?jump->speed44:relaunched?relaunched->speed44:0;
                s16 prior_resource=previous?previous->resource_after:jump?jump->resource_before:relaunched?relaunched->resource_after:0;
                if(jump&&jump->phase==5)prior_resource=s16(std::max(0,int(prior_resource)-(jump->entry?150:100)));
                if(e.phase<15||e.phase>17||e.stage_before>2||e.next_stage>2||e.resource_before<0||e.resource_before>10000||
                   e.resource_after<0||e.resource_before!=prior_resource||e.speed_before!=prior_speed||
                   !std::isfinite(e.timer_before)||!std::isfinite(e.timer568)||std::abs(e.timer_before)>1e5f||std::abs(e.timer568)>1e5f)return false;
                for(f32 value:{e.speed_before,e.move_speed,e.speed44})if(!std::isfinite(value)||value<0||value>30)return false;
                const bool touchdown=o.kind==NativeBorgMotionKind::JumpTouchdown;
                if(controller_recovery_push_frame&&(e.phase!=17||*o.controller_timer698!=0.f))return false;
                auto expected=actor.position;f32 speed=prior_speed,vertical=actor.vertical_speed,change=0,acceleration=bits_to_f32(0xbd8f5c29u);
                f32 timer=e.timer_before;u8 stage=1,next=e.phase;bool callback=false;int cost=0;
                if(e.phase==15){
                    if(e.entry){
                        if((!jump&&!relaunched&&(!previous||previous->phase!=17))||(jump&&(jump->phase!=5&&jump->phase!=6))||
                            (jump&&jump->ground_loss&&last_offer_.walk_ledge&&last_offer_.walk_ledge->timer698>0&&!controller_ground_loss_air_policy)||
                           e.stage_before||timer<5||timer>9||std::floor(timer)!=timer)return false;
                        speed=f32(speed*.5f);vertical=f32(vertical*.5f);
                        if((o.kind!=NativeBorgMotionKind::AirDashWindupEntry&&!windup_entry_floor)||touchdown||
                           (!air_floor&&o.proposed_position!=actor.position))return false;
                    }else{
                        if(!previous||previous->next_selector!=15||e.stage_before!=1||timer!=previous->timer568)return false;
                        timer=f32(timer-1);if(timer<=0){next=16;stage=0;cost=450;}
                        if(o.kind!=NativeBorgMotionKind::AirDashWindup&&o.kind!=NativeBorgMotionKind::AirDashWindupFloor)return false;
                    }
                }else if(e.phase==16){
                    if(e.entry){
                        if(!previous||previous->next_selector!=16||previous->phase!=15||e.stage_before||timer!=previous->timer568)return false;
                        speed=30;vertical=0;change=bits_to_f32(0xbd4ccccdu);timer=30;
                        if(o.kind!=NativeBorgMotionKind::AirDashActiveEntry&&o.kind!=NativeBorgMotionKind::AirDashActiveEntryFloor)return false;
                    }else{
                        if(!previous||previous->next_selector!=16||e.stage_before!=previous->next_stage||timer!=previous->timer568)return false;
                        stage=e.stage_before;change=previous->planar_accel;
                        if(stage==1){timer=f32(timer-1);if(timer<=0){stage=2;change=0;}}
                        if(o.kind!=NativeBorgMotionKind::AirDashActive&&o.kind!=NativeBorgMotionKind::AirDashActiveFloor&&!touchdown)return false;
                    }
                    cost=50;callback=stage==2;
                    if(e.resource_before<=cost){next=17;stage=0;callback=false;}
                }else{
                    if(!previous||(e.entry?(previous->next_selector!=17&&!(previous->phase==16&&previous->next_stage==2)):
                        previous->next_selector!=17)||timer!=previous->timer568||e.control)return false;
                    if(controller_ground_loss_air_policy&&*o.controller_timer698>0&&e.drift_heading)return false;
                     //Push evidence records the acceleration used for integration;
                     //the response clears it before the next recovery update.
                     change=last_offer_.kind==NativeBorgMotionKind::AirDashRecoveryPush?0.f:previous->planar_accel;
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
                 if(e.move_speed!=speed||e.speed44!=(controller_recovery_push_frame?f32(retained*push_damping):retained)||e.vertical_before!=vertical||e.planar_accel!=change||
                   e.vertical_accel!=acceleration||e.timer568!=timer||e.next_stage!=stage||e.callback!=callback||
                   e.resource_after!=std::max(0,int(e.resource_before)-cost))return false;
                if(moves){f32 x=0,z=0;source_heading_xz(e.travel5ae,&x,&z);
                    expected[0]=std::fma(1.f,f32(speed*x),expected[0]);expected[1]=std::fma(vertical,1.f,expected[1]);
                    expected[2]=std::fma(1.f,f32(speed*z),expected[2]);}
                if(touchdown){
                    if(!callback||(e.phase==17&&e.lower_done))return false;
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
                    next=7;
                 }else if(controller_recovery_push_frame){
                     if(!project_integrated_push(expected,vertical,acceleration))return false;
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
                const auto* previous=actor.source_ground_motion&&last_offer_.relaunch_motion?&*last_offer_.relaunch_motion:nullptr;
                const auto* air=actor.source_ground_motion&&last_offer_.air_motion?&*last_offer_.air_motion:nullptr;
                const auto* jump=actor.source_ground_motion&&last_offer_.jump_motion?&*last_offer_.jump_motion:nullptr;
                const f32 speed=previous?previous->speed44:air?air->speed44:jump?jump->speed44:0;
                const s16 resource=previous?previous->resource_after:air?air->resource_after:jump?jump->resource_before:0;
                if(!std::isfinite(e.speed_before)||e.speed_before<0||e.speed_before>30||e.speed_before!=speed||
                   e.resource_before!=resource||resource<=0||e.coarse57d>3||e.lower_before>3||e.upper_before>3||
                   e.upper_before>e.lower_before||e.entry!=(e.lower_before==0))return false;
                if(e.entry){
                    if(previous||(!air&&!jump)||(air&&air->phase!=16&&air->phase!=17)||
                        (jump&&(jump->phase!=6||(jump->ground_loss&&!controller_ground_loss_relaunch_policy)))||e.upper_before||
                       o.kind!=(relaunch_floor?NativeBorgMotionKind::RelaunchEntryFloor:NativeBorgMotionKind::RelaunchEntry)||o.entry_vertical_delta!=15)return false;
                }else if(!previous||e.lower_before!=previous->lower_after||e.upper_before!=previous->upper_after||
                         e.coarse57d!=previous->coarse57d||o.entry_vertical_delta)return false;
                const u8 program=e.lower_before==2||e.lower_before==3?15:14;
                u8 lower=e.entry?1:e.lower_before==2?3:e.lower_before;
                if(!e.entry&&lower==1&&e.lower_done)lower=2;
                u8 upper=e.upper_before;const u8 upper_program=upper>=2?15:14;
                if(!upper)upper=1;else if(upper==1&&lower>1)upper=2;else if(upper==2)upper=3;
                if(e.lower_program!=program||e.upper_program!=upper_program||e.lower_after!=lower||e.upper_after!=upper||
                   (!e.entry&&o.kind!=(relaunch_floor?(program==14?NativeBorgMotionKind::RelaunchWindupFloor:NativeBorgMotionKind::RelaunchActiveFloor):
                       (program==14?NativeBorgMotionKind::RelaunchWindup:NativeBorgMotionKind::RelaunchActive))))return false;
                f32 sine=0,cosine=1;source_heading_xz(e.direction?8192:0,&sine,&cosine);
                const f32 change=f32(bits_to_f32(0x3f4ccccdu)*sine);
                const f32 acceleration=std::fma(bits_to_f32(0x3f4ccccdu),cosine,0.f);
                const f32 moving=e.direction?speed:f32(speed*bits_to_f32(0x3f7ae148u));
                const f32 vertical=e.entry?f32(actor.vertical_speed+15.f):actor.vertical_speed;
                const f32 accelerated=std::max(0.f,std::fma(change,1.f,moving));
                const f32 retained=change>0||!controller_ground_loss_relaunch_policy?std::min(8.f,accelerated):accelerated;
                if(e.move_speed!=moving||e.speed44!=retained||e.vertical_before!=vertical||e.planar_accel!=change||
                   e.vertical_accel!=acceleration||e.resource_after!=std::max(0,int(resource)-(e.entry?150:100)))return false;
                auto expected=actor.position;f32 x=0,z=0;source_heading_xz(e.travel5ae,&x,&z);
                expected[0]=std::fma(1.f,f32(moving*x),expected[0]);expected[1]=std::fma(vertical,1.f,expected[1]);
                expected[2]=std::fma(1.f,f32(moving*z),expected[2]);
                const f32 integrated=std::clamp(std::fma(acceleration,1.f,vertical),-30.f,18.f);
                if(relaunch_floor){
                    const auto& floor=*o.relaunch_floor;
                    if(!controller_relaunch_floor_policy||floor.integrated_position!=expected||floor.vertical_speed!=integrated||
                       floor.acceleration!=acceleration||floor.response_factor!=(integrated<0?bits_to_f32(0x3f4ccccdu):1.f)||
                       o.vertical_speed!=f32(integrated*floor.response_factor))return false;
                    expected[1]=floor.surface.height;
                }else if(o.vertical_speed!=integrated)return false;
                if(o.proposed_position!=expected)return false;
            }
            if(jump_policy){
                const auto* previous=actor.source_ground_motion&&last_offer_.jump_motion?&*last_offer_.jump_motion:nullptr;
                if(o.kind==NativeBorgMotionKind::Ground&&previous&&previous->next_selector&&o.landing_target!=12)return false;
                if(o.jump_motion){
                    const auto& e=*o.jump_motion;
                    if(controller_fall_push_frame&&(e.phase!=6||e.ground_loss||(e.air_dash_origin&&!controller_air_fall_push_policy)||
                       (e.relaunch_origin&&!controller_relaunch_fall_push_policy)||*o.controller_timer698>0))return false;
                    const f32 speed_limit=e.air_dash_origin||(controller_ground_loss_relaunch_policy&&e.relaunch_origin)?30.f:25.f;
                    if(e.phase<4||e.phase>7||e.resource_before<0||e.resource_before>10000||
                       e.ground_loss!=(!jump_path)||!std::isfinite(e.speed_before)||e.speed_before<0||e.speed_before>speed_limit||
                       !std::isfinite(e.move_speed)||e.move_speed<0||e.move_speed>speed_limit||!std::isfinite(e.speed44)||e.speed44<0||e.speed44>speed_limit||
                       !std::isfinite(e.vertical_before)||!std::isfinite(e.planar_accel)||!std::isfinite(e.vertical_accel))return false;
                    if(e.air_dash_origin&&(!controller_air_policy||e.ground_loss||e.relaunch_origin))return false;
                    if(e.relaunch_coarse57d>3||(!e.relaunch_origin&&e.relaunch_coarse57d)||
                       (e.relaunch_origin&&(!controller_relaunch_policy||e.ground_loss)))return false;
                    const f32 previous_speed=previous?previous->speed44:
                        controller_air_policy&&last_offer_.air_motion?last_offer_.air_motion->speed44:
                        controller_relaunch_policy&&last_offer_.relaunch_motion?last_offer_.relaunch_motion->speed44:
                        actor.source_ground_motion&&last_offer_.walk_ledge?last_offer_.walk_ledge->speed44:0.f;
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
                            e.air_dash_origin!=(previous?previous->air_dash_origin:bool(controller_air_policy&&last_offer_.air_motion))||
                            e.relaunch_origin!=(previous&&previous->relaunch_origin)||
                            e.relaunch_coarse57d!=(previous?previous->relaunch_coarse57d:0)||
                           (!first&&(prior!=NativeBorgMotionKind::JumpLanding||!previous||previous->next_selector!=7))||
                           e.next_selector!=(!e.entry&&e.lower_done?0:7)||e.move_speed||e.speed44||e.vertical_before||
                           e.planar_accel||e.vertical_accel||e.drift_heading||o.proposed_position!=actor.position||o.vertical_speed)return false;
                    }else if(e.ground_loss){
                        if(e.phase!=6||!ground_loss||e.entry!=(o.kind==NativeBorgMotionKind::GroundLossEntry)||
                           e.next_selector!=(o.kind==NativeBorgMotionKind::GroundLossTouchdown?7:6)||
                           e.planar_accel!=0||e.vertical_accel!=-1||e.vertical_before!=actor.vertical_speed||
                           e.speed44!=o.walk_ledge->speed44||e.move_speed!=e.speed44||e.travel5ae!=o.walk_ledge->travel5ae||
                           e.drift_heading!=o.walk_ledge->drift_heading)return false;
                    }else{
                        const auto* air_previous=controller_air_policy&&last_offer_.air_motion?&*last_offer_.air_motion:nullptr;
                        const auto* relaunch_previous=controller_relaunch_policy&&last_offer_.relaunch_motion?&*last_offer_.relaunch_motion:nullptr;
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
                             if(!project_integrated_push(expected,vertical,acceleration))return false;
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
            if(ordered_policy&&(o.kind==NativeBorgMotionKind::Ground||departure)){
                const auto* previous=actor.source_ground_motion&&last_offer_.ordered_ground?&*last_offer_.ordered_ground:nullptr;
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
                       !native_borg_flat_surface(current)||!native_borg_flat_surface(selected)||
                       current.height!=actor.position[1]||o.proposed_position[1]!=current.height||
                       (departure?(!dash_ledge_policy||selected.height>=f32(current.height-60.f)||!e.move_speed):
                        selected.height!=current.height))return false;
                    auto expected=actor.position;
                    if(e.move_speed&&!native_gred_ground_position(actor.position,e.move_speed,e.travel5ae,current.normal,&expected))return false;
                    if(o.proposed_position!=expected)return false;
                }
            }
            const bool relaunch_entry_prior=prior==NativeBorgMotionKind::JumpAir||prior==NativeBorgMotionKind::StaticPush||
                native_borg_motion_windup_update(prior)||native_borg_motion_active_entry(prior)||
                native_borg_motion_active_update(prior)||native_borg_motion_recovery(prior)||
                (controller_ground_loss_relaunch_policy&&(prior==NativeBorgMotionKind::GroundLossEntry||prior==NativeBorgMotionKind::GroundLossAir));
            const bool windup_entry_prior=prior==NativeBorgMotionKind::JumpAir||prior==NativeBorgMotionKind::StaticPush||
                native_borg_motion_recovery(prior)||native_borg_motion_relaunch(prior)||
                (controller_air_policy&&(prior==NativeBorgMotionKind::GroundLossEntry||prior==NativeBorgMotionKind::GroundLossAir));
            if(ledger_frame){
                //Normal G-Red, rank1/fixed60Hz, separately scoped ground loss.
                //numeric validation; no original history/actor callback ACK.
                const auto& e=*o.walk_ledge;
                if(!terrain||e.frame_start!=actor.position||!std::isfinite(e.timer698)||e.timer698<0||e.timer698>30||
                   !std::isfinite(e.speed44)||e.speed44<0||e.speed44>(dash_ledge_policy?25.f:12.f))return false;
                if(actor.source_ground_motion&&!last_offer_.walk_ledge&&
                   (!jump_policy||o.kind!=NativeBorgMotionKind::Ground||!last_offer_.jump_motion||
                    (last_offer_.jump_motion->next_selector&&o.landing_target!=12)))return false;
                const auto previous_timer=controller_landing_jump_policy&&actor.source_ground_motion?*last_offer_.controller_timer698:
                    actor.source_ground_motion&&last_offer_.walk_ledge?last_offer_.walk_ledge->timer698:0.f;
                const f32 timer=o.kind==NativeBorgMotionKind::GroundLossEntry?30.f:std::max(0.f,previous_timer-1.f);
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
                        if(departure&&(!o.ordered_ground||!native_borg_flat_surface(current)||!native_borg_flat_surface(selected)))return false;
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
                        prior!=NativeBorgMotionKind::GroundLossEntry&&prior!=NativeBorgMotionKind::GroundLossAir)||
                       (entry&&actor.vertical_speed!=0)||e.travel5ae!=last_offer_.walk_ledge->travel5ae||o.facing!=actor.facing||
                       e.speed44!=f32(last_offer_.walk_ledge->speed44*bits_to_f32(0x3f7d70a4u))||
                       (e.timer698>0&&e.drift_heading))return false;
                    auto expected=actor.position;f32 x=0,z=0;
                    if(e.drift_heading){
                        source_heading_xz(*e.drift_heading,&x,&z);
                        expected[0]=std::fma(4.f,x,expected[0]);expected[2]=std::fma(4.f,z,expected[2]);
                    }
                    source_heading_xz(e.travel5ae,&x,&z);
                    expected[0]=std::fma(1.f,f32(e.speed44*x),expected[0]);
                    expected[1]=std::fma(actor.vertical_speed,1.f,expected[1]);
                    expected[2]=std::fma(1.f,f32(e.speed44*z),expected[2]);
                    if(o.proposed_position[0]!=expected[0]||o.proposed_position[2]!=expected[2]||actor.position[1]<selected.height-.01f)return false;
                    if(o.kind==NativeBorgMotionKind::GroundLossTouchdown){
                        if(controller_ground_loss_surface_policy){
                            if(!surface_landing_height(expected,std::max(-30.f,f32(actor.vertical_speed-1.f)),-1.f,&expected[1])||
                               expected[1]!=selected.height||o.proposed_position!=expected)return false;
                        }else if(!native_borg_flat_surface(selected)||actor.vertical_speed>=0||expected[1]>selected.height||o.proposed_position[1]!=selected.height||o.vertical_speed!=0)return false;
                    }else if(o.proposed_position[1]!=expected[1]||expected[1]<=selected.height||
                             o.vertical_speed!=std::max(-30.f,f32(actor.vertical_speed-1.f)))return false;
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
                     (controller_air_fall_push_policy&&native_borg_motion_recovery(prior)&&last_offer_.air_motion&&
                     last_offer_.air_motion->phase==17&&last_offer_.air_motion->next_selector==6&&last_offer_.air_motion->lower_done)||
                    (controller_relaunch_fall_push_policy&&native_borg_motion_relaunch(prior)&&last_offer_.relaunch_motion);
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
                   (relaunch_push?!relaunch_prior:active_push?!active_prior:windup_push?!windup_prior:recovery_push?!recovery_prior:(prior!=NativeBorgMotionKind::JumpAir&&prior!=NativeBorgMotionKind::StaticPush&&!history_entry))||
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
                s16 angle=0;BattleMvpVec projected;
                if(!source_surface_angle(surface.normal,&angle)||angle!=e.angle||angle<=10240||
                   !source_surface_push_position(e.integrated_position,surface.normal,surface.height,angle,&projected)||
                   projected!=o.proposed_position)return false;
                f32 dot=0;for(unsigned k=0;k<3;++k)dot=f32(dot+f32(f32(e.integrated_position[k]-actor.position[k])*surface.normal[k]));
                if(dot>0)return false;
                NativeBorgContactRequest pre{actor.position,other.position,pre_delta,config_.arena_half,config_.actor_radius,config_.actor_height,config_.step_height,false,other.health!=0};
                pre.vertical_end_y=e.integrated_position[1];NativeBorgContactResult checked;
                if(!native_borg_resolve_contact(pre,terrain,&checked)||checked.body_hit||checked.bounds_hit||checked.terrain_blocked||
                   checked.position[0]!=e.integrated_position[0]||checked.position[2]!=e.integrated_position[2])return false;
            }
            if(!vertical&&(prior_phase==1||prior_phase==2||prior_phase==3))return false;
            if(vertical&&prior_phase&&moving!=(unsigned(prior)>=unsigned(NativeBorgMotionKind::JumpTakeoff)))return false;
            const bool air=phase==2||phase==3;
            NativeBorgContactRequest request{actor.position,other.position,delta,config_.arena_half,config_.actor_radius,config_.actor_height,config_.step_height,air?false:actor.grounded,other.health!=0};
            if(air)request.vertical_end_y=o.proposed_position[1];
            if(!native_borg_resolve_contact(request,terrain,&source_contact))return false;
            if((push||floor_snap||ledge_policy||ordered_policy)&&(source_contact.body_hit||source_contact.bounds_hit||source_contact.terrain_blocked||
               source_contact.position[0]!=o.proposed_position[0]||source_contact.position[2]!=o.proposed_position[2]))return false;
            if(vertical){
                f32 y=0;
                if((!moving&&(delta!=BattlePoint{}||o.facing!=actor.facing))||
                   !floor(terrain,source_contact.position[0],source_contact.position[2],actor.position[1],&y)||
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
                    if(prior_phase!=2||actor.vertical_speed>=0||o.vertical_speed||
                       actor.position[1]+actor.vertical_speed>y+.01f||std::abs(o.proposed_position[1]-y)>.01f)return false;
                }else if(delta!=BattlePoint{}||(prior_phase!=3&&prior_phase!=4))return false;
                if(!air&&(!actor.grounded||o.vertical_speed||std::abs(o.proposed_position[1]-y)>.01f))return false;
                //No original standing-on-Borg/vertical-solid response is known.
                //Refuse an overlap reached from above rather than tunnelling or
                //inventing a floor on the opponent. Shared planar sweep handles
                //the ordinary inward horizontal contact before this check.
                if(moving&&air&&other.health&&std::min(actor.position[1],o.proposed_position[1])<other.position[1]+config_.actor_height&&
                   std::max(actor.position[1],o.proposed_position[1])>other.position[1]-config_.actor_height&&
                   std::hypot(source_contact.position[0]-other.position[0],source_contact.position[2]-other.position[2])<2*config_.actor_radius-.001f)return false;
                source_contact.position[1]=o.proposed_position[1];source_contact.grounded=phase!=2;
            }else if(!source_contact.grounded||std::abs(source_contact.position[1]-o.proposed_position[1])>.01f)return false;
        }
        if(opponent_ai)pads[1]=ai_input();
        //Preflight terrain before mutating a tick. A missing grid is not floor0.
        std::array<f32,2> ground{};
        for(unsigned i=0;i<2;++i)if(!floor(terrain,actors_[i].position[0],actors_[i].position[2],actors_[i].position[1],&ground[i]))return false;
        ++tick_;
        for(auto& effect:impacts_)if(effect.ticks)--effect.ticks;
        for(unsigned i=0;i<2;++i){
            auto& actor=actors_[i];if(!actor.health){actor.action=BattleMvpAction::Defeated;continue;}
            for(auto& cooldown:actor.cooldown)if(cooldown)--cooldown;
            if(actor.attack_ticks)--actor.attack_ticks;
            if(actor.hit_flash)--actor.hit_flash;
            if(i==0&&source_offer){
                actor.position=source_contact.position;actor.velocity=source_contact.actual_delta;actor.facing=source_offer->facing;
                actor.movement_speed=std::hypot(actor.velocity.x,actor.velocity.z);actor.travel_distance+=actor.movement_speed;
                actor.grounded=source_contact.grounded;actor.vertical_speed=source_offer->vertical_speed;actor.action=source_offer->action;actor.source_ground_motion=true;
                actor.takeoff_ticks=actor.landing_ticks=actor.dash_ticks=actor.dash_cooldown=actor.air_dashes_used=0;actor.dash_airborne=false;
                actor.move_forward=actor.movement_speed?(actor.velocity.x*actor.facing.x+actor.velocity.z*actor.facing.z)/actor.movement_speed:0;
                actor.move_right=actor.movement_speed?(-actor.velocity.x*actor.facing.z+actor.velocity.z*actor.facing.x)/actor.movement_speed:0;
                actor.aim_facing=native_borg_direction({actor.position[0],actor.position[2]},{actors_[1].position[0],actors_[1].position[2]},actor.facing);actor.upper_body_yaw=0;
                continue;
            }
            actor.source_ground_motion=false;
            if(actor.landing_ticks)--actor.landing_ticks;
            if(actor.takeoff_ticks)--actor.takeoff_ticks;
            if(actor.dash_cooldown)--actor.dash_cooldown;
            if(actor.dash_ticks)--actor.dash_ticks;
            if(actor.grounded&&pads[i].b&&!previous_[i].b){actor.grounded=false;actor.vertical_speed=config_.jump_speed;actor.takeoff_ticks=BattleMvpActor::takeoff_duration;actor.landing_ticks=0;actor.dash_ticks=0;actor.air_dashes_used=0;}
            if(!actor.grounded){
                if(!(actor.dash_ticks&&actor.dash_airborne)){actor.position[1]+=actor.vertical_speed;actor.vertical_speed-=config_.gravity;}
                if(actor.position[1]<=ground[i]){actor.position[1]=ground[i];actor.vertical_speed=0;actor.grounded=true;actor.landing_ticks=BattleMvpActor::landing_duration;actor.takeoff_ticks=0;actor.dash_ticks=0;actor.air_dashes_used=0;}
            }
            f32 x=axis(pads[i].stick_x),z=axis(pads[i].stick_y);
            x+=f32(pads[i].dpad_right)-f32(pads[i].dpad_left);z+=f32(pads[i].dpad_down)-f32(pads[i].dpad_up);unit(x,z,false);
            const auto direction=native_borg_move_direction(x,z,i==0&&player_frame?*player_frame:NativeBorgMovementFrame{});
            const bool has_input=direction.x!=0||direction.z!=0;
            const bool dash_held=pads[i].l||pads[i].trigger_l_pressed,previous_dash=previous_[i].l||previous_[i].trigger_l_pressed;
            if(dash_held&&!previous_dash&&!actor.dash_ticks&&!actor.dash_cooldown&&(actor.grounded||actor.air_dashes_used<config_.air_dashes)){
                auto dash=has_input?direction:actor.facing;unit(dash.x,dash.z,true);
                const f32 front=dash.x*actor.facing.x+dash.z*actor.facing.z,side=-dash.x*actor.facing.z+dash.z*actor.facing.x;
                actor.dash_motion=std::abs(side)>std::abs(front)?(side<0?BattleMvpDashDirection::Left:BattleMvpDashDirection::Right):(front<0?BattleMvpDashDirection::Backward:BattleMvpDashDirection::Forward);
                actor.dash_direction=dash;actor.dash_ticks=actor.dash_duration=config_.dash_duration;actor.dash_cooldown=config_.dash_cooldown;actor.dash_airborne=!actor.grounded;
                if(actor.dash_airborne){++actor.air_dashes_used;actor.vertical_speed=0;actor.takeoff_ticks=0;actor.facing=dash;} //classified air dash always faces its travel direction
            }
            if(actor.dash_ticks){
                if(has_input){actor.dash_direction=direction;unit(actor.dash_direction.x,actor.dash_direction.z,true);}
                if(actor.dash_airborne)actor.facing=actor.dash_direction;
                else{
                    const auto dash=actor.dash_direction;const f32 front=dash.x*actor.facing.x+dash.z*actor.facing.z,side=-dash.x*actor.facing.z+dash.z*actor.facing.x;
                    actor.dash_motion=std::abs(side)>std::abs(front)?(side<0?BattleMvpDashDirection::Left:BattleMvpDashDirection::Right):(front<0?BattleMvpDashDirection::Backward:BattleMvpDashDirection::Forward);
                }
                actor.velocity={actor.dash_direction.x*config_.dash_speed,actor.dash_direction.z*config_.dash_speed};
            }
            else{
                //All burst exits (expiry, jump, landing, collision or ledge) return
                //to the same bounded controller, not a flight of leftover dash speed.
                const f32 speed=std::hypot(actor.velocity.x,actor.velocity.z);
                if(speed>config_.move_speed){actor.velocity.x*=config_.move_speed/speed;actor.velocity.z*=config_.move_speed/speed;}
                actor.velocity=native_borg_velocity(actor.velocity,{direction.x*config_.move_speed,direction.z*config_.move_speed},actor.grounded?(has_input?config_.ground_acceleration:config_.ground_braking):config_.air_acceleration);
            }
            const bool was_dashing=actor.dash_ticks!=0;
            const auto& other=actors_[1-i];
            NativeBorgContactResult contact;
            if(!native_borg_resolve_contact({actor.position,other.position,actor.velocity,config_.arena_half,config_.actor_radius,config_.actor_height,config_.step_height,actor.grounded,other.health!=0},terrain,&contact)){ready_=false;return false;}
            actor.position=contact.position;actor.grounded=contact.grounded;actor.velocity=contact.actual_delta;
            actor.movement_speed=std::hypot(actor.velocity.x,actor.velocity.z);actor.travel_distance+=actor.movement_speed;
            if(actor.dash_ticks&&(contact.terrain_blocked||contact.body_hit||actor.movement_speed<config_.dash_speed*.99f||(!actor.grounded&&!actor.dash_airborne)))actor.dash_ticks=0;
            const bool moving=actor.movement_speed>.05f;
            if(moving&&has_input&&!was_dashing)actor.facing=native_borg_turn(actor.facing,actor.velocity,config_.turn_per_tick);
            actor.aim_facing=native_borg_direction({actor.position[0],actor.position[2]},{actors_[1-i].position[0],actors_[1-i].position[2]},actor.facing);
            const f32 speed=std::max(actor.movement_speed,.0001f);
            actor.move_forward=(actor.velocity.x*actor.facing.x+actor.velocity.z*actor.facing.z)/speed;
            actor.move_right=(-actor.velocity.x*actor.facing.z+actor.velocity.z*actor.facing.x)/speed;
            if(pads[i].a&&!actor.cooldown[0])fire(i,false,muzzles);
            if(pads[i].x&&!actor.cooldown[1])fire(i,true,muzzles);
            if(actor.attack_ticks)actor.upper_body_yaw=native_borg_aim_delta(actor.facing,actor.aim_facing);
            else actor.upper_body_yaw+=std::clamp(-actor.upper_body_yaw,-.2f,.2f);
            actor.action=actor.dash_ticks?(actor.dash_airborne?BattleMvpAction::AirDash:BattleMvpAction::GroundDash):!actor.grounded?BattleMvpAction::Jump:actor.landing_ticks?BattleMvpAction::Land:actor.attack_ticks?BattleMvpAction::Attack:moving?BattleMvpAction::Move:BattleMvpAction::Idle;
        }
        for(auto& shot:projectiles_)if(shot.active){
            shot.previous=shot.position;
            if(shot.weapon==BattleMvpWeapon::Bomb){
                if(!shot.landed){for(unsigned k=0;k<3;++k)shot.position[k]+=shot.velocity[k];shot.velocity[1]-=config_.gravity*.65f;
                    f32 y=0;if(!floor(terrain,shot.position[0],shot.position[2],shot.position[1],&y)){ready_=false;return false;}
                    if(shot.position[1]<=y){shot.position[1]=y;shot.landed=true;}}
                if(--shot.life==0){impact(shot.position,true);for(unsigned i=0;i<2;++i)if(i!=shot.owner&&actors_[i].health){f32 squared=0;for(unsigned k=0;k<3;++k){const f32 d=actors_[i].position[k]+(k==1?config_.actor_height*.5f:0)-shot.position[k];squared+=d*d;}if(squared<=180.f*180.f)damage(i,35);}shot.active=false;}
            }else{
                const unsigned target=1-shot.owner;f32 t=0;bool hit=false;
                if(actors_[target].health&&battle_swept_hit({shot.position[0],shot.position[2]},{shot.velocity[0],shot.velocity[2]},{actors_[target].position[0],actors_[target].position[2]},config_.actor_radius+8,&t)){
                    const f32 y=shot.position[1]+shot.velocity[1]*t;hit=y>=actors_[target].position[1]-8&&y<=actors_[target].position[1]+config_.actor_height+8;
                }
                if(hit){for(unsigned k=0;k<3;++k)shot.position[k]+=shot.velocity[k]*t;damage(target,shot.weapon==BattleMvpWeapon::EnergyBeam?12:shot.weapon==BattleMvpWeapon::Shuriken?4:6);impact(shot.position,false);shot.active=false;}
                else{for(unsigned k=0;k<3;++k)shot.position[k]+=shot.velocity[k];if(--shot.life==0)shot.active=false;}
            }
            if(std::abs(shot.position[0])>config_.arena_half+300||std::abs(shot.position[2])>config_.arena_half+300||shot.position[1]<-10000)shot.active=false;
        }
        for(auto& actor:actors_)if(!actor.health)actor.action=BattleMvpAction::Defeated;
        if(!actors_[0].health||!actors_[1].health)result_=!actors_[0].health?(!actors_[1].health?BattleMvpResult::Draw:BattleMvpResult::SasukeWins):BattleMvpResult::GRedWins;
        if(source_offer){
            last_offer_=*source_offer;last_receipt_.scope_=motion_scope_;last_receipt_.tick_=tick_;last_receipt_.serial_=source_offer->serial;
            last_receipt_.position_=actors_[0].position;last_receipt_.facing_=actors_[0].facing;last_receipt_.contact_=source_contact;
            motion_producer_=source_offer->producer;last_receipt_.producer_=source_offer->producer;
            last_receipt_.grounded_=actors_[0].grounded;last_receipt_.vertical_speed_=actors_[0].vertical_speed;
        }else last_receipt_={};
        previous_=pads;return true;
    }
    bool ready()const{return ready_;}bool paused()const{return paused_;}u32 tick()const{return tick_;}
    const auto& actors()const{return actors_;}const auto& projectiles()const{return projectiles_;}const auto& impacts()const{return impacts_;}
    const BattleMvpConfig& config()const{return config_;}BattleMvpResult result()const{return result_;}
    u32 shots_spawned()const{return shots_;}u32 hits()const{return hits_;}
    bool observe_motion(NativeBorgMotionObservation* out)const{
        if(!out||!ready_||paused_||result_!=BattleMvpResult::Fighting||!motion_scope_||motion_scope_->owner_!=this||actors_[0].model!=0x615||!actors_[0].health)return false;
        *out={motion_scope_,tick_,actors_[0].model,actors_[0].position,actors_[0].facing,actors_[0].grounded,actors_[0].vertical_speed};return true;
    }
    std::shared_ptr<const NativeBorgMotionProducer> new_motion_producer(bool vertical_jump=false)const{
        return new_motion_producer(vertical_jump?NativeBorgMotionPolicy::VerticalJump:NativeBorgMotionPolicy::GroundOnly);
    }
    std::shared_ptr<const NativeBorgMotionProducer> new_motion_producer(NativeBorgMotionPolicy policy)const{
        NativeBorgMotionObservation observed;
        const auto& actor=actors_[0];
         if(unsigned(policy)>unsigned(NativeBorgMotionPolicy::GroundRecoveryPush)||!observe_motion(&observed)||actor.source_ground_motion||!actor.grounded||actor.vertical_speed||
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
    bool motion_observation_current(const NativeBorgMotionObservation& observed)const{
        return ready_&&!paused_&&result_==BattleMvpResult::Fighting&&motion_scope_&&motion_scope_->owner_==this&&
            observed.scope.lock()==motion_scope_&&observed.tick==tick_&&observed.model==actors_[0].model&&
            observed.position==actors_[0].position&&observed.facing==actors_[0].facing&&observed.grounded==actors_[0].grounded&&observed.vertical_speed==actors_[0].vertical_speed;
    }
    bool motion_receipt(const NativeBorgMotionOffer& offer,NativeBorgMotionReceipt* out)const{
        if(!out||!motion_receipt_current(last_receipt_)||offer.observed.scope.lock()!=motion_scope_||offer.producer.lock()!=last_offer_.producer.lock()||offer.serial!=last_offer_.serial||
           offer.observed.tick!=last_offer_.observed.tick||offer.observed.position!=last_offer_.observed.position||offer.observed.facing!=last_offer_.observed.facing||
           offer.observed.model!=last_offer_.observed.model||offer.observed.grounded!=last_offer_.observed.grounded||
           offer.proposed_position!=last_offer_.proposed_position||offer.facing!=last_offer_.facing||offer.action!=last_offer_.action||
           offer.kind!=last_offer_.kind||offer.vertical_speed!=last_offer_.vertical_speed||offer.observed.vertical_speed!=last_offer_.observed.vertical_speed||
            offer.entry_vertical_delta!=last_offer_.entry_vertical_delta||offer.static_push!=last_offer_.static_push||offer.relaunch_floor!=last_offer_.relaunch_floor||
            offer.air_dash_floor!=last_offer_.air_dash_floor||offer.walk_ledge!=last_offer_.walk_ledge||
            offer.ordered_ground!=last_offer_.ordered_ground||offer.jump_motion!=last_offer_.jump_motion||
            offer.landing_target!=last_offer_.landing_target||offer.air_motion!=last_offer_.air_motion||
            offer.relaunch_motion!=last_offer_.relaunch_motion||offer.surface_landing!=last_offer_.surface_landing||
            offer.controller_timer698!=last_offer_.controller_timer698)return false;
        *out=last_receipt_;return true;
    }
    bool motion_receipt_current(const NativeBorgMotionReceipt& receipt)const{
        return ready_&&motion_scope_&&motion_scope_->owner_==this&&receipt.scope_.lock()==motion_scope_&&!receipt.producer_.expired()&&receipt.producer_.lock()==motion_producer_.lock()&&receipt.serial_&&receipt.tick_==tick_&&
            actors_[0].source_ground_motion&&receipt.position_==actors_[0].position&&receipt.facing_==actors_[0].facing&&
            receipt.grounded_==actors_[0].grounded&&receipt.vertical_speed_==actors_[0].vertical_speed;
    }
private:
    static f32 axis(s32 value){return f32(std::clamp(value,-128,127))/(value<0?128.f:127.f);}
    static void unit(f32& x,f32& z,bool always){const f32 length=std::sqrt(x*x+z*z);if(length>0&&(always||length>1)){x/=length;z/=length;}}
    static bool floor(const IBattleMvpTerrain* terrain,f32 x,f32 z,f32 current,f32* out){if(!terrain){*out=0;return true;}return terrain->height(x,z,current,out)&&std::isfinite(*out)&&std::abs(*out)<=20000;}
    void face(unsigned i){f32 x=actors_[1-i].position[0]-actors_[i].position[0],z=actors_[1-i].position[2]-actors_[i].position[2];unit(x,z,true);if(x||z)actors_[i].facing={x,z};}
    GamePadState ai_input()const{
        GamePadState pad;if(!actors_[1].health)return pad;
        f32 x=actors_[0].position[0]-actors_[1].position[0],z=actors_[0].position[2]-actors_[1].position[2];const f32 distance=std::sqrt(x*x+z*z);unit(x,z,true);
        const f32 side=(tick_/120)%2?1.f:-1.f;
        const f32 approach=distance>700?1.f:distance<300?-.7f:0.f;
        f32 mx=x*approach-z*side*.5f,mz=z*approach+x*side*.5f;unit(mx,mz,false);
        pad.stick_x=s8(std::clamp(int(mx*127),-127,127));pad.stick_y=s8(std::clamp(int(mz*127),-127,127));pad.a=true;pad.b=tick_%240==100;return pad;
    }
    void fire(unsigned owner,bool secondary,const std::array<std::optional<BattleMvpVec>,2>* muzzles){
        const auto weapon=owner?(secondary?BattleMvpWeapon::Bullet:BattleMvpWeapon::Shuriken):(secondary?BattleMvpWeapon::Bomb:BattleMvpWeapon::EnergyBeam);
        const unsigned count=weapon==BattleMvpWeapon::Shuriken?3:1;unsigned free=0;for(const auto& shot:projectiles_)free+=!shot.active;
        if(free<count||next_id_>std::numeric_limits<u32>::max()-count)return;
        auto& actor=actors_[owner];const auto aim=actor.aim_facing;BattleMvpVec muzzle{actor.position[0]+aim.x*(config_.actor_radius+12),actor.position[1]+config_.actor_height*.6f,actor.position[2]+aim.z*(config_.actor_radius+12)};
        if(muzzles&&(*muzzles)[owner])muzzle=*(*muzzles)[owner];
        const auto& target=actors_[1-owner];const f32 dx=target.position[0]-muzzle[0],dy=target.position[1]+config_.actor_height*.6f-muzzle[1],dz=target.position[2]-muzzle[2],horizontal=std::sqrt(dx*dx+dz*dz);
        const f32 speed=weapon==BattleMvpWeapon::EnergyBeam?60:weapon==BattleMvpWeapon::Bullet?34:weapon==BattleMvpWeapon::Shuriken?24:10;
        const f32 pitch=weapon==BattleMvpWeapon::Bomb?10:std::clamp(speed*dy/std::max(horizontal,1.f),-speed,speed);
        for(unsigned n=0;n<count;++n){const f32 spread=count==3?(f32(n)-1)*.11f:0;const f32 c=std::cos(spread),s=std::sin(spread);
            for(auto& shot:projectiles_)if(!shot.active){shot={true,++next_id_,u8(owner),weapon,muzzle,muzzle,{speed*(aim.x*c-aim.z*s),pitch,speed*(aim.z*c+aim.x*s)},weapon==BattleMvpWeapon::Bomb?70u:120u,false};++shots_;break;}}
        actor.cooldown[secondary?1:0]=weapon==BattleMvpWeapon::Bomb?120:weapon==BattleMvpWeapon::EnergyBeam?18:24;actor.attack_ticks=10;
    }
    void damage(unsigned target,unsigned amount){auto& actor=actors_[target];actor.health-=std::min(actor.health,amount);actor.hit_flash=8;++hits_;}
    void impact(const BattleMvpVec& position,bool explosion){impacts_[impact_cursor_]={position,18,explosion};impact_cursor_=(impact_cursor_+1)%impacts_.size();}
    BattleMvpConfig config_;std::array<BattleMvpActor,2> actors_{};std::array<BattleMvpProjectile,max_projectiles> projectiles_{};
    std::array<BattleMvpImpact,16> impacts_{};std::array<GamePadState,2> previous_{};unsigned impact_cursor_=0;u32 tick_=0,next_id_=0,shots_=0,hits_=0;
    BattleMvpResult result_=BattleMvpResult::Fighting;bool ready_=false,paused_=false;
    std::shared_ptr<const NativeBorgMotionScope> motion_scope_;NativeBorgMotionOffer last_offer_;NativeBorgMotionReceipt last_receipt_;
    std::weak_ptr<const NativeBorgMotionProducer> motion_producer_;
};
}
