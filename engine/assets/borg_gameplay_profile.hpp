#pragma once
#include <optional>
#include "engine/assets/borg_catalog_profile.hpp"
#include <cmath>

namespace gf {
inline constexpr unsigned kBorgBattleMoves=8,kBorgBattleStocks=8,kBorgBattleAttacks=16,kBorgBattleReleases=8,kBorgBattleVolley=8;
inline constexpr unsigned kBorgBattlePullUnits=40;
inline constexpr unsigned kBorgBattleEmitters=4;
inline constexpr unsigned kBorgBattleAnchorSets=2,kBorgBattleAnchors=8;
inline constexpr unsigned kBorgBattleMovementFx=8;
inline constexpr unsigned kBorgBattleHoldTicks=600,kBorgBattleRepeatCycles=16;
inline constexpr std::size_t kBorgGameplayBytes=16u<<10;
enum class BorgBattleMovement:u8 {Grounded,NormalGRed,Stationary};
enum class BorgBattleButton:u8 {B,X};
enum class BorgBattleContext:u8 {Any,Grounded,Airborne};
enum class BorgBattleActivation:u8 {Press,ReleaseWindow,Tap,DoubleTap};
enum class BorgAttackVisual:u8 {Bullet,Beam,Shuriken};
struct BorgBattleSocket {u8 bone=0;std::array<f32,3> local{};bool aim_height=false;};
struct BorgProjectileLook {u16 size=0;std::array<u8,3> rgb{};};
enum class BorgMovementCue:u8 {Takeoff=1,AirJump=2,Landing=4,Rise=8};
inline constexpr std::array<std::string_view,4> kBorgMovementCueNames{"takeoff","air-jump","landing","rise"};
enum class BorgMovementLook:u8 {Jet,Puff};
struct BorgMovementFx {
    std::string id;BorgMovementCue event=BorgMovementCue::Takeoff;BorgMovementLook style=BorgMovementLook::Puff;
    u8 bone=0,duration=0;u16 size=0;std::array<f32,3> local{};
};
enum class BorgProjectileFloor:u8 {Ignore,Retire,Land,StageRetire};
enum class BorgBlastTrigger:u8 {Actor,Floor,Impact,Expiry,Remote,Stage};
enum class BorgProjectileReturn:u8 {None,Actor,Floor,Expiry,Remote,Stage};
enum class BorgGuidanceTarget:u8 {Snapshot,Tracking};
struct BorgBattleGuidance {BorgGuidanceTarget target=BorgGuidanceTarget::Snapshot;f32 turn=0;u16 ticks=0;};
struct BorgBattleRewindHit {u8 set=0,speed=0,fallback=255;};
struct BorgBattleBlast {
    BorgBlastTrigger trigger=BorgBlastTrigger::Actor;
    u16 radius=0,damage=0;
    bool include_owner=false;
};
struct BorgBattleClip {u8 group=0,choice=0;};
struct BorgBattleStockDefinition {
    std::string id;u16 capacity=0,refill_ticks=0; // zero is explicit return-only stock
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
    std::optional<BorgBattleRewindHit> rewind_hit;
    u8 plant_on_stage=255;
    std::optional<BorgBattleSocket> socket;
    std::optional<BorgProjectileLook> look;
};
inline bool borg_projectile_return_reachable(const BorgBattleProjectileDefinition& attack,BorgProjectileReturn reason){
    switch(reason){
    case BorgProjectileReturn::None:case BorgProjectileReturn::Expiry:return true;
    case BorgProjectileReturn::Actor:return !attack.ignore_actors;
    case BorgProjectileReturn::Floor:return attack.floor==BorgProjectileFloor::Retire;
    case BorgProjectileReturn::Remote:return attack.blast&&attack.blast->trigger==BorgBlastTrigger::Remote;
    case BorgProjectileReturn::Stage:return attack.floor==BorgProjectileFloor::StageRetire;
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
struct BorgBattleRelease {u16 frame=0;u8 stock=255,cost=0,projectile=0;};
struct BorgBattleDetonation {u16 frame=0;u8 projectile=0;bool landed_only=false;};
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
struct BorgBattleAnchorSet {std::string id;u8 capacity=0;};
struct BorgBattleAnchorMarker {u16 frame=0;u8 set=0,stock=255,cost=0,rewind_speed=0;};
struct BorgBattleMove {
    std::string id;BorgBattleButton button=BorgBattleButton::B;BorgBattleContext context=BorgBattleContext::Any;
    u16 priority=0,duration=0;BorgBattleClip clip;
    u16 pose_start=0;
    std::vector<BorgBattleRelease> releases;
    BorgBattleActivation activation=BorgBattleActivation::Press;
    u16 minimum_hold=0,maximum_hold=0,tap_window=0;
    u8 hold_cycles=0; // maximum total cycles including entry, zero means no repeat
    f32 entry_range=0; // zero is no proximity predicate; not a contact observation
    std::optional<BorgBattleMelee> melee;
    std::optional<BorgBattleHeldFire> held_fire;
    std::optional<BorgBattleDetonation> detonation;
    std::optional<BorgBattleGuard> guard;
    std::vector<BorgBattleEmitterDeploy> emitters;
    std::vector<BorgBattleAnchorMarker> anchors;
    unsigned last_marker()const{
        if(held_fire)return duration-1; // every frame in the presentation loop must exist
        unsigned deploy=0;
        for(const auto& marker:emitters)deploy=std::max<unsigned>(deploy,marker.frame);
        for(const auto& marker:anchors)deploy=std::max<unsigned>(deploy,marker.frame);
        return std::max<unsigned>({releases.empty()?0u:unsigned(releases.back().frame),melee?melee->last:0u,deploy,
                                   detonation?detonation->frame:0u,guard?guard->last:0u});
    }
};
struct BorgGameplayProfile {
    std::string variant,rig_prefix,pose_profile;
    BorgBattleMovement movement=BorgBattleMovement::Grounded;
    u16 health=0;f32 walk=0,jump=0,dash=0;
    u8 air_jump_points=0;
    f32 flight_lift=0;u16 flight_ticks=0;
    // idle, walk, airborne, dash; stationary only uses idle. Source movement
    // supplies all lower-body poses itself.
    std::array<BorgBattleClip,4> locomotion{};
    std::vector<BorgBattleStockDefinition> stocks;
    std::vector<BorgBattleProjectileDefinition> projectiles;
    std::vector<BorgBattleMove> moves;
    std::vector<BorgBattleEmitter> emitters;
    std::vector<BorgBattleAnchorSet> anchor_sets;
    std::vector<BorgMovementFx> movement_fx;
    std::array<std::string,4> movement_sound{};
};
inline bool borg_battle_emission_valid(const BorgGameplayProfile& p,u8 stock,u8 cost,u8 projectile){
    if(projectile>=p.projectiles.size()||(cost?(stock>=p.stocks.size()||cost>p.stocks[stock].capacity):stock!=255))return false;
    const auto& shot=p.projectiles[projectile];
    const bool returns=shot.stock_return!=BorgProjectileReturn::None;
    const bool return_only=cost&&!p.stocks[stock].refill_ticks;
    // Each successfully created carrier may return exactly one paid unit.
    return returns==return_only&&(!returns||cost==shot.count)&&
        (!shot.rewind_hit||shot.rewind_hit->fallback==255||!return_only);
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
    if(!borg_gameplay_detail::id(p.variant)||!p.health||p.health>10000||unsigned(p.movement)>2||
       p.moves.size()>kBorgBattleMoves||p.stocks.size()>kBorgBattleStocks||p.projectiles.size()>kBorgBattleAttacks||
       p.emitters.size()>kBorgBattleEmitters||
       p.anchor_sets.size()>kBorgBattleAnchorSets||(!p.anchor_sets.empty()&&p.movement!=BorgBattleMovement::Grounded)||
       p.movement_fx.size()>kBorgBattleMovementFx||(!p.movement_fx.empty()&&p.movement==BorgBattleMovement::Stationary)||
       p.air_jump_points>8||(p.air_jump_points&&p.movement!=BorgBattleMovement::Grounded)||
       (p.flight_ticks?(p.movement!=BorgBattleMovement::Grounded||p.air_jump_points||p.flight_ticks>120||
                        !finite(p.flight_lift,2,16)):p.flight_lift!=0)||
       p.moves.empty()!=(p.movement==BorgBattleMovement::Stationary))return false;
    if(p.movement==BorgBattleMovement::Grounded&&(!finite(p.walk,1,16)||!finite(p.jump,2,24)||!finite(p.dash,p.walk,32)))return false;
    if(p.movement==BorgBattleMovement::Stationary&&(!p.stocks.empty()||!p.projectiles.empty()))return false;
    for(const auto& c:p.locomotion)if(!clip(c))return false;
    for(const auto& s:p.stocks)if(!s.capacity||s.capacity>99||s.refill_ticks>3600)return false;
    for(const auto& a:p.projectiles)
        if(unsigned(a.visual)>2||unsigned(a.floor)>3||!a.count||a.count>kBorgBattleVolley||!finite(a.speed,1,140)||
           (a.socket&&(a.socket->bone>=32||!std::all_of(a.socket->local.begin(),a.socket->local.end(),
                                                     [&](f32 v){return finite(v,-500,500); })))||
           (a.look&&(!a.look->size||a.look->size>180))||
           !finite(a.rise,-100,100)||!finite(a.gravity,0,8)||!finite(a.spread,0,.8f)||!finite(a.radius,1,60)||
           !a.lifetime||a.lifetime>600||(!a.damage&&!a.blast&&a.plant_on_stage==255&&!a.rewind_hit)||a.damage>10000||
           (a.ignore_actors&&(a.damage||a.walk_ticks||a.pull_units))||!borg_projectile_return_reachable(a,a.stock_return)||
           (a.guidance&&(unsigned(a.guidance->target)>1||!finite(a.guidance->turn,.001f,1.f)||
                        !a.guidance->ticks||a.guidance->ticks>a.lifetime))||
           (a.rewind_hit&&(p.movement!=BorgBattleMovement::Grounded||a.ignore_actors||a.pull_units||
                         a.count!=1||!a.rewind_hit->speed||a.rewind_hit->speed>16||a.rewind_hit->set>=p.anchor_sets.size()||
                         (a.rewind_hit->fallback!=255&&(a.rewind_hit->fallback>=p.projectiles.size()||
                          p.projectiles[a.rewind_hit->fallback].rewind_hit||p.projectiles[a.rewind_hit->fallback].plant_on_stage!=255||
                          p.projectiles[a.rewind_hit->fallback].stock_return!=BorgProjectileReturn::None))))||
           (a.plant_on_stage!=255&&(a.plant_on_stage>=p.anchor_sets.size()||a.floor!=BorgProjectileFloor::StageRetire||
                                   a.rewind_hit||a.count!=1))||
           (a.blast&&(!a.blast->radius||a.blast->radius>1200||!a.blast->damage||a.blast->damage>10000||
                      (a.blast->include_owner&&p.movement==BorgBattleMovement::NormalGRed)||
                      !borg_blast_trigger_reachable(a,a.blast->trigger)))||
           a.pull_units>kBorgBattlePullUnits||(a.pull_units&&p.movement!=BorgBattleMovement::Grounded)||
           (a.walk_ticks?(a.walk_ticks>600||a.walk_percent<25||a.walk_percent>200):a.walk_percent!=0))return false;
    for(const auto& e:p.emitters){
        if(e.watches>=p.moves.size()||e.projectile>=p.projectiles.size()||
           (p.moves[e.watches].releases.empty()&&!p.moves[e.watches].held_fire)||
           p.projectiles[e.projectile].stock_return!=BorgProjectileReturn::None||
           p.projectiles[e.projectile].rewind_hit||p.projectiles[e.projectile].plant_on_stage!=255||p.projectiles[e.projectile].socket||
           !finite(e.side,-120,120)||!finite(e.height,0,240)||!finite(e.behind,0,180)||!e.lifetime||e.lifetime>600)
            return false;
    }
    for(const auto& set:p.anchor_sets)if(!set.capacity||set.capacity>kBorgBattleAnchors)return false;
    for(const auto& path:p.movement_sound)if(!path.empty()&&
        (p.movement==BorgBattleMovement::Stationary||!borg_package_asset_path_valid(path)))return false;
    for(const auto& fx:p.movement_fx){
        const auto event=u8(fx.event);
        if((event!=1&&event!=2&&event!=4&&event!=8)||unsigned(fx.style)>1||!fx.size||fx.size>180||fx.bone>=32||
           (fx.event==BorgMovementCue::Rise?fx.duration!=0:(!fx.duration||fx.duration>60))||
           !std::all_of(fx.local.begin(),fx.local.end(),[&](f32 x){return finite(x,-500,500);}))return false;
    }
    std::array<const BorgBattleMove*,2> gestures{};
    for(const auto& m:p.moves){
        const bool multi=m.activation==BorgBattleActivation::Tap||m.activation==BorgBattleActivation::DoubleTap;
        if(m.duration<2||m.duration>600||(m.releases.empty()&&!m.melee&&!m.held_fire&&!m.detonation&&!m.guard&&m.emitters.empty()&&m.anchors.empty())||
           m.releases.size()>kBorgBattleReleases||
           unsigned(m.button)>1||unsigned(m.context)>2||!clip(m.clip)||m.pose_start>600||
           !finite(m.entry_range,0,1200)||
           unsigned(m.activation)>3||
           (m.activation==BorgBattleActivation::ReleaseWindow?(m.minimum_hold>m.maximum_hold||m.maximum_hold>kBorgBattleHoldTicks||m.tap_window):
             multi?(m.minimum_hold||!m.maximum_hold||m.maximum_hold>60||!m.tap_window||m.tap_window>60):
                   (m.minimum_hold||m.maximum_hold||m.tap_window))||
           (m.hold_cycles&&(m.hold_cycles<2||m.hold_cycles>kBorgBattleRepeatCycles||m.activation!=BorgBattleActivation::Press||m.melee||
                             m.releases.size()!=1||!m.releases.front().cost)))return false;
        if(m.held_fire){
            const auto& h=*m.held_fire;
            if(!m.releases.empty()||m.melee||m.detonation||m.guard||!m.emitters.empty()||!m.anchors.empty()||
               m.activation!=BorgBattleActivation::Press||m.hold_cycles||
               !h.warmup||h.warmup>600||!h.fast_interval||h.fast_interval>h.slow_interval||h.slow_interval>600||
               h.cooldown>600||!borg_battle_emission_valid(p,h.stock,h.cost,h.projectile))return false;
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
               a.rewind_speed>16||(a.rewind_speed&&a.cost)||
               (a.cost?(a.stock>=p.stocks.size()||!p.stocks[a.stock].refill_ticks||a.cost>p.stocks[a.stock].capacity):a.stock!=255))
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
        if(m.melee){
            const auto& a=*m.melee;
            if(!a.first||a.last<a.first||a.last>=m.duration||!a.damage||a.damage>10000||
               !finite(a.reach,1,220)||!finite(a.radius,1,40)||!finite(a.height,-200,200)||!finite(a.half_height,1,100))return false;
        }
        unsigned prior=0;
        for(const auto& r:m.releases){
            if(r.frame<=prior||r.frame>=m.duration||!borg_battle_emission_valid(p,r.stock,r.cost,r.projectile))return false;
            prior=r.frame;
        }
    }
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
    struct HitRef {unsigned line;std::string projectile;bool pull=false;u16 amount=0,ticks=0;};
    struct InputRef {unsigned line;std::string move;BorgBattleActivation activation;u16 first=0,second=0;};
    struct MeleeRef {unsigned line;std::string move;BorgBattleMelee shape;};
    struct RangeRef {unsigned line;std::string move;f32 distance;};
    struct HeldFireRef {unsigned line;std::string move,stock,projectile;BorgBattleHeldFire spec;};
    struct BlastRef {unsigned line;std::string projectile;BorgBattleBlast spec;};
    struct ActorContactRef {unsigned line;std::string projectile;};
    struct DetonationRef {unsigned line;std::string move,projectile;u16 frame=0;bool landed_only=false;};
    struct ReturnRef {unsigned line;std::string projectile;BorgProjectileReturn reason;};
    struct GuardRef {unsigned line;std::string move;BorgBattleGuard spec;};
    struct EmitterRef {unsigned line;std::string watches,projectile;BorgBattleEmitter spec;};
    struct DeployRef {unsigned line;std::string move,emitter;u16 frame=0;};
    struct AnchorRef {unsigned line;std::string move,set,stock;BorgBattleAnchorMarker marker;};
    struct RewindHitRef {unsigned line;std::string projectile,set;u8 speed;};
    struct StageAnchorRef {unsigned line;std::string projectile,set;};
    struct AnchorFallbackRef {unsigned line;std::string projectile,fallback;};
    struct SocketRef {unsigned line;std::string projectile;BorgBattleSocket spec;};
    struct LookRef {unsigned line;std::string projectile;BorgProjectileLook spec;};
    struct PoseStartRef {unsigned line;std::string move;u16 frame;};
    struct GuidanceRef {unsigned line;std::string projectile;BorgBattleGuidance spec;};
    std::vector<ReleaseRef> refs;
    std::vector<HitRef> hits;
    std::vector<InputRef> inputs;
    std::vector<MeleeRef> melee;
    std::vector<RangeRef> ranges;
    std::vector<HeldFireRef> held_fire;
    std::vector<BlastRef> blasts;
    std::vector<ActorContactRef> passes;
    std::vector<DetonationRef> detonations;
    std::vector<ReturnRef> returns;
    std::vector<GuardRef> guards;
    std::vector<EmitterRef> emitters;
    std::vector<DeployRef> deployments;
    std::vector<AnchorRef> anchors;
    std::vector<RewindHitRef> rewind_hits;
    std::vector<StageAnchorRef> stage_anchors;
    std::vector<AnchorFallbackRef> anchor_fallbacks;
    std::vector<SocketRef> sockets;
    std::vector<LookRef> looks;
    std::vector<PoseStartRef> starts;
    std::vector<GuidanceRef> guidance;
    BorgGameplayProfile next;bool has_movement=false;
    unsigned locomotion=0;
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
        }else if(key=="health"){
            if(next.health||!number(value,1,10000,&next.health))return fail("invalid or duplicate health");
        }else if(key=="powered-flight"){
            std::array<std::string_view,2> a;
            if(next.flight_ticks||!split_fields(value,&a)||!scalar(a[0],2,16,&next.flight_lift)||
               !number(a[1],1,120,&next.flight_ticks))return fail("invalid or duplicate powered-flight policy");
        }else if(key=="air-jump-points"){
            u16 count=0;
            if(next.air_jump_points||!number(value,1,8,&count))return fail("invalid or duplicate air-jump count");
            next.air_jump_points=u8(count);
        }else if(key=="movement"){
            if(has_movement)return fail("duplicate movement");
            has_movement=true;
            if(value=="normal-gred-gfmm9")next.movement=BorgBattleMovement::NormalGRed;
            else if(value=="stationary-v1")next.movement=BorgBattleMovement::Stationary;
            else{
                std::array<std::string_view,4> a;
                if(!split_fields(value,&a)||a[0]!="grounded-v1"||!scalar(a[1],1,16,&next.walk)||
                   !scalar(a[2],2,24,&next.jump)||!scalar(a[3],next.walk,32,&next.dash))return fail("unsupported movement or walk/jump/dash values");
            }
        }else if(key=="idle"||key=="walk"||key=="air"||key=="dash"){
            const unsigned at=key=="idle"?0:key=="walk"?1:key=="air"?2:3;
            if((locomotion&(1u<<at))||!clip(value,&next.locomotion[at]))return fail("invalid or duplicate locomotion clip");
            locomotion|=1u<<at;
        }else if(key=="stock"){
            std::array<std::string_view,3> a;BorgBattleStockDefinition s;
            if(!split_fields(value,&a)||!id(a[0])||index(next.stocks,a[0])!=next.stocks.size()||
               next.stocks.size()>=kBorgBattleStocks||!number(a[1],1,99,&s.capacity)||
               (a[2]!="on-return"&&!number(a[2],1,3600,&s.refill_ticks)))
                return fail("invalid or duplicate stock");
            s.id=a[0];next.stocks.push_back(std::move(s));
        }else if(key=="projectile"){
            std::array<std::string_view,11> a;BorgBattleProjectileDefinition p;u16 count=0;
            if(!split_fields(value,&a)||!id(a[0])||index(next.projectiles,a[0])!=next.projectiles.size()||
               next.projectiles.size()>=kBorgBattleAttacks||(a[1]!="bullet"&&a[1]!="beam"&&a[1]!="shuriken")||
               !number(a[2],1,kBorgBattleVolley,&count)||
               !scalar(a[3],1,140,&p.speed)||!scalar(a[4],-100,100,&p.rise)||!scalar(a[5],0,8,&p.gravity)||
               !scalar(a[6],0,.8f,&p.spread)||!scalar(a[7],1,60,&p.radius)||!number(a[8],1,600,&p.lifetime)||
               !number(a[9],0,10000,&p.damage)||(a[10]!="ignore-floor"&&a[10]!="stop-on-floor"&&a[10]!="land-on-floor"&&a[10]!="stop-on-stage"))
                return fail("invalid or duplicate projectile");
            p.id=a[0];p.visual=a[1]=="beam"?BorgAttackVisual::Beam:a[1]=="shuriken"?BorgAttackVisual::Shuriken:BorgAttackVisual::Bullet;
            p.count=u8(count);p.floor=a[10]=="land-on-floor"?BorgProjectileFloor::Land:
                a[10]=="stop-on-floor"?BorgProjectileFloor::Retire:
                a[10]=="stop-on-stage"?BorgProjectileFloor::StageRetire:BorgProjectileFloor::Ignore;
            next.projectiles.push_back(std::move(p));
        }else if(key=="blast"){
            std::array<std::string_view,5> a;BorgBattleBlast spec;
            if(blasts.size()>=kBorgBattleAttacks||!split_fields(value,&a)||!id(a[0])||
               (a[1]!="actor"&&a[1]!="floor"&&a[1]!="stage"&&a[1]!="impact"&&a[1]!="expiry"&&a[1]!="remote")||
               !number(a[2],1,1200,&spec.radius)||!number(a[3],1,10000,&spec.damage)||
               (a[4]!="opponent"&&a[4]!="all"))return fail("invalid blast trigger, radius, damage or recipients");
            spec.trigger=a[1]=="actor"?BorgBlastTrigger::Actor:a[1]=="floor"?BorgBlastTrigger::Floor:
                a[1]=="impact"?BorgBlastTrigger::Impact:a[1]=="expiry"?BorgBlastTrigger::Expiry:
                a[1]=="stage"?BorgBlastTrigger::Stage:BorgBlastTrigger::Remote;
            spec.include_owner=a[4]=="all";blasts.push_back({line,std::string(a[0]),spec});
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
               (a[1]!="actor"&&a[1]!="floor"&&a[1]!="stage"&&a[1]!="expiry"&&a[1]!="remote"))
                return fail("invalid stock-return disposition");
            returns.push_back({line,std::string(a[0]),a[1]=="actor"?BorgProjectileReturn::Actor:
                a[1]=="floor"?BorgProjectileReturn::Floor:a[1]=="expiry"?BorgProjectileReturn::Expiry:
                a[1]=="stage"?BorgProjectileReturn::Stage:BorgProjectileReturn::Remote});
        }else if(key=="actor-contact"){
            std::array<std::string_view,2> a;
            if(passes.size()>=kBorgBattleAttacks||!split_fields(value,&a)||!id(a[0])||a[1]!="ignore")
                return fail("unsupported actor-contact policy");
            passes.push_back({line,std::string(a[0])});
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
        }else if(key=="origin"){
            std::array<std::string_view,6> a;BorgBattleSocket spec;u16 bone=0;
            if(sockets.size()>=kBorgBattleAttacks||!split_fields(value,&a)||!id(a[0])||!number(a[1],0,31,&bone)||
               !scalar(a[2],-500,500,&spec.local[0])||!scalar(a[3],-500,500,&spec.local[1])||!scalar(a[4],-500,500,&spec.local[2])||
               (a[5]!="fixed"&&a[5]!="target-height"))
                return fail("invalid launch bone or local offset");
            spec.bone=u8(bone);spec.aim_height=a[5]=="target-height";sockets.push_back({line,std::string(a[0]),spec});
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
               (a[3]!="any"&&a[3]!="grounded"&&a[3]!="airborne")||!number(a[4],2,600,&m.duration)||!clip(a[5],&m.clip)||
               std::any_of(next.moves.begin(),next.moves.end(),[&](const auto& move){return move.priority==m.priority;}))
                return fail("invalid move or ambiguous priority");
            m.id=a[0];m.button=a[1]=="b"?BorgBattleButton::B:BorgBattleButton::X;
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
        }else if(key=="pose-start"){
            std::array<std::string_view,2> a;u16 frame=0;
            if(starts.size()>=kBorgBattleMoves||!split_fields(value,&a)||!id(a[0])||!number(a[1],1,600,&frame))
                return fail("invalid pose start");
            starts.push_back({line,std::string(a[0]),frame});
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
        }else if(key=="detonate"){
            std::array<std::string_view,4> a;u16 frame=0;
            if(detonations.size()>=kBorgBattleMoves||!split_fields(value,&a)||!id(a[0])||!number(a[1],1,599,&frame)||
               !id(a[2])||(a[3]!="any"&&a[3]!="landed"))return fail("invalid detonation marker or selector");
            detonations.push_back({line,std::string(a[0]),std::string(a[2]),frame,a[3]=="landed"});
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
    }else if(next.rig_prefix.empty()||next.pose_profile.empty()||locomotion!=(movement==BorgBattleMovement::Stationary?1u:15u))
        return fail("exact rig and locomotion clip bindings required");
    if(next.moves.empty()!=(movement==BorgBattleMovement::Stationary))return fail("only a stationary target has no moves");
    if(movement==BorgBattleMovement::Stationary&&(!next.stocks.empty()||!next.projectiles.empty()))return fail("stationary target cannot declare stock or attacks");
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
        if(pi==next.projectiles.size()||next.projectiles[pi].ignore_actors)return fail("unknown projectile or duplicate actor-contact");
        auto& a=next.projectiles[pi];
        if(a.damage||a.pull_units||a.walk_ticks)return fail("ignoring actor contact cannot also apply direct-hit effects");
        a.ignore_actors=true;
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
    for(const auto& r:returns){
        line=r.line;const auto pi=index(next.projectiles,r.projectile);
        if(pi==next.projectiles.size()||next.projectiles[pi].stock_return!=BorgProjectileReturn::None)
            return fail("unknown projectile or duplicate stock return");
        auto& shot=next.projectiles[pi];
        if(!borg_projectile_return_reachable(shot,r.reason))return fail("stock return cannot occur under the projectile's lifetime policy");
        shot.stock_return=r.reason;
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
    for(const auto& r:sockets){
        line=r.line;const auto pi=index(next.projectiles,r.projectile);
        if(pi==next.projectiles.size()||next.projectiles[pi].socket)return fail("unknown projectile or duplicate origin");
        next.projectiles[pi].socket=r.spec;
    }
    for(const auto& r:looks){
        line=r.line;const auto pi=index(next.projectiles,r.projectile);
        if(pi==next.projectiles.size()||next.projectiles[pi].look)return fail("unknown projectile or duplicate look");
        next.projectiles[pi].look=r.spec;
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
        if(!borg_battle_emission_valid(next,release.stock,release.cost,release.projectile))
            return fail("stock return requires one unit from return-only stock per carrier");
        m.releases.push_back(release);
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
    for(auto& a:anchors){
        line=a.line;
        const auto mi=index(next.moves,a.move),si=index(next.anchor_sets,a.set),pi=index(next.stocks,a.stock);
        if(mi==next.moves.size()||si==next.anchor_sets.size()||(a.marker.cost&&(pi==next.stocks.size()||
           !next.stocks[pi].refill_ticks||a.marker.cost>next.stocks[pi].capacity)))return fail("unresolved anchor marker or stock pool");
        auto& m=next.moves[mi];
        if(m.held_fire||m.hold_cycles||m.anchors.size()>=kBorgBattleAnchors||a.marker.frame>=m.duration||
           (!m.anchors.empty()&&a.marker.frame<=m.anchors.back().frame))return fail("duplicate or incompatible anchor marker");
        a.marker.set=u8(si);a.marker.stock=u8(a.marker.cost?pi:255);m.anchors.push_back(a.marker);
    }
    for(const auto& m:next.moves)if(m.releases.empty()&&!m.melee&&!m.held_fire&&!m.detonation&&!m.guard&&m.emitters.empty()&&m.anchors.empty())
        return fail("move needs an executable marker or behavior: "+m.id);
    if(!borg_gameplay_profile_valid(next))return fail("invalid executable definition");
    *out=std::move(next);if(error)error->clear();return true;
}
}
