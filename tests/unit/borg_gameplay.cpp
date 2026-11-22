#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/native_battle_mvp.hpp"

namespace {
using namespace gf;
constexpr std::string_view player=R"(GFBORG-GAMEPLAY 1
variant=normal
movement=grounded-v1|8|13|14
health=250
rig-prefix=visual/prefix
pose-profile=visual/pose
idle=0:0
walk=0:1
air=0:2
dash=0:3
stock=primary|2|12
projectile=shot|beam|1|60|0|0|0|8|30|19|ignore-floor
projectile=twin|bullet|2|30|5|1|0.06|8|30|10|stop-on-floor
on-hit=twin|walk-speed|25|3
move=b-shot|b|10|any|4|0:4
move=x-volley|x|20|grounded|6|0:5
release=b-shot|2|primary|1|shot
release=x-volley|2|primary|1|twin
release=x-volley|4|none|0|shot
)";
constexpr std::string_view target=R"(GFBORG-GAMEPLAY 1
variant=normal
movement=stationary-v1
health=500
rig-prefix=visual/prefix
pose-profile=visual/pose
idle=0:0
)";
void check(bool ok,const char* name){
    if(!ok){std::fprintf(stderr,"Borg API: %s\n",name);std::abort();}
}
std::shared_ptr<const BorgGameplayProfile> parse(std::string_view text){
    BorgGameplayProfile p;std::string error;
    if(!borg_read_gameplay_profile(text,&p,&error)){std::fputs(error.c_str(),stderr);std::abort();}
    return std::make_shared<const BorgGameplayProfile>(std::move(p));
}
bool has(const NativeBattleMvp& world,BorgBattleEventKind k,unsigned actor=0){
    const auto events=world.package_events();
    return std::any_of(events.begin(),events.end(),[&](auto e){return e.kind==k&&e.actor==actor;});
}
struct PullGround final:IBattleMvpTerrain {
    enum Kind {Flat,Step,Unknown,HighTarget} kind=Flat;
    bool height(f32,f32 z,f32,f32* out)const override{
        if(kind==Unknown&&z>11&&z<81)return false;
        *out=kind==Step&&z>11&&z<81?12.f:kind==HighTarget&&z>140?300.f:0.f;
        return true;
    }
};
struct WallGround final:IBattleMvpTerrain {
    f32 plane=60;bool available=true;
    bool height(f32,f32,f32,f32* out)const override{*out=0;return true;}
    bool supports_projectile_stage()const override{return true;}
    bool projectile_stage(const std::array<f32,3>& start,const std::array<f32,3>& end,NativeBorgStageProjectileHit* out)const override{
        if(!available)return false;
        *out={};
        if(start[2]<plane&&end[2]>=plane)*out={true,(plane-start[2])/(end[2]-start[2]),2,7};
        return true;
    }
};
}
int main(){
    const auto p=parse(player),t=parse(target);
    check(p->moves[1].releases[0].projectile==1&&p->moves[1].releases[1].stock==255&&p->projectiles[1].walk_ticks==3,
        "typed move, volley, stock and hit bindings");
    BorgGameplayProfile unchanged=*p;std::string error;
    for(auto bad:{"release=b-shot|3|ghost|1|shot","release=x-volley|3|none|0|shot",
                  "on-hit=twin|walk-speed|50|3","playable=yes","projectile=bad|bullet|2|NaN|2|1|0|5|20|5|ignore-floor"}){
        check(!borg_read_gameplay_profile(std::string(player)+bad,&unchanged,&error)&&!error.empty()&&unchanged.moves.size()==2,
              "broken definitions refuse atomically");
    }
    BorgBattleActionState action;action.reset(*p);
    std::vector<BorgBattleEventKind> trace;
    unsigned attempts=0;
    auto step=[&](bool b,bool x,bool ground,bool interrupted){
        trace.clear();
        action.visit(*p,{u8((b?1:0)|(x?2:0)),0,0},ground,interrupted,[&](const auto&,u8,u8){++attempts;return u32(0);},
            [&](auto kind,u8,u16,u32,u16){trace.push_back(kind);});
    };
    step(true,true,false,false);check(action.move==0&&!attempts&&action.stocks[0].current==2,"airborne X declines; no debit on press");
    step(false,false,false,false);step(false,false,false,true);
    check(action.move==255&&!attempts&&trace==std::vector{BorgBattleEventKind::Interrupted},"interruption before marker leaves stock intact");
    step(true,true,true,false);check(action.move==1,"explicit priority wins simultaneous press");
    step(false,false,true,false);step(false,false,true,false);
    check(attempts==1&&action.stocks[0].current==1&&!action.stocks[0].refill_clock&&
          trace==std::vector{BorgBattleEventKind::Marker,BorgBattleEventKind::StockSpent,BorgBattleEventKind::EmitRefused},
          "paid release before allocation; no refund or early refill");
    step(false,false,true,false);step(false,false,true,false);
    check(attempts==2&&action.stocks[0].current==1,"explicit second marker and stockless release");
    step(false,false,true,false);step(false,false,true,false);
    step(true,false,true,false);step(false,false,true,false);step(false,false,true,false);
    check(action.stocks[0].current==0&&action.stocks[0].refill_clock==12,"last debit starts its own clock");
    for(int i=0;i<11;++i)step(false,false,true,false);
    check(action.stocks[0].current==0,"clock does not refill early");
    step(false,false,true,false);check(action.stocks[0].current==2&&trace==std::vector{BorgBattleEventKind::StockRefilled},"eligible refill");

    NativeBattleMvp world;BattleMvpConfig config;config.packages={p,t};config.spawn={BattlePoint{0,0},BattlePoint{0,220}};
    check(world.start(config)&&world.actor_count()==2&&world.maximum_health(1)==500,"admit exact definitions as live actors");
    const auto old=world.presentation_scope();
    const auto tick=[&](bool b=false,bool x=false){
        std::array<BorgBattleInputs,2> press{};press[0].pressed=u8((b?1:0)|(x?2:0));
        check(world.step({},false,nullptr,nullptr,nullptr,nullptr,press),"live API step");
    };
    tick(false,true);tick();tick();
    check(world.shots_spawned()==2&&has(world,BorgBattleEventKind::Emitted)&&world.package_action(0).stocks[0].current==1,
          "two allocated projectiles from one live paid marker");
    const auto& a=world.projectiles()[0];const auto& b=world.projectiles()[1];
    check(a.id!=b.id&&a.velocity[0]!=b.velocity[0]&&a.position[1]==65.f&&a.velocity[1]==4.f,"first ballistic tick and symmetric spread");
    tick();check(a.position[1]==69.f&&a.velocity[1]==3.f,"authored gravity affects vertical trajectory only");
    bool saw_hit=false,refusal=false;
    for(unsigned i=0;i<15;++i){
        tick();saw_hit|=has(world,BorgBattleEventKind::Hit);refusal|=has(world,BorgBattleEventKind::ModifierRefused);
    }
    check(saw_hit&&refusal&&world.hits()==3&&world.actors()[1].health==461,"resolved hits, damage and stationary modifier refusal");
    tick(true);tick();
    check(world.start(config)&&!world.owns_motion_scope(old)&&!world.shots_spawned()&&world.package_events().empty()&&
        world.package_action(0).stocks[0].current==2,"restart retires old requests, stock, effects and world identity");
    tick();tick();tick();check(!world.shots_spawned(),"old release cannot fire in new session");

    config.packages={p,p};check(world.start(config),"two independent authored actors");
    tick(false,true);bool applied=false,expired=false;
    unsigned applied_at=0;
    for(unsigned i=0;i<16;++i){
        GamePadState away;if(applied)away.dpad_up=true;
        check(world.step({GamePadState{},away},false),"live modifier updates");
        if(has(world,BorgBattleEventKind::ModifierApplied)){applied=true;applied_at=world.tick();}
        if(has(world,BorgBattleEventKind::ModifierExpired)){
            expired=true;
            check(world.tick()==applied_at+3&&std::abs(world.actors()[1].movement_speed-2.f)<.001f,
                "modifier scales real motion through the last recipient update");
        }
    }
    check(applied&&expired&&world.actors()[1].movement_speed>2.f,"expiry restores unmodified movement");

    auto pool=*p;
    pool.projectiles[0].count=8;pool.projectiles[0].speed=1;pool.projectiles[0].lifetime=600;
    pool.stocks[0].capacity=20;
    config.packages={std::make_shared<const BorgGameplayProfile>(pool),t};
    config.spawn[1]={0,1200};
    check(world.start(config),"pool saturation test world");
    for(unsigned i=0;i<16;++i){tick(true);tick();tick();tick();tick();}
    check(world.shots_spawned()==128&&world.package_action(0).stocks[0].current==4&&!world.hits(),"all slots remain occupied");
    tick(true);tick();tick();
    check(has(world,BorgBattleEventKind::EmitRefused)&&world.shots_spawned()==128&&world.package_action(0).stocks[0].current==3,
        "live full pool refuses the entire paid volley without refund or overwrite");

    const auto grappler=parse(std::string(player)+"on-hit=shot|pull-self|36\non-hit=twin|pull-self|24\n");
    check(grappler->projectiles[0].pull_units==36&&grappler->projectiles[1].pull_units==24&&grappler->projectiles[1].walk_ticks==3,
        "independent hit effects compose");
    for(auto bad:{"on-hit=shot|pull-self|0","on-hit=shot|pull-self|41","on-hit=missing|pull-self|8"})
        check(!borg_read_gameplay_profile(std::string(player)+bad,&unchanged,&error),"out-of-range or unresolved pull refusal");
    check(!borg_read_gameplay_profile(std::string(player)+"on-hit=shot|pull-self|4\non-hit=shot|pull-self|7",&unchanged,&error),
        "a second pull declaration cannot replace the first");
    auto unsupported=*grappler;unsupported.movement=BorgBattleMovement::NormalGRed;
    check(!borg_gameplay_profile_valid(unsupported),"source-owned movement cannot be displaced by authored effects");

    PullGround terrain;
    config.packages={grappler,t};config.spawn[1]={0,220};
    auto pull_step=[&](bool b=false,bool x=false,bool jump=false){
        std::array<BorgBattleInputs,2> press{};press[0].pressed=u8((b?1:0)|(x?2:0));
        GamePadState pad;pad.b=jump;
        check(world.step({pad,{}},false,&terrain,nullptr,nullptr,nullptr,press),"pull battle step");
    };
    auto reach_pull=[&](bool x=false){
        pull_step(!x,x);
        for(unsigned i=0;i<20;++i){
            pull_step();
            const auto events=world.package_events();
            const auto queued=std::find_if(events.begin(),events.end(),[](auto e){return e.kind==BorgBattleEventKind::PullQueued;});
            if(queued==events.end())continue;
            const auto hit=std::find_if(events.begin(),queued,[&](auto e){
                return e.kind==BorgBattleEventKind::Hit&&e.object==queued->object&&e.recipient==queued->recipient;
            });
            check(queued->recipient==1&&hit!=queued&&world.actors()[0].position==BattleMvpVec{}&&!has(world,BorgBattleEventKind::PullApplied),
                "accepted hit stages its actual recipient; root unchanged on contact tick");
            return queued->object;
        }
        check(false,"hit did not queue pull");return u32(0);
    };
    check(world.start(config,&terrain),"flat grapple fixtures");
    const auto object=reach_pull();const auto queued_tick=world.tick();
    const auto facing=world.actors()[0].facing;
    check(world.set_paused(true),"pause with pull pending");pull_step();
    check(world.tick()==queued_tick&&world.actors()[0].position==BattleMvpVec{},"pause does not consume a pull");
    check(world.set_paused(false),"resume pending pull");pull_step();
    const auto receipt=world.package_events().front();
    check(receipt.kind==BorgBattleEventKind::PullApplied&&receipt.object==object&&receipt.tick==queued_tick+1&&receipt.value==36&&
        world.actors()[0].position==BattleMvpVec{0,0,36}&&world.actors()[0].facing==facing&&world.actors()[0].velocity==BattlePoint{},
        "matching deferred result, actual root and preserved body heading, no residual velocity");
    pull_step();check(!has(world,BorgBattleEventKind::PullApplied)&&world.actors()[0].position[2]==36,"request is single use");
    for(auto [kind,reason]:{std::pair{PullGround::Step,BorgBattlePullRefusal::TerrainOrBody},
                            std::pair{PullGround::Unknown,BorgBattlePullRefusal::SupportUnavailable},
                            std::pair{PullGround::Flat,BorgBattlePullRefusal::OwnerBusy}}){
        terrain.kind=kind;
        check(world.start(config,&terrain),"refusal fixture");
        const auto source=reach_pull();
        pull_step(false,false,reason==BorgBattlePullRefusal::OwnerBusy);
        const auto event=world.package_events().front();
        check(event.kind==BorgBattleEventKind::PullRefused&&event.object==source&&event.value==u16(reason)&&
              world.actors()[0].position[2]==0,"failed request leaves the planar root unchanged");
    }
    terrain.kind=PullGround::Flat;
    config.spawn[1]={0,90};
    check(world.start(config,&terrain),"body obstruction fixture");
    reach_pull();pull_step();
    check(has(world,BorgBattleEventKind::PullRefused)&&world.actors()[0].position[2]==0,"body blocking refuses rather than partially commits");
    config.spawn[1]={0,220};
    check(world.start(config,&terrain),"volley ownership");
    reach_pull(true);
    check(has(world,BorgBattleEventKind::PullRefused),"two same-tick hits cannot queue two owner pulls");
    pull_step();check(world.actors()[0].position[2]==24,"first queued volley pull wins");
    auto fragile=*t;fragile.health=1;config.packages[1]=std::make_shared<const BorgGameplayProfile>(fragile);
    check(world.start(config,&terrain),"lethal contact fixture");
    reach_pull();
    check(has(world,BorgBattleEventKind::PullCancelled)&&world.result()==BattleMvpResult::GRedWins,"a round-ending hit cancels the pending pull");
    config.packages[1]=t;
    check(world.start(config,&terrain),"restart fixture");
    reach_pull();
    check(world.start(config,&terrain),"restart while a request is pending");
    pull_step();check(world.actors()[0].position==BattleMvpVec{}&&world.package_events().empty(),"new round cannot consume old pull");
    terrain.kind=PullGround::HighTarget;
    check(world.start(config,&terrain),"miss fixture with target on a different layer");
    pull_step(true);
    for(unsigned i=0;i<34;++i){pull_step();check(!has(world,BorgBattleEventKind::PullQueued),"a miss supplies no contact destination");}
    check(!world.hits()&&world.actors()[0].position==BattleMvpVec{},"no movement or hit from a miss");

    const auto activation_text=std::string(player)+
        "activation=x-volley|release-window|1|3\n"
        "move=charged|x|30|any|6|0:5\nactivation=charged|release-window|4|600\n"
        "release=charged|2|primary|2|shot\nrepeat=b-shot|3\n";
    const auto charged=parse(activation_text);
    for(auto bad:{"repeat=charged|3","activation=b-shot|release-window|0|2","activation=unknown|release-window|1|3"})
        check(!borg_read_gameplay_profile(activation_text+bad,&unchanged,&error)&&!error.empty(),
            "duplicate and incompatible input policies refuse");
    check(!borg_read_gameplay_profile(std::string(player)+"repeat=x-volley|3",&unchanged,&error),"repeat cannot span multiple markers");
    constexpr auto b_bit=BorgBattleInputs::b,x_bit=BorgBattleInputs::x;
    action.reset(*charged);attempts=0;
    u16 charge_ticks=0;
    const auto gesture=[&](BorgBattleInputs in,bool interrupted=false,bool ground=true){
        trace.clear();
        action.visit(*charged,in,ground,interrupted,[&](const auto&,u8,u8){return ++attempts;},
            [&](auto kind,u8,u16,u32,u16 value){trace.push_back(kind);if(kind==BorgBattleEventKind::ButtonRelease)charge_ticks=value;});
    };
    gesture({x_bit,x_bit,0});gesture({0,x_bit,0});gesture({0,x_bit,0});
    check(action.move==255&&attempts==0&&action.stocks[0].current==2,"holding release-only button neither starts nor pays");
    gesture({0,0,x_bit});
    check(action.move==1&&charge_ticks==3&&trace==std::vector{BorgBattleEventKind::ButtonRelease,BorgBattleEventKind::Started}&&
        action.stocks[0].current==2,"inclusive tap ceiling selects move; button release is not the animation marker");
    gesture({});gesture({});
    check(attempts==1&&trace==std::vector{BorgBattleEventKind::Marker,BorgBattleEventKind::StockSpent,BorgBattleEventKind::Emitted},
        "only animation marker pays and emits tap");
    action.reset(*charged);
    gesture({x_bit,x_bit,0});for(unsigned i=0;i<3;++i)gesture({0,x_bit,0});
    gesture({0,0,x_bit});
    check(action.move==2&&charge_ticks==4&&action.stocks[0].current==2,"lower charge threshold");
    gesture({});gesture({},true);
    check(attempts==1&&action.stocks[0].current==2&&trace==std::vector{BorgBattleEventKind::Interrupted},
        "hit before charged marker consumes nothing");
    gesture({x_bit,x_bit,0});gesture({0,x_bit,0},true);gesture({0,x_bit,0});gesture({0,0,x_bit});
    check(action.move==255&&trace.empty(),"interrupt clears hold; still-held button does not rearm it");
    gesture({x_bit,x_bit,0});for(unsigned i=0;i<600;++i)gesture({0,x_bit,0});
    gesture({0,0,x_bit});
    check(action.move==255&&trace.empty(),"overlong hold is not truncated into a valid release window");
    action.reset(*charged);
    gesture({b_bit,b_bit,0});gesture({0,b_bit,0});gesture({0,b_bit,0});gesture({0,b_bit,0});gesture({0,b_bit,0});
    check(action.move==0&&action.cycle==2&&action.frame==0&&action.stocks[0].current==1&&attempts==2&&
        trace==std::vector{BorgBattleEventKind::Repeated},"held repeat checks availability without paying or emitting");
    for(unsigned i=0;i<4;++i)gesture({0,b_bit,0});
    check(action.move==255&&action.stocks[0].current==0&&attempts==3&&
        trace==std::vector{BorgBattleEventKind::RepeatStockRefused,BorgBattleEventKind::Completed},"eligible repeat pays on its own marker then stops at empty stock");
    action.reset(*charged);action.stocks[0].current=4;
    gesture({b_bit,b_bit,0});
    for(unsigned i=0;i<12;++i)gesture({0,b_bit,0});
    check(action.move==255&&action.stocks[0].current==1&&attempts==6&&
        trace==std::vector{BorgBattleEventKind::Completed},"maximum counts initial and repeat cycles");

    terrain.kind=PullGround::Flat;
    config.packages={charged,t};check(world.start(config,&terrain),"live charge fixture");
    const auto world_gesture=[&](BorgBattleInputs input){
        check(world.step({},false,&terrain,nullptr,nullptr,nullptr,{input,{}}),"live timed gesture");
    };
    world_gesture({x_bit,x_bit,0});world_gesture({0,x_bit,0});
    check(world.set_paused(true)&&world.set_paused(false),"world pause boundary");
    world_gesture({0,0,x_bit});check(!has(world,BorgBattleEventKind::Started),"pause discards pending charge");
    world_gesture({x_bit,x_bit,0});world_gesture({0,x_bit,0});
    check(world.start(config,&terrain),"restart during charge");
    world_gesture({0,0,x_bit});check(!has(world,BorgBattleEventKind::Started),"restart cannot inherit held charge");
    const auto blade_text=std::string(player)+
        "move=cut|b|30|grounded|10|0:1\nrange=cut|170\nmelee=cut|4|7|90|12|60|22|18\n";
    const auto blade=parse(blade_text);
    for(auto bad:{"range=cut|175","melee=cut|3|6|95|12|60|22|15","melee=missing|3|6|60|12|60|22|15","repeat=cut|3"})
        check(!borg_read_gameplay_profile(blade_text+bad,&unchanged,&error),"duplicate, missing or incompatible melee binding refuses");
    auto outside=blade_text;outside.replace(outside.find("cut|4|7"),7,"cut|4|10");
    check(!borg_read_gameplay_profile(outside,&unchanged,&error),"melee end must fit move");

    config.packages={blade,t};config.spawn[1]={0,120};
    check(world.start(config,&terrain),"close melee fixture");
    tick(true);
    check(world.package_action(0).move==2&&!world.shots_spawned()&&!world.hits(),"higher priority close B selects without a hit or shot");
    for(unsigned i=0;i<3;++i)tick();
    check(!world.hits()&&!has(world,BorgBattleEventKind::MeleeArmed),"wind-up cannot hit early");
    tick();
    auto contact=world.package_events();
    check(contact.size()==2&&contact[0].kind==BorgBattleEventKind::MeleeArmed&&contact[0].frame==4&&
        contact[1].kind==BorgBattleEventKind::Hit&&contact[1].recipient==1&&contact[1].object==contact[0].object&&
        contact[1].value==18&&world.actors()[1].health==482,"actor-bound shape authorizes the real hit in its window");
    for(unsigned i=0;i<4;++i)tick();
    check(world.hits()==1&&has(world,BorgBattleEventKind::MeleeRetired)&&!world.shots_spawned()&&world.package_action(0).stocks[0].current==2,
        "window retires, cannot duplicate damage, and is not a projectile or a debit");
    config.spawn[1]={0,171};
    check(world.start(config,&terrain),"outside entry range");
    tick(true);tick();tick();
    check(world.shots_spawned()==1&&world.package_action(0).move==0,"standard shot when close B is ineligible");
    config.spawn[1]={0,165};
    check(world.start(config,&terrain),"close gate, but outside live shape");
    tick(true);bool armed_window=false,retired_window=false;
    for(unsigned i=0;i<10;++i){
        tick();armed_window|=has(world,BorgBattleEventKind::MeleeArmed);retired_window|=has(world,BorgBattleEventKind::MeleeRetired);
    }
    check(armed_window&&retired_window&&!world.hits()&&!world.shots_spawned(),"an admitted melee can whiff without falling back to a shot");
    config.packages={blade,p};config.spawn[1]={0,120};
    check(world.start(config,&terrain),"interruption before marker");
    check(world.step({},false,&terrain,nullptr,nullptr,nullptr,{BorgBattleInputs{b_bit,0,0},BorgBattleInputs{b_bit,0,0}}),
        "both start independent attacks");
    bool interrupted_cut=false;
    for(unsigned i=0;i<10;++i){
        check(world.step({},false,&terrain),"interruption visit");
        interrupted_cut|=has(world,BorgBattleEventKind::Interrupted);
        check(!has(world,BorgBattleEventKind::MeleeArmed),"prior damage interrupts before arming melee");
    }
    check(interrupted_cut&&world.actors()[1].health==250,"interrupted cut cannot damage recipient");

    const auto rotary_text=std::string(player)+
        "move=rotary|b|40|grounded|10|0:1\nheld-fire=rotary|primary|1|shot|8|8|2|8\n";
    const auto rotary=parse(rotary_text);
    for(auto bad:{"held-fire=rotary|none|0|shot|8|8|2|0","release=rotary|2|none|0|shot",
                  "activation=rotary|release-window|1|3","held-fire=b-shot|primary|1|shot|8|8|2|0",
                  "held-fire=x-volley|primary|1|shot|8|2|8|0"})
        check(!borg_read_gameplay_profile(rotary_text+bad,&unchanged,&error),
              "held fire refuses mixed trigger/cadence rules and invalid numbers");
    auto reset_rotary=*rotary;reset_rotary.moves.back().held_fire->cooldown=0;
    auto free_rotary=reset_rotary;free_rotary.moves.back().held_fire->stock=255;free_rotary.moves.back().held_fire->cost=0;
    check(borg_gameplay_profile_valid(reset_rotary)&&borg_gameplay_profile_valid(free_rotary),"tested reset and independent stockless policy");
    unsigned emissions=0;bool pool_accepts=true;u16 spin_readout=0,interval=0;
    auto fire=[&](const BorgGameplayProfile& spec,BorgBattleInputs input,bool hit=false,bool grounded=true){
        trace.clear();
        action.visit(spec,input,grounded,hit,[&](const auto&,u8,u8){++emissions;return pool_accepts?u32(emissions):0;},
            [&](auto kind,u8,u16,u32,u16 value){
                trace.push_back(kind);
                if(kind==BorgBattleEventKind::HeldFireStarted||kind==BorgBattleEventKind::HeldFireStopped)spin_readout=value;
                if(kind==BorgBattleEventKind::HeldFireCadence)interval=value;
            });
    };
    action.reset(*rotary);action.stocks[0].current=20;
    fire(*rotary,{b_bit,b_bit,0});
    check(!emissions&&spin_readout==0&&trace.back()==BorgBattleEventKind::HeldFireStarted,"cold entry does not spend or fire");
    std::vector<unsigned> firing_ticks;std::vector<u16> intervals;
    for(unsigned i=1;i<=13;++i){
        fire(*rotary,{0,b_bit,0});
        if(std::find(trace.begin(),trace.end(),BorgBattleEventKind::Emitted)!=trace.end()){
            firing_ticks.push_back(i);intervals.push_back(interval);
        }
    }
    check(firing_ticks==std::vector{5u,8u,10u,12u}&&intervals==std::vector<u16>{5,2,2,2}&&
          action.stocks[0].current==16,"held fire ramps to cap and pays once per cadence, independent of pose loop");
    fire(*rotary,{0,0,b_bit});
    check(emissions==4&&spin_readout==875&&action.move==255&&trace==std::vector{BorgBattleEventKind::HeldFireStopped},
        "release on a due shot stops before any debit");
    fire(*rotary,{});check(emissions==4&&trace.empty(),"cooling never fires");
    fire(*rotary,{b_bit,b_bit,0});
    check(spin_readout==625&&emissions==4,"short gap retains only the explicitly configured spin, not a shot timer");
    fire(*rotary,{0,b_bit,0});fire(*rotary,{0,b_bit,0});fire(*rotary,{0,b_bit,0});
    check(emissions==5&&action.stocks[0].current==15,"quick re-press reaches the first emission sooner");
    fire(*rotary,{0,b_bit,0},true);check(action.move==255&&emissions==5&&trace.back()==BorgBattleEventKind::Interrupted,"damage cancels held fire before another emission");
    fire(*rotary,{0,b_bit,0});check(action.move==255,"still-held input cannot restart an interrupted move");
    for(unsigned i=0;i<8;++i)fire(*rotary,{});
    fire(*rotary,{b_bit,b_bit,0});check(spin_readout==0,"a full cooldown restores cold start");
    fire(*rotary,{0,b_bit,0},false,false);check(action.move==255&&trace.back()==BorgBattleEventKind::HeldFireStopped,"leaving the move context stops the stream");

    action.reset(reset_rotary);emissions=0;pool_accepts=false;
    fire(reset_rotary,{b_bit,b_bit,0});for(unsigned i=0;i<5;++i)fire(reset_rotary,{0,b_bit,0});
    check(emissions==1&&action.stocks[0].current==1&&trace.back()==BorgBattleEventKind::EmitRefused,
        "pool refusal occurs after debit and does not create a backlog");
    fire(reset_rotary,{0,0,b_bit});
    check(spin_readout==0&&action.move==255,"reset policy loses all speed on release");
    fire(reset_rotary,{b_bit,b_bit,0});
    check(!spin_readout&&emissions==1,"instant re-press in reset mode starts cold");
    action.reset(free_rotary);pool_accepts=true;emissions=0;
    fire(free_rotary,{b_bit,b_bit,0});for(unsigned i=0;i<5;++i)fire(free_rotary,{0,b_bit,0});
    check(emissions==1&&action.stocks[0].current==2&&trace==std::vector{BorgBattleEventKind::HeldFireCadence,BorgBattleEventKind::Emitted},
        "cadence can be independently stockless");
    action.reset(*rotary);
    check(action.spin==std::array<u32,kBorgBattleMoves>{}&&action.fire_gap==0&&action.move==255,"round reset discards spin and cadence");
    const std::string bombs=std::string(player)+
        "projectile=bomb|bullet|1|20|0|0|0|8|5|0|ignore-floor\n"
        "actor-contact=bomb|ignore\nblast=bomb|expiry|200|28|opponent\n"
        "move=bomb-throw|x|40|grounded|6|0:1\nrelease=bomb-throw|2|primary|1|bomb\n";
    const auto bomber=parse(bombs);
    for(auto bad:{"blast=bomb|floor|100|15|all","blast=unknown|expiry|100|15|all",
                  "actor-contact=shot|ignore","actor-contact=bomb|ignore","on-hit=bomb|walk-speed|50|8"})
        check(!borg_read_gameplay_profile(bombs+bad,&unchanged,&error),"duplicate, impossible, or unresolved blast/contact rule");
    const auto replace_one=[](std::string text,std::string_view original,std::string_view replacement){
        const auto pos=text.find(original);check(pos!=text.npos,"find text fixture");text.replace(pos,original.size(),replacement);return text;
    };
    check(!borg_read_gameplay_profile(replace_one(bombs,"blast=bomb|expiry","blast=bomb|actor"),&unchanged,&error),
        "disabling a collision also disables its trigger");
    check(!borg_read_gameplay_profile(replace_one(bombs,"blast=bomb|expiry|200|28|opponent\n",""),&unchanged,&error),
        "an otherwise inert carrier needs an effect");
    config.packages={bomber,t};config.spawn[1]={0,120};terrain.kind=PullGround::Flat;
    check(world.start(config,&terrain),"expiry blast fixture");
    pull_step(false,true);pull_step();pull_step();
    check(world.shots_spawned()==1&&world.projectiles()[0].from_package&&!world.projectiles()[0].package_damage&&
        world.package_action(0).stocks[0].current==1,"a paid blast-only carrier is still a package object");
    for(unsigned i=0;i<3;++i){
        pull_step();
        check(!has(world,BorgBattleEventKind::BlastTriggered)&&!has(world,BorgBattleEventKind::Hit)&&
             !has(world,BorgBattleEventKind::ProjectileContact),"fuse carrier ignores the actor, independent of its area filter");
    }
    pull_step();
    auto area=world.package_events();
    check(area.size()>=3,"expiry outcome sequence exists");
    area=area.last(3);
    check(area[0].kind==BorgBattleEventKind::BlastTriggered&&area[0].value==4&&
          area[1].kind==BorgBattleEventKind::BlastHit&&area[1].recipient==1&&area[1].value==28&&area[2].kind==BorgBattleEventKind::Retired&&
          area[0].object==area[1].object&&area[1].object==area[2].object&&world.actors()[0].health==250&&world.actors()[1].health==472,
          "expiry, opponent-only damage and retirement are distinct outcomes");
    pull_step();check(world.hits()==1&&!has(world,BorgBattleEventKind::BlastHit),"blast is a one-time pulse, not a repeated damaging field");
    config.spawn[1]={0,500};check(world.start(config,&terrain),"expiry misses all targets");
    pull_step(false,true);for(unsigned i=0;i<6;++i)pull_step();
    check(has(world,BorgBattleEventKind::BlastTriggered)&&!has(world,BorgBattleEventKind::BlastHit),"an area trigger does not guarantee a recipient");
    config.spawn[1]={0,120};
    const auto contact_bombs=replace_one(replace_one(bombs,"actor-contact=bomb|ignore\n",""),
        "blast=bomb|expiry|200|28|opponent","blast=bomb|actor|200|28|all");
    config.packages[0]=parse(contact_bombs);check(world.start(config,&terrain),"contact blast fixture");
    pull_step(false,true);pull_step();pull_step();
    area=world.package_events();
    check(area.size()==8&&has(world,BorgBattleEventKind::ProjectileContact)&&has(world,BorgBattleEventKind::BlastTriggered)&&
          !has(world,BorgBattleEventKind::Hit)&&world.hits()==2&&world.actors()[0].health==222&&world.actors()[1].health==472,
          "accepted contact triggers area without direct damage; explicit all policy includes caster once");
    const auto floor_bombs=replace_one(replace_one(bombs,"20|0|0|0|8|5|0|ignore-floor","20|-80|0|0|8|5|0|stop-on-floor"),
                                       "blast=bomb|expiry","blast=bomb|floor");
    config.packages[0]=parse(floor_bombs);check(world.start(config,&terrain),"floor blast fixture");
    pull_step(false,true);pull_step();pull_step();
    check(has(world,BorgBattleEventKind::BlastTriggered)&&!has(world,BorgBattleEventKind::ProjectileContact)&&
          world.actors()[1].health==472,"floor can trigger a blast without actor contact");
    terrain.kind=PullGround::Unknown;check(world.start(config,&terrain),"unknown floor fixture");
    pull_step(false,true);pull_step();pull_step();
    check(!has(world,BorgBattleEventKind::BlastTriggered)&&world.package_events().back().kind==BorgBattleEventKind::Retired&&
          world.package_events().back().value==3,"unknown floor neither fabricates a blast nor hits");
    terrain.kind=PullGround::Flat;config.packages[0]=bomber;check(world.start(config,&terrain),"pending fuse restart fixture");
    pull_step(false,true);pull_step();pull_step();
    check(world.shots_spawned()==1&&world.start(config,&terrain),"restart while fuse is live");
    for(unsigned i=0;i<8;++i)pull_step();
    check(!world.shots_spawned()&&!world.hits()&&world.actors()[1].health==500,"old fuse cannot detonate in replacement world");

    const auto remotes=replace_one(replace_one(bombs,"20|0|0|0|8|5|0|ignore-floor","20|-80|0|0|8|25|0|land-on-floor"),
        "blast=bomb|expiry","blast=bomb|remote")+"move=trigger|b|50|any|6|0:1\ndetonate=trigger|2|bomb|landed\n";
    const auto remote=parse(remotes);
    for(auto bad:{"detonate=trigger|3|bomb|landed","detonate=trigger|1|shot|any","detonate=unknown|1|bomb|any"})
        check(!borg_read_gameplay_profile(remotes+bad,&unchanged,&error),"duplicate or unresolved remote command");
    check(!borg_read_gameplay_profile(replace_one(remotes,"blast=bomb|remote","blast=bomb|expiry"),&unchanged,&error),
        "a remote marker cannot arm a timed fuse");
    check(!borg_read_gameplay_profile(replace_one(remotes,"land-on-floor","stop-on-floor"),&unchanged,&error),
        "landed filter cannot be paired with retirement on floor");
    const auto request=[&](u8 left=0,u8 right=0){
        check(world.step({},false,&terrain,nullptr,nullptr,nullptr,{BorgBattleInputs{left,0,0},BorgBattleInputs{right,0,0}}),
            "deployable battle step");
    };
    const auto trigger_result=[&](unsigned owner,u16 count){
        const auto events=world.package_events();
        return std::any_of(events.begin(),events.end(),[&](auto e){
            return e.kind==BorgBattleEventKind::Detonation&&e.actor==owner&&e.value==count;
        });
    };
    config.packages={remote,remote};config.spawn[1]={0,120};
    check(world.start(config,&terrain),"opposing owners with identical package identity");
    request(x_bit,x_bit);request();request();
    check(world.shots_spawned()==2&&world.projectiles()[0].landed&&world.projectiles()[1].landed&&
          has(world,BorgBattleEventKind::ProjectileLanded)&&has(world,BorgBattleEventKind::ProjectileLanded,1)&&!world.hits(),
          "both charges land without dealing damage");
    const auto resting=world.projectiles()[0].position;
    for(unsigned i=0;i<4;++i)request();
    check(world.projectiles()[0].position==resting&&world.projectiles()[0].velocity==BattleMvpVec{},
        "landed carrier holds position");
    request(b_bit);request();
    check(!world.hits()&&world.projectiles()[0].active,"physical detonation press cannot fire before the marker");
    request();
    check(trigger_result(0,1)&&has(world,BorgBattleEventKind::BlastHit)&&world.actors()[1].health==222&&
          world.actors()[0].health==250&&!world.projectiles()[0].active&&world.projectiles()[1].active&&
          world.package_action(0).stocks[0].current==1,"trigger retires only the matching owner payload, with no second debit");
    for(unsigned i=0;i<4;++i)request();
    request(b_bit);request();request();
    check(trigger_result(0,0)&&world.hits()==1&&world.projectiles()[1].active,"repeat signal cannot reuse old charge or touch opponent charge");

    config.packages={remote,t};check(world.start(config,&terrain),"landed charge lifetime");
    request(x_bit);for(unsigned i=0;i<26;++i){request();check(!has(world,BorgBattleEventKind::BlastTriggered),"expired remote charge does not blast");}
    check(!world.projectiles()[0].active&&!world.hits(),"expiry clears landed remote");
    request(b_bit);request();request();check(trigger_result(0,0),"expired charge refuses subsequent marker");
    check(world.start(config,&terrain),"multiple eligible charges");
    for(unsigned n=0;n<2;++n){request(x_bit);for(unsigned i=0;i<6;++i)request();}
    request(b_bit);request();request();
    check(trigger_result(0,2)&&world.hits()==2&&world.actors()[1].health==444&&
          !world.projectiles()[0].active&&!world.projectiles()[1].active,
          "one signal consumes the complete eligible owner set, once per charge");
    check(world.start(config,&terrain),"support-change fixture");
    request(x_bit);for(unsigned i=0;i<6;++i)request();
    request(b_bit);request();
    terrain.kind=PullGround::Unknown;request();terrain.kind=PullGround::Flat;
    check(trigger_result(0,0)&&!world.projectiles()[0].active&&!world.hits(),"a grounded charge that lost support cannot detonate");
    check(world.start(config,&terrain),"restart with planted charge");
    request(x_bit);request();request();
    check(world.projectiles()[0].active&&world.start(config,&terrain),"replace round");
    request(b_bit);request();request();
    check(trigger_result(0,0)&&!world.hits()&&!world.shots_spawned(),"new round signal has no authority over an old charge");

    config.packages[0]=parse(replace_one(remotes,"20|-80|0|0","20|0|0|0"));
    check(world.start(config,&terrain),"airborne charge test");
    request(x_bit);for(unsigned i=0;i<6;++i)request();
    request(b_bit);request();request();
    check(trigger_result(0,0)&&world.projectiles()[0].active&&!world.projectiles()[0].landed,"landed selector refuses airborne charge");
    config.packages[0]=parse(replace_one(replace_one(remotes,"20|-80|0|0","20|0|0|0"),
        "detonate=trigger|2|bomb|landed","detonate=trigger|2|bomb|any"));
    check(world.start(config,&terrain),"explicit midair trigger fixture");
    request(x_bit);for(unsigned i=0;i<6;++i)request();
    request(b_bit);request();request();
    check(trigger_result(0,1)&&has(world,BorgBattleEventKind::BlastTriggered)&&
        !world.projectiles()[0].active,"any selector deliberately admits an airborne charge");
    config.packages[0]=parse(replace_one(replace_one(remotes,"blast=bomb|remote","blast=bomb|expiry"),
        "move=trigger|b|50|any|6|0:1\ndetonate=trigger|2|bomb|landed\n",""));
    check(world.start(config,&terrain),"land-and-fuse fixture");
    request(x_bit);request();request();
    check(world.projectiles()[0].landed&&!world.hits(),"timer can continue on the floor");
    for(unsigned i=0;i<24;++i)request();
    check(has(world,BorgBattleEventKind::BlastHit)&&world.hits()==1&&!world.projectiles()[0].active,
        "timed stationary bomb expires with a pulse without remote input");
    const auto returns=replace_one(remotes,"release=bomb-throw|2|primary|1|bomb","release=bomb-throw|2|charge-pool|1|bomb")+
        "stock=charge-pool|1|on-return\nreturn-stock=bomb|remote\n";
    const auto return_profile=parse(returns);
    for(const auto& bad:{
            replace_one(returns,"on-return","10"),
            replace_one(returns,"|charge-pool|1|bomb","|none|0|bomb"),
            replace_one(returns,"projectile=bomb|bullet|1","projectile=bomb|bullet|2"),
            replace_one(returns,"return-stock=bomb|remote","return-stock=bomb|floor"),
            replace_one(returns,"return-stock=bomb|remote\n",""),
            returns+"return-stock=bomb|remote"})
        check(!borg_read_gameplay_profile(bad,&unchanged,&error),"mismatched stock policy or unqualified return refuses");
    config.packages={return_profile,t};check(world.start(config,&terrain),"return-only stock fixture");
    request(x_bit);request();request();
    check(world.projectiles()[0].package_return_stock==1&&world.package_action(0).stocks[1].current==0&&
          world.package_action(0).stocks[1].refill_clock==0,"live object carries the paid channel; no depletion timer");
    for(unsigned i=0;i<4;++i)request();
    request(x_bit);request();request();
    check(!world.package_action(0).stocks[1].current&&has(world,BorgBattleEventKind::StockRefused)&&world.shots_spawned()==1,
          "stock cannot be reused while its charge is active");
    for(unsigned i=0;i<4;++i)request();
    request(b_bit);request();request();
    const auto returned=world.package_events().back();
    check(trigger_result(0,1)&&returned.kind==BorgBattleEventKind::StockReturned&&returned.object==1&&returned.value==1&&
          world.package_action(0).stocks[1].current==1&&world.package_action(0).stocks[0].current==2,
          "remote retirement restores exactly the charged channel after the blast");
    for(unsigned i=0;i<4;++i)request();
    request(b_bit);request();request();
    check(trigger_result(0,0)&&!has(world,BorgBattleEventKind::StockReturned),"duplicate signal cannot return stock twice");
    check(world.start(config,&terrain),"nonmatching expiry fixture");
    request(x_bit);for(unsigned i=0;i<38;++i)request();
    check(!world.projectiles()[0].active&&!world.package_action(0).stocks[1].current&&!world.package_action(0).stocks[1].refill_clock,
          "expiry under remote-only policy neither returns nor replenishes");
    const auto expiry_returns=parse(replace_one(returns,"return-stock=bomb|remote","return-stock=bomb|expiry"));
    config.packages[0]=expiry_returns;check(world.start(config,&terrain),"explicit expiry return fixture");
    request(x_bit);for(unsigned i=0;i<26;++i)request();
    check(has(world,BorgBattleEventKind::StockReturned)&&!has(world,BorgBattleEventKind::BlastTriggered)&&
          world.package_action(0).stocks[1].current==1,"stock-return disposition is independent of blast trigger");
    config.packages[0]=parse(replace_one(replace_one(replace_one(returns,
        "projectile=bomb|bullet|1","projectile=bomb|bullet|2"),"charge-pool|1|on-return","charge-pool|2|on-return"),
        "|charge-pool|1|bomb","|charge-pool|2|bomb"));
    check(world.start(config,&terrain),"one paid unit per volley carrier");
    request(x_bit);for(unsigned i=0;i<6;++i)request();
    check(world.shots_spawned()==2&&world.package_action(0).stocks[1].current==0,"two carriers consume two units");
    request(b_bit);request();request();
    const auto return_events=world.package_events();
    check(trigger_result(0,2)&&world.package_action(0).stocks[1].current==2&&
          std::count_if(return_events.begin(),return_events.end(),[](auto e){return e.kind==BorgBattleEventKind::StockReturned;})==2,
          "one return per distinct owned object");
    action.reset(*return_profile);
    unsigned failed=0;
    const auto refuse_allocation=[&](u8 mask){
        trace.clear();
        action.visit(*return_profile,{mask,0,0},true,false,[&](const auto&,u8,u8 channel){
            check(channel==1,"paid channel offered to allocator");++failed;return u32(0);
        },[&](auto kind,u8,u16,u32,u16){trace.push_back(kind);});
    };
    refuse_allocation(x_bit);refuse_allocation(0);refuse_allocation(0);
    for(unsigned i=0;i<32;++i)refuse_allocation(0);
    check(failed==1&&!action.stocks[1].current&&!action.stocks[1].refill_clock,"allocation failure creates no return or timer");
    const auto guided=replace_one(std::string(player),"60|0|0|0|8|30","18|0|0|0|8|30")+
        "guidance=shot|tracking|0.14|25\n";
    // Build from the bounded text view; priority and contact remain those of
    // the ordinary action, only its emitted object's travel policy differs.
    for(const auto& bad:{guided+"guidance=shot|tracking|0.14|25",guided+"guidance=unknown|tracking|0.14|25",
                         replace_one(guided,"0.14|25","0.14|31"),replace_one(guided,"0.14|25","1.1|25")})
        check(!borg_read_gameplay_profile(bad,&unchanged,&error),"invalid, duplicate or unresolved guidance refuses");
    const auto follow=parse(guided),snapshot=parse(replace_one(guided,"|tracking|","|snapshot|"));
    const auto short_follow=parse(replace_one(guided,"0.14|25","0.01|2"));
    const auto follow_case=[&](std::shared_ptr<const BorgGameplayProfile> profile,bool should_hit){
        config.packages={std::move(profile),p};config.spawn[1]={0,360};
        check(world.start(config,&terrain),"planar guidance fixture");
        request(b_bit);request();request();
        auto shot=world.projectiles()[0];
        check(shot.active&&shot.package_guide_point==BattlePoint{0,360}&&shot.package_guide_target==1&&
              has(world,BorgBattleEventKind::GuidanceBound)&&!world.hits(),"registered aim target isn't a hit");
        const auto turn=world.config().packages[0]->projectiles[0].guidance->turn;
        bool curved=false,ended=false;
        for(unsigned i=0;i<30;++i){
            GamePadState moving;moving.dpad_right=true;
            check(world.step({GamePadState{},moving},false,&terrain),"target movement during guidance");
            const auto& next=world.projectiles()[0];
            if(shot.active&&next.active){
                const BattlePoint before{shot.velocity[0],shot.velocity[2]},after{next.velocity[0],next.velocity[2]};
                const auto angle=std::abs(native_borg_aim_delta(before,after));
                check(angle<=(shot.package_guide_ticks?turn+.00001f:.00001f)&&
                    std::abs(std::hypot(after.x,after.z)-18)<.0001f&&next.velocity[1]==0,
                    "bounded planar turn preserves speed and vertical velocity; expired guide coasts");
                curved|=angle>.001f;
            }
            ended|=has(world,BorgBattleEventKind::GuidanceEnded);
            shot=next;
        }
        check(bool(world.hits())==should_hit&&world.shots_spawned()==1,"real overlap, not guided aim, determines damage");
        return std::pair{curved,ended};
    };
    check(follow_case(follow,true).first,"tracking catches the moving target");
    check(follow_case(snapshot,false).second,"frozen point misses and the guide eventually expires");
    check(follow_case(short_follow,false)==std::pair{true,true},"brief capped turn ends without continued steering");
    check(world.start(config,&terrain),"restart guided battle");
    request(b_bit);request();request();
    check(world.projectiles()[0].active&&world.start(config,&terrain),"restart with a live guided projectile");
    request();
    check(!world.shots_spawned()&&!world.hits()&&world.package_events().empty(),"no target link survives round replacement");
    auto fragile_target=*t;fragile_target.health=1;
    config.packages={parse(replace_one(guided,"shot|beam|1|18","shot|beam|2|140")),
                     std::make_shared<const BorgGameplayProfile>(fragile_target)};
    config.spawn[1]={0,120};
    check(world.start(config,&terrain),"target loss within a volley");
    request(b_bit);request();request();
    const auto loss=world.package_events();
    check(world.hits()==1&&world.projectiles()[1].active&&!world.projectiles()[1].package_guide_ticks&&
          std::any_of(loss.begin(),loss.end(),[](auto e){
              return e.kind==BorgBattleEventKind::GuidanceEnded&&e.object==2&&e.recipient==1&&e.value==2;
          }),"a defeated target ends guidance and does not transfer the second shot");
    const auto guard_text=std::string(player)+"move=guard|x|50|grounded|18|0:1\nguard=guard|1|12|0.5|1|shots\n";
    const auto guard=parse(guard_text);
    for(const auto& bad:{guard_text+"guard=guard|1|12|0.5|2|shots",replace_one(guard_text,"|0.5|1|shots","|4|1|shots"),
                         replace_one(guard_text,"|1|12|0.5","|1|18|0.5"),guard_text+"repeat=guard|2"})
        check(!borg_read_gameplay_profile(bad,&unchanged,&error),"invalid, duplicate or incompatible guard window");
    const auto guard_direct=parse(replace_one(guard_text,"|shots","|direct"));
    const auto guard_all=parse(replace_one(guard_text,"|shots","|all"));
    config.packages={parse(contact_bombs),guard};config.spawn[1]={0,120};
    check(world.start(config,&terrain),"front guard against contact blast");
    request(x_bit,x_bit);request();
    check(has(world,BorgBattleEventKind::GuardArmed,1)&&!world.hits(),"guard arms on first animation frame");
    request();
    check(has(world,BorgBattleEventKind::GuardBlocked,1)&&has(world,BorgBattleEventKind::GuardBroken,1)&&
          !has(world,BorgBattleEventKind::ProjectileContact)&&!has(world,BorgBattleEventKind::BlastTriggered)&&
          !world.hits()&&world.actors()[1].health==250&&!world.actors()[1].hit_flash,
          "guarded actor contact retires the shot without direct hit or contact blast");
    config.packages[0]=p;check(world.start(config,&terrain),"guard budget on a volley");
    request(x_bit,x_bit);request();request();
    const auto volley_guard=world.package_events();
    check(world.hits()==1&&world.actors()[1].health==240&&
          std::count_if(volley_guard.begin(),volley_guard.end(),[](auto e){return e.kind==BorgBattleEventKind::GuardBlocked;})==1&&
          std::none_of(volley_guard.begin(),volley_guard.end(),[](auto e){return e.kind==BorgBattleEventKind::ModifierApplied&&e.object==1;}),
          "budget blocks only one round and guarded effects do not leak");
    for(const auto& [defense,blocked]:{std::pair{guard,false},std::pair{guard_direct,true}}){
        config.packages={blade,defense};check(world.start(config,&terrain),"melee guard mask comparison");
        request(b_bit,x_bit);for(unsigned i=0;i<4;++i)request();
        check(has(world,BorgBattleEventKind::GuardBlocked,1)==blocked&&world.hits()==unsigned(!blocked),
            "contact policy is independent for shots and actor-bound melee");
        for(unsigned i=0;i<6;++i)request();
        check(world.hits()==unsigned(!blocked),"a blocked cut cannot hit again in the same window");
    }
    for(const auto& [defense,blocked]:{std::pair{guard_direct,false},std::pair{guard_all,true}}){
        config.packages={parse(floor_bombs),defense};check(world.start(config,&terrain),"area guard mask comparison");
        request(x_bit,x_bit);request();request();
        check(has(world,BorgBattleEventKind::BlastTriggered)&&has(world,BorgBattleEventKind::GuardBlocked,1)==blocked&&
            has(world,BorgBattleEventKind::BlastHit)!=blocked,"explosion life is independent of a recipient blocking it");
    }
    config.packages={p,guard};config.spawn[1]={0,280};
    check(world.start(config,&terrain),"side pass outside cone");
    for(unsigned i=0;i<5;++i){
        GamePadState side;side.dpad_right=true;
        check(world.step({GamePadState{},side},false,&terrain),"defender turns its body away");
    }
    request(b_bit,x_bit);
    for(unsigned i=0;i<10;++i){
        request();check(!has(world,BorgBattleEventKind::GuardBlocked),"aim direction cannot turn the guard cone");
    }
    check(world.hits()==1&&world.actors()[1].health==231,"shot outside body-facing cone bypasses guard");
    const auto stage_text=replace_one(std::string(player),"60|0|0|0|8|30|19|ignore-floor","60|0|0|0|8|30|0|stop-on-stage")+
        "blast=shot|impact|160|19|opponent\n";
    const auto stage_profile=parse(stage_text);
    const auto target_only=parse(replace_one(stage_text,"blast=shot|impact","blast=shot|actor"));
    const auto stage_only=parse(replace_one(stage_text,"blast=shot|impact","blast=shot|stage"));
    check(!borg_read_gameplay_profile(replace_one(stage_text,"blast=shot|impact","blast=shot|floor"),&unchanged,&error),
        "stage segment is not the legacy endpoint-floor policy");
    check(!borg_read_gameplay_profile(replace_one(stage_text,"blast=shot|impact","blast=shot|stage")+
        "return-stock=twin|stage\n",&unchanged,&error),"stage return requires a stage-solid carrier");
    WallGround wall;
    config.packages={stage_profile,t};config.spawn[1]={0,120};
    check(!world.start(config,&terrain),"missing stage geometry prevents admission, even if a floor exists");
    const auto stage_fire=[&](std::shared_ptr<const BorgGameplayProfile> profile){
        config.packages[0]=std::move(profile);
        check(world.start(config,&wall),"supported stage fixture");
        for(unsigned n=0;n<3;++n)
            check(world.step({},false,&wall,nullptr,nullptr,nullptr,{BorgBattleInputs{u8(!n?b_bit:0),0,0},{}}),
                "stage projectile update");
    };
    stage_fire(stage_profile);
    auto events=world.package_events();
    const auto& wall_contact=world.projectiles()[0].position;
    check(has(world,BorgBattleEventKind::StageImpact)&&has(world,BorgBattleEventKind::BlastHit)&&
          !has(world,BorgBattleEventKind::ProjectileContact)&&std::abs(wall_contact[0])<.001f&&
          std::abs(wall_contact[1]-60)<.001f&&std::abs(wall_contact[2]-60)<.001f&&
          events.back().value==8&&world.actors()[1].health==481,"wall earlier than body supplies blast centre and layer");
    stage_fire(target_only);
    check(has(world,BorgBattleEventKind::StageImpact)&&!has(world,BorgBattleEventKind::BlastTriggered)&&
          world.actors()[1].health==500,"target-only blast still retires silently at a stage hit");
    wall.plane=95;
    stage_fire(stage_only);
    check(has(world,BorgBattleEventKind::ProjectileContact)&&!has(world,BorgBattleEventKind::StageImpact)&&
          !has(world,BorgBattleEventKind::BlastTriggered)&&world.package_events().back().value==1,
          "an earlier actor contact wins and cannot trigger a stage-only blast");
    wall.plane=70;
    stage_fire(stage_profile);
    check(has(world,BorgBattleEventKind::StageImpact)&&!has(world,BorgBattleEventKind::ProjectileContact),
          "exact contact tie gives static geometry precedence");
    wall.plane=500;
    stage_fire(target_only);
    check(has(world,BorgBattleEventKind::ProjectileContact)&&has(world,BorgBattleEventKind::BlastHit)&&
          !has(world,BorgBattleEventKind::StageImpact),"a computed clear stage segment can still hit the actor");
    wall.available=false;
    stage_fire(stage_profile);
    check(!has(world,BorgBattleEventKind::ProjectileContact)&&!has(world,BorgBattleEventKind::BlastTriggered)&&
          world.package_events().back().value==9&&world.actors()[1].health==500,
          "unavailable segment does not masquerade as a miss, actor hit or blast");
    std::puts("Borg gameplay API: guards, guidance, blast arbitration, remotes, stock returns and restart PASS");
}
