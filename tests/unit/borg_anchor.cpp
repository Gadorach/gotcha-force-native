#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/native_battle_mvp.hpp"

namespace {
using namespace gf;
constexpr std::string_view wire=R"(GFBORG-GAMEPLAY 1
variant=normal
health=180
movement=grounded-v1|7|13|14
rig-prefix=body.prefix
pose-profile=body.pose
idle=0:0
walk=0:1
air=0:7
dash=0:2
stock=wire-stock|4|150
anchor-set=wire|2
move=place|x|20|any|4|0:1
place-anchor=place|1|wire|wire-stock|1
move=back|b|10|grounded|4|0:2
rewind=back|1|wire|8
)";
constexpr std::string_view dummy=R"(GFBORG-GAMEPLAY 1
variant=normal
health=500
movement=stationary-v1
rig-prefix=body.prefix
pose-profile=body.pose
idle=0:0
)";
void check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"Borg anchors: %s\n",why);std::abort();}}
std::shared_ptr<const BorgGameplayProfile> parse(std::string_view text){
    BorgGameplayProfile out;std::string error;
    if(!borg_read_gameplay_profile(text,&out,&error)){std::fputs(error.c_str(),stderr);std::abort();}
    return std::make_shared<const BorgGameplayProfile>(std::move(out));
}
struct Terrain final:IBattleMvpTerrain {
    bool boundary=false,raised_target=false;
    bool stage_ready=false,stage_readable=true,stage_hit=true;
    bool height(f32,f32 z,f32,f32* out)const override{
        *out=raised_target&&z>300?300.f:boundary&&z>4&&z<20?22.f:0.f;return true;
    }
    bool supports_projectile_stage()const override{return stage_ready;}
    bool projectile_stage(const std::array<f32,3>& start,const std::array<f32,3>& end,NativeBorgStageProjectileHit* out)const override{
        if(!stage_readable)return false;
        *out={};
        if(stage_hit&&start[1]>0&&end[1]<=0)*out={true,start[1]/(start[1]-end[1]),1,42};
        return true;
    }
};
}
int main(){
    using enum BorgBattleEventKind;
    const auto profile=parse(wire);BorgGameplayProfile invalid;std::string error;
    for(auto row:{"anchor-set=wire|4","anchor-set=too-many|9","place-anchor=place|1|wire|none|0",
                  "rewind=back|3|wire|17","place-anchor=place|2|unknown|none|0","rewind=none|1|wire|8"})
        check(!borg_read_gameplay_profile(std::string(wire)+row,&invalid,&error),"invalid marker or capacity refuses");
    NativeBattleMvp world;BattleMvpConfig config;Terrain terrain;
    config.packages={profile,parse(dummy)};config.spawn={BattlePoint{},BattlePoint{0,900}};
    const auto step=[&](u8 pressed=0,bool travel=false,bool jump=false){
        GamePadState pad;pad.dpad_down=travel;pad.b=jump;
        check(world.step({pad,{}},false,&terrain,nullptr,nullptr,nullptr,{BorgBattleInputs{pressed,0,0},{}}),"native tick");
    };
    const auto seen=[&](BorgBattleEventKind kind){
        const auto events=world.package_events();
        return std::any_of(events.begin(),events.end(),[&](auto e){return e.kind==kind;});
    };
    const auto anchors=[&]{
        step(BorgBattleInputs::x);
        check(!world.anchor_paths()[0][0].count&&world.package_action(0).stocks[0].current==4,"press does not store a point");
        step();
        check(seen(AnchorPlaced)&&world.anchor_paths()[0][0].count==1,"first marker stores accepted position");
        for(unsigned i=0;i<10;++i)step(0,true);
        step(BorgBattleInputs::x,true);step(0,true);
        check(seen(AnchorPlaced)&&world.anchor_paths()[0][0].count==2,"second marker appends rather than overwrites");
        for(unsigned i=0;i<8;++i)step(0,true);
        step(BorgBattleInputs::x);step();
        check(seen(AnchorRefused)&&world.package_action(0).stocks[0].current==2,"full set refuses before taking stock");
        for(unsigned i=0;i<6;++i)step();
        return world.anchor_paths()[0][0];
    };
    check(world.start(config,&terrain),"start flat fixture");
    const auto path=anchors();
    check(path.points[0].position==BattleMvpVec{}&&path.points[1].position[2]>20.f,"uneven anchors stored in world space");
    step(BorgBattleInputs::b);step();
    check(seen(RewindQueued)&&!world.anchor_paths()[0][0].count&&!seen(RewindReached),"mark consumes exact anchor set but does not move yet");
    std::vector<u32> visited;
    for(unsigned i=0;i<24&&!seen(RewindCompleted);++i){
        const auto old=world.actors()[0].position;
        step();
        check(std::abs(world.actors()[0].position[2]-old[2])<8.001f,"per-tick travel cap");
        for(auto e:world.package_events())if(e.kind==RewindReached)visited.push_back(e.object);
    }
    check(seen(RewindCompleted)&&visited==std::vector{path.points[1].id,path.points[0].id}&&
        std::abs(world.actors()[0].position[2])<.001f&&!world.shots_spawned(),"reverse traversal of real roots, without producing attacks");
    check(world.start(config,&terrain),"partial path test");
    const auto blocked=anchors();terrain.boundary=true;
    step(BorgBattleInputs::b);step();
    visited.clear();
    for(unsigned i=0;i<24&&!seen(RewindStopped);++i){
        const auto old=world.actors()[0].position;
        step();
        for(auto e:world.package_events())if(e.kind==RewindReached)visited.push_back(e.object);
        if(seen(RewindStopped))check(world.actors()[0].position==old,"refused step leaves last accepted root");
    }
    check(seen(RewindStopped)&&visited==std::vector{blocked.points[1].id}&&world.actors()[0].position[2]>=20,
          "route stops at unsupported older segment instead of skipping to oldest");

    terrain.boundary=false;
    check(world.start(config,&terrain),"airborne anchor test");
    step(BorgBattleInputs::x,false,true);step();
    check(seen(AnchorPlaced)&&world.anchor_paths()[0][0].points[0].position[1]>0,"placement supports a midair accepted pose");
    for(unsigned i=0;i<70;++i)step();
    step(BorgBattleInputs::b);step();step();
    check(seen(RewindStopped)&&!seen(RewindReached)&&world.actors()[0].position[1]==0,"midair destination refuses without teleporting");
    check(world.start(config,&terrain),"restart ownership fixture");
    anchors();step(BorgBattleInputs::b);step();
    check(seen(RewindQueued)&&world.start(config,&terrain),"replace round with traversal pending");
    step();
    check(world.package_events().empty()&&world.actors()[0].position==BattleMvpVec{}&&world.package_action(0).stocks[0].current==4,
        "old anchor path cannot move the new actor or modify stock");
    const std::string wire_shot=std::string(wire)+
        "projectile=link|beam|1|11|0|0|0|8|60|9|ignore-floor\non-hit=link|rewind-recipient|wire|16\n"
        "move=link-release|b|30|grounded|5|0:1\nrelease=link-release|1|none|0|link\n";
    const auto carrier=parse(wire_shot);
    for(auto row:{"on-hit=link|rewind-recipient|wire|12","on-hit=link|pull-self|6",
                  "on-hit=link|rewind-recipient|no-set|12","emitter=copy|link-release|link|0|50|15|10"})
        check(!borg_read_gameplay_profile(wire_shot+row,&invalid,&error),"conflicting or unqualified carrier rejects");
    config.packages={carrier,profile};config.spawn[1]={0,360};
    check(world.start(config,&terrain),"recipient route requires a grounded authored target");
    step(BorgBattleInputs::b);step();
    check(seen(AnchorRefused)&&!world.shots_spawned()&&world.package_action(0).stocks[0].current==4,"no path refuses before allocation or debit");
    check(world.start(config,&terrain),"sealed launch fixture");
    const auto original=anchors();
    step(BorgBattleInputs::b);step();
    check(seen(AnchorPathSealed)&&world.shots_spawned()==1&&!world.anchor_paths()[0][0].count&&
          world.projectiles()[0].package_rewind_path.count==2&&!seen(RewindQueued),"successful launch seals the set without moving the recipient");
    const u32 carrier_id=world.projectiles()[0].id;
    const auto wire_step=[&](u8 left=0,u8 right=0,bool side=false){
        GamePadState pad;pad.dpad_right=side;
        check(world.step({pad,{}},false,&terrain,nullptr,nullptr,nullptr,
            {BorgBattleInputs{left,0,0},BorgBattleInputs{right,0,0}}),"wire tick");
    };
    for(unsigned i=0;i<6;++i)wire_step(0,0,true);
    wire_step(BorgBattleInputs::x,0,true);wire_step(0,0,true);
    check(seen(AnchorPlaced)&&world.anchor_paths()[0][0].count==1&&world.projectiles()[0].active,
          "owner can place a new set while the first shot is in flight");
    const auto new_anchor=world.anchor_paths()[0][0].points[0].id;
    visited.clear();bool queued=false,finished=false;
    for(unsigned i=0;i<70&&!finished;++i){
        const auto before=world.actors()[1].position;
        wire_step(0,0,i<18);
        for(auto e:world.package_events()){
            if(e.kind==RewindQueued){
                queued=true;
                check(e.actor==0&&e.recipient==1&&e.object==carrier_id&&e.value==2&&seen(Hit)&&world.actors()[1].position==before,
                      "accepted contact queues, but does not apply, the recipient path");
            }
            if(e.kind==RewindReached){check(e.actor==0&&e.recipient==1,"trace separates caster credit and moved actor");visited.push_back(e.object);}
            finished|=e.kind==RewindCompleted;
        }
    }
    check(queued&&finished&&visited==std::vector{original.points[1].id,original.points[0].id}&&
          world.anchor_paths()[0][0].points[0].id==new_anchor&&world.actors()[1].position[2]<.001f&&world.actors()[0].position[0]>100.f,
          "original recipient trace finishes newest-first, ignores new anchors, and doesn't move caster");

    terrain.raised_target=true;
    check(world.start(config,&terrain),"miss fixture on another floor layer");
    step(BorgBattleInputs::x);for(unsigned i=0;i<4;++i)step();
    step(BorgBattleInputs::b);step();
    for(unsigned i=0;i<61;++i){step();check(!seen(RewindQueued),"miss or expiry never queues recipient movement");}
    check(!world.hits()&&!world.anchor_paths()[0][0].count,"expired carrier does not put the old path back into storage");
    terrain.raised_target=false;
    config.packages[1]=parse(std::string(wire)+"move=guard|x|40|grounded|15|0:1\nguard=guard|1|11|1.0|1|shots\n");
    config.spawn[1]={0,120};
    check(world.start(config,&terrain),"blocked carrier fixture");
    step(BorgBattleInputs::x);for(unsigned i=0;i<4;++i)step();
    wire_step(BorgBattleInputs::b,BorgBattleInputs::x);
    bool blocked_hit=false;
    for(unsigned i=0;i<10;++i){
        wire_step();blocked_hit|=seen(GuardBlocked);
        check(!seen(RewindQueued),"guarded contact cannot apply the path");
    }
    check(blocked_hit&&!world.hits(),"guard blocked the launched carrier");

    const std::string arrow=std::string(wire)+
        "projectile=plant-arrow|bullet|1|30|-80|0|0|8|3|0|stop-on-stage\n"
        "plant-anchor=plant-arrow|wire\nactor-contact=plant-arrow|ignore\n"
        "move=plant|x|40|any|4|0:1\nrelease=plant|1|none|0|plant-arrow\n";
    for(auto row:{"plant-anchor=plant-arrow|wire","plant-anchor=unknown|wire",
                  "on-hit=plant-arrow|rewind-recipient|wire|8","emitter=copy|plant|plant-arrow|0|50|15|10"})
        check(!borg_read_gameplay_profile(arrow+row,&invalid,&error),"stage placement refuses ambiguous authority");
    config.packages={parse(arrow),profile};config.spawn[1]={0,120};
    check(!world.start(config,&terrain),"no stage provider means no admission for a plant arrow");
    terrain.stage_ready=true;
    check(world.start(config,&terrain),"stage provider");
    step(BorgBattleInputs::x);
    check(!world.anchor_paths()[0][0].count,"input alone plants nothing");
    step();
    const auto planted=world.anchor_paths()[0][0].points[0];
    check(seen(StageImpact)&&seen(AnchorPlanted)&&world.anchor_paths()[0][0].count==1&&
          std::abs(planted.position[1])<.001f&&std::abs(planted.position[2]-76.5f)<.001f&&
          !world.hits()&&planted.position!=world.actors()[0].position,"anchor uses accepted stage contact, not muzzle or actor root");
    for(unsigned i=0;i<3;++i)step();
    step(BorgBattleInputs::x);step();
    check(world.anchor_paths()[0][0].count==2,"accepted stage hits append");
    for(unsigned i=0;i<3;++i)step();
    step(BorgBattleInputs::x);step();
    check(seen(StageImpact)&&seen(AnchorRefused)&&!seen(AnchorPlanted)&&world.anchor_paths()[0][0].points[0].id==planted.id,
          "a full set can refuse a valid impact without replacing prior points");
    terrain.stage_readable=false;
    check(world.start(config,&terrain),"unavailable query after admission");
    step(BorgBattleInputs::x);step();
    check(!seen(StageImpact)&&!seen(AnchorPlanted)&&!world.anchor_paths()[0][0].count,"unavailable stage produces no anchor");
    terrain.stage_readable=true;terrain.stage_hit=false;
    check(world.start(config,&terrain),"clear stage miss");
    step(BorgBattleInputs::x);step();step();step();
    check(seen(Retired)&&!seen(AnchorPlanted)&&!world.anchor_paths()[0][0].count,"expiry doesn't create a point");
    auto actor_arrow=arrow;
    actor_arrow.erase(actor_arrow.find("actor-contact=plant-arrow|ignore\n"),std::string_view("actor-contact=plant-arrow|ignore\n").size());
    // Put the recipient inside the first segment's actor envelope so an
    // actor-only contact consumes the carrier without a valid stage point.
    config.spawn[1]={0,91};config.packages[0]=parse(actor_arrow);
    check(world.start(config,&terrain),"actor contact without a stage hit");
    step(BorgBattleInputs::x);step();
    check(seen(ProjectileContact)&&!seen(AnchorPlanted)&&!world.hits()&&!world.anchor_paths()[0][0].count,
          "actor contact cannot supply a terrain anchor");
    auto gesture_text=std::string(wire);
    auto b=gesture_text.find("move=back|b");
    check(b!=gesture_text.npos,"gesture input fixture");
    gesture_text[b+10]='x';
    gesture_text+="activation=place|tap|4|6\nactivation=back|double-tap|4|6\n";
    const auto gesture_profile=parse(gesture_text);
    auto incompatible=gesture_text;
    incompatible.replace(incompatible.find("double-tap|4|6"),14,"double-tap|4|7");
    check(!borg_read_gameplay_profile(incompatible,&invalid,&error),"gesture window must match on the same physical button");
    config.packages={gesture_profile,profile};
    check(world.start(config,&terrain),"double tap fixture");
    const auto input=[&](u8 pressed=0,u8 held=0,u8 released=0){
        check(world.step({},false,&terrain,nullptr,nullptr,nullptr,{BorgBattleInputs{pressed,held,released},{}}),"gesture update");
    };
    constexpr auto x=BorgBattleInputs::x;
    input(x,x);input(0,0,x);
    for(unsigned i=0;i<5;++i){input();check(!seen(Tap)&&!world.anchor_paths()[0][0].count,"first tap stays pending");}
    input();check(seen(Tap)&&!world.anchor_paths()[0][0].count,"single tap selects only after waiting");
    input();check(seen(AnchorPlaced)&&world.anchor_paths()[0][0].count==1,"single tap spends only at its placement marker");
    for(unsigned i=0;i<3;++i)input();
    input(x,x);input(0,0,x);
    for(unsigned i=0;i<4;++i)input();
    input(x,x);input(0,0,x);
    check(seen(DoubleTap)&&!seen(Tap)&&world.anchor_paths()[0][0].count==1,"last permitted second release selects rewind only");
    input();check(seen(RewindQueued)&&!world.anchor_paths()[0][0].count&&world.package_action(0).stocks[0].current==3,
        "double tap cannot spend stock or add a second point");
    check(world.start(config,&terrain),"pending tap lifecycle");
    input(x,x);input(0,0,x);
    check(world.set_paused(true)&&world.set_paused(false),"pause resets gestures");
    for(unsigned i=0;i<8;++i){input();check(!seen(Tap)&&!seen(DoubleTap),"pause cannot replay first tap");}
    input(x,x);input(0,0,x);
    check(world.start(config,&terrain),"restart during a gesture");
    for(unsigned i=0;i<8;++i){input();check(!seen(Tap)&&!seen(DoubleTap),"no pending tap in new round");}
    input(x,x);for(unsigned i=0;i<5;++i)input(0,x);input(0,0,x);
    for(unsigned i=0;i<8;++i){input();check(!seen(Tap)&&!seen(DoubleTap),"long hold is not a short press");}

    const std::string fallback=wire_shot+
        "projectile=plain|beam|1|60|0|0|0|8|40|7|ignore-floor\nempty-anchors=link|plain\n";
    check(!borg_read_gameplay_profile(fallback+"empty-anchors=link|link",&invalid,&error),"no recursive or duplicate fallback");
    check(!borg_read_gameplay_profile(wire_shot+"empty-anchors=link|link",&invalid,&error),"no fallback to another anchor carrier");
    config.packages={parse(fallback),profile};config.spawn[1]={0,120};
    check(world.start(config,&terrain),"empty wire with explicit ordinary shot");
    step(BorgBattleInputs::b);step();
    check(seen(AnchorFallback)&&seen(Hit)&&!seen(AnchorPathSealed)&&!seen(RewindQueued)&&world.actors()[1].health==173&&
          !world.projectiles()[0].package_rewind_path.count&&world.package_action(0).stocks[0].current==4,
          "fallback damages the contacted actor without granting movement or an extra debit");
    std::puts("Borg anchors: stage points, sealed paths, double-tap, empty-path fallback and restart PASS");
}
