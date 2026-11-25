#include <cstdio>
#include <cstdlib>
#include <bit>
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
    f32 plane=60;bool available=true,plane_is_floor=false;
    mutable unsigned queries=0;
    bool height(f32,f32,f32,f32* out)const override{*out=0;return true;}
    bool supports_projectile_stage()const override{return true;}
    bool projectile_stage(const std::array<f32,3>& start,const std::array<f32,3>& end,NativeBorgStageProjectileHit* out)const override{
        ++queries;
        if(!available)return false;
        *out={};
        if(plane_is_floor){
            if(start[1]>0&&end[1]<=0)*out={true,start[1]/(start[1]-end[1]),1,7};
        }else if(start[2]<plane&&end[2]>=plane)*out={true,(plane-start[2])/(end[2]-start[2]),2,7};
        return true;
    }
};
struct CommitMuzzle final:IBorgBattleOrigins {
    mutable unsigned visits=0;
    bool socket(unsigned owner,const BattleMvpActor& actor,u32 tick,u8 move,u16 frame,const BorgBattleSocket& spec,
                BattleMvpVec* out)const override{
        ++visits;
        check(owner==0&&tick==11&&move==0&&frame==10&&spec.bone==1&&actor.package_aim_heading&&
              std::abs(actor.aim_facing.x)<1e-5f&&actor.aim_facing.z>.999f,"committed aim reaches the socket sampler");
        *out={actor.position[0]+12,actor.position[1]+65,actor.position[2]+30};
        return true;
    }
};
void aim_commit_cases(){
    constexpr std::string_view setup=R"(GFBORG-GAMEPLAY 1
variant=normal
movement=grounded-v1|12|13|14
health=250
rig-prefix=visual/prefix
pose-profile=visual/pose
idle=0:0
walk=0:1
air=0:2
dash=0:3
projectile=beam|beam|1|60|0|0|0|8|30|19|ignore-floor
origin=beam|1|12|65|30|target-height
move=cannon|x|10|any|13|0:4
aim-lock=cannon|1
release=cannon|10|none|0|beam
)";
    const auto p=parse(setup);
    BorgGameplayProfile rejected;
    for(const auto* extra:{"aim-lock=cannon|0","aim-lock=other|0","guidance=beam|snapshot|0.1|10"})
        check(!borg_read_gameplay_profile(std::string(setup)+extra,&rejected,nullptr),"refuse duplicate, unknown or steering aim override");
    for(bool moving:{false,true}){
        NativeBattleMvp world;BattleMvpConfig config;WallGround wall;CommitMuzzle origin;
        config.packages={p,p};config.spawn={BattlePoint{0,0},BattlePoint{0,400}};
        check(world.start(config,&wall),"beam test on synthetic wall terrain");
        for(unsigned i=0;i<24;++i){
            GamePadState recipient;
            if(moving&&i>=2)recipient.dpad_right=true;
            check(world.step({GamePadState{},recipient},false,&wall,nullptr,nullptr,nullptr,
                  {BorgBattleInputs{u8(i==0?BorgBattleInputs::x:0),0,0},{}},&origin),"committed beam visit");
            if(i==1)check(has(world,BorgBattleEventKind::AimCommitted)&&world.actors()[0].package_aim_heading&&!world.shots_spawned(),
                "commits on the authored frame before release");
            if(i==10)check(world.shots_spawned()==1&&world.projectiles()[0].velocity==BattleMvpVec{0,0,60},
                "offset muzzle does not retarget the committed release");
        }
        check(origin.visits==1&&wall.queries==0&&!world.actors()[0].package_aim_heading&&world.hits()==unsigned(!moving),
              "non-occluding beam still requires overlap and clears aim after the move");
    }
}
void body_heading_release_cases(){
    const auto base=std::string(player)+"origin=shot|0|0|0|0|fixed\n";
    BorgGameplayProfile refused;
    check(!borg_read_gameplay_profile(base+"launch-heading=shot|body\nlaunch-heading=shot|body\n",&refused,nullptr)&&
          !borg_read_gameplay_profile(std::string(player)+"origin=shot|0|0|0|0|target-height\nlaunch-heading=shot|body\n",&refused,nullptr),
          "body heading refuses duplicate bindings or a target-pitch socket");
    struct Muzzle final:IBorgBattleOrigins{
        bool socket(unsigned,const BattleMvpActor&,u32,u8,u16,const BorgBattleSocket&,BattleMvpVec* out)const override{
            *out={18,60,54};return true;
        }
    } muzzle;
    for(bool body:{false,true}){
        NativeBattleMvp world;BattleMvpConfig config;
        config.packages={parse(base+(body?"launch-heading=shot|body\n":"")),parse(target)};
        config.spawn={BattlePoint{0,0},BattlePoint{0,1000}};
        check(world.start(config),"body-heading release fixture");
        GamePadState right;right.dpad_right=true;
        for(unsigned i=0;i<3;++i)check(world.step({right,GamePadState{}},false,nullptr,nullptr,nullptr,nullptr,
                      {BorgBattleInputs{u8(i==0?BorgBattleInputs::b:0),0,0},{}},&muzzle),"body-heading release tick");
        const auto& actor=world.actors()[0];const auto& shot=world.projectiles()[0];
        check(shot.active&&shot.previous==BattleMvpVec{18,60,54}&&actor.facing.x>.1f&&actor.aim_facing.x<.01f,
              "muzzle position, movement heading and target aim stay independent");
        if(body)check(std::abs(shot.velocity[0]-actor.facing.x*60)<.001f&&std::abs(shot.velocity[2]-actor.facing.z*60)<.001f,
                      "body mode uses accepted heading at release");
        else check(shot.velocity[0]<0&&shot.velocity[2]>59,"ordinary socket launch retains targeted aim");
    }
}
void sustained_beam_cases(){
    constexpr std::string_view setup=R"(GFBORG-GAMEPLAY 1
variant=normal
movement=grounded-v1|8|13|14
health=250
rig-prefix=visual/prefix
pose-profile=visual/pose
idle=0:0
walk=0:1
air=0:2
dash=0:3
projectile=ray|beam|1|60|0|0|0|8|12|7|ignore-floor
sustained-beam=ray|700|2|0.05
look=ray|90|255|210|65
move=channel|x|10|any|17|0:4
release=channel|2|none|0|ray
)";
    const auto p=parse(setup),plain=parse(target),recover=parse(std::string(target)+"beam-recovery=3\n");
    std::string clipped_text(setup);
    clipped_text.replace(clipped_text.find("ignore-floor"),12,"clip-on-stage");
    const auto clipped=parse(clipped_text);
    BorgGameplayProfile rejected;
    for(const auto* extra:{"aim-lock=channel|0","sustained-beam=ray|110|1|0.1","guidance=ray|snapshot|0.1|10",
                            "sustained-beam=ghost|110|1|0.1","beam-recovery=121"})
        check(!borg_read_gameplay_profile(std::string(setup)+extra,&rejected,nullptr),"sustained beam rejects ambiguous policy");
    auto invalid_clip=clipped_text;
    invalid_clip.erase(invalid_clip.find("sustained-beam="),std::string_view("sustained-beam=ray|700|2|0.05\n").size());
    check(!borg_read_gameplay_profile(invalid_clip,&rejected,nullptr),"travelling bolt cannot opt into sustained clipping");
    for(unsigned scenario=0;scenario<7;++scenario){
        NativeBattleMvp world;BattleMvpConfig config;WallGround wall;
        config.packages={scenario>=4?clipped:p,scenario==1?recover:scenario==3?p:plain};
        config.spawn={BattlePoint{0,0},BattlePoint{0,400}};
        if(scenario==6)wall.plane=500;
        if(scenario>=4)check(!world.start(config),"clipped beam requires qualified stage queries at start");
        check(world.start(config,&wall),"bind sustained beam fixture");
        for(unsigned i=0;i<18;++i){
            GamePadState caster;
            if(scenario==2&&i>=2)caster.dpad_right=true;
            if(scenario==4&&i==7)wall.plane=900;
            if(scenario==5&&i==3)wall.available=false;
            check(world.step({caster,GamePadState{}},false,&wall,nullptr,nullptr,nullptr,
                {BorgBattleInputs{u8(i==0?BorgBattleInputs::x:0),0,0},
                 BorgBattleInputs{u8(i==0&&scenario==3?BorgBattleInputs::x:0),0,0}}),"active beam visit");
            const auto& shot=world.projectiles()[0];
            if(i==2)check(shot.active&&shot.sustained&&shot.package_radius==8&&shot.package_look.size==90&&
                          std::abs(shot.velocity[0])<20&&std::abs(std::hypot(shot.velocity[0],shot.velocity[2])-700)<.001f,
                          "one shared object begins along release aim without a same-visit turn");
            if(i==3&&scenario==2)check(shot.velocity[0]>20&&shot.velocity[0]<60,"one steering step is bounded");
            if(i==4&&scenario==1)check(world.hits()==1,"damage pulse is skipped during recovery");
            if(i==2&&scenario==3)check(world.shots_spawned()==2&&!world.projectiles()[1].active&&world.actors()[0].health==250,
                                      "a hit interrupts the other active owner's beam before its next damage check");
            if(i==2&&(scenario==4||scenario==5))check(shot.beam_fraction>0&&shot.beam_fraction<.02f&&!world.hits(),
                "near wall clips the segment before the target without retiring it");
            if(i==3&&scenario==5)check(!shot.active&&!world.hits(),"refused stage query retires the beam before damage");
            if(i==7&&scenario==4)check(shot.active&&shot.beam_fraction==1&&!world.hits(),
                "the visual endpoint refreshes even on updates without a damage pulse");
            if(i==2&&scenario==6)check(shot.beam_fraction<1&&world.hits()==1,"target ahead of obstruction receives damage");
            if(i==14)check(!shot.active,"beam expires once and does not linger on the last sample");
        }
        const auto hits=world.hits();
        check(wall.queries==(scenario<4?0u:scenario==5?2u:12u)&&world.actors()[1].health==(scenario==3?250u:500u)-7*hits,
              "only clipped beams query stage each visit; visible width does not define contact");
        check(scenario==2?(hits>0&&hits<6):hits==(scenario==1||scenario==4?3u:scenario==5?0u:6u),
              "damage tracks eligible exposure, not a guaranteed total");
    }
    std::string swept_text(clipped_text);
    const auto steer=swept_text.find("|700|2|0.05");
    swept_text.replace(steer,std::string_view("|700|2|0.05").size(),"|700|2|0");
    swept_text+="beam-sweep=ray|330|490|32\n";
    const auto swept=parse(swept_text);
    check(!borg_read_gameplay_profile(clipped_text+"beam-sweep=ray|330|490|32\n",&rejected,nullptr),
          "scripted sweep cannot steer");
    NativeBattleMvp world;BattleMvpConfig config;WallGround floor;
    floor.plane_is_floor=true;config.packages={swept,plain};config.spawn={BattlePoint{0,0},BattlePoint{200,900}};
    check(world.start(config,&floor),"bind scripted sweep");
    BattleMvpVec seed{},delta{};
    unsigned contacts=0;
    for(unsigned i=0;i<18;++i){
        GamePadState caster;if(i>=3)caster.dpad_right=true;
        check(world.step({caster,GamePadState{}},false,&floor,nullptr,nullptr,nullptr,
                         {BorgBattleInputs{u8(i==0?BorgBattleInputs::x:0),0,0},{}}),"sweep step");
        const auto& shot=world.projectiles()[0];
        if(i==2){seed=shot.position;delta=shot.velocity;check(delta[1]==-490,"sweep has fixed pitch");}
        if(i==13){
            check(shot.active&&shot.velocity==delta&&std::abs(std::hypot(shot.position[0]-seed[0],shot.position[2]-seed[2])-330)<.001f&&
                shot.position[1]==seed[1],"sweep traverses the declared straight path independently of caster input");
        }
        contacts+=has(world,BorgBattleEventKind::StageImpact);
    }
    unsigned effects=0;
    for(const auto& effect:world.impacts())if(effect.explosion){
        check(std::abs(effect.position[1])<.001f&&effect.radius==32,"visual explosion at accepted terrain contact");
        ++effects;
    }
    check(contacts==6&&effects==6&&floor.queries==12&&!world.hits(),"sweep terrain effects have a cadence and do not deal blast damage");
}
void bomb_proximity_cases(){
    constexpr std::string_view setup=R"(GFBORG-GAMEPLAY 1
variant=normal
movement=grounded-v1|8|13|14
health=250
rig-prefix=visual/prefix
pose-profile=visual/pose
idle=0:0
walk=0:1
air=0:2
dash=0:3
stock=bombs|1|on-return
projectile=charge|bullet|1|20|-60|1|0|8|12|0|land-on-floor
actor-contact=charge|ignore
blast=charge|expiry|230|20|opponent
return-stock=charge|expiry-or-proximity
move=drop|x|10|any|5|0:4
release=drop|1|bombs|1|charge
)";
    const auto armed_text=std::string(setup)+"bomb-proximity=charge|230|4|blast\n";
    const auto ordinary=parse(armed_text);
    const auto quiet=parse(std::string(setup)+"bomb-proximity=charge|230|4|retire\n");
    BorgGameplayProfile bad;
    check(!borg_read_gameplay_profile(setup,&bad,nullptr)&&
          !borg_read_gameplay_profile(std::string(setup)+"bomb-proximity=charge|230|12|blast\n",&bad,nullptr),
          "proximity return requires a valid trigger");
    const auto has_event=[](auto events,BorgBattleEventKind kind){
        return std::find_if(events.begin(),events.end(),[kind](auto e){return e.kind==kind;});
    };
    for(unsigned scenario=0;scenario<4;++scenario){
        NativeBattleMvp world;BattleMvpConfig cfg;PullGround ground;
        cfg.packages={scenario==1?quiet:ordinary,parse(target)};
        cfg.spawn={BattlePoint{0,0},BattlePoint{0,scenario==2?900.f:200.f}};
        check(world.start(cfg,&ground),"bomb authored fixture");
        unsigned proximity=0,blasts=0,returns=0;
        for(unsigned i=0;i<18;++i){
            if(i==3&&scenario==3)ground.kind=PullGround::Unknown;
            check(world.step({},false,&ground,nullptr,nullptr,nullptr,
                {BorgBattleInputs{u8(i==0?BorgBattleInputs::x:0),0,0},{}}),"bomb visit");
            if(i==3)check(world.package_action(0).stocks[0].current==0&&!has(world,BorgBattleEventKind::BombProximity),
                "unarmed or unsupported bomb does not sense an opponent");
            const auto events=world.package_events();
            const auto prox=has_event(events,BorgBattleEventKind::BombProximity),blast=has_event(events,BorgBattleEventKind::BlastTriggered),
                retired=has_event(events,BorgBattleEventKind::Retired),returned=has_event(events,BorgBattleEventKind::StockReturned);
            if(prox!=events.end()){
                ++proximity;check(i==5&&prox->recipient==1&&prox<retired,"armed landed bomb records its proximity recipient before retirement");
            }
            if(blast!=events.end()){++blasts;check(blast<retired,"blast precedes retirement");}
            if(returned!=events.end()){++returns;check(retired<returned,"stock return follows retirement");}
        }
        check(proximity==unsigned(scenario<2)&&blasts==unsigned(scenario==0||scenario==2)&&returns==unsigned(scenario!=3)&&
              world.actors()[1].health==(scenario==0?480u:500u),
              "blast, quiet retirement, fuse, and unknown support keep independent results");
    }
    const auto magnetic=parse(std::string(setup)+"bomb-proximity=charge|100|4|blast\nbomb-attraction=charge|320|3|6\n");
    check(!borg_read_gameplay_profile(armed_text+"bomb-attraction=charge|229|3|6\n",&bad,nullptr),
          "attraction must reach farther than the blast trigger");
    for(unsigned scenario=0;scenario<3;++scenario){
        NativeBattleMvp world;BattleMvpConfig cfg;WallGround wall;
        wall.plane=scenario==1?76.f:500.f;
        cfg.packages={magnetic,parse(target)};cfg.spawn={BattlePoint{0,0},BattlePoint{0,scenario==2?600.f:200.f}};
        check(!world.start(cfg)&&world.start(cfg,&wall),"magnetic carrier requires the stage query");
        f32 landed_z=0;
        for(unsigned i=0;i<18;++i){
            check(world.step({},false,&wall,nullptr,nullptr,nullptr,
                {BorgBattleInputs{u8(i==0?BorgBattleInputs::x:0),0,0},{}}),"magnetic bomb visit");
            const auto& shot=world.projectiles()[0];
            if(i==4){landed_z=shot.position[2];check(shot.landed&&shot.bomb_travel_speed==0,"no attraction before arming");}
            if(i==5)check(shot.position[2]==landed_z+(scenario==0?3.f:0.f),"first accepted attraction step or refusal");
            if(i==6)check(shot.position[2]==landed_z+(scenario==0?9.f:0.f),"second step caps at authored speed");
            if(i==7)check(shot.position[2]==landed_z+(scenario==0?15.f:0.f)&&!has(world,BorgBattleEventKind::BombProximity)&&
                world.package_action(0).stocks[0].current==0,"capped motion does not consume the sensor or return stock");
        }
        check((scenario==2?wall.queries==0:wall.queries>0)&&world.package_action(0).stocks[0].current==1,
              "out of range skips wall queries and only the terminal fuse returns stock");
    }
    const auto with_shards=std::string(setup)+
        "projectile=shard|bullet|4|28|6|1|0|3|10|4|stop-on-floor\n"
        "bomb-proximity=charge|100|4|retire\nbomb-fuse-tail=charge|3\nblast-children=charge|shard\n";
    const auto fragmentation=parse(with_shards);
    check(!borg_read_gameplay_profile(with_shards+"blast-children=shard|charge\n",&bad,nullptr),
          "fragment definitions cannot recurse");
    NativeBattleMvp world;BattleMvpConfig cfg;PullGround ground;
    cfg.packages={fragmentation,parse(target)};cfg.spawn={BattlePoint{0,0},BattlePoint{0,560}};
    check(world.start(cfg,&ground),"delayed fragmentation fixture");
    unsigned arm_tick=0,burst_tick=0;
    for(unsigned i=0;i<22;++i){
        check(world.step({},false,&ground,nullptr,nullptr,nullptr,
              {BorgBattleInputs{u8(i==0?BorgBattleInputs::x:0),0,0},{}},nullptr),"delayed bomb visit");
        const auto events=world.package_events();
        if(has(world,BorgBattleEventKind::BombFuseCommitted))arm_tick=world.tick();
        if(arm_tick&&!burst_tick)check(world.tick()-arm_tick<=3,"bounded tail");
        if(has(world,BorgBattleEventKind::BlastChildren)){
            burst_tick=world.tick();
            const auto blast=has_event(events,BorgBattleEventKind::BlastTriggered);
            const auto returned=has_event(events,BorgBattleEventKind::StockReturned);
            const auto children=has_event(events,BorgBattleEventKind::BlastChildren);
            check(blast<returned&&returned<children&&children->value==4&&children->object==1,
                  "blast and one stock return precede the deferred child allocation");
            f32 x=0,z=0;unsigned count=0;
            for(const auto& shot:world.projectiles())if(shot.active){
                ++count;x+=shot.velocity[0];z+=shot.velocity[2];
                check(shot.id>1&&shot.owner==0&&shot.born_tick==burst_tick&&shot.position==BattleMvpVec{0,0,74}&&shot.life==10,
                      "fragment starts at blast origin, inherits actor credit, and waits for the next visit");
            }
            check(count==4&&std::abs(x)<.001f&&std::abs(z)<.001f,"even radial dispersion");
        }
        if(arm_tick&&!burst_tick)check(world.shots_spawned()==1&&world.package_action(0).stocks[0].current==0&&world.projectiles()[0].active,
              "fuse tail retains the carrier and spent stock");
    }
    check(arm_tick&&burst_tick==arm_tick+3&&world.shots_spawned()==5&&world.package_action(0).stocks[0].current==1,
          "one authored fuse tail and one child volley");
}
void smoke_cases(){
    constexpr std::string_view smoke_setup=R"(GFBORG-GAMEPLAY 1
variant=normal
movement=grounded-v1|8|13|14
health=250
rig-prefix=visual/prefix
pose-profile=visual/pose
idle=0:0
walk=0:1
air=0:2
dash=0:3
projectile=can|bullet|1|20|-60|1|0|8|5|0|land-on-floor
actor-contact=can|ignore
blast=can|expiry|100|0|opponent
move=cloud|x|10|any|4|0:4
release=cloud|1|none|0|can
)";
    constexpr std::string_view aimer_setup=R"(GFBORG-GAMEPLAY 1
variant=normal
movement=grounded-v1|8|13|14
health=250
rig-prefix=visual/prefix
pose-profile=visual/pose
idle=0:0
walk=0:1
air=0:2
dash=0:3
projectile=bolt|bullet|1|8|10|0|0|3|8|1|ignore-floor
move=committed|b|10|any|12|0:4
aim-lock=committed|0
release=committed|7|none|0|bolt
)";
    BorgGameplayProfile rejected;
    check(!borg_read_gameplay_profile(smoke_setup,&rejected,nullptr),"zero-damage blast needs an actual smoke effect");
    for(const bool obstructs:{false,true}){
        NativeBattleMvp world;BattleMvpConfig cfg;
        cfg.packages={parse(std::string(smoke_setup)+(obstructs?"blast-smoke=can|110|5|block-aim\n":"blast-smoke=can|110|5|visual\n")),parse(aimer_setup)};
        cfg.spawn={BattlePoint{0,0},BattlePoint{0,400}};
        check(world.start(cfg),"smoke fixture");
        const auto facing=world.actors()[1].facing;
        unsigned born=0;
        for(unsigned i=0;i<17;++i){
            GamePadState target_pad;if(i>=2)target_pad.dpad_right=true;
            check(world.step({GamePadState{},target_pad},false,nullptr,nullptr,nullptr,nullptr,
                  {BorgBattleInputs{u8(i==0?BorgBattleInputs::x:0),0,0},BorgBattleInputs{u8(i==0?BorgBattleInputs::b:0),0,0}}),
                  "smoke step");
            if(has(world,BorgBattleEventKind::SmokeCreated)){
                born=world.tick();check(world.actors()[1].package_target_visible&&world.hits()==0,"creation cannot block aim earlier in the visit or deal damage");
            }
            if(born&&world.tick()>born&&world.tick()<=born+5)
                check(world.actors()[1].package_target_visible!=obstructs,"visual and mechanical clouds differ");
            if(i==7)check(world.actors()[1].package_aim_heading==facing&&world.shots_spawned()==2,
                          "committed shot still releases through the new cloud");
        }
        check(born&&world.actors()[1].package_target_visible&&
              std::none_of(world.smoke_clouds().begin(),world.smoke_clouds().end(),[](const auto& c){return c.parent!=0;}),
              "cloud expiry restores acquisition and leaves no live volume");
        check(world.start(cfg)&&world.smoke_clouds()[0].parent==0,"restart clears cloud ownership");
    }
}
void power_field_cases(){
    constexpr std::string_view setup=R"(GFBORG-GAMEPLAY 1
variant=normal
movement=grounded-v1|8|13|14
health=250
rig-prefix=visual/prefix
pose-profile=visual/pose
idle=0:0
walk=0:1
air=0:2
dash=0:3
projectile=bolt|beam|1|140|0|0|0|8|20|10|ignore-floor
move=place|x|10|any|4|0:4
move=fire|b|15|any|4|0:4
release=fire|1|none|0|bolt
)";
    const auto placed=std::string(setup)+"power-field=place|1|20|30|100|60|200|150\n";
    std::string stopped(placed);
    stopped.replace(stopped.find("ignore-floor"),12,"stop-on-stage");
    BorgGameplayProfile invalid;
    check(!borg_read_gameplay_profile(placed+"power-field=place|1|20|8|100|60|200|150\n",&invalid,nullptr),
          "cannot declare two fields on the same move");
    for(unsigned scenario=0;scenario<5;++scenario){
        NativeBattleMvp world;BattleMvpConfig config;WallGround wall;
        config.packages={parse(scenario==1?std::string(setup)+"power-field=place|1|20|30|190|60|200|150\n":
                              scenario==2?stopped:scenario==3?std::string(setup)+"power-field=place|1|20|2|100|60|200|150\n":placed),
                         scenario==4?parse(placed):parse(target)};
        config.spawn={BattlePoint{0,0},BattlePoint{0,190}};
        check(world.start(config,&wall),"power field placement");
        const auto visit=[&](u8 a=0,u8 b=0){
            check(world.step({},false,&wall,nullptr,nullptr,nullptr,{BorgBattleInputs{a,0,0},BorgBattleInputs{b,0,0}}),
                  "power field visit");
        };
        visit(BorgBattleInputs::x,scenario==4?BorgBattleInputs::x:0);visit();
        check(has(world,BorgBattleEventKind::PowerFieldPlaced)&&!world.shots_spawned(),"placement is independent of projectile allocation");
        const auto position=world.fields()[0].centre;
        visit();visit();visit();visit(BorgBattleInputs::b);visit();
        const bool powered=scenario==0||scenario==4;
        check(has(world,BorgBattleEventKind::ShotPowered)==powered&&
              world.actors()[1].health==(scenario==4?250u:500u)-(powered?20u:scenario==2?0u:10u),
              "only a crossing before a receiver or wall boosts direct damage once");
        const auto& bolt=world.projectiles()[0];
        check(bolt.package_radius==8&&bolt.power_visual_percent==(powered?150:100),"visual width changes independently of collision");
        if(world.fields()[0].id)check(world.fields()[0].centre==position,"placed sphere doesn't chase the target");
        check(world.start(config,&wall)&&!world.fields()[0].id,"field state does not survive restart");
    }
}
void slow_field_cases(){
    constexpr std::string_view setup=R"(GFBORG-GAMEPLAY 1
variant=normal
movement=grounded-v1|8|13|14
health=250
rig-prefix=visual/prefix
pose-profile=visual/pose
idle=0:0
walk=0:1
air=0:2
dash=0:3
projectile=bolt|beam|1|100|0|0|0|8|14|10|ignore-floor
guidance=bolt|tracking|0.1|9
move=place|x|20|any|4|0:4
move=fire|b|15|any|4|0:4
release=fire|1|none|0|bolt
)";
    for(bool break_lock:{false,true}){
        const auto p=parse(std::string(setup)+"slow-field=place|1|25|24|200|60|50|50|"+
                           (break_lock?"break-guidance\n":"keep-guidance\n"));
        NativeBattleMvp world;BattleMvpConfig config;
        config.packages={p,p};config.spawn={BattlePoint{0,0},BattlePoint{0,400}};
        check(world.start(config),"slow-field fixture");
        const auto visit=[&](u8 a=0,u8 b=0){
            check(world.step({},false,nullptr,nullptr,nullptr,nullptr,{BorgBattleInputs{a,0,0},BorgBattleInputs{b,0,0}}),"slow visit");
        };
        visit(BorgBattleInputs::x);visit();
        check(has(world,BorgBattleEventKind::SlowFieldPlaced)&&world.fields()[0].centre==BattleMvpVec{0,60,200},"fixed enemy field");
        visit(0,BorgBattleInputs::b);visit();
        const auto& shot=world.projectiles()[0];
        check(shot.position[2]==246&&shot.package_guide_ticks==8,"ordinary approach does not lose guidance");
        visit();
        check(std::abs(shot.position[2]-185.5f)<.001f&&shot.velocity[2]==-100&&
              has(world,BorgBattleEventKind::FieldGuidanceBroken,1)==break_lock&&shot.package_guide_ticks==(break_lock?0:7),
              "field slows only the inside part, plus optional permanent guide break");
        visit();
        check(std::abs(shot.position[2]-96.f)<.001f&&shot.velocity[2]==-100&&shot.package_guide_ticks==(break_lock?0:6),
              "remaining outside segment restores normal speed");
        check(world.start(config)&&!world.fields()[0].id,"restart clears field state");
    }
    const auto actor_profile=parse(std::string(setup)+"slow-field=place|1|60|11|400|50|50|100|keep-guidance\n");
    NativeBattleMvp world;BattleMvpConfig config;config.packages={actor_profile,actor_profile};
    config.spawn={BattlePoint{0,0},BattlePoint{0,400}};
    check(world.start(config),"slow movement fixture");
    for(unsigned i=0;i<24;++i){
        GamePadState mover;if(i>=2)mover.dpad_right=true;
        check(world.step({GamePadState{},mover},false,nullptr,nullptr,nullptr,nullptr,
                         {BorgBattleInputs{u8(i==0?BorgBattleInputs::x:0),0,0},{}}),"actor in slow sphere");
        if(i==10)check(std::abs(world.actors()[1].movement_speed-4)<.001f&&world.actors()[1].package_target_visible,
                       "enemy walk slows without affecting aiming visibility");
    }
    check(std::abs(world.actors()[1].movement_speed-8)<.001f&&!world.fields()[0].id,
          "normal walk cap returns without a lingering slow status");
    const auto combined=std::string(setup)+"field=place|1|box|110|45|250|30|200|50|80|195|220|82\n"
        "field-effect=place|actor-speed|source|150\nfield-effect=place|actor-speed|enemies|50\n"
        "field-effect=place|shot-damage|allies|200\nfield-effect=place|shot-speed|enemies|75\n";
    BorgGameplayProfile rejected;
    check(!borg_read_gameplay_profile(combined+"field-effect=place|actor-speed|source|125\n",&rejected,nullptr),
           "duplicate effect and scope are refused");
    check(!borg_read_gameplay_profile(combined+"field-texture=place|../unsafe.rgba\n",&rejected,nullptr),
           "field visual must be a safe local asset path");
    const auto complex=parse(combined);config.packages={complex,complex};
    check(world.start(config),"registered field effects fixture");
    for(unsigned i=0;i<15;++i){
        GamePadState mover;if(i>=2)mover.dpad_right=true;
        check(world.step({mover,mover},false,nullptr,nullptr,nullptr,nullptr,
                         {BorgBattleInputs{u8(i==0?BorgBattleInputs::x:0),0,0},{}}),"mixed field visit");
        if(i==1)check(has(world,BorgBattleEventKind::FieldPlaced)&&world.fields()[0].spec.effects.size()==4&&
                      world.fields()[0].spec.shape==BorgFieldShape::Box,"one field owns both scopes and both subject classes");
        if(i==14)check(std::abs(world.actors()[0].movement_speed-11.7f)<.01f&&
                       std::abs(world.actors()[1].movement_speed-4)<.001f,"source speeds up while the same box slows an enemy");
    }
}
void resident_beam_field_cases(){
    constexpr std::string_view setup=R"(GFBORG-GAMEPLAY 1
variant=normal
movement=grounded-v1|8|13|14
health=250
rig-prefix=visual/prefix
pose-profile=visual/pose
idle=0:0
walk=0:1
air=0:2
dash=0:3
projectile=ray|beam|1|60|0|0|0|8|10|7|ignore-floor
sustained-beam=ray|700|2|0.05
move=place|x|20|any|4|0:4
move=channel|b|10|any|17|0:4
release=channel|1|none|0|ray
)";
    const auto power=parse(std::string(setup)+"power-field=place|1|45|7|250|60|200|150\n");
    const auto slow=parse(std::string(setup)+"slow-field=place|1|100|8|200|60|100|25|break-guidance\n");
    std::string clipped(setup);
    clipped.replace(clipped.find("ignore-floor"),12,"clip-on-stage");
    const auto clipped_power=parse(clipped+"power-field=place|1|45|7|250|60|200|150\n");
    for(unsigned scenario=0;scenario<4;++scenario){
        NativeBattleMvp world;BattleMvpConfig config;WallGround wall;
        wall.plane=170;
        config.packages={scenario==2?clipped_power:power,scenario==3?slow:parse(target)};
        config.spawn={BattlePoint{0,0},BattlePoint{0,scenario==1?190.f:400.f}};
        check(world.start(config,&wall),"resident field fixture");
        const auto visit=[&](u8 a=0,u8 b=0){
            check(world.step({},false,&wall,nullptr,nullptr,nullptr,{BorgBattleInputs{a,0,0},BorgBattleInputs{b,0,0}}),
                  "resident field visit");
        };
        visit(scenario!=3?BorgBattleInputs::x:0,scenario==3?BorgBattleInputs::x:0);
        for(unsigned i=0;i<4;++i)visit();
        visit(BorgBattleInputs::b);visit(); // tick 7, first pulse
        const auto& ray=world.projectiles()[0];
        const unsigned initial=scenario==3?250:500;
        const unsigned first_damage=scenario==0?14:scenario==1?7:0;
        check(ray.active&&ray.package_damage==7&&ray.package_radius==8&&world.actors()[1].health==initial-first_damage,
              "only a downstream contact gets power; clipped and slowed reach excludes farther targets");
        if(scenario<2)check(ray.beam_power_entry>0&&ray.beam_power_entry<ray.beam_fraction&&ray.beam_power_visual==150,
            "boosted portion has an independent draw width");
        else check(ray.beam_fraction<.4f&&!ray.power_field&&ray.beam_power_visual==100,"obstruction precedes power");
        visit();visit(); // tick 9, power field expires
        if(scenario<2)check(!ray.power_field&&world.actors()[1].health==initial-first_damage-7,
                            "resident boost is recalculated, not latched after expiry");
        if(scenario==3){
            check(ray.beam_fraction<.4f&&world.hits()==0,"slow field doesn't create a delayed hit");
            visit();check(ray.beam_fraction==1&&world.hits()==0,"reach restores on next sample, not pulse timer");
            visit();check(world.hits()==1&&world.actors()[1].health==243,"the original next eligible pulse can hit");
        }
    }
}
void overlapping_field_cases(){
    constexpr std::string_view field_setup=R"(GFBORG-GAMEPLAY 1
variant=normal
movement=grounded-v1|8|13|14
health=250
rig-prefix=visual/prefix
pose-profile=visual/pose
idle=0:0
walk=0:1
air=0:2
dash=0:3
move=first|x|20|any|4|0:4
move=second|b|10|any|4|0:4
field=first|1|sphere|20|20|20|40|300|60|80|150|198|80
field-effect=first|shot-speed|enemies|50
)";
    constexpr std::string_view projectile_setup=R"(GFBORG-GAMEPLAY 1
variant=normal
movement=grounded-v1|8|13|14
health=250
rig-prefix=visual/prefix
pose-profile=visual/pose
idle=0:0
walk=0:1
air=0:2
dash=0:3
projectile=bolt|beam|1|140|0|0|0|8|20|10|ignore-floor
move=fire|b|10|any|4|0:4
release=fire|1|none|0|bolt
)";
    const auto shooter=parse(projectile_setup);
    for(const bool overlap:{false,true}){
        const auto casters=parse(std::string(field_setup)+"field=second|1|box|20|20|20|40|"+
                           (overlap?"285":"255")+"|60|80|150|198|80\nfield-effect=second|shot-speed|enemies|25\n");
        NativeBattleMvp world;BattleMvpConfig config;
        config.packages={casters,shooter};config.spawn={BattlePoint{0,0},BattlePoint{0,400}};
        check(world.start(config),"mixed-shape field fixture");
        const auto visit=[&](u8 a=0,u8 b=0){
            check(world.step({},false,nullptr,nullptr,nullptr,nullptr,
                {BorgBattleInputs{a,0,0},BorgBattleInputs{b,0,0}}),"multiple slab visits");
        };
        visit(BorgBattleInputs::x);for(unsigned i=0;i<4;++i)visit();
        visit(BorgBattleInputs::b, BorgBattleInputs::b);visit(); // placement and shot use the same tick
        const auto& shot=world.projectiles()[0];
        const f32 expected=overlap?284.f:267.75f;
        check(world.fields()[0].id&&world.fields()[1].id&&std::abs(shot.position[2]-expected)<.001f&&shot.velocity[2]==-140,
              "traversal spends one update across both fields, with the smallest cap in overlap");
        if(overlap){visit();check(std::abs(shot.position[2]-201.f)<.001f,"next visit exits the overlapping fields without losing nominal speed");}
    }
    const auto two_fields=std::string(field_setup)+
        "field=second|1|box|20|20|20|40|255|60|80|150|198|80\nfield-effect=second|shot-speed|enemies|25\n";
    BorgGameplayProfile rejected;
    check(!borg_read_gameplay_profile(two_fields+"field-follow=first|opponent\n",&rejected,nullptr)&&
          !borg_read_gameplay_profile(two_fields+"field-follow=first|source\nfield-follow=first|source\n",&rejected,nullptr),
          "follow must belong to one source-owned declared field");
    for(bool follow:{false,true}){
        NativeBattleMvp world;BattleMvpConfig config;
        config.packages={parse(two_fields+(follow?"field-follow=first|source\n":"")),shooter};
        config.spawn={BattlePoint{0,0},BattlePoint{0,400}};
        check(world.start(config),"field follow fixture");
        check(world.step({},false,nullptr,nullptr,nullptr,nullptr,{BorgBattleInputs{BorgBattleInputs::x,0,0},{}})&&
              world.step({},false)&&world.fields()[0].centre==BattleMvpVec{0,60,300},"placement snapshots offset");
        GamePadState right;right.dpad_right=true;
        for(unsigned i=0;i<6;++i)check(world.step({right,GamePadState{}},false),"source walks after placement");
        const auto& field=world.fields()[0];
        const auto& actor=world.actors()[0];
        check(field.centre==BattleMvpVec{follow?actor.position[0]:0.f,60,300}&&field.offset==BattleMvpVec{0,60,300},
              "fixed field stays put; following field tracks translation without rotating its offset");
        check(world.start(config)&&!world.fields()[0].id,"restart clears attached fields");
    }
    struct FieldSocket final:IBorgBattleOrigins {
        mutable unsigned visits=0;
        u32 refusal=0;
        bool socket(unsigned owner,const BattleMvpActor& actor,u32 tick,u8 move,u16 frame,
                    const BorgBattleSocket& spec,BattleMvpVec* out)const override{
            check(owner==0&&move==actor.package_move&&frame==actor.package_frame&&
                  spec.bone==12&&spec.local==BattleMvpVec{1,2,3}&&!spec.aim_height,
                  "socket uses current accepted pose instead of deployment action when that action is over");
            ++visits;
            *out={f32(tick)*7,60,300};
            if(tick==refusal)(*out)[0]=NAN; // a nonfinite sample must not publish
            return true;
        }
    };
    std::string socket_setup(two_fields);
    socket_setup.replace(socket_setup.find("|300|60|"),std::string_view("|300|60|").size(),"|0|0|");
    socket_setup+="field-socket=first|12|1|2|3\n";
    check(!borg_read_gameplay_profile(two_fields+"field-socket=first|12|1|2|3\n",&rejected,nullptr),
          "ambiguous root and socket offsets are refused");
    for(unsigned scenario=0;scenario<3;++scenario){
        NativeBattleMvp world;BattleMvpConfig config;FieldSocket sample;
        config.packages={parse(socket_setup+(scenario?"field-follow=first|source\n":"")),shooter};
        config.spawn={BattlePoint{0,0},BattlePoint{0,400}};
        sample.refusal=scenario==2?6:0;
        check(world.start(config),"field socket fixture");
        const auto step=[&](u8 button=0){
            check(world.step({},false,nullptr,nullptr,nullptr,nullptr,{BorgBattleInputs{button,0,0},{}},&sample),
                "pose-anchored field tick");
        };
        step(BorgBattleInputs::x);step();
        check(world.fields()[0].id&&world.fields()[0].centre==BattleMvpVec{14,60,300},"place at actual release socket");
        step();step();step();
        check(world.actors()[0].package_move==BorgBattleActionState::none&&
              world.fields()[0].centre==BattleMvpVec{scenario?35.f:14.f,60,300},
              "only following field resamples the new idle pose");
        step();
        check(scenario==2?!world.fields()[0].id&&has(world,BorgBattleEventKind::FieldAnchorLost):
              world.fields()[0].id&&sample.visits==(scenario?5u:1u),"uncertain socket retires rather than using stale centre or fallback");
    }
}
void blast_accuracy_cases(){
    constexpr std::string_view ink_bomb=R"(GFBORG-GAMEPLAY 1
variant=normal
movement=grounded-v1|8|13|14
health=250
rig-prefix=visual/prefix
pose-profile=visual/pose
idle=0:0
walk=0:1
air=0:2
dash=0:3
projectile=can|bullet|1|20|-60|1|0|8|5|0|land-on-floor
actor-contact=can|ignore
blast=can|expiry|170|10|opponent
move=throw|x|10|any|4|0:4
release=throw|1|none|0|can
)";
    constexpr std::string_view launcher=R"(GFBORG-GAMEPLAY 1
variant=normal
movement=grounded-v1|8|13|14
health=250
rig-prefix=visual/prefix
pose-profile=visual/pose
idle=0:0
walk=0:1
air=0:2
dash=0:3
projectile=bolt|beam|1|25|70|0|0|6|8|11|ignore-floor
move=fire|b|10|any|4|0:4
release=fire|1|none|0|bolt
)";
    BorgGameplayProfile refused;
    const std::string sensitive=std::string(launcher)+"accuracy-sensitive=bolt\n";
    check(!borg_read_gameplay_profile(sensitive+"guidance=bolt|tracking|0.1|6\n",&refused,nullptr),
          "guided travellers must not silently consume the direct-shot accuracy policy");
    f32 previous_deflection=0;
    for(unsigned scenario=0;scenario<5;++scenario){
        NativeBattleMvp world;BattleMvpConfig config;
        config.packages={parse(std::string(ink_bomb)+(scenario==0?"blast-ink=can|20|50|220\n":"blast-accuracy=can|20|0.4\n")),
                         parse(scenario==2?std::string(launcher):sensitive)};
        config.spawn={BattlePoint{0,0},BattlePoint{0,200}};
        check(world.start(config),"start accuracy fixture");
        const unsigned press=scenario==3?27:16;
        for(unsigned i=0;i<=press+1;++i){
            check(world.step({},false,nullptr,nullptr,nullptr,nullptr,
                         {BorgBattleInputs{u8(i==0?BorgBattleInputs::x:0),0,0},
                          BorgBattleInputs{u8(i==press?BorgBattleInputs::b:0),0,0}}),"accuracy visit");
            if(has(world,BorgBattleEventKind::BlastHit)){
                check(has(world,BorgBattleEventKind::AccuracyApplied)==(scenario!=0)&&
                      bool(world.ink_overlay(1))==(scenario==0)&&!world.ink_overlay(0),
                      "accepted recipient alone gets the declared visual or accuracy effect");
            }
        }
        const bool affected=scenario==1||scenario==4;
        const auto& shot=world.projectiles()[0];
        check(world.shots_spawned()==2&&shot.active&&shot.owner==1&&world.actors()[1].health==240&&
              has(world,BorgBattleEventKind::AccuracyAffected,1)==affected,
              "only eligible shots inside the status window are affected");
        check(std::abs(shot.previous[0])<.001f&&std::abs(shot.previous[1]-60)<.001f&&std::abs(shot.previous[2]-146)<.001f&&
              std::abs(std::hypot(shot.velocity[0],shot.velocity[2])-25)<.001f&&
              world.actors()[1].aim_facing==BattlePoint{0,-1}&&shot.package_damage==11,
              "no change to muzzle, lock direction, projectile speed or damage");
        if(affected){
            check(std::abs(shot.velocity[0])>.01f&&std::abs(shot.velocity[0])<=25*std::sin(.4f)+.001f,
                  "single-projectile launch has bounded deterministic yaw error");
            if(scenario==4)check(shot.velocity[0]==previous_deflection,"same release sequence reproduces the yaw");
            previous_deflection=shot.velocity[0];
        }else check(shot.velocity[0]==0,"visual ink, ineligible shots and expired penalties preserve launch aim");
    }
}
void movement_and_attraction_cases(){
    const auto pulse=parse(std::string(player)+"walk-pulse=16|8|120|20|8|13\n");
    const auto core_base=std::string(player)+
        "move=well|x|80|any|4|0:4\nfield=well|1|sphere|170|170|170|10|400|50|95|50|160|80\n";
    BorgGameplayProfile invalid;
    check(!borg_read_gameplay_profile(std::string(player)+"walk-pulse=1|8|120|20|8|13\n",&invalid,nullptr)&&
          !borg_read_gameplay_profile(core_base+"field-core-damage=well|enemies|171|7|3\n",&invalid,nullptr),
          "pulse stride and core extent are bounded");
    const auto debit_base=std::string(player)+
        "move=well|x|80|any|4|none\nfield=well|1|sphere|170|170|170|10|400|50|95|50|160|80\n"
        "field-core-damage=well|enemies|170|7|3\n";
    check(!borg_read_gameplay_profile(debit_base+"field-debit=well|primary|0\n",&invalid,nullptr)&&
          !borg_read_gameplay_profile(debit_base+"field-debit=well|missing|1\n",&invalid,nullptr),
          "field debit needs a bounded cost and a declared stock");
    {
        const auto debit=parse(debit_base+"field-debit=well|primary|1\n");
        NativeBattleMvp world;BattleMvpConfig config;config.packages={debit,parse(target)};
        config.spawn={BattlePoint{0,0},BattlePoint{0,140}};
        check(world.start(config),"field debit fixture");
        for(unsigned i=0;i<4;++i)check(world.step({},false,nullptr,nullptr,nullptr,nullptr,
            {BorgBattleInputs{u8(i==0?BorgBattleInputs::x:0),0,0},BorgBattleInputs{}}),"field debit step");
        check(world.fields()[0].id,"field debit places the field");
        check(world.package_action(0).stocks[0].current==1,"field debit spends one stock");
        check(world.package_action(0).stocks[0].previous==2,"field debit keeps the previous count");
    }
    {
        const auto charge=parse(std::string(player)+
            "move=charge-move|x|80|any|20|none\n"
            "field=charge-move|1|sphere|90|90|90|30|60|40|190|90|220|120\n"
            "field-follow=charge-move|source\nfield-core-damage=charge-move|enemies|90|10|6\n"
            "move-charge=charge-move|20\n");
        NativeBattleMvp world;BattleMvpConfig config;config.packages={charge,parse(target)};
        config.spawn={BattlePoint{0,0},BattlePoint{0,140}};
        check(world.start(config),"move charge fixture");
        for(unsigned i=0;i<24;++i)check(world.step({},false,nullptr,nullptr,nullptr,nullptr,
            {BorgBattleInputs{u8(i==0?BorgBattleInputs::x:0),0,0},BorgBattleInputs{}}),"move charge step");
        check(world.actors()[0].position[2]>10,"move charge moves the actor forward");
        check(world.fields()[0].id&&world.actors()[1].health<500,
              "move charge keeps the following field and damages");
    }
    for(unsigned scenario=0;scenario<4;++scenario){
        NativeBattleMvp world;BattleMvpConfig config;config.packages={pulse,parse(target)};
        config.spawn={BattlePoint{0,0},BattlePoint{0,140}};
        check(world.start(config),"walk pulse fixture");
        unsigned born=0;
        auto wave_origin=BattleMvpVec{};
        for(unsigned i=0;i<20;++i){
            GamePadState pad;
            pad.dpad_down=!born&&scenario!=3&&i<7;
            pad.b=scenario==1&&i==0;pad.l=scenario==2&&i==0;
            check(world.step({pad,{}},false),"walk pulse step");
            if(has(world,BorgBattleEventKind::WalkPulseCreated)){
                check(!born&&world.actors()[0].grounded&&!world.hits(),"walk pulse starts after accepted walking and has no same-tick damage");
                born=world.tick();wave_origin=world.actors()[0].position;
            }
            for(const auto& wave:world.walk_pulses())if(wave.id)
                check(wave.origin==wave_origin&&wave.radius(world.tick())>=8,"shockwave grows from a fixed ground point");
        }
        check(bool(born)==(scenario==0)&&world.hits()==unsigned(scenario==0)&&
              world.actors()[0].health==250&&world.actors()[1].health==(scenario==0?487u:500u),
              "only grounded walking pulses hit the opposing actor, once per wave");
    }
    for(bool enemy:{false,true}){
        NativeBattleMvp world;BattleMvpConfig config;
        config.packages={parse(core_base+"field-core-damage=well|"+(enemy?"enemies":"source")+"|40|7|3\n"),parse(target)};
        config.spawn={BattlePoint{0,0},BattlePoint{0,400}};
        check(world.start(config),"field core fixture");
        for(unsigned i=0;i<16;++i)check(world.step({},false,nullptr,nullptr,nullptr,nullptr,
            {BorgBattleInputs{u8(i==0?BorgBattleInputs::x:0),0,0},{}}),"field core tick");
        check(world.hits()==(enemy?3u:0u)&&world.actors()[0].health==250&&world.actors()[1].health==(enemy?479u:500u),
              "field damages on its own cadence and only in the chosen scope and core");
    }
    constexpr std::string_view caster=R"(GFBORG-GAMEPLAY 1
variant=normal
movement=grounded-v1|8|13|14
health=250
rig-prefix=visual/prefix
pose-profile=visual/pose
idle=0:0
walk=0:1
air=0:2
dash=0:3
move=well|x|80|any|4|0:4
field=well|1|box|170|170|170|10|300|100|95|50|160|80
)";
    for(bool enemy:{false,true}){
        NativeBattleMvp world;BattleMvpConfig config;
        config.packages={parse(std::string(caster)+"field-shot-pull=well|"+(enemy?"enemies":"source")+"|10\n"),parse(player)};
        config.spawn={BattlePoint{0,0},BattlePoint{0,400}};
        check(world.start(config),"shot attraction fixture");
        for(unsigned i=0;i<3;++i)check(world.step({},false,nullptr,nullptr,nullptr,nullptr,
            {BorgBattleInputs{u8(i==0?BorgBattleInputs::x:0),0,0},BorgBattleInputs{u8(i==0?BorgBattleInputs::b:0),0,0}}),
            "shot attraction tick");
        const auto& shot=world.projectiles()[0];
        check(shot.active&&shot.owner==1&&shot.package_damage==19&&world.hits()==0,"pull does not retarget ownership or credit");
        const f32 norm=std::hypot(40.f,46.f);
        check(std::abs(shot.velocity[1]-(enemy?400.f/norm:0.f))<.001f&&
              std::abs(shot.velocity[2]-(-60.f-(enemy?460.f/norm:0.f)))<.001f,
              "pull bends a travelling round toward the point only for the affected scope");
    }
    const auto actor_field=[&](unsigned forward,bool cone,std::string_view scope="enemies"){
        return std::string(player)+"move=well|x|80|any|4|0:4\nfield=well|1|sphere|550|550|550|10|"+
               std::to_string(forward)+"|50|95|50|160|80\nfield-actor-pull=well|"+std::string(scope)+"|12|"+
               (cone?"0.6":"0")+"\n";
    };
    check(!borg_read_gameplay_profile(actor_field(0,true)+"field-actor-pull=well|source|4|0\n",&invalid,nullptr)&&
          !borg_read_gameplay_profile(std::string(player)+"field-actor-pull=b-shot|enemies|12|0\n",&invalid,nullptr)&&
          !borg_read_gameplay_profile(core_base+"field-shot-pull=well|enemies|2\nfield-actor-pull-point=well|0|0|0\n",&invalid,nullptr),
          "actor pull only binds once to an explicit field");
    for(unsigned scenario=0;scenario<6;++scenario){
        NativeBattleMvp world;BattleMvpConfig config;WallGround wall;wall.plane=10;wall.available=scenario!=5;
        config.packages={parse(actor_field(scenario==2||scenario==3?450:0,scenario!=3,
                                           scenario==4?"source":"enemies")),parse(player)};
        config.spawn={BattlePoint{0,400},BattlePoint{0,0}};
        const auto* terrain=scenario==1||scenario==5?&wall:nullptr;
        check(world.start(config,terrain),"actor magnet fixture");
        const auto visit=[&](bool open=false,GamePadState target_input={}){
            check(world.step({GamePadState{},target_input},false,terrain,nullptr,nullptr,nullptr,
                {BorgBattleInputs{u8(open?BorgBattleInputs::x:0),0,0},{}}),"actor magnet visit");
        };
        visit(true);visit();
        const auto start=world.actors()[1].position;
        const auto heading=world.actors()[1].facing;
        check(has(world,BorgBattleEventKind::FieldPlaced)&&start==BattleMvpVec{0,0,0},"no pull on the placement visit");
        GamePadState sideways;sideways.dpad_left=true;visit(false,sideways);
        const f32 moved=scenario==0?12.f:scenario==3?-12.f:0.f;
        check(world.actors()[1].position[2]==moved&&world.actors()[1].health==250&&!world.shots_spawned()&&
              has(world,BorgBattleEventKind::FieldActorPulled)==(moved!=0)&&
              has(world,BorgBattleEventKind::FieldActorPullRefused)==(scenario==1||scenario==5),
              "actor-only pull obeys cone, scope, solid terrain and unknown queries");
        if(moved)check(world.actors()[1].position[0]==0&&world.actors()[1].facing==heading&&world.actors()[1].velocity==BattlePoint{},
                       "accepted attraction replaces one walk step without turning or adding momentum");
        GamePadState jump;jump.b=true;visit(false,jump);
        check(!world.actors()[1].grounded&&!has(world,BorgBattleEventKind::FieldActorPulled),"jump input takes priority");
    }
    for(unsigned scenario=0;scenario<5;++scenario){
        NativeBattleMvp world;BattleMvpConfig config;
        const auto point=scenario==3?"-200|40|450":scenario==2?"-200|0|450":"-200|0|0";
        auto declaration=actor_field(500,false)+"field-actor-pull-point=well|"+point+"\n";
        if(scenario==4)declaration+="field-actor-polarity=well|away\n";
        if(scenario==1){
            // The new pull point is close enough but the receiver is
            // outside the detector. They must be tested independently.
            const auto at=declaration.find("|sphere|550|550|550|");
            declaration.replace(at,std::string_view("|sphere|550|550|550|").size(),"|sphere|90|90|90|");
        }
        config.packages={parse(declaration),parse(player)};
        config.spawn={BattlePoint{0,0},BattlePoint{0,400}};
        check(world.start(config),"separate actor pull point fixture");
        for(unsigned i=0;i<3;++i)check(world.step({},false,nullptr,nullptr,nullptr,nullptr,
            {BorgBattleInputs{u8(i==0?BorgBattleInputs::x:0),0,0},{}}),"separate actor pull point visit");
        const bool accepted=scenario==0||scenario==4;
        check(has(world,BorgBattleEventKind::FieldActorPulled)==accepted&&
              world.actors()[1].position[2]==(scenario==4?412.f:accepted?388.f:400.f),
              "detection centre stays fixed while the effect has a separate point, dead zone and polarity");
    }
}
void drifting_field_cases(){
    const auto text=std::string(player)+"move=well|x|80|any|4|0:4\n"
        "field=well|1|sphere|260|260|260|8|0|50|95|50|160|80\n"
        "field-core-damage=well|enemies|200|7|1\nfield-drift=well|20|10|";
    BorgGameplayProfile invalid;
    check(!borg_read_gameplay_profile(text+"active\nfield-follow=well|source\n",&invalid,nullptr)&&
          !borg_read_gameplay_profile(text+"unknown\n",&invalid,nullptr),
          "startup drift cannot replace a following anchor or accept unknown phase");
    for(bool while_moving:{false,true}){
        NativeBattleMvp world;BattleMvpConfig config;
        config.packages={parse(text+(while_moving?"active\n":"after-stop\n")),parse(target)};
        config.spawn={BattlePoint{0,0},BattlePoint{0,180}};
        check(world.start(config),"field drift fixture");
        for(unsigned i=0;i<10;++i){
            check(world.step({},false,nullptr,nullptr,nullptr,nullptr,
                       {BorgBattleInputs{u8(i==0?BorgBattleInputs::x:0),0,0},{}}),"field drift tick");
            if(i==1)check(world.fields()[0].centre[2]==0&&!world.hits(),"at placement no startup translation");
            if(i==2)check(world.fields()[0].centre[2]==20&&world.fields()[0].speed==10&&
                          world.hits()==unsigned(while_moving),"first drift step and opt-in effects");
            if(i>=3&&i<9)check(world.fields()[0].centre[2]==30&&world.fields()[0].active(),"braking settles exactly and activates field");
        }
        check(!world.fields()[0].id&&world.hits()==(while_moving?7u:6u),"lifetime still starts at placement");
    }
    const auto setup=std::string(player)+
        "move=old|x|80|any|4|0:4\nfield=old|1|sphere|40|40|40|30|0|50|95|50|160|80\n"
        "field-effect=old|shot-speed|enemies|50\nfield-slot=old|scope|on-place\n"
        "move=new|b|81|any|4|0:4\nfield=new|1|sphere|40|40|40|30|0|50|95|50|160|80\n"
        "field-effect=new|shot-speed|enemies|25\nfield-drift=new|20|10|after-stop\n";
    check(!borg_read_gameplay_profile(setup+"field-slot=old|scope|on-ready\n",&invalid,nullptr)&&
          !borg_read_gameplay_profile(setup+"field-slot=new|scope|on-ready\nfield-slot=new|scope|on-place\n",&invalid,nullptr),
          "slot binding requires a valid phase and a unique declaration");
    for(unsigned scenario=0;scenario<3;++scenario){
        const bool on_ready=scenario!=0,outside=scenario==2;
        auto declaration=setup+std::string(on_ready?"field-slot=new|scope|on-ready\n":"field-slot=new|scope|on-place\n");
        NativeBattleMvp world;BattleMvpConfig config;
        // Both opponents can share slot names without ownership aliasing.
        config.packages={parse(declaration),parse(declaration)};
        config.spawn=outside?std::array{BattlePoint{0,0},BattlePoint{0,200}}:
                             std::array{BattlePoint{0,0},BattlePoint{0,300}};
        if(outside){
            const auto at=declaration.find("sphere|40|40|40|30|0|50",declaration.find("field=new"));
            declaration.replace(at,std::string_view("sphere|40|40|40|30|0|50").size(),"sphere|40|40|40|30|500|50");
            config.arena_half=510;config.packages[0]=parse(declaration);
        }
        check(world.start(config),"owned field-slot fixture");
        const auto visit=[&](u8 press=0,u8 other=0){
            check(world.step({},false,nullptr,nullptr,nullptr,nullptr,
                {BorgBattleInputs{press,0,0},BorgBattleInputs{other,0,0}}),"owned field-slot tick");
        };
        visit(BorgBattleInputs::x,BorgBattleInputs::x);visit();
        const auto incumbent=world.fields()[0].id,foreign=world.fields()[1].id;
        visit();visit();visit();visit(BorgBattleInputs::b);visit();
        const auto contains=[&](u32 id){
            return std::any_of(world.fields().begin(),world.fields().end(),[&](const auto& f){return f.id==id;});
        };
        check(contains(incumbent)==on_ready&&contains(foreign)&&has(world,BorgBattleEventKind::FieldReplaced)==!on_ready,
              "immediate and deferred replacement have different publication boundaries");
        visit();visit();
        check(contains(incumbent)==outside&&contains(foreign)&&
              has(world,BorgBattleEventKind::FieldReplaced)==(on_ready&&!outside),
              "settled drift replaces just its own incumbent; failed travel leaves it active");
    }
}
void fence_cases(){
    const auto text=std::string(player)+R"(
anchor-set=fence|3
move=wire|x|80|any|6|0:4
place-anchor=wire|1|fence|none|0
place-anchor=wire|2|fence|none|0
place-anchor-offset=wire|1|130|50
place-anchor-offset=wire|2|300|50
anchor-fence=fence|12|17|12
)";
    BorgGameplayProfile invalid;
    check(!borg_read_gameplay_profile(text+"move=r|x|81|any|4|0:4\nrewind=r|1|fence|12\n",&invalid,nullptr)&&
          !borg_read_gameplay_profile(text+"place-anchor-offset=wire|1|0|0\n",&invalid,nullptr)&&
          !borg_read_gameplay_profile(text+"fence-recoil=missing|12\n",&invalid,nullptr),"fence bindings reject invalid links and rewind reuse");
    for(unsigned scenario=0;scenario<3;++scenario){
        NativeBattleMvp world;BattleMvpConfig config;
        const auto defender=std::string(player)+(scenario==2?"move=g|x|80|any|6|0:4\nguard=g|1|5|3.1|1|all\n":"");
        config.packages={parse(text+(scenario==1?"":"fence-recoil=fence|12\n")),parse(defender)};
        config.spawn={BattlePoint{0,0},BattlePoint{0,340}};
        check(world.start(config),"electric fence fixture");
        unsigned recoils=0,blocks=0;
        for(unsigned i=0;i<15;++i){
            check(world.step({},false,nullptr,nullptr,nullptr,nullptr,
               {BorgBattleInputs{u8(i==0?BorgBattleInputs::x:0),0,0},
                BorgBattleInputs{u8(i==0&&scenario==2?BorgBattleInputs::x:0),0,0}}),"electric fence visit");
            if(i==1)check(world.anchor_paths()[0][0].count==1&&world.fence_links().empty()&&!world.hits(),"one anchor has no effect");
            if(i==2)check(world.fence_links().size()==1&&world.fence_links()[0].end==BattleMvpVec{0,50,300},
                          "second anchor makes a finite link at its accepted height");
            recoils+=has(world,BorgBattleEventKind::FenceRecoilApplied);
            blocks+=has(world,BorgBattleEventKind::GuardBlocked,1);
        }
        check(world.fence_links().empty()&&world.anchor_paths()[0][0].count==0&&recoils==unsigned(!scenario)&&
              world.actors()[0].health==250&&world.actors()[1].health==(scenario==2?250u:233u)&&blocks==unsigned(scenario==2),
              "fence expires, owner is immune, one entry can damage or be guarded, only opt-in recoil moves");
        check(world.actors()[1].position[2]==(scenario?340.f:352.f),"recoil takes one accepted step away");
    }
}
void anchor_return_cases(){
    const auto text=std::string(player)+R"(
stock=wire-stock|2|on-return
anchor-set=return-wire|2
anchor-return=return-wire|on-reach
move=plant|x|80|any|5|0:4
place-anchor=plant|1|return-wire|wire-stock|1
place-anchor=plant|2|return-wire|wire-stock|1
place-anchor-offset=plant|1|-20|0
move=retract|b|81|any|4|0:4
rewind=retract|1|return-wire|12
)";
    const auto profile=parse(text);
    BorgGameplayProfile refused;
    check(!borg_read_gameplay_profile(text+"anchor-return=return-wire|on-reach\n",&refused,nullptr)&&
          !borg_read_gameplay_profile(text+"on-hit=shot|rewind-recipient|return-wire|12\n",&refused,nullptr)&&
          !borg_read_gameplay_profile(text+"anchor-fence=return-wire|10|5|20\n",&refused,nullptr),
          "return-bearing anchor sets refuse duplicate, recipient and fence binding");
    for(bool blocked:{false,true}){
        NativeBattleMvp world;BattleMvpConfig config;WallGround wall;
        wall.available=!blocked;wall.plane=1600;
        config.packages={profile,parse(target)};config.spawn={BattlePoint{0,0},BattlePoint{0,1000}};
        check(world.start(config,&wall),"return-bearing wire fixture");
        unsigned paid=0,returned=0,stopped=0;
        for(unsigned i=0;i<15;++i){
            check(world.step({},false,&wall,nullptr,nullptr,nullptr,
               {BorgBattleInputs{u8(!i?BorgBattleInputs::x:i==6?BorgBattleInputs::b:0),0,0},{}}),"return-bearing wire tick");
            paid+=has(world,BorgBattleEventKind::StockSpent);
            returned+=has(world,BorgBattleEventKind::StockReturned);
            stopped+=has(world,BorgBattleEventKind::RewindStopped);
            if(i==4)check(paid==2&&world.package_action(0).stocks[1].current==0&&!returned,"stored points don't return stock early");
        }
        check(paid==2&&stopped==unsigned(blocked)&&world.package_action(0).stocks[1].current==(blocked?1:2)&&
              world.actors()[0].position[2]==(blocked?0.f:-20.f),"only reached points return; refusal does not roll back a reached point");
        // Each reached point can publish its own return even on one visit.
        check(returned==(blocked?1u:2u),"one return per accepted arrival");
    }
}
void add_debit_stock_cases(){
    const auto declaration=std::string(player)+"stock-clock=primary|add-on-debit\n";
    const auto spec=parse(declaration);
    BorgGameplayProfile unchanged;
    auto multi=declaration;
    const auto at=multi.find("release=b-shot|2|primary|1|shot");
    multi.replace(at,std::string_view("release=b-shot|2|primary|1|shot").size(),"release=b-shot|2|primary|2|shot");
    check(!borg_read_gameplay_profile(multi,&unchanged,nullptr)&&
          !borg_read_gameplay_profile(declaration+"stock-clock=primary|add-on-debit\n",&unchanged,nullptr)&&
          !borg_read_gameplay_profile(declaration+"stock=returned|2|on-return\nstock-clock=returned|add-on-debit\n",&unchanged,nullptr),
          "debit clock is opt-in on exactly one timed pool and refuses multi-unit spends");
    BorgBattleActionState state;state.reset(*spec);
    unsigned attempts=0,debits=0,refills=0,refusals=0;
    auto visit=[&](bool b=false){
        state.visit(*spec,{u8(b?BorgBattleInputs::b:0),0,0},true,false,
            [&](const auto&,u8,u8 channel)->u32{check(channel==255,"timed operation is not object return");++attempts;return 0;},
            [&](auto kind,u8,u16,u32,u16){debits+=kind==BorgBattleEventKind::StockSpent;
                refills+=kind==BorgBattleEventKind::StockRefilled;refusals+=kind==BorgBattleEventKind::StockRefused;});
    };
    visit(true);visit();visit(); // tick 3: stock 1, debt 12
    check(state.stocks[0].current==1&&state.stocks[0].refill_clock==12&&attempts==1,
          "first debit starts countdown before depletion even when projectile allocation fails");
    visit();visit();visit(true);visit();visit(); // tick 8: 7 remaining + 12
    check(state.stocks[0].current==0&&state.stocks[0].refill_clock==19&&attempts==2,
          "second debit extends the existing clock rather than replacing it");
    visit();visit();visit(true);visit();visit();
    check(state.stocks[0].current==0&&state.stocks[0].refill_clock==14&&attempts==2&&debits==2&&refusals==1,
          "insufficient stock cannot add debt or request emission");
    visit();visit();check(!state.stocks[0].current,"exact first boundary still holds stock at zero");
    visit();check(state.stocks[0].current==1&&refills==1,"one unit returns at the observed interval crossing");
    for(unsigned i=0;i<11;++i)visit();
    check(state.stocks[0].current==2&&!state.stocks[0].refill_clock&&refills==2,"clock exhaustion restores capacity");
}
void entry_stock_cases(){
    const auto text=std::string(player)+"stock-clock=primary|add-on-debit\nentry-stock=b-shot|primary|1\n"
        "move=empty|b|1|any|4|0:4\nrelease=empty|2|none|0|twin\n";
    const auto spec=parse(text);
    BorgGameplayProfile invalid;
    check(!borg_read_gameplay_profile(text+"entry-stock=empty|primary|3\n",&invalid,nullptr)&&
          !borg_read_gameplay_profile(text+"entry-stock=b-shot|primary|1\n",&invalid,nullptr)&&
          !borg_read_gameplay_profile(text+"entry-stock=none|primary|1\n",&invalid,nullptr),
          "entry stock binds one existing timed or untimed pool within capacity");
    for(unsigned scenario=0;scenario<3;++scenario){
        BorgBattleActionState state;state.reset(*spec);
        if(!scenario)state.stocks[0].current=0;
        using Event=BorgBattleEventKind;
        std::vector<Event> trace;
        std::string emitted;
        const auto visit=[&](bool press){
            state.visit(*spec,{u8(press?BorgBattleInputs::b:0),0,0},true,false,
                [&](const auto& definition,u8,u8)->u32{emitted=definition.id;return 1;},
                [&](Event kind,u8,u16,u32,u16){trace.push_back(kind);});
        };
        const auto before=state.stocks[0];
        visit(true);
        check(state.stocks[0]==before&&emitted.empty()&&
              trace==(!scenario?std::vector<Event>{Event::EntryStockRefused,Event::Started}:std::vector<Event>{Event::Started}),
              "check-only entry leaves stock untouched and allows eligible lower-priority fallback");
        if(scenario==2)state.stocks[0].current=0; // pool became unavailable after entry
        trace.clear();visit(false);visit(false);
        check(emitted==(scenario==2?"":scenario==1?"shot":"twin")&&
              (std::count(trace.begin(),trace.end(),Event::StockSpent)==int(scenario==1))&&
              (std::count(trace.begin(),trace.end(),Event::StockRefused)==int(scenario==2)),
              "release rechecks the debit and does not substitute another move after entry");
    }
}
void aim_gated_release_cases(){
    const auto text=std::string(player)+"release-aim-gate=b-shot|2|0.25|3\n";
    const auto spec=parse(text);
    BorgGameplayProfile refused;
    check(!borg_read_gameplay_profile(std::string(player)+"release-aim-gate=b-shot|1|0.25|3\n",&refused,nullptr)&&
          !borg_read_gameplay_profile(text+"aim-lock=b-shot|0\n",&refused,nullptr)&&
          !borg_read_gameplay_profile(text+"release-aim-gate=b-shot|2|0.25|3\n",&refused,nullptr),
          "gated release can only bind its own unique cue");
    for(unsigned scenario=0;scenario<4;++scenario){
        BorgBattleActionState action;action.reset(*spec);
        using Event=BorgBattleEventKind;
        unsigned markers=0,spent=0,emitted=0,interrupted=0,step=0,fired=0;
        for(;step<8;++step){
            // A release edge happens one visit before the actual pose cue.
            const BorgBattleInputs input{u8(step==0?BorgBattleInputs::b:0),
                u8(step==0?BorgBattleInputs::b:0),u8(step==1?BorgBattleInputs::b:0)};
            const std::optional<f32> aim=scenario==0&&step==2?.1f:scenario==1&&step==3?.2f:std::optional<f32>{};
            action.visit(*spec,input,true,scenario==3&&step==3,
                [&](const auto&,u8,u8)->u32{++emitted;fired=step;return 1;},
                [&](Event event,u8,u16,u32,u16){markers+=event==Event::Marker;spent+=event==Event::StockSpent;
                    interrupted+=event==Event::Interrupted;},{},0,nullptr,aim);
            if(step<2)check(!markers&&!spent&&!emitted,"release input can't replace the cue");
            if(step==2&&scenario)check(action.release_pending&&action.frame==2&&!spent,"cue waits without payment");
            if(step==4&&scenario==2)check(action.frame==2&&!emitted,"wait holds cue frame and does not run early");
        }
        const std::array expected{2u,3u,5u,0u};
        check(markers==1&&fired==expected[scenario]&&spent==unsigned(scenario!=3)&&emitted==spent&&
              interrupted==unsigned(scenario==3)&&!action.release_pending,"aim, bounded timeout and interruption resolve once");
    }
}
void grouped_stock_return_cases(){
    constexpr std::string_view body=R"(GFBORG-GAMEPLAY 1
variant=normal
movement=grounded-v1|8|13|14
health=250
rig-prefix=visual/prefix
pose-profile=visual/pose
idle=0:0
walk=0:1
air=0:2
dash=0:3
stock=pair|1|on-return
projectile=first|bullet|1|25|0|0|0|3|3|7|ignore-floor
projectile=second|bullet|1|30|0|0|0|3|6|7|ignore-floor
origin=first|0|0|0|0|fixed
origin=second|1|0|0|0|fixed
move=pair-move|b|10|any|4|0:4
release=pair-move|1|pair|1|first
release-extra=pair-move|1|second
)";
    const auto text=std::string(body)+"release-return=pair-move|1|all-retired\n";
    const auto spec=parse(text);
    BorgGameplayProfile invalid;
    check(!borg_read_gameplay_profile(body,&invalid,nullptr)&&
          !borg_read_gameplay_profile(text+"return-stock=first|expiry\n",&invalid,nullptr)&&
          !borg_read_gameplay_profile(text+"release-return=pair-move|1|all-retired\n",&invalid,nullptr),
          "grouped return must be explicit and mutually exclusive with projectile returns");
    struct Origins final:IBorgBattleOrigins{
        unsigned accept=0;
        mutable unsigned offered=0;
        bool socket(unsigned,const BattleMvpActor&,u32,u8,u16,const BorgBattleSocket& s,BattleMvpVec* out)const override{
            offered|=1u<<s.bone;
            if(!(accept&(1u<<s.bone)))return false;
            *out={0,60,54};return true;
        }
    };
    for(unsigned mask=0;mask<4;++mask){
        NativeBattleMvp world;BattleMvpConfig config;Origins origins;origins.accept=mask;
        const auto expected=unsigned(std::popcount(mask));
        config.packages={spec,parse(target)};config.spawn={BattlePoint{0,0},BattlePoint{0,1200}};
        check(world.start(config),"grouped stock fixture");
        unsigned debit=0,returned=0,retired=0,last=0;
        for(unsigned i=0;i<12;++i){
            check(world.step({},false,nullptr,nullptr,nullptr,nullptr,
                {BorgBattleInputs{u8(i==0?BorgBattleInputs::b:0),0,0},{}},&origins),"grouped stock tick");
            if(has(world,BorgBattleEventKind::StockSpent)){
                ++debit;last=world.tick();
                check(!world.package_action(0).stocks[0].current&&origins.offered==3,"both requests observed behind a single debit");
            }
            for(const auto& e:world.package_events())if(e.kind==BorgBattleEventKind::Retired){++retired;last=world.tick();}
            if(has(world,BorgBattleEventKind::StockReturned)){
                ++returned;
                check(retired==expected&&world.tick()==last+1,"one owner visit after both branches resolve");
            }
        }
        check(debit==1&&returned==1&&world.package_action(0).stocks[0].current==1&&
              world.shots_spawned()==expected&&!world.hits(),"no hit or second credit required");
    }
}
void ordered_followup_cases(){
    const auto prefix=std::string(player)+
        "stock=pack|200|30\n"
        "projectile=third|bullet|1|30|0|0|0|4|8|3|ignore-floor\n"
        "projectile=fourth|bullet|1|32|0|0|0|4|8|3|ignore-floor\n"
        "projectile=fifth|bullet|1|34|0|0|0|4|8|3|ignore-floor\n"
        "projectile=sixth|bullet|1|36|0|0|0|4|8|3|ignore-floor\n"
        "move=cannon|x|80|any|4|0:4\nrelease=cannon|1|pack|1|shot\n";
    const auto text=prefix+"release-followup=cannon|1|twin\n"
        "release-followup=cannon|1|third\nrelease-followup=cannon|1|fourth\n"
        "release-followup=cannon|1|fifth\nrelease-followup=cannon|1|sixth\n";
    const auto spec=parse(text);
    BorgGameplayProfile rejected;
    check(!borg_read_gameplay_profile(text+"release-followup=cannon|1|shot\n",&rejected,nullptr)&&
          !borg_read_gameplay_profile(text+"release-extra=cannon|1|shot\n",&rejected,nullptr)&&
          !borg_read_gameplay_profile(text+"release-return=cannon|1|all-retired\n",&rejected,nullptr)&&
          !borg_read_gameplay_profile(text+"stock=bad|256|30\n",&rejected,nullptr),
          "ordered releases and expanded stock stay bounded");
    const std::array<std::string_view,6> expected{"shot","twin","third","fourth","fifth","sixth"};
    for(unsigned mask=0;mask<(1u<<expected.size());++mask){
        BorgBattleActionState state;state.reset(*spec);
        unsigned visited=0,paid=0,success=0;
        for(unsigned i=0;i<2;++i)state.visit(*spec,{u8(i?0:BorgBattleInputs::x),0,0},true,false,
            [&](const auto& projectile,u8,u8 channel)->u32{
                check(channel==255&&visited<expected.size()&&projectile.id==expected[visited],
                      "one marker attempts all six authored requests in order");
                const auto j=visited++;
                return mask&(1u<<j)?j+1:0;
            },[&](auto kind,u8,u16,u32,u16){
                paid+=kind==BorgBattleEventKind::StockSpent;
                success+=kind==BorgBattleEventKind::Emitted||kind==BorgBattleEventKind::ExtraEmitted;
            });
        check(visited==expected.size()&&paid==1&&state.stocks[1].current==199&&success==unsigned(std::popcount(mask)),
              "each branch result is independent of the single debit and preceding results");
    }
}
void cruise_boost_cases(){
    const auto text=std::string(player)+"cruise-boost=100|10|6|4|2|3\n";
    BorgGameplayProfile invalid;
    check(!borg_read_gameplay_profile(text+"air-jump-points=2\n",&invalid,nullptr)&&
          !borg_read_gameplay_profile(text+"cruise-boost=100|10|6|4|2|3\n",&invalid,nullptr),
          "cruise refuses competing movement budgets");
    NativeBattleMvp world;BattleMvpConfig config;
    config.packages={parse(text),parse(target)};config.spawn={BattlePoint{0,0},BattlePoint{0,1200}};
    check(world.start(config)&&!world.actors()[0].grounded&&world.actors()[0].position[1]==100,"cruise spawns airborne");
    const auto visit=[&](bool boost){
        GamePadState pad;pad.b=boost;
        check(world.step({pad,GamePadState{}},false),"cruise tick");
        check(!world.actors()[0].grounded&&world.actors()[0].position[1]==100&&!world.actors()[0].landing_ticks,
              "constant terrain-relative height does not enter landing");
    };
    const auto& actor=world.actors()[0];
    visit(true);check(actor.package_boost_meter.value736==4&&actor.movement_speed==14,"fresh boost pays and accelerates");
    visit(true);check(actor.package_boost_meter.value736==0&&actor.package_boosting,"depletion cannot skip the latch");
    visit(true);check(!actor.package_boost_meter.value736&&actor.package_boosting&&actor.movement_speed==8,
                     "empty latch holds state without free boost speed");
    visit(true);check(!actor.package_boosting&&actor.package_boost_meter.delay73a==2,"exit visit does not run refill early");
    visit(true);visit(true);
    check(!actor.package_boost_meter.value736&&!actor.package_boost_meter.delay73a,"delay-to-zero visit does not refill");
    visit(true);check(actor.package_boost_meter.value736==4&&!actor.package_boosting,"airborne cruise refills without restarting held boost");
    visit(false);visit(true);check(actor.package_boost_meter.value736==4&&actor.package_boosting,
                                  "fresh boost after enough airborne recovery");
    check(world.start(config)&&world.actors()[0].package_boost_meter.value736==10&&!world.actors()[0].package_boosting,
          "restart resets authored movement budget");
}
void flight_children_cases(){
    constexpr std::string_view body=R"(GFBORG-GAMEPLAY 1
variant=normal
movement=grounded-v1|8|13|14
health=250
rig-prefix=visual/prefix
pose-profile=visual/pose
idle=0:0
walk=0:1
air=0:2
dash=0:3
stock=ball|4|90
projectile=ball|orb|1|55|0|0|0|9|12|0|ignore-floor
actor-contact=ball|ignore
projectile=child|bullet|2|21|6|0|0.4|4|6|3|ignore-floor
flight-children=ball|child|1|2|2
move=toss|b|10|any|4|0:4
release=toss|1|ball|1|ball
)";
    BorgGameplayProfile rejected;
    check(!borg_read_gameplay_profile(std::string(body)+"flight-children=child|ball|1|2|2\n",&rejected,nullptr),
          "flight child chains are refused instead of multiplying recursively");
    for(unsigned scenario=0;scenario<3;++scenario){
        std::string example(body);
        if(scenario){
            const auto row=example.find("|55|0|0|0|9|12|0|ignore-floor");
            example.replace(row,std::string_view("|55|0|0|0|9|12|0|ignore-floor").size(),
                            scenario==1?"|55|0|0|0|9|12|0|stop-on-stage":"|55|-100|0|0|9|12|0|land-on-floor");
        }
        NativeBattleMvp world;BattleMvpConfig config;WallGround wall;
        wall.plane=150;config.packages={parse(example),parse(target)};
        config.spawn={BattlePoint{0,0},BattlePoint{0,1200}};
        check(world.start(config,&wall),"flight carrier starts");
        unsigned volleys=0;
        for(unsigned i=0;i<14;++i){
            check(world.step({},false,&wall,nullptr,nullptr,nullptr,
                 {BorgBattleInputs{u8(i==0?BorgBattleInputs::b:0),0,0},{}}),"flight carrier update");
            if(i==1)check(world.shots_spawned()==1,"nothing emits on the release tick");
            for(const auto& event:world.package_events())if(event.kind==BorgBattleEventKind::FlightChildren){
                ++volleys;
                check(event.object==1&&event.value==2&&(i==2||i==4),"only scheduled surviving flight updates emit");
                const auto& parent=world.projectiles()[0];
                unsigned fresh=0;
                for(const auto& shot:world.projectiles())if(shot.active&&shot.id!=1&&shot.born_tick==world.tick()){
                    ++fresh;check(shot.owner==0&&shot.position==parent.position&&shot.life==6&&shot.velocity[2]>20,
                                  "child copies accepted position and direction, starts next visit");
                }
                check(fresh==2,"all-or-nothing child volley allocation");
            }
        }
        check(volleys==(scenario?0u:2u)&&world.shots_spawned()==(scenario?1u:5u)&&world.package_action(0).stocks[0].current==3,
              "landing and blocking wall suppress children; no additional stock is spent");
    }
}
void impact_children_cases(){
    constexpr std::string_view body=R"(GFBORG-GAMEPLAY 1
variant=normal
movement=grounded-v1|8|13|14
health=250
rig-prefix=visual/prefix
pose-profile=visual/pose
idle=0:0
walk=0:1
air=0:2
dash=0:3
stock=ball|1|on-return
projectile=ball|orb|1|55|0|0|0|9|12|0|stop-on-stage
projectile=child|bullet|3|21|6|0|0|4|6|3|ignore-floor
return-stock=ball|stage
move=toss|b|10|any|4|0:4
release=toss|1|ball|1|ball
)";
    const auto stage=std::string(body)+"impact-children=ball|child|stage|radial\n";
    BorgGameplayProfile rejected;
    check(!borg_read_gameplay_profile(stage+"impact-children=child|ball|actor|forward\n",&rejected,nullptr),
          "impact child chains are refused");
    for(const bool hit:{false,true}){
        NativeBattleMvp world;BattleMvpConfig config;WallGround wall;wall.plane=hit?150.f:1600.f;
        config.packages={parse(stage),parse(target)};config.spawn={BattlePoint{0,0},BattlePoint{0,1200}};
        check(world.start(config,&wall),"impact children fixture");
        unsigned attempts=0,stock_returns=0;
        for(unsigned i=0;i<15;++i){
            check(world.step({},false,&wall,nullptr,nullptr,nullptr,
                {BorgBattleInputs{u8(i==0?BorgBattleInputs::b:0),0,0},{}}),"impact visit");
            stock_returns+=has(world,BorgBattleEventKind::StockReturned);
            if(has(world,BorgBattleEventKind::ImpactChildren)){
                ++attempts;
                check(has(world,BorgBattleEventKind::StageImpact)&&
                      !has(world,BorgBattleEventKind::BlastTriggered)&&world.package_action(0).stocks[0].current==1,
                      "impact children are independent of blast and stock return");
                unsigned count=0;
                for(const auto& child:world.projectiles())if(child.active&&child.born_tick==world.tick()){
                    ++count;check(child.owner==0&&std::abs(child.position[2]-150)<.001f&&child.life==6,
                                  "children start on the accepted wall point and don't advance yet");
                }
                check(count==3,"accepted impact emits complete volley");
            }
        }
        check(attempts==unsigned(hit)&&stock_returns==unsigned(hit)&&world.shots_spawned()==(hit?4u:1u),
              "clear fuse expiry emits no impact children or return");
    }
    const auto guard=parse(std::string(player)+"move=guard|x|50|grounded|18|0:1\nguard=guard|1|12|0.5|1|shots\n");
    const auto actor_impact=parse(std::string(body)+"impact-children=ball|child|actor|forward\n");
    for(unsigned scenario=0;scenario<4;++scenario){
        NativeBattleMvp world;BattleMvpConfig config;WallGround wall;
        wall.plane=scenario==0?167.f:1600.f;
        config.packages={scenario==0?parse(stage):actor_impact,guard};
        config.spawn={BattlePoint{0,0},BattlePoint{0,scenario==3?1200.f:220.f}};
        check(world.start(config,&wall),"impact ordering with directional guard");
        unsigned blocked=0,children=0,stage_hits=0;
        for(unsigned i=0;i<(scenario==3?6u:4u);++i){
            check(world.step({},false,&wall,nullptr,nullptr,nullptr,
                {BorgBattleInputs{u8(i==0?BorgBattleInputs::b:0),0,0},
                 BorgBattleInputs{u8(i==0&&scenario!=2?BorgBattleInputs::x:0),0,0}}),"guard/impact visit");
            if(i<=2)check(!has(world,BorgBattleEventKind::GuardBlocked,1)&&world.projectiles()[0].active==bool(i),
                          "height overlap alone cannot intercept a travelling shot");
            blocked+=has(world,BorgBattleEventKind::GuardBlocked,1);
            children+=has(world,BorgBattleEventKind::ImpactChildren);
            stage_hits+=has(world,BorgBattleEventKind::StageImpact);
            if(has(world,BorgBattleEventKind::GuardBlocked,1))
                check(std::abs(world.projectiles()[0].position[2]-169.f)<.001f,
                      "guard resolves at body entry, not at the previous projectile position");
        }
        check(blocked==unsigned(scenario==1)&&children==unsigned(scenario==0||scenario==2)&&stage_hits==unsigned(scenario==0),
              "wall before body, accepted body, blocked body and far miss use distinct terminal paths");
    }
    constexpr std::string_view trail_row="impact-trail=ball|stage|3|2|20|10|90|2|23|3\n";
    const auto trail_text=std::string(body)+std::string(trail_row);
    constexpr std::string_view world_row="impact-pattern-space=ball|world\n";
    check(!borg_read_gameplay_profile(trail_text+std::string(trail_row),&rejected,nullptr)&&
          !borg_read_gameplay_profile(std::string(body)+"impact-trail=ball|floor|3|2|20|10|90|2|23|3\n",&rejected,nullptr)&&
          !borg_read_gameplay_profile(std::string(body)+"impact-trail=ball|stage|32|2|20|80|90|2|23|3\n",&rejected,nullptr),
          "duplicate, unreachable or unbounded visual trails are refused");
    check(!borg_read_gameplay_profile(std::string(body)+std::string(world_row),&rejected,nullptr)&&
          !borg_read_gameplay_profile(trail_text+std::string(world_row)+std::string(world_row),&rejected,nullptr),
          "impact pattern coordinates require a single binding to a declared pattern");
    for(unsigned scenario=0;scenario<4;++scenario){
        NativeBattleMvp world;BattleMvpConfig config;WallGround wall;
        wall.plane=scenario==1?1600.f:150.f;
        wall.available=scenario!=2;
        const bool world_space=scenario==3,accepted=!scenario||world_space;
        config.packages={parse(world_space?trail_text+std::string(world_row):trail_text),parse(target)};
        config.spawn={BattlePoint{0,0},BattlePoint{0,1200}};
        check(world.start(config,&wall),"visual-only carrier admits");
        unsigned starts=0,marks=0;
        u32 born=0;
        const std::array<BattleMvpVec,3> expected{{{world_space?30.f:-30.f,60,150},{0,60,190},{world_space?-50.f:50.f,60,150}}};
        for(unsigned i=0;i<17;++i){
            check(world.step({},false,&wall,nullptr,nullptr,nullptr,
                  {BorgBattleInputs{u8(i==0?BorgBattleInputs::b:0),0,0},{}}),"visual trail visit");
            if(has(world,BorgBattleEventKind::ImpactTrailStarted)){
                ++starts;born=world.tick();
                check(!world.projectiles()[0].active&&has(world,BorgBattleEventKind::StageImpact)&&
                      has(world,BorgBattleEventKind::StockReturned),"trail starts at accepted stage retirement alongside stock return");
            }
            for(const auto& mark:world.impacts())if(mark.radius==23&&mark.ticks==3){
                check(marks<expected.size()&&world.tick()==born+2*(marks+1)&&mark.total_ticks==3,
                      "visual marks have bounded cadence and lifetime");
                for(unsigned a=0;a<3;++a)check(std::abs(mark.position[a]-expected[marks][a])<.001f,
                                             "marks rotate and advance around the captured impact point");
                ++marks;
            }
            check(world.shots_spawned()<=1&&!world.hits()&&!has(world,BorgBattleEventKind::BlastTriggered),
                  "trail doesn't allocate projectiles or apply damage");
        }
        check(starts==unsigned(accepted)&&marks==(accepted?3u:0u)&&
              std::none_of(world.impacts().begin(),world.impacts().end(),[](const auto& mark){return mark.radius==23&&mark.ticks;}),
              "only accepted surface contact schedules a trail, and all marks expire");
    }
    {
        constexpr std::string_view slow_trail="impact-trail=ball|stage|3|30|20|10|90|2|23|3\n";
        NativeBattleMvp world;BattleMvpConfig config;WallGround wall;wall.plane=150;
        config.packages={parse(std::string(body)+std::string(slow_trail)),parse(target)};
        config.spawn={BattlePoint{0,0},BattlePoint{0,1200}};
        check(world.start(config,&wall),"trail slot cap fixture");
        unsigned accepted=0,refused=0,returned=0;
        for(unsigned i=0;i<45;++i){
            check(world.step({},false,&wall,nullptr,nullptr,nullptr,
                {BorgBattleInputs{u8(i%5==0?BorgBattleInputs::b:0),0,0},{}}),"trail slot cap visit");
            accepted+=has(world,BorgBattleEventKind::ImpactTrailStarted);
            refused+=has(world,BorgBattleEventKind::ImpactTrailRefused);
            returned+=has(world,BorgBattleEventKind::StockReturned);
        }
        check(accepted==8&&refused==1&&returned==9,"full visual pool does not reject impact or stock return");
        check(world.start(config,&wall),"restart with pending trails");
        for(unsigned i=0;i<31;++i)check(world.step({},false,&wall),"empty restarted world");
        check(std::none_of(world.impacts().begin(),world.impacts().end(),[](const auto& mark){return mark.radius==23;}),
              "restart discards pending visual marks");
    }
    const auto hazard=[&](unsigned height){
        return std::string(body)+"impact-hazard=ball|stage|1|2|20|0|0|1|18|"+std::to_string(height)+"|5|11\n";
    };
    check(!borg_read_gameplay_profile(hazard(601),&rejected,nullptr)&&
          !borg_read_gameplay_profile(hazard(0)+"impact-hazard=ball|stage|1|2|20|0|0|1|18|0|5|11\n",&rejected,nullptr),
          "hazard height and duplicates are validated");
    constexpr std::string_view timing="impact-pattern-timing=ball|4|3|2\n";
    check(!borg_read_gameplay_profile(hazard(60)+std::string(timing),&rejected,nullptr)&&
          !borg_read_gameplay_profile(hazard(60)+"impact-pattern-timing=ball|0|121|2\n",&rejected,nullptr)&&
          !borg_read_gameplay_profile(hazard(60)+"impact-pattern-timing=ball|0|3|0\nimpact-pattern-timing=ball|0|0|2\n",&rejected,nullptr),
          "timing needs reachable patterns and can't be merged across repeated rows");
    for(unsigned scenario=0;scenario<5;++scenario){
        NativeBattleMvp world;BattleMvpConfig config;WallGround wall;wall.plane=150;
        config.actor_height=120; // At release height, both vertical axes overlap.
        const auto guard_all=parse(std::string(player)+"move=guard|x|50|grounded|18|0:1\nguard=guard|1|12|0.5|1|all\n");
        config.packages={parse(hazard(scenario==1?0:60)),guard_all};
        config.spawn={BattlePoint{0,0},BattlePoint{0,scenario<=1?230.f:scenario==4?300.f:229.f}};
        check(world.start(config,&wall),"impact hazard fixture");
        unsigned starts=0,places=0,blocks=0;
        u32 born=0,id=0;
        const bool guarded=scenario==3;
        const bool hit=scenario==1||scenario==2;
        for(unsigned i=0;i<14;++i){
            check(world.step({},false,&wall,nullptr,nullptr,nullptr,
                {BorgBattleInputs{u8(i==0?BorgBattleInputs::b:0),0,0},
                 BorgBattleInputs{u8(i==0&&guarded?BorgBattleInputs::x:0),0,0}}),"hazard visit");
            if(has(world,BorgBattleEventKind::ImpactHazardStarted)){
                ++starts;born=world.tick();
                check(has(world,BorgBattleEventKind::StageImpact)&&has(world,BorgBattleEventKind::StockReturned),
                      "hazard takes accepted impact, not stock ownership");
            }
            for(const auto& event:world.package_events()){
                if(event.kind==BorgBattleEventKind::ImpactHazardPlaced){
                    ++places;id=event.object;
                    check(id>1&&world.tick()==born+2&&!world.hits(),"hazard placed after delay without a same-tick hit");
                }
                if(event.kind==BorgBattleEventKind::Hit)
                    check(event.object==id&&event.recipient==1&&event.value==11&&world.tick()==born+3,
                          "accepted hazard hit has distinct identity and credit");
                if(event.kind==BorgBattleEventKind::GuardBlocked){
                    ++blocks;check(event.object==id,"guard resolves the placed hazard, not the original carrier");
                }
            }
        }
        check(starts==1&&places==1&&blocks==unsigned(guarded)&&world.shots_spawned()==1&&
              world.hits()==unsigned(hit)&&world.actors()[1].health==(hit?239u:250u)&&world.actors()[0].health==250,
              "sphere tangency, capsule tangency, actual contact, guard and miss remain distinct");
    }
    {
        NativeBattleMvp world;BattleMvpConfig config;WallGround wall;wall.plane=150;
        config.actor_height=120;
        config.packages={parse(hazard(60)+std::string(trail_row)+std::string(timing)),parse(target)};
        config.spawn={BattlePoint{0,0},BattlePoint{0,229}};
        check(world.start(config,&wall),"separate impact clocks");
        unsigned placements=0,first_mark=0,hits=0;
        u32 born=0;
        for(unsigned i=0;i<15;++i){
            check(world.step({},false,&wall,nullptr,nullptr,nullptr,
                {BorgBattleInputs{u8(i==0?BorgBattleInputs::b:0),0,0},{}}),"deferred pattern visit");
            if(has(world,BorgBattleEventKind::ImpactHazardStarted))born=world.tick();
            if(has(world,BorgBattleEventKind::ImpactHazardPlaced)){
                ++placements;check(born&&world.tick()==born+3&&!world.hits(),"explicit first placement age");
            }
            if(!first_mark&&std::any_of(world.impacts().begin(),world.impacts().end(),[](const auto& mark){return mark.radius==23&&mark.ticks;})){
                first_mark=world.tick();check(born&&first_mark==born+4,"visual pattern has its own first placement age");
            }
            if(has(world,BorgBattleEventKind::Hit)){
                ++hits;check(born&&world.tick()==born+5,"contact warmup elapses independently of placement and art");
            }
        }
        check(placements==1&&first_mark&&hits==1,"deferred clocks resolve once");
    }
    {
        NativeBattleMvp world;BattleMvpConfig config;WallGround wall;wall.plane=150;
        config.packages={parse(std::string(body)+"impact-hazard=ball|stage|5|1|20|0|0|1|18|60|60|11\n"),parse(target)};
        config.spawn={BattlePoint{0,0},BattlePoint{0,1200}};
        check(world.start(config,&wall),"bounded hazard pool fixture");
        unsigned placed=0,refused=0,returned=0;
        for(unsigned i=0;i<47;++i){
            check(world.step({},false,&wall,nullptr,nullptr,nullptr,
                 {BorgBattleInputs{u8(i<40&&i%5==0?BorgBattleInputs::b:0),0,0},{}}),"bounded hazard pool visit");
            placed+=has(world,BorgBattleEventKind::ImpactHazardPlaced);
            refused+=has(world,BorgBattleEventKind::ImpactHazardRefused);
            returned+=has(world,BorgBattleEventKind::StockReturned);
        }
        check(placed==32&&refused==8&&returned==8&&!world.hits(),"hazard refusal doesn't retry or cancel stock return");
    }
}
}
int main(){
    const std::string scoped=std::string(player)+"body-record=0\n";
    check(!parse(player)->record_zero_body&&parse(scoped)->record_zero_body,"record-zero is a declared authored body scope");
    BorgGameplayProfile invalid_scope;
    std::string scope_error;
    check(!borg_read_gameplay_profile(std::string(player)+"body-record=1\n",&invalid_scope,&scope_error)&&
          !borg_read_gameplay_profile(scoped+"body-record=0\n",&invalid_scope,&scope_error),
          "unsupported and duplicate body scopes refuse");
    auto mixed_scope=*parse(scoped);mixed_scope.movement=BorgBattleMovement::NormalGRed;
    check(!borg_gameplay_profile_valid(mixed_scope),"record-zero scope cannot override the exact source body binder");
    const std::string highlight=scoped+"body-material=immutable-highlight\n";
    check(!parse(scoped)->body_immutable_highlight&&parse(highlight)->body_immutable_highlight&&
          !borg_read_gameplay_profile(highlight+"body-material=immutable-highlight\n",&invalid_scope,&scope_error)&&
          !borg_read_gameplay_profile(scoped+"body-material=guess\n",&invalid_scope,&scope_error),
          "immutable highlight is an explicit, singular body material policy");
    auto mixed_highlight=*parse(std::string(player)+"body-material=immutable-highlight\n");
    mixed_highlight.movement=BorgBattleMovement::NormalGRed;
    check(!borg_gameplay_profile_valid(mixed_highlight),"immutable highlight cannot override the exact source body binder");
    const std::string loop=std::string(player)+"idle-loop=authored-bit\n";
    check(!parse(player)->idle_authored_loop&&parse(loop)->idle_authored_loop&&
          !borg_read_gameplay_profile(loop+"idle-loop=authored-bit\n",&invalid_scope,&scope_error)&&
          !borg_read_gameplay_profile(std::string(player)+"idle-loop=guess\n",&invalid_scope,&scope_error),
           "idle loop is an explicit, singular package policy");
    const std::string walk_loop=std::string(player)+"walk-loop=authored-bit\n";
    check(!parse(player)->walk_authored_loop&&parse(walk_loop)->walk_authored_loop&&
          !parse(walk_loop)->idle_authored_loop&&
          !borg_read_gameplay_profile(walk_loop+"walk-loop=authored-bit\n",&invalid_scope,&scope_error)&&
          !borg_read_gameplay_profile(std::string(player)+"walk-loop=guess\n",&invalid_scope,&scope_error),
          "walk loop is a separate explicit role");
    const std::string static_cruise=
        "GFBORG-GAMEPLAY 1\nvariant=normal\nhealth=250\nmovement=grounded-v1|6|13|12\n"
        "rig-prefix=visual/prefix\npose-profile=visual/pose\nbody-record=0\nbody-motion=static-cruise\n"
        "cruise-boost=240|240|12|12|8|4\nstock=main|5|180\n"
        "projectile=shot|bullet|1|32|0|0|0|12|70|8|ignore-floor\n"
        "move=b-shot|b|10|airborne|28|none\nrelease=b-shot|12|main|1|shot\n";
    const auto static_profile=parse(static_cruise);
    check(static_profile->static_cruise_body&&static_profile->moves[0].static_pose&&
          !borg_read_gameplay_profile(static_cruise+"idle=0:0\n",&invalid_scope,&scope_error),
          "static flight has an explicit body and pose policy, never an implicit clip");
    auto invalid_static=*static_profile;
    invalid_static.cruise.reset();
    check(!borg_gameplay_profile_valid(invalid_static),"static body cannot use unrelated movement");
    invalid_static=*static_profile;
    invalid_static.moves[0].pose_start=1;
    check(!borg_gameplay_profile_valid(invalid_static),"static body has no action frame to seek");
    auto animated_cruise=static_cruise;
    constexpr std::string_view static_policy="body-motion=static-cruise";
    animated_cruise.replace(animated_cruise.find(static_policy),static_policy.size(),"cruise-entry=4:6");
    const auto animated_profile=parse(animated_cruise);
    check(animated_profile->cruise_entry&&animated_profile->cruise_entry->group==4&&animated_profile->cruise_entry->choice==6&&
          !animated_profile->static_cruise_body,"animated flight uses one exact explicit entry");
     for(auto row:{"idle=0:0\n","cruise-entry=2:0\n","body-motion=static-cruise\n","idle-loop=authored-bit\n","walk-loop=authored-bit\n"})
        check(!borg_read_gameplay_profile(animated_cruise+row,&invalid_scope,&scope_error),
              "cruise entry conflicts with alternate pose sources");
    auto invalid_jet=*animated_profile;
    invalid_jet.cruise.reset();
    check(!borg_gameplay_profile_valid(invalid_jet),"cruise entry cannot masquerade as ground movement");
    invalid_jet=*animated_profile;invalid_jet.moves[0].context=BorgBattleContext::Any;
    check(!borg_gameplay_profile_valid(invalid_jet),"cruise entry shot is scoped to airborne release");
    invalid_jet=*animated_profile;invalid_jet.moves[0].static_pose=false;
    check(!borg_gameplay_profile_valid(invalid_jet),"cruise entry shot does not replace the entry pose");
    auto static_ground=static_cruise;
    static_ground.replace(static_ground.find(static_policy),static_policy.size(),"body-motion=static-grounded");
    const auto boost=static_ground.find("cruise-boost=");
    static_ground.erase(boost,static_ground.find('\n',boost)-boost+1);
    static_ground.replace(static_ground.find("|airborne|"),10,"|grounded|");
    const auto ground_body=parse(static_ground);
    check(ground_body->static_body()&&ground_body->static_grounded_body&&!ground_body->cruise,
          "static grounded body uses no clips or cruise rules");
    for(auto row:{"idle=0:0\n","cruise-boost=240|240|12|12|8|4\n","body-motion=static-cruise\n"})
        check(!borg_read_gameplay_profile(static_ground+row,&invalid_scope,&scope_error),
              "static ground policy rejects animation and cruise substitutes");
    auto wrong_ground=*ground_body;
    wrong_ground.moves[0].context=BorgBattleContext::Airborne;
    check(!borg_gameplay_profile_valid(wrong_ground),"static ground policy limits releases to ground");
    auto record_orb=static_ground;
    record_orb.replace(record_orb.find("|bullet|"),8,"|orb|");
    const auto record_profile=parse(record_orb+"projectile-record=shot|8|25\n");
    check(record_profile->projectiles[0].record_look&&record_profile->projectiles[0].record_look->record==8,
          "projectile record is an explicit visual binding");
    const auto trail_text=record_orb+"projectile-record=shot|8|25\nprojectile-record-trail=shot|9|5|1|15\n";
    const auto trail=parse(trail_text);
    check(trail->projectiles[0].record_trail&&trail->projectiles[0].record_trail->count==5,
          "projectile record trail is bounded independently");
    check(!borg_read_gameplay_profile(record_orb+"projectile-record-trail=shot|9|5|1|15\n",&invalid_scope,&scope_error)&&
          !borg_read_gameplay_profile(trail_text+"projectile-record-trail=shot|9|5|1|15\n",&invalid_scope,&scope_error)&&
          !borg_read_gameplay_profile(record_orb+"projectile-record=shot|8|25\nprojectile-record-trail=shot|9|6|0|15\n",
                                      &invalid_scope,&scope_error),"record trail rejects unbound, duplicate or unbounded declarations");
    for(const auto* row:{"projectile-record=shot|0|100\n","projectile-record=shot|32|100\n",
                         "projectile-record=shot|8|0\n","projectile-record=unknown|8|25\n",
                         "projectile-record=shot|8|25\nprojectile-record=shot|9|25\n",
                         "projectile-record=shot|8|25\nlook=shot|12|255|255|255\n"})
        check(!borg_read_gameplay_profile(record_orb+row,&invalid_scope,&scope_error),
              "invalid or conflicting record appearance refuses");
    std::string no_dash(player);
    const auto dash_line=no_dash.find("\ndash=");
    check(dash_line!=std::string::npos,"fixture has a dash binding");
    no_dash.erase(dash_line+1,no_dash.find('\n',dash_line+1)-dash_line);
    check(parse(no_dash+"dash-policy=disabled\n")->dash_disabled&&
          !borg_read_gameplay_profile(no_dash,&invalid_scope,&scope_error)&&
          !borg_read_gameplay_profile(std::string(player)+"dash-policy=disabled\n",&invalid_scope,&scope_error),
              "an absent dash clip is valid only with the explicit disabled policy");
    const auto on_contact=std::string(player)+"impact-record-bank=visual/effects.hsd\nimpact-record=shot|actor|22|10|18\n";
    check(parse(on_contact)->projectiles[0].impact_record->ticks==18&&
          !borg_read_gameplay_profile(std::string(player)+"impact-record=shot|actor|22|10|18\n",&invalid_scope,&scope_error)&&
          !borg_read_gameplay_profile(on_contact+"impact-record=shot|actor|22|10|18\n",&invalid_scope,&scope_error)&&
          !borg_read_gameplay_profile(std::string(player)+"impact-record-bank=visual/effects.hsd\nimpact-record=shot|stage|22|10|18\n",
                                      &invalid_scope,&scope_error),"impact record requires exact bank, accepted lane and bounded unique row");
    const std::string growth="impact-record-growth=shot|25\n";
    check(parse(std::string(player)+growth+on_contact.substr(std::string_view(player).size()))->projectiles[0].impact_record->end_scale==25&&
          !borg_read_gameplay_profile(std::string(player)+growth,&invalid_scope,&scope_error)&&
          !borg_read_gameplay_profile(on_contact+growth+growth,&invalid_scope,&scope_error)&&
          !borg_read_gameplay_profile(on_contact+"impact-record-growth=shot|401\n",&invalid_scope,&scope_error)&&
          !borg_read_gameplay_profile(std::string(player)+"impact-record-bank=visual/effects.hsd\nimpact-record=shot|actor|22|10|1\n"+growth,
                                      &invalid_scope,&scope_error),"growth requires a unique bound visual with bounded scale and duration");
    const auto first=borg_impact_record_appearance(10,25,18,18),middle=borg_impact_record_appearance(10,25,18,9),
               last=borg_impact_record_appearance(10,25,18,1),fixed=borg_impact_record_appearance(10,0,18,1);
    check(std::abs(first.scale-.1f)<1e-6f&&first.alpha==1&&middle.scale>first.scale&&middle.scale<last.scale&&
          middle.alpha==.5f&&std::abs(last.scale-.25f)<1e-6f&&last.alpha==1.f/18&&
          fixed.scale==first.scale&&fixed.alpha==1,"visual samples expand and fade by remaining ticks; legacy appearance stays fixed");
    const auto hit_cue=std::string(player)+"impact-sound=shot|actor|audio/hit.pcm\n";
    check(parse(hit_cue)->projectiles[0].impact_sound->path=="audio/hit.pcm"&&
          !borg_read_gameplay_profile(hit_cue+"impact-sound=shot|actor|audio/hit.pcm\n",&invalid_scope,&scope_error)&&
          !borg_read_gameplay_profile(std::string(player)+"impact-sound=shot|stage|audio/hit.pcm\n",&invalid_scope,&scope_error)&&
          !borg_read_gameplay_profile(std::string(player)+"impact-sound=shot|actor|../hit.pcm\n",&invalid_scope,&scope_error),
          "impact cue is lane-qualified and package-local independent of visual declarations");
    {
        NativeBattleMvp world;BattleMvpConfig config;
        config.packages={parse(no_dash+"dash-policy=disabled\n"),parse(target)};
        check(world.start(config),"disabled dash fixture");
        GamePadState pad;pad.dpad_right=true;pad.l=true;
        check(world.step({pad,{}},false)&&world.actors()[0].grounded&&!world.actors()[0].dash_ticks,
              "ground dash refuses without suppressing the movement controller");
        pad.l=false;pad.b=true;
        for(unsigned i=0;i<18&&world.actors()[0].grounded;++i)check(world.step({pad,{}},false),"jump with dash disabled");
        check(!world.actors()[0].grounded,"jump is still enabled");
        pad.l=true;pad.b=false;
        check(world.step({pad,{}},false)&&!world.actors()[0].dash_ticks&&!world.actors()[0].air_dashes_used,
              "air dash refuses without spending a charge");
    }
    const auto pose_free=parse(std::string(player)+"move=keep-motion|b|30|any|6|none\n"
                                    "release=keep-motion|2|primary|1|shot\n");
    check(pose_free->moves.back().static_pose&&!pose_free->static_cruise_body,
          "a paid projectile move can keep ordinary locomotion");
    // A field is a visible executable marker, so a pose-free ground move may be
    // field-only; the same move may not also carry a release.
    const auto field_only=parse(std::string(player)+"move=breath|b|30|grounded|48|none\n"
                                       "field=breath|12|sphere|600|600|600|30|500|70|255|150|60|170\n"
                                       "field-follow=breath|source\n"
                                       "field-core-damage=breath|enemies|600|6|8\n");
    check(field_only->moves.back().static_pose&&field_only->moves.back().field.has_value(),
          "a pose-free field attack keeps ordinary locomotion");
    check(!borg_read_gameplay_profile(std::string(player)+"move=breath|b|30|grounded|48|none\n"
                                      "field=breath|12|sphere|600|600|600|30|500|70|255|150|60|170\n"
                                      "field-core-damage=breath|enemies|600|6|8\n"
                                      "release=breath|2|primary|1|shot\n",&invalid_scope,&scope_error),
          "a pose-free field move cannot also release a projectile");
    for(auto text:{"move=keep-motion|b|30|any|6|none\nrelease=keep-motion|2|none|0|shot\n",
                   "move=keep-motion|b|30|any|6|none\nmelee=keep-motion|1|2|4|50|15|0|10\n",
                   "move=keep-motion|b|30|any|6|none\nrelease=keep-motion|2|primary|1|shot\npose-start=keep-motion|1\n"})
        check(!borg_read_gameplay_profile(std::string(player)+text,&invalid_scope,&scope_error),
              "pose-free ground move cannot fake an animation or an unpaid/nonprojectile move");
    aim_commit_cases();
    body_heading_release_cases();
    sustained_beam_cases();
    bomb_proximity_cases();
    smoke_cases();
    blast_accuracy_cases();
    flight_children_cases();
    impact_children_cases();
    power_field_cases();
    slow_field_cases();
    resident_beam_field_cases();
    overlapping_field_cases();
    movement_and_attraction_cases();
    drifting_field_cases();
    fence_cases();
    anchor_return_cases();
    add_debit_stock_cases();
    entry_stock_cases();
    aim_gated_release_cases();
    cruise_boost_cases();
    grouped_stock_return_cases();
    ordered_followup_cases();
    {
        const auto text=std::string(player)+"release-extra=b-shot|2|twin\n";
        const auto paired=parse(text);
        BorgGameplayProfile invalid;
        for(const auto& bad:{text+"release-extra=b-shot|2|twin\n",std::string(player)+"release-extra=x-volley|4|twin\n",
                             std::string(player)+"release-extra=b-shot|3|twin\n",std::string(player)+"release-extra=b-shot|2|unknown\n"})
            check(!borg_read_gameplay_profile(bad,&invalid,nullptr),"refuse ambiguous or unpaid extra releases");
        for(unsigned mask=0;mask<8;++mask){
            using Event=BorgBattleEventKind;
            BorgBattleActionState state;state.reset(*paired);
            const bool paid=mask<4;
            if(!paid)state.stocks[0].current=0;
            std::vector<Event> trace;std::vector<std::string> attempted;
            auto step=[&](u8 press){
                state.visit(*paired,{press,0,0},true,false,
                    [&](const auto& def,u8,u8 channel)->u32{
                        check(channel==255,"extra release has no inherited stock token");
                        attempted.push_back(def.id);
                        return def.id=="shot"?(mask&1?1:0):(mask&2?2:0);
                    },
                    [&](Event event,u8,u16,u32,u16){trace.push_back(event);});
            };
            step(BorgBattleInputs::b);step(0);trace.clear();step(0);
            if(paid)check(attempted==std::vector<std::string>{"shot","twin"}&&state.stocks[0].current==1&&
                trace==std::vector<Event>{Event::Marker,Event::StockSpent,mask&1?Event::Emitted:Event::EmitRefused,
                                          mask&2?Event::ExtraEmitted:Event::ExtraEmitRefused},
                "paid cue attempts both releases in order regardless of earlier outcome");
            else check(attempted.empty()&&!state.stocks[0].current&&trace==std::vector<Event>{Event::Marker,Event::StockRefused},
                "stock refusal does not visit either release");
        }
    }
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
    const auto single=remotes+"detonation-policy=trigger|first-eligible\n";
    check(!borg_read_gameplay_profile(single+"detonation-policy=trigger|first-eligible\n",&unchanged,&error)&&
          !borg_read_gameplay_profile(remotes+"detonation-policy=bomb-throw|first-eligible\n",&unchanged,&error),
          "single-use policy requires exactly one declared detonation");
    config.packages[0]=parse(single);
    check(world.start(config,&terrain),"consumable detonation fixture");
    for(unsigned n=0;n<2;++n){request(x_bit);for(unsigned i=0;i<6;++i)request();}
    request(b_bit);request();request();
    check(trigger_result(0,1)&&world.hits()==1&&!world.projectiles()[0].active&&world.projectiles()[1].active,
          "first eligible projectile visit consumes the command before the blast");
    for(unsigned i=0;i<4;++i){request();check(world.projectiles()[1].active&&!has(world,BorgBattleEventKind::BlastTriggered),
                                              "same command cannot carry into later ticks");}
    request(b_bit);request();request();
    check(trigger_result(0,1)&&world.hits()==2&&!world.projectiles()[1].active,"a fresh marker can detonate the survivor");
    config.packages[0]=remote;
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
