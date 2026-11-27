#include <cstdio>
#include <cstdlib>
#include "engine/core/headless_backends.hpp"
#include "engine/gameplay/portable_roster_ai.hpp"
#include "platform/pc/account_save_storage.hpp"
#include "platform/pc/portable_battle_host.hpp"
#include "platform/pc/portable_challenge_launch.hpp"

namespace {
using namespace gf;
using namespace gf::pc;
using Status=PortableBattleStatus;

void check(bool condition,const char* message){
    if(!condition){std::fprintf(stderr,"portable duel: %s\n",message);std::abort();}
}

void saved_force_cases(){
    MixedBorgForceView force;
    const PackageForceSlotView exact{7,{5,{"example:flight-stick","Flight Stick"},false,SavedPackageVariant{"clear","Clear"}},
        PackageForceSlotStatus::MetadataReady,80};
    PortableBattleForceSnapshot out;
    check(portable_single_saved_force(force,12,{},&out).status==Status::Denied,"empty Force");
    force.slots.push_back({7,{},exact});
    check(portable_single_saved_force(force,12,{},&out).status==Status::Ready&&out.revision==12&&
        out.members.size()==1&&out.members[0].package_id=="example:flight-stick"&&out.members[0].variant_id=="clear",
        "pin exact saved variant");
    const auto prior=out;
    check(portable_single_saved_force(force,12,{70},&out).status==Status::Denied&&out==prior,"mode budget refusal");
    force.slots.push_back({8,RetailForceSlotView{},{}});
    check(portable_single_saved_force(force,12,{},&out).status==Status::Unsupported&&out==prior,"mixed Force not truncated");
    force.slots.erase(force.slots.begin());
    check(portable_single_saved_force(force,12,{},&out).status==Status::Unsupported&&out==prior,"retail not substituted");
    force.slots.front().packaged=exact;
    check(portable_single_saved_force(force,12,{},&out).status==Status::Denied&&out==prior,"overlap refused");
    force.slots.front().retail.reset();
    force.slots.front().packaged->saved.disabled=true;
    check(portable_single_saved_force(force,12,{},&out).status==Status::Denied&&out==prior,"disabled copy refused");
    force.slots.front().packaged->saved.disabled=false;
    force.slots.front().packaged->saved.variant.reset();
    check(portable_single_saved_force(force,12,{},&out).status==Status::Missing&&out==prior,"no default variant fallback");
}

std::shared_ptr<const BorgGameplayProfile> definition(std::string_view movement,std::string_view body){
    BorgGameplayProfile profile;
    std::string error;
    const auto valid=borg_read_gameplay_profile("GFBORG-GAMEPLAY 1\nvariant=normal\nmovement="+std::string(movement)+
        "\nrig-prefix=body.prefix\npose-profile=body.pose\nidle=0:0\n"+std::string(body),&profile,&error);
    check(valid,error.c_str());
    return std::make_shared<const BorgGameplayProfile>(std::move(profile));
}

void duel_cases(){
    const auto unarmed=definition("stationary-v1","health=1\n");
    const auto armed=definition("grounded-v1|8|13|14","health=15\nwalk=0:1\nair=0:2\ndash=0:3\n"
        "projectile=shot|beam|1|60|0|0|0|8|30|19|ignore-floor\n"
        "move=fire|b|10|any|4|0:4\nrelease=fire|2|none|0|shot\n");
    BattleMvpConfig config;
    config.spawn={BattlePoint{0,0},BattlePoint{0,220}};
    config.packages={unarmed,armed};
    NativeBattleMvp world;
    using End=PortableDuelEnd;
    check(portable_duel_end(world,1)==End::Fighting,"unstarted world has no result");
    check(world.start(config),"start exact authored duel");
    check(portable_duel_end(world,1)==End::Fighting,"tick zero has no result");
    auto step_enemy=[&](){
        const auto enemy=portable_duel_enemy_input(world);
        check(enemy.attack.valid(),"AI command respects action input shape");
        check(world.step({GamePadState{},enemy.movement},false,nullptr,nullptr,nullptr,nullptr,{{{},enemy.attack}}),
            "authored duel advances through shared input path");
    };
    const auto first=portable_duel_enemy_input(world);
    check(first.attack.pressed==BorgBattleInputs::b&&first.attack.held==BorgBattleInputs::b,"AI presses primary on tick zero");
    step_enemy();
    check(portable_duel_end(world,1)==End::Timeout&&!portable_duel_campaign_outcome(End::Timeout),"expiry is not a win or loss");
    while(world.tick()<40&&portable_duel_end(world,0)==End::Fighting)step_enemy();
    check(world.hits()==1&&portable_duel_end(world,world.tick())==End::Loss&&
        portable_duel_campaign_outcome(End::Loss)==PortableBattleOutcome::Loss,"AI's actual KO has precedence over same-tick expiry");
    config.packages={armed,unarmed};
    check(world.start(config),"start flipped authored duel");
    while(world.tick()<40&&portable_duel_end(world,0)==End::Fighting){
        BorgBattleInputs player;
        if(!world.tick())player.pressed=player.held=BorgBattleInputs::b;
        check(world.step({},false,nullptr,nullptr,nullptr,nullptr,{player,{}}),"player steps");
    }
    check(world.hits()==1&&portable_duel_end(world,world.tick())==End::Win&&
        portable_duel_campaign_outcome(End::Win)==PortableBattleOutcome::Win,"only player KO produces win");
    check(!portable_duel_campaign_outcome(End::Fighting)&&!portable_duel_campaign_outcome(End::Draw)&&
        !portable_duel_campaign_outcome(End::Cancelled),"non-decisions do not produce a campaign result");
}

void roster_control_and_camera_cases(){
    const auto fighter=definition("grounded-v1|8|13|14","health=200\nwalk=0:1\nair=0:2\ndash=0:3\n"
        "projectile=shot|beam|1|60|0|0|0|8|30|19|ignore-floor\n"
        "move=fire|b|10|any|4|0:4\nrelease=fire|2|none|0|shot\n");
    BattleMvpConfig config;config.declared_actors=6;config.packages.fill(fighter);config.package_strafe=true;
    config.spawn={BattlePoint{-300,0},BattlePoint{300,0},BattlePoint{-300,-240},
                  BattlePoint{300,-240},BattlePoint{-300,240},BattlePoint{300,240}};
    NativeBattleMvp world;
    check(world.start(config),"six-actor portable control setup");
    const auto control=NativeBattleMvpView::roster_movement_frame(world);
    const auto initial_camera=NativeBattleMvpView::roster_camera_for(world,800,480);
    const auto initial_target=world.target_of(0);
    const auto facing0=world.actors()[0].facing;
    const auto facing1=world.actors()[1].facing;
    std::array<GamePadState,kBattleMvpMaxActors> local_pads{};
    const std::array<BorgBattleInputs,kBattleMvpMaxActors> no_attacks{};
    for(unsigned i=0;i<2;++i)local_pads[i].dpad_right=local_pads[i].r=true;
    local_pads[2].dpad_right=true;
    const NativeBorgMovementFrame invalid{{0,0},{1,0}};
    check(!world.step_roster(local_pads,no_attacks,false,nullptr,{},&control,nullptr,nullptr,{},{&invalid})&&world.tick()==0,
          "invalid peer camera frame is refused without stepping");
    check(world.step_roster(local_pads,no_attacks,false,nullptr,{},&control,nullptr,nullptr,{},{&control}),
          "both locals can use the shared camera frame");
    for(unsigned i=0;i<2;++i){
        const auto& actor=world.actors()[i];
        const auto step=actor.velocity;
        check(step.x*control.right.x+step.z*control.right.z>0&&std::abs(step.x*control.forward.x+step.z*control.forward.z)<.0001f,
              "right moves each local along the screen-right axis");
        f32 before=0,after=0;
        const BattleMvpVec start{config.spawn[i].x,0,config.spawn[i].z};
        check(initial_camera.project(start.data(),&before,nullptr,nullptr)&&
              initial_camera.project(actor.position.data(),&after,nullptr,nullptr)&&after>before,
              "both local actors really move right in the shared view");
    }
    check(world.actors()[0].facing.x==facing0.x&&world.actors()[0].facing.z==facing0.z&&
          world.actors()[1].facing.x==facing1.x&&world.actors()[1].facing.z==facing1.z,
          "movement basis does not override strafe body headings");
    check(world.actors()[2].velocity.x>0&&world.actors()[2].velocity.z==0,
          "later slots still consume native arena-relative input");
    check(NativeBattleMvpView::roster_camera_for(world,800,480).target[0]<initial_camera.target[0],
          "camera reframes as a local actor moves beyond the original group bounds");
    local_pads={};local_pads[0].dpad_left=true;
    for(unsigned tick=0;tick<40;++tick)
        check(world.step_roster(local_pads,no_attacks,false,nullptr,{},&control,nullptr,nullptr,{},{&control}),
              "roster advances with a shared movement basis");
    check(world.target_of(0)!=initial_target,"walking changes the nearest aim target");
    const auto reframed=NativeBattleMvpView::roster_camera_for(world,800,480);
    const auto reframed_basis=NativeBattleMvpView::movement_frame(reframed);
    check(std::hypot(reframed_basis.forward.x-control.forward.x,reframed_basis.forward.z-control.forward.z)<.0001f,
          "camera does not rotate on retarget");
    check(world.start(config),"restart the roster for CPU input checks");
    PortableRosterAi cpu;
    check(portable_duel_enemy_input(world).attack==BorgBattleInputs{}&&
          cpu.input(world,0).attack==BorgBattleInputs{}&&cpu.input(world,1).attack==BorgBattleInputs{}&&
          cpu.input(world,6).attack==BorgBattleInputs{},
          "roster AI does not borrow either local player or an absent slot");
    unsigned cpu_started=0;
    for(unsigned tick=0;tick<37;++tick){
        std::array<GamePadState,kBattleMvpMaxActors> pads{};
        std::array<BorgBattleInputs,kBattleMvpMaxActors> inputs{};
        for(unsigned slot=2;slot<world.actor_count();++slot){
            const auto ai=cpu.input(world,slot);
            pads[slot]=ai.movement;inputs[slot]=ai.attack;
            check(ai.attack.valid()&&!ai.movement.a,"roster AI feeds authored edges rather than the legacy fire bit");
        }
        check(world.step_roster(pads,inputs,false),"roster package input visit");
        for(const auto& e:world.package_events())if(e.kind==BorgBattleEventKind::Started)cpu_started|=1u<<e.actor;
    }
    check(cpu_started==0x3c,"all four CPU slots visit their primary move, and locals stay idle");
    for(const auto& [width,height]:{std::pair{800,480},std::pair{320,720}}){
        const auto camera=NativeBattleMvpView::roster_camera_for(world,width,height);
        const auto direction=native_borg_direction({camera.eye[0],camera.eye[2]},{camera.target[0],camera.target[2]});
        check(std::hypot(direction.x-control.forward.x,direction.z-control.forward.z)<.0001f,
              "the roster framing does not change the local movement basis");
        for(const auto& actor:world.actors())if(actor.health)
            for(f32 x:{-config.actor_radius,config.actor_radius})
                for(f32 y:{0.f,config.actor_height})
                    for(f32 z:{-config.actor_radius,config.actor_radius}){
                        const BattleMvpVec point{actor.position[0]+x,actor.position[1]+y,actor.position[2]+z};
                        f32 screen_x=0,screen_y=0;
                        check(camera.project(point.data(),&screen_x,&screen_y,nullptr)&&screen_x>=0&&screen_x<=width&&
                              screen_y>=0&&screen_y<=height,"roster frame contains every living body proxy");
                    }
    }
}

void roster_cpu_attack_cases(){
    constexpr auto movement="grounded-v1|8|13|15";
    const std::string common="health=1000\nwalk=0:1\nair=0:2\ndash=0:3\n";
    const std::string bolt="projectile=shot|bullet|1|55|0|0|0|8|45|19|ignore-floor\n";
    const auto target=definition("stationary-v1","health=1000\n");
    const auto melee=definition(movement,common+
        "move=slash|b|10|grounded|14|0:4\nrange=slash|150\nmelee=slash|4|7|95|24|60|30|19\n");
    const auto charge=definition(movement,common+bolt+
        "move=charge|b|10|grounded|12|0:4\nactivation=charge|release-window|24|60\nrelease=charge|2|none|0|shot\n");
    const auto secondary=definition(movement,common+bolt+
        "move=secondary|x|10|any|12|0:4\nrelease=secondary|2|none|0|shot\n");
    const auto airborne=definition(movement,common+bolt+
        "move=air-shot|b|10|airborne|12|0:4\nactivation=air-shot|release-window|6|30\nrelease=air-shot|2|none|0|shot\n");
    BattleMvpConfig config;config.declared_actors=6;
    config.packages={target,target,melee,charge,secondary,airborne};
    config.spawn={BattlePoint{0,600},BattlePoint{0,0},BattlePoint{0,-580},
                  BattlePoint{550,500},BattlePoint{-600,0},BattlePoint{570,900}};
    NativeBattleMvp world;
    PortableRosterAi cpu;
    check(world.start(config),"CPU approach and input fixture starts");
    const auto before=world.actors()[2].position;
    bool slash_hit=false,charged=false,secondary_pressed=false,air_jump=false;
    unsigned emitters=0;
    for(unsigned n=0;n<140;++n){
        std::array<GamePadState,6> pads{};
        std::array<BorgBattleInputs,6> inputs{};
        for(unsigned slot=2;slot<world.actor_count();++slot){
            const auto ai=cpu.input(world,slot);
            pads[slot]=ai.movement;inputs[slot]=ai.attack;
            const auto same_tick=cpu.input(world,slot);
            check(ai.attack.valid()&&!ai.movement.a&&same_tick.attack==ai.attack&&same_tick.movement==ai.movement,
                  "CPU issues valid stable pad samples for each tick");
        }
        if(!n)check(!inputs[2].pressed&&pads[2].stick_y>0,"melee CPU closes rather than attacking at the old orbit distance");
        secondary_pressed|=inputs[4].pressed==BorgBattleInputs::x;
        air_jump|=pads[5].b;
        check(world.step_roster(pads,inputs,false),"simulate profile-guided CPU inputs");
        for(const auto& e:world.package_events()){
            if(e.actor==2&&e.kind==BorgBattleEventKind::Hit&&e.recipient==1)slash_hit=true;
            if(e.actor==3&&e.kind==BorgBattleEventKind::ButtonRelease&&e.value==25)charged=true;
            if(e.kind==BorgBattleEventKind::Emitted)emitters|=1u<<e.actor;
        }
    }
    check(slash_hit&&world.actors()[2].position[2]>before[2]+400,"melee CPU approaches and hits a real body");
    check(charged&&secondary_pressed&&air_jump&&emitters==0x38,
          "long grounded charge, X-only fallback and air-only charge all emit through the scheduler");

    const auto held=definition(movement,common+bolt+
        "stock=mag|2|90\nmove=held|b|10|grounded|12|0:4\nheld-fire=held|mag|1|shot|30|26|26|10\n");
    config.declared_actors=3;config.packages={target,target,held};config.spawn[2]={0,-500};
    check(world.start(config),"held CPU fixture starts");
    cpu.reset();
    unsigned releases=0;
    for(unsigned n=0;n<37;++n){
        std::array<GamePadState,3> pads{};
        std::array<BorgBattleInputs,3> inputs{};
        const auto ai=cpu.input(world,2);
        pads[2]=ai.movement;inputs[2]=ai.attack;
        if(n==10){
            check(ai.attack.held&&world.set_paused(true)&&cpu.input(world,2).attack==BorgBattleInputs{}&&world.set_paused(false),
                  "pause cannot advance a held CPU input");
        }
        check(world.step_roster(pads,inputs,false),"simulate bounded held fire");
        for(const auto& e:world.package_events())if(e.actor==2&&e.kind==BorgBattleEventKind::Emitted)++releases;
    }
    check(releases==1&&world.package_action(2).stocks[0].current==1,"CPU holds through the slow cadence and pays normal stock");
    check(world.start(config),"CPU round restart");
    cpu.reset();
    check(cpu.input(world,2).attack.pressed==BorgBattleInputs::b,"CPU round restart clears its prior hold and wait");

    const auto field=definition(movement,common+
        "move=field|x|10|grounded|14|0:4\nfield=field|2|sphere|165|165|165|14|20|50|90|150|250|150\n"
        "field-core-damage=field|enemies|145|9|2\n");
    config.packages={target,target,field};
    check(world.start(config),"field-only CPU fixture starts");
    cpu.reset();
    bool field_hit=false;
    for(unsigned n=0;n<100;++n){
        const auto ai=cpu.input(world,2);
        check(world.step_roster(std::array<GamePadState,3>{{{},{},ai.movement}},
                               std::array<BorgBattleInputs,3>{{{},{},ai.attack}},false),"simulate a field-only CPU");
        for(const auto& e:world.package_events())if(e.actor==2&&e.kind==BorgBattleEventKind::Hit&&e.recipient==1)field_hit=true;
    }
    check(field_hit,"damaging field-only X approaches and hits through ordinary field updates");
}

void roster_cpu_height_cases(){
    const auto idle=definition("stationary-v1","health=500\n");
    const std::string clips="health=500\nwalk=0:1\nair=0:2\ndash=0:3\n";
    const auto melee=[&](std::string_view context){
        return definition("grounded-v1|8|13|14",clips+"move=slash|b|10|"+std::string(context)+
            "|14|0:4\nrange=slash|150\nmelee=slash|4|7|95|24|60|35|19\n");
    };
    const auto cruise=[&](unsigned height){
        return definition("grounded-v1|0.4|11|0.4",clips+"cruise-boost="+std::to_string(height)+"|18|4|2|7|4\n"
            "projectile=poke|bullet|1|55|0|0|0|8|30|1|ignore-floor\n"
            "move=fire|b|10|any|8|0:4\nrelease=fire|2|none|0|poke\n");
    };
    BattleMvpConfig config;config.declared_actors=3;
    config.spawn={BattlePoint{600,600},BattlePoint{0,0},BattlePoint{0,-120}};
    NativeBattleMvp world;
    PortableRosterAi cpu;
    struct Seen {bool jumped=false,pressed=false,hit=false;};
    const auto simulate=[&](std::string_view context,unsigned height){
        config.packages={idle,cruise(height),melee(context)};
        check(world.start(config),"start CPU height fixture");
        cpu.reset();
        Seen seen;
        for(unsigned tick=0;tick<55;++tick){
            const auto ai=cpu.input(world,2);
            if(!tick)check(!ai.attack.pressed,"CPU does not immediately swing below the cruise body");
            seen.jumped|=ai.movement.b;
            seen.pressed|=bool(ai.attack.pressed);
            check(world.step_roster(std::array<GamePadState,3>{{{},{},ai.movement}},
                                   std::array<BorgBattleInputs,3>{{{},{},ai.attack}},false),"advance CPU height fixture");
            for(const auto& e:world.package_events())
                if(e.actor==2&&e.kind==BorgBattleEventKind::Hit&&e.recipient==1)seen.hit=true;
        }
        return seen;
    };
    const auto reachable=simulate("any",140);
    check(reachable.jumped&&reachable.pressed&&reachable.hit,"air-capable melee jumps and hits a reachable cruise body");
    const auto grounded=simulate("grounded",140);
    check(!grounded.jumped&&!grounded.pressed&&!grounded.hit,"ground-only strike is not invented in the air");
    const auto too_high=simulate("airborne",500);
    check(!too_high.jumped&&!too_high.pressed&&!too_high.hit,"unreachable height does not create futile hops or swings");
}

void roster_cpu_height_rival_cases(){
    struct Terrain final:IBattleMvpTerrain {
        std::array<StageBodyPartition,1> boxes{{{"remote-wall",1000,-6,1200,6,true}}};
        std::span<const StageBodyPartition> body_partitions()const override{return boxes;}
        bool height(f32,f32,f32,f32* out)const override{if(!out)return false;*out=0;return true;}
    } terrain;
    const auto idle=definition("stationary-v1","health=500\n");
    const std::string clips="health=500\nwalk=0:1\nair=0:2\ndash=0:3\n";
    const auto cruise=[&](unsigned height){
        return definition("grounded-v1|0.4|11|0.4",clips+"cruise-boost="+std::to_string(height)+"|18|4|2|7|4\n"
            "projectile=shot|bullet|1|55|0|0|0|8|45|19|ignore-floor\n"
            "move=parked|b|10|grounded|8|0:4\nrelease=parked|2|none|0|shot\n");
    };
    const auto melee=[&](std::string_view context,bool fallback=false){
        return definition("grounded-v1|8|13|14",clips+"move=slash|b|10|"+std::string(context)+
            "|14|0:4\nrange=slash|145\nmelee=slash|4|7|95|24|60|35|19\n"+
            (fallback?"projectile=shot|bullet|1|55|0|0|0|8|45|19|ignore-floor\n"
                      "move=shot|x|1|any|8|0:4\nrelease=shot|2|none|0|shot\n":""));
    };
    BattleMvpConfig config;config.declared_actors=3;config.teams_declared=true;config.local_aim_cycle=true;
    config.teams={0,0,1};config.spawn={BattlePoint{0,0},BattlePoint{0,-450},BattlePoint{0,-120}};
    NativeBattleMvp world;PortableRosterAi cpu;
    const auto start=[&](const IBattleMvpTerrain* stage){
        cpu.reset();
        check(world.start(config,stage)&&world.target_of(2)==0,"nearer rival stays the ordinary closest body");
    };
    config.packages={cruise(500),idle,melee("grounded")};
    start(&terrain);
    bool hit=false;
    for(unsigned n=0;n<100&&!hit;++n){
        const auto ai=cpu.input(world,2,&terrain);
        if(!n)check(ai.aim_target==1&&ai.movement.stick_y<0&&!ai.attack.pressed,
                    "melee-only CPU approaches the lower rival instead of fixating on the cruiser");
        const std::array<u8,1> hints{ai.aim_target};
        check(world.step_roster(std::array<GamePadState,3>{{{},{},ai.movement}},
            std::array<BorgBattleInputs,3>{{{},{},ai.attack}},false,&terrain,{},nullptr,nullptr,nullptr,{},
            {nullptr,0,ai.aim_target==255?std::span<const u8>{}:std::span<const u8>(hints)}),"step with native one-tick rival hint");
        if(!n)check(world.target_of(2)==0&&world.aim_target_of(2)==1,"hint changes aim, not the nearest body");
        for(const auto& e:world.package_events())if(e.kind==BorgBattleEventKind::Hit&&e.actor==2){
            check(e.recipient==1,"real contact resolves at the lower rival");hit=true;
        }
    }
    check(hit,"lower rival is hit by a real authored strike");
    start(nullptr);
    check(cpu.input(world,2).aim_target==255,"no hint is added outside partitioned native rosters");

    config.packages[2]=melee("grounded",true);start(&terrain);
    const auto ranged=cpu.input(world,2,&terrain);
    check(ranged.aim_target==255&&ranged.attack.pressed==BorgBattleInputs::x,"a ranged fallback retains the nearer flying rival");
    config.packages={cruise(140),idle,melee("any")};start(&terrain);
    const auto hop=cpu.input(world,2,&terrain);
    check(hop.aim_target==255&&hop.movement.b,"attainable cruiser still gets the ordinary melee hop");
    config.packages={cruise(500),cruise(500),melee("grounded")};start(&terrain);
    check(cpu.input(world,2,&terrain).aim_target==255,"do not replace an unreachable target with another unreachable cruiser");

    config.packages={cruise(500),idle,melee("airborne")};config.spawn[1]={130,-160};start(&terrain);
    const auto takeoff=cpu.input(world,2,&terrain);
    const std::array<u8,1> lower{1};
    check(takeoff.aim_target==1&&takeoff.movement.b&&
          world.step_roster(std::array<GamePadState,3>{{{},{},takeoff.movement}},std::array<BorgBattleInputs,3>{},
                            false,&terrain,{},nullptr,nullptr,nullptr,{}, {nullptr,0,lower})&&
          !world.actors()[2].grounded&&world.target_of(2)==0&&cpu.input(world,2,&terrain).aim_target==1,
          "the CPU still prefers the lower rival after leaving the floor for its own ordinary hop");

    config.packages={idle,cruise(500),melee("grounded")};
    config.spawn[0]={0,105};config.spawn[1]={0,-450};terrain.boxes[0]={"sealed",-1800,-6,1800,6,true};
    start(&terrain);
    const auto sealed=cpu.input(world,2,&terrain);
    check(sealed.aim_target==255&&!sealed.movement.stick_y&&!sealed.attack.pressed,
          "a clear route does not justify choosing an unreachable cruiser over an opponent behind a wall");
}

void roster_cpu_stock_cases(){
    const auto idle=definition("stationary-v1","health=1000\n");
    const std::string body="health=1000\nwalk=0:1\nair=0:2\ndash=0:3\n"
        "projectile=shot|bullet|1|55|0|0|0|8|45|9|ignore-floor\n";
    const auto make=[&](std::string text){return definition("grounded-v1|8|13|14",body+text);};
    BattleMvpConfig config;config.declared_actors=3;
    config.spawn={BattlePoint{600,700},BattlePoint{0,0},BattlePoint{0,-500}};
    NativeBattleMvp world;
    PortableRosterAi cpu;
    struct Seen {
        std::vector<std::pair<u8,u32>> starts;
        std::vector<u8> emitted;
        std::vector<u32> fields;
        u32 refill=0,release=0,debit=0;
        unsigned refusals=0;
    };
    const auto simulate=[&](std::shared_ptr<const BorgGameplayProfile> profile,unsigned ticks){
        config.packages={idle,idle,std::move(profile)};
        check(world.start(config),"start CPU stock fixture");
        cpu.reset();
        Seen seen;
        for(unsigned n=0;n<ticks;++n){
            const auto ai=cpu.input(world,2);
            if(!seen.release&&ai.attack.released)seen.release=world.tick();
            check(world.step_roster(std::array<GamePadState,3>{{{},{},ai.movement}},
                                   std::array<BorgBattleInputs,3>{{{},{},ai.attack}},false),"advance stock input");
            for(const auto& e:world.package_events())if(e.actor==2){
                if(e.kind==BorgBattleEventKind::Started)seen.starts.emplace_back(e.move,e.tick);
                if(e.kind==BorgBattleEventKind::Emitted)seen.emitted.push_back(e.move);
                if(e.kind==BorgBattleEventKind::FieldPlaced)seen.fields.push_back(e.tick);
                if(e.kind==BorgBattleEventKind::StockRefilled&&!seen.refill)seen.refill=e.tick;
                if(e.kind==BorgBattleEventKind::StockSpent&&!seen.debit)seen.debit=e.tick;
                if(e.kind==BorgBattleEventKind::StockRefused||e.kind==BorgBattleEventKind::EntryStockRefused)++seen.refusals;
            }
        }
        return seen;
    };
    const auto switched=simulate(make(
        "stock=mag|1|55\nmove=primary|b|10|any|10|0:4\nentry-stock=primary|mag|1\nrelease=primary|2|mag|1|shot\n"
        "move=alternate|x|20|any|10|0:4\nrelease=alternate|2|none|0|shot\n"),80);
    const std::vector<u8> fallback_sequence{0,1,0};
    check(switched.emitted==fallback_sequence&&!switched.refusals,
          "use stocked B, switch to X while empty, then return to B after the real refill");

    const auto waited=simulate(make(
        "stock=mag|1|31\nmove=primary|b|20|any|10|0:4\nrelease=primary|2|mag|1|shot\n"
        "move=shadowed|b|10|any|10|0:4\nrelease=shadowed|2|none|0|shot\n"),58);
    check(waited.starts.size()==2&&waited.starts[0].first==0&&waited.starts[1].first==0&&
          waited.refill&&waited.starts[1].second==waited.refill+1&&!waited.refusals,
          "an empty later debit cannot be bypassed by guessing a lower-priority B; retry immediately after refill");

    const auto approach=simulate(make(
        "move=slash|b|10|any|14|0:4\nrange=slash|150\nmelee=slash|4|7|95|24|60|30|19\n"
        "move=alternate|x|20|any|10|0:4\nrelease=alternate|2|none|0|shot\n"),100);
    check(!approach.starts.empty()&&approach.starts.front().first==1&&
          std::any_of(approach.starts.begin(),approach.starts.end(),[](auto e){return e.first==0;}),
          "use a ranged X while approaching for short-range B");

    const auto exhausted=simulate(make("stock=mag|1|90\nmove=held|b|10|any|12|0:4\n"
        "held-fire=held|mag|1|shot|12|6|6|0\n"),28);
    check(exhausted.emitted.size()==1&&exhausted.debit&&exhausted.release==exhausted.debit&&!exhausted.refusals,
          "release the held weapon as soon as the accepted debit empties it");

    config.spawn[2]={0,-118};
    const std::string paid="stock=cell|1|55\nmove=paid|b|10|any|14|0:4\n"
        "field=paid|3|sphere|168|168|168|14|0|50|90|150|250|150\n"
        "field-core-damage=paid|enemies|165|9|2\nfield-debit=paid|cell|1\n";
    const auto field_wait=simulate(make(paid+
        "move=shadowed|b|1|any|10|0:4\nrange=shadowed|165\nrelease=shadowed|2|none|0|shot\n"),80);
    check(field_wait.fields.size()==2&&field_wait.starts.size()==2&&!field_wait.refusals&&field_wait.emitted.empty()&&
          field_wait.refill&&field_wait.starts.back()==std::pair<u8,u32>{0,field_wait.refill+1},
          "sole paid field waits for its actual refill without bypassing the higher B");
    const auto field_x=simulate(make(paid+
        "move=alternate|x|1|any|10|0:4\nrelease=alternate|2|none|0|shot\n"),80);
    check(field_x.fields.size()==2&&field_x.emitted==std::vector<u8>{1}&&!field_x.refusals,
          "a ready X is still usable while the B field's pool is empty");
    const auto mixed=simulate(make(paid+"release=paid|2|none|0|shot\n"),48);
    check(mixed.fields.size()==1&&mixed.emitted==std::vector<u8>{0,0}&&mixed.refusals==1,
          "a field debit does not suppress an independent projectile on the same move");
}

void roster_aim_cases(){
    const auto shooter=definition("grounded-v1|8|13|14","health=200\nwalk=0:1\nair=0:2\ndash=0:3\n"
        "projectile=shot|beam|1|40|0|0|0|8|30|19|ignore-floor\n"
        "guidance=shot|tracking|0.1|12\n"
        "move=fire|b|10|any|6|0:4\nrelease=fire|2|none|0|shot\n"
        "projectile=committed|bullet|1|40|0|0|0|8|30|19|ignore-floor\n"
        "move=commit|x|20|any|6|0:4\naim-lock=commit|0\nrelease=commit|2|none|0|committed\n");
    const auto fragile=definition("stationary-v1","health=19\n");
    const auto healthy=definition("stationary-v1","health=200\n");
    BattleMvpConfig config;config.declared_actors=6;config.local_aim_cycle=true;
    config.packages={shooter,fragile,healthy,fragile,healthy,healthy};
    config.spawn={BattlePoint{0,0},BattlePoint{0,300},BattlePoint{260,0},
                  BattlePoint{0,520},BattlePoint{480,0},BattlePoint{350,430}};
    NativeBattleMvp world;
    check(world.start(config)&&world.aim_target_of(0)==1&&world.aim_target_of(1)==0,"round begins with nearest aim");
    const auto tick=[&](u8 cycle=0,u8 fire=0){
        std::array<BorgBattleInputs,6> inputs{};
        inputs[0].pressed=inputs[0].held=fire;
        return world.step_roster(std::array<GamePadState,6>{},inputs,false,nullptr,{},nullptr,nullptr,nullptr,{},{nullptr,cycle});
    };
    check(!tick(4)&&world.tick()==0,"CPU aim edge refused atomically");
    check(world.set_paused(true)&&tick(3)&&world.tick()==0&&world.aim_target_of(0)==1&&
          world.aim_target_of(1)==0&&world.set_paused(false),"pause does not queue target changes");
    const auto heading=world.actors()[0].facing;
    check(tick(3,BorgBattleInputs::b)&&world.aim_target_of(0)==3&&world.aim_target_of(1)==2,
          "both locals cycle independently, skipping allied slots");
    check(world.target_of(0)==1&&tick()&&tick(),"guided direct volley uses manual aim instead of nearest enemy");
    const auto& first=world.projectiles().front();
    check(first.active&&first.package_guide_target==3&&std::abs(first.velocity[0])<.001f&&first.velocity[2]>0,
          "guided release is directed toward the chosen enemy");
    check(tick(1)&&first.active&&first.package_guide_target==3&&world.aim_target_of(0)==5&&world.aim_target_of(1)==2,
          "cycling P1 does not redirect a live projectile or change the other local");
    check(world.actors()[0].facing==heading&&world.actors()[0].aim_facing.x>.5f,
          "changing aim does not force the body heading");
    check(tick(1)&&world.aim_target_of(0)==1,
          "aim wraps without redirecting a live projectile");
    for(unsigned i=0;i<8;++i)check(tick(),"first shot reaches the intervening enemy");
    check(!world.actors()[1].health&&world.actors()[3].health==19&&world.last_receiver()==1&&
          world.aim_target_of(0)==3&&world.aim_target_of(1)==255,
          "real contact can hit a closer enemy and defeated locks and locals fall back correctly");
    // After losing slot 1, cycle 3 -> 5 -> 3 without admitting defeated slot 1.
    check(tick(1)&&world.aim_target_of(0)==5&&tick(1,BorgBattleInputs::x)&&world.aim_target_of(0)==3,
          "cycle wraps past a defeated and several allied slots");
    check(tick(1)&&world.aim_target_of(0)==5&&tick(),"cycle between committed aim and release");
    const auto& second=world.projectiles().front();
    check(second.active&&std::abs(second.velocity[0])<.001f&&second.velocity[2]>0,
          "aim committed before cycling is still used at release");
    check(tick(1)&&world.aim_target_of(0)==3,"reselect the target of the committed shot");
    for(unsigned i=0;i<16;++i)check(tick(),"second shot reaches the unblocked enemy");
    check(!world.actors()[3].health&&world.aim_target_of(0)==5&&world.last_receiver()==3&&world.hits()==2,
          "the second real contact defeats the selected enemy and auto-aim resumes");
    check(world.start(config)&&world.aim_target_of(0)==1&&world.aim_target_of(1)==0,"restart clears both local selections");
    config.local_aim_cycle=false;
    check(world.start(config)&&!tick(1)&&world.tick()==0&&world.aim_target_of(0)==1,"other modes reject local cycling");
}

void roster_cpu_aim_cases(){
    struct Terrain final:IBattleMvpTerrain {
        std::array<StageBodyPartition,1> boxes{{{"wall",-1800,-6,1800,6}}};
        std::span<const StageBodyPartition> body_partitions()const override{return boxes;}
        bool height(f32,f32,f32,f32* out)const override{if(!out)return false;*out=0;return true;}
    } terrain;
    const auto idle=definition("stationary-v1","health=200\n");
    const auto melee=definition("grounded-v1|8|13|14","health=200\nwalk=0:1\nair=0:2\ndash=0:3\n"
        "move=slash|b|10|any|14|0:4\nrange=slash|145\nmelee=slash|4|7|95|24|60|35|19\n");
    BattleMvpConfig config;config.declared_actors=4;config.teams_declared=true;config.local_aim_cycle=true;
    config.teams={0,0,1,1};config.packages={idle,idle,melee,idle};
    config.spawn={BattlePoint{0,130},BattlePoint{0,-420},BattlePoint{0,-100},BattlePoint{400,-200}};
    NativeBattleMvp world;
    PortableRosterAi cpu;
    check(world.start(config,&terrain)&&world.target_of(2)==0,"nearest enemy starts behind partition");
    const auto ai=cpu.input(world,2,&terrain);
    check(ai.aim_target==1&&ai.movement.stick_y<0&&!ai.attack.pressed,
          "short-range CPU turns toward a clear opponent rather than the blocked nearest");
    const auto tick=[&](std::span<const u8> aims,const IBattleMvpTerrain* stage){
        std::array<GamePadState,4> pads{};pads[2]=ai.movement;
        return world.step_roster(pads,std::array<BorgBattleInputs,4>{},false,stage,{},nullptr,nullptr,nullptr,{},
                                 {nullptr,0,aims});
    };
    const std::array<u8,1> wrong_count{1};
    const std::array<u8,2> valid{1,255};
    check(!tick(wrong_count,&terrain)&&!tick(valid,nullptr)&&world.tick()==0,
          "CPU hint needs exactly the CPU slots and an authored partition stage");
    for(u8 invalid:{u8(2),u8(3),u8(5)}){
        const std::array<u8,2> refused{invalid,255};
        check(!tick(refused,&terrain)&&world.tick()==0,"self, ally and absent CPU hints are refused atomically");
    }
    check(tick(valid,&terrain)&&world.target_of(2)==0&&world.aim_target_of(2)==1&&
          world.actors()[2].aim_facing.z<0&&world.aim_target_of(0)==2,
          "the same tick uses the planned aim without changing nearest or local aim");
    check(tick({},&terrain)&&world.aim_target_of(2)==0,"unsupplied CPU hint expires on the next accepted tick");
}

void roster_cpu_detour_dash_cases(){
    struct Terrain final:IBattleMvpTerrain {
        std::array<StageBodyPartition,1> boxes{{{"wall",-620,-6,620,6}}};
        std::span<const StageBodyPartition> body_partitions()const override{return boxes;}
        bool height(f32,f32,f32,f32* out)const override{if(!out)return false;*out=0;return true;}
    } terrain;
    const auto idle=definition("stationary-v1","health=200\n");
    const auto melee=[](bool dash){
        return definition("grounded-v1|2|13|15",std::string("health=200\nwalk=0:1\nair=0:2\n")+
            (dash?"dash=0:3\n":"dash-policy=disabled\n")+
            "move=slash|b|10|grounded|14|0:4\nrange=slash|145\nmelee=slash|4|7|95|24|60|35|19\n");
    };
    BattleMvpConfig config;config.declared_actors=3;config.teams_declared=true;
    config.teams={0,0,1};config.packages={idle,idle,melee(true)};
    config.spawn={BattlePoint{0,180},BattlePoint{100,300},BattlePoint{0,-280}};
    NativeBattleMvp world;PortableRosterAi cpu;
    const auto start=[&](){cpu.reset();check(world.start(config,&terrain),"start detour dash fixture");};
    start();
    for(unsigned n=0;n<10;++n){
        const auto ai=cpu.input(world,2,&terrain);
        if(!n)check(ai.movement.l,"slow melee CPU requests a dash on the opening clear leg");
        check(!ai.attack.pressed&&!ai.movement.b,"detour does not start an attack or jump at the wall");
        check(world.step_roster(std::array<GamePadState,3>{{{},{},ai.movement}},std::array<BorgBattleInputs,3>{},
                               false,&terrain),"step detour dash through shared body motion");
    }
    check(world.actors()[2].travel_distance>100&&world.actors()[2].position[2]<-6&&world.actors()[2].dash_ticks,
          "engine accelerates the CPU along its original side of the wall");
    config.spawn={BattlePoint{605,180},BattlePoint{585,360},BattlePoint{605,-100}};
    start();
    const auto corner=cpu.input(world,2,&terrain);
    check(!corner.movement.l&&corner.movement.stick_x>0&&!corner.attack.pressed,"walk the short leg to a nearby corner");
    config.spawn={BattlePoint{0,180},BattlePoint{100,300},BattlePoint{0,-280}};
    config.packages[2]=melee(false);start();
    check(!cpu.input(world,2,&terrain).movement.l,"detours preserve a package's disabled dash policy");
    config.packages[2]=melee(true);
    terrain.boxes[0].min_x=-1800;terrain.boxes[0].max_x=1800;start();
    const auto sealed=cpu.input(world,2,&terrain);
    check(!sealed.movement.l&&!sealed.movement.stick_x&&!sealed.movement.stick_y&&!sealed.attack.pressed,
          "sealed route still requests neither movement nor attack");
}

void roster_cpu_route_cost_cases(){
    struct Terrain final:IBattleMvpTerrain {
        std::array<StageBodyPartition,2> boxes{{{"wall",-620,-6,620,6,true},{"sealed",-61,-1800,-56,1800,true}}};
        unsigned count=1;
        std::span<const StageBodyPartition> body_partitions()const override{return std::span(boxes).first(count);}
        bool height(f32,f32,f32,f32* out)const override{if(!out)return false;*out=0;return true;}
    } wall;
    BattleMvpConfig config;config.declared_actors=3;config.teams_declared=true;config.local_aim_cycle=true;
    const auto idle=definition("stationary-v1","health=400\n");
    const auto melee=definition("grounded-v1|8|13|14","health=400\nwalk=0:1\nair=0:2\ndash=0:3\n"
        "move=slash|b|10|grounded|14|0:4\nrange=slash|145\nmelee=slash|4|7|95|24|60|35|19\n");
    config.teams={0,0,1};config.packages={idle,idle,melee};
    config.spawn={BattlePoint{-120,180},BattlePoint{620,200},BattlePoint{0,-180}};
    const auto path=[&](unsigned rival){
        return battle_partition_route(wall,config.spawn[2],config.spawn[rival],config.arena_half,config.actor_radius);
    };
    const auto longer=path(0),shorter=path(1);
    check(longer.count>1&&shorter.count>1&&longer.points[0].x<0&&shorter.points[0].x>0&&
         battle_partition_route_length(config.spawn[2],shorter)+config.actor_radius<
         battle_partition_route_length(config.spawn[2],longer),"both rivals are routed, but their first legs and costs differ");
    NativeBattleMvp world;PortableRosterAi cpu;
    check(world.start(config,&wall)&&world.target_of(2)==0,"the geometrically nearer rival is still slot 0");
    bool hit=false;
    for(unsigned n=0;n<140&&!hit;++n){
        const auto ai=cpu.input(world,2,&wall);
        if(!n)check(ai.aim_target==1&&ai.movement.stick_x>0&&!ai.attack.pressed,"take the shorter routed approach");
        const std::array<u8,1> hints{ai.aim_target};
        check(world.step_roster(std::array<GamePadState,3>{{{},{},ai.movement}},
            std::array<BorgBattleInputs,3>{{{},{},ai.attack}},false,&wall,{},nullptr,nullptr,nullptr,{},
            {nullptr,0,ai.aim_target==255?std::span<const u8>{}:std::span<const u8>(hints)}),"advance the selected route");
        if(!n)check(world.target_of(2)==0&&world.aim_target_of(2)==1,"route hint changes aim, not the nearest body");
        for(const auto& e:world.package_events())if(e.kind==BorgBattleEventKind::Hit&&e.actor==2){
            check(e.recipient==1,"physical contact hits the shorter-route rival");hit=true;
        }
    }
    check(hit,"shorter route reaches a live melee hit");
    config.local_aim_cycle=false;cpu.reset();
    check(world.start(config,&wall),"start with aim hints disabled");
    const auto nearest=cpu.input(world,2,&wall);
    check(nearest.aim_target==255&&nearest.movement.stick_x<0,"without hints, keep the ordinary route toward the nearest body");
    config.local_aim_cycle=true;
    config.spawn[0]={-60,180};config.spawn[1]={95,180};cpu.reset();
    const auto saving=battle_partition_route_length(config.spawn[2],path(0))-battle_partition_route_length(config.spawn[2],path(1));
    check(saving>0&&saving<config.actor_radius&&world.start(config,&wall)&&world.target_of(2)==0,
          "small route saving remains inside the nearest-rival preference");
    check(cpu.input(world,2,&wall).aim_target==255,"a marginal saving does not switch sides");

    config.spawn[0]={-120,180};config.spawn[1]={620,200};wall.count=2;cpu.reset();
    check(!path(0).count&&path(1).count&&world.start(config,&wall)&&world.target_of(2)==0,
          "the nearer rival is sealed off but the farther route is valid");
    check(cpu.input(world,2,&wall).aim_target==1,"unreachable rival cannot win the route comparison");

    wall.count=1;cpu.reset();
    config.spawn={BattlePoint{615,140},BattlePoint{-700,-200},BattlePoint{600,-170}};
    check(path(0).count>1&&path(1).count==1&&
          battle_partition_route_length(config.spawn[2],path(0))<battle_partition_route_length(config.spawn[2],path(1))&&
          world.start(config,&wall)&&world.target_of(2)==0,"short wall detour competes with a much longer clear walk");
    hit=false;
    for(unsigned n=0;n<90&&!hit;++n){
        const auto ai=cpu.input(world,2,&wall);
        if(!n)check(ai.aim_target==255&&ai.movement.stick_x>0,"don't abandon a short detour for a distant straight rival");
        const std::array<u8,1> hints{ai.aim_target};
        check(world.step_roster(std::array<GamePadState,3>{{{},{},ai.movement}},
            std::array<BorgBattleInputs,3>{{{},{},ai.attack}},false,&wall,{},nullptr,nullptr,nullptr,{},
            {nullptr,0,ai.aim_target==255?std::span<const u8>{}:std::span<const u8>(hints)}),"advance shorter detour");
        for(const auto& e:world.package_events())if(e.kind==BorgBattleEventKind::Hit&&e.actor==2){
            check(e.recipient==0,"short detour lands on the actual nearer rival");hit=true;
        }
    }
    check(hit,"short detour reaches a hit while the long straight alternative is still distant");
    config.spawn[1]={100,-200};cpu.reset();
    check(world.start(config,&wall),"start a more modest route comparison");
    const auto modest=cpu.input(world,2,&wall);
    check(modest.aim_target==1&&modest.movement.stick_x<0,"keep the clear approach when the detour doesn't halve the distance");
}

void roster_cpu_corner_melee_cases(){
    struct Terrain final:IBattleMvpTerrain {
        std::array<StageBodyPartition,1> boxes{{{"wall",-620,-6,620,6,true}}};
        std::span<const StageBodyPartition> body_partitions()const override{return boxes;}
        bool height(f32,f32,f32,f32* out)const override{if(!out)return false;*out=0;return true;}
    } wall;
    const auto idle=definition("stationary-v1","health=400\n");
    const auto melee=definition("grounded-v1|8|13|14","health=400\nwalk=0:1\nair=0:2\ndash=0:3\n"
        "move=slash|b|10|grounded|14|0:4\nrange=slash|145\nmelee=slash|4|7|95|24|60|35|19\n");
    BattleMvpConfig config;config.declared_actors=3;config.teams_declared=true;config.local_aim_cycle=true;
    config.teams={0,0,1};config.packages={idle,idle,melee};
    config.spawn={BattlePoint{619,88},BattlePoint{1020,-100},BattlePoint{680,-30}};
    NativeBattleMvp world;PortableRosterAi cpu;
    const auto visit=[&](bool ready){
        cpu.reset();
        check(world.start(config,&wall)&&world.target_of(2)==0,"corner fixture has a nearer rival and clear farther alternative");
        bool hit=false;
        for(unsigned tick=0;tick<10;++tick){
            const auto ai=cpu.input(world,2,&wall);
            if(!tick){
                check(bool(ai.attack.pressed)==ready,"melee intent uses the strike line rather than the body clearance");
                if(ready)check(ai.aim_target==255&&!ai.movement.stick_x&&!ai.movement.stick_y&&!ai.movement.l,
                               "keep the nearest rival and hold position for the strike");
            }
            const std::array<u8,1> hints{ai.aim_target};
            check(world.step_roster(std::array<GamePadState,3>{{{},{},ai.movement}},
                std::array<BorgBattleInputs,3>{{{},{},ai.attack}},false,&wall,{},nullptr,nullptr,nullptr,{},
                {nullptr,0,ai.aim_target==255?std::span<const u8>{}:std::span<const u8>(hints)}),"advance corner fixture");
            for(const auto& e:world.package_events())if(e.kind==BorgBattleEventKind::Hit&&e.actor==2){
                check(e.recipient==0,"normal melee contact still selects the actual nearby body");hit=true;
            }
        }
        check(hit==ready,"a clear strike lands; flagged occlusion remains solid");
    };
    check(!battle_partition_route_clear(wall,config.spawn[2],config.spawn[0],config.actor_radius)&&
          native_borg_melee_path_clear(&wall,config.spawn[2],config.spawn[0]),"movement does not fit but the corner strike does");
    visit(true);
    config.spawn[2]={0,-65};config.spawn[0]={0,65};
    visit(false);
    wall.boxes[0].blocks_melee=false;
    visit(true);
}

void roster_cpu_body_turn_cases(){
    const auto idle=definition("stationary-v1","health=400\n");
    const std::string slash="health=400\nwalk=0:1\nair=0:2\ndash=0:3\n"
        "move=slash|b|10|grounded|14|0:4\nrange=slash|145\nmelee=slash|4|7|95|24|60|35|19\n";
    BattleMvpConfig config;config.declared_actors=3;config.teams_declared=true;config.teams={0,0,1};
    config.packages={definition("grounded-v1|8|13|14",slash),idle,definition("grounded-v1|2.5|13|15",slash)};
    config.spawn={BattlePoint{0,1520},BattlePoint{800,1500},BattlePoint{0,1690}};
    const auto visit=[&](bool explicit_roster,bool local=false){
        auto round=config;
        round.local_aim_cycle=explicit_roster;
        const unsigned actor=local?0:2,rival=local?2:0;
        if(local){
            std::swap(round.packages[actor],round.packages[rival]);
            std::swap(round.spawn[actor],round.spawn[rival]);
        }
        NativeBattleMvp world;PortableRosterAi cpu;
        check(world.start(round),"start blocked turn fixture");
        std::array<GamePadState,3> pads{};
        std::array<BorgBattleInputs,3> inputs{};
        pads[actor].stick_y=127;
        for(unsigned n=0;n<32;++n)check(world.step_roster(pads,inputs,false),"move the CPU away until it faces the boundary");
        pads[actor]={};pads[rival].stick_y=127;
        for(unsigned n=0;n<36;++n)check(world.step_roster(pads,inputs,false),"rival closes against the CPU's back");
        const auto gap=[&](){return world.actors()[actor].position[2]-world.actors()[rival].position[2];};
        check(world.target_of(actor)==rival&&world.actors()[actor].facing.z>.95f&&
              std::abs(gap()-2*config.actor_radius)<.01f,"seed the blocked turn with real movement and solid body contact");
        const auto pinned=world.actors()[actor].position;
        pads={};
        bool hit=false;
        for(unsigned n=0;n<45&&!hit;++n){
            auto ai=local?PortableRosterInput{}:cpu.input(world,actor);
            if(local)ai.movement.stick_y=-45;
            if(!n)check(ai.movement.stick_y<0&&!ai.attack.pressed,"a CPU facing away requests movement toward the rival");
            pads[actor]=ai.movement;inputs[actor]=ai.attack;
            const auto before=world.actors()[actor].facing;
            check(world.step_roster(pads,inputs,false),"advance CPU turn with the native mover");
            const auto facing=world.actors()[actor].facing;
            check(gap()>=2*config.actor_radius-.01f&&std::abs(world.actors()[actor].position[2]-pinned[2])<.01f&&
                  std::abs(native_borg_aim_delta(before,facing))<=config.turn_per_tick+.001f&&(!local||facing==before),
                  "turn remains rate limited and the bodies remain solid");
            for(const auto& e:world.package_events())if(e.kind==BorgBattleEventKind::Hit&&e.actor==actor){
                check(e.recipient==rival,"melee contacts the actual rival");hit=true;
            }
        }
        return hit;
    };
    check(!visit(false)&&visit(true)&&!visit(true,true),"the roster CPU turns and strikes without changing local controls");
}

void roster_cpu_direct_dash_cases(){
    const auto idle=definition("stationary-v1","health=400\n");
    const auto melee=[](bool dash,bool fallback=false){
        return definition("grounded-v1|2|13|15",std::string("health=400\nwalk=0:1\nair=0:2\n")+
            (dash?"dash=0:3\n":"dash-policy=disabled\n")+
            "move=slash|b|10|grounded|14|0:4\nrange=slash|145\nmelee=slash|4|7|95|24|60|35|19\n"+
            (fallback?"projectile=shot|bullet|1|55|0|0|0|8|45|19|ignore-floor\n"
                      "move=shot|x|1|any|8|0:4\nrelease=shot|2|none|0|shot\n":""));
    };
    BattleMvpConfig config;config.declared_actors=3;config.teams_declared=true;
    config.teams={0,0,1};config.packages={idle,idle,melee(true)};
    config.spawn={BattlePoint{0,330},BattlePoint{700,600},BattlePoint{0,0}};
    NativeBattleMvp world;PortableRosterAi cpu;
    const auto start=[&](const IBattleMvpTerrain* terrain=nullptr){
        cpu.reset();check(world.start(config,terrain),"start direct dash fixture");
    };
    const auto visit=[&](bool dash){
        config.packages[2]=melee(dash);start();
        bool hit=false;
        for(unsigned n=0;n<40;++n){
            const auto ai=cpu.input(world,2);
            if(!n)check(ai.movement.l==dash&&!ai.attack.pressed,"medium gap uses only an allowed dash");
            check(!ai.attack.pressed||!world.actors()[2].dash_ticks,"attack waits for the movement burst");
            check(world.step_roster(std::array<GamePadState,3>{{{},{},ai.movement}},
                                   std::array<BorgBattleInputs,3>{{{},{},ai.attack}},false),"step direct approach");
            hit|=std::any_of(world.package_events().begin(),world.package_events().end(),[](const auto& e){
                return e.actor==2&&e.kind==BorgBattleEventKind::Hit&&e.recipient==0;
            });
        }
        return hit;
    };
    check(visit(true)&&!visit(false),"faster approach lands a real hit before the slow control can reach");
    config.packages[2]=melee(true);
    config.spawn[0]={0,250};start();
    check(!cpu.input(world,2).movement.l,"short gap keeps a walk reserve");
    config.spawn[0]={0,330};config.dash_duration=24;start();
    check(!cpu.input(world,2).movement.l,"the same gap refuses a longer declared burst");
    config.dash_duration=12;config.spawn[0]={0,560};
    config.packages[2]=melee(true,true);start();
    const auto ranged=cpu.input(world,2);
    check(ranged.attack.pressed==BorgBattleInputs::x&&!ranged.movement.l,"ready ranged X takes precedence over a dash");

    struct Stage final:IBattleMvpTerrain {
        std::array<StageBodyPartition,1> boxes{{{"tight-wall",43,100,95,230}}};
        std::span<const StageBodyPartition> body_partitions()const override{return boxes;}
        bool height(f32,f32,f32,f32* out)const override{if(!out)return false;*out=0;return true;}
    } stage;
    config.packages[2]=melee(true);start(&stage);
    const auto tight=cpu.input(world,2,&stage);
    check(!tight.movement.l&&tight.movement.stick_y>0,"walk if only the padded burst clips the wall");
}

void roster_cpu_ally_pass_cases(){
    struct Terrain final:IBattleMvpTerrain {
        std::array<StageBodyPartition,2> walls{{{"left",-180,-50,-50,120},{"right",50,-50,180,120}}};
        unsigned count=1;
        std::span<const StageBodyPartition> body_partitions()const override{return std::span(walls).first(count);}
        bool height(f32,f32,f32,f32* out)const override{if(!out)return false;*out=0;return true;}
    } terrain;
    const auto idle=definition("stationary-v1","health=400\n");
    const auto melee=definition("grounded-v1|8|13|14","health=400\nwalk=0:1\nair=0:2\ndash=0:3\n"
        "move=slash|b|10|grounded|14|0:4\nrange=slash|145\nmelee=slash|4|7|95|24|60|35|19\n");
    BattleMvpConfig config;config.declared_actors=3;config.teams_declared=true;
    config.teams={0,1,1};config.packages={idle,idle,melee};
    config.spawn={BattlePoint{0,330},BattlePoint{0,115},BattlePoint{0,0}};
    NativeBattleMvp world;PortableRosterAi cpu;
    const auto start=[&](const IBattleMvpTerrain* stage=nullptr){
        cpu.reset();check(world.start(config,stage)&&world.target_of(2)==0,"start close approach with an ally in front");
    };
    start();
    bool hit=false;
    for(unsigned n=0;n<95&&!hit;++n){
        const auto ai=cpu.input(world,2);
        if(!n)check(ai.movement.stick_x<0&&!ai.movement.l&&!ai.attack.pressed&&ai.aim_target==255,
                    "start a ground sidestep instead of dashing into the ally");
        check(world.step_roster(std::array<GamePadState,3>{{{},{},ai.movement}},
                               std::array<BorgBattleInputs,3>{{{},{},ai.attack}},false),"advance existing solid-body simulation");
        const auto& p=world.actors()[2].position;
        check(std::hypot(p[0],p[2]-115.f)>=2*config.actor_radius-.01f&&world.target_of(2)==0,
              "navigation neither crosses the ally's body nor changes the intended rival");
        for(const auto& event:world.package_events())if(event.actor==2&&event.kind==BorgBattleEventKind::Hit){
            check(event.recipient==0,"contact hits the actual enemy");hit=true;
        }
    }
    check(hit,"melee CPU walks around a parked ally and lands its move");
    start(&terrain);
    const auto around_wall=cpu.input(world,2,&terrain);
    check(around_wall.movement.stick_x>0&&!around_wall.movement.l,"use the other flank if the first crosses a partition");
    terrain.count=2;start(&terrain);
    const auto sealed=cpu.input(world,2,&terrain);
    check(!sealed.movement.stick_x&&!sealed.movement.stick_y&&!sealed.movement.l&&!sealed.attack.pressed,
          "wait when both flanks are blocked");

    config.packages[1]=definition("grounded-v1|0.4|11|0.4","health=400\nwalk=0:1\nair=0:2\ndash=0:3\ncruise-boost=200|18|4|2|7|4\n"
        "projectile=shot|bullet|1|55|0|0|0|8|45|19|ignore-floor\n"
        "move=parked|b|10|grounded|8|0:4\nrelease=parked|2|none|0|shot\n");
    start();
    const auto under=cpu.input(world,2);
    check(!under.movement.stick_x&&under.movement.l,"a cruiser above body height is not a planar blocker");
}

void roster_cpu_detour_ally_cases(){
    struct Terrain final:IBattleMvpTerrain {
        std::array<StageBodyPartition,1> boxes{{{"wall",-620,-6,620,6,true}}};
        std::span<const StageBodyPartition> body_partitions()const override{return boxes;}
        bool height(f32,f32,f32,f32* out)const override{if(!out)return false;*out=0;return true;}
    } terrain;
    const auto idle=definition("stationary-v1","health=500\n");
    const auto melee=definition("grounded-v1|8|13|14","health=500\nwalk=0:1\nair=0:2\ndash=0:3\n"
        "move=slash|b|10|grounded|14|0:4\nrange=slash|145\nmelee=slash|4|7|95|24|60|35|19\n");
    BattleMvpConfig config;config.declared_actors=3;config.teams_declared=true;
    config.teams={0,1,1};config.packages={idle,idle,melee};
    const BattlePoint target{500,170},from{300,-300};
    const auto route=battle_partition_route(terrain,from,target,config.arena_half,config.actor_radius);
    check(route.count>1&&route.points[0].x>620,"fixture has a genuine first routed leg");
    const auto direction=native_borg_direction(from,route.points[0]);
    const BattlePoint ally{from.x+125*direction.x,from.z+125*direction.z};
    config.spawn={target,ally,from};
    NativeBattleMvp world;PortableRosterAi cpu;
    check(world.start(config,&terrain),"start wall detour with a parked ally");
    bool passed_wall=false,hit=false;
    for(unsigned n=0;n<180&&!hit;++n){
        const auto ai=cpu.input(world,2,&terrain);
        if(!n){
            const auto& pad=ai.movement;
            check(!pad.l&&(pad.stick_x||pad.stick_y)&&std::abs(pad.stick_x*direction.x+pad.stick_y*direction.z)<2,
                  "sidestep the body blocking the first routed leg rather than dashing into it");
        }
        const auto before=world.actors()[2].position[2];
        check(world.step_roster(std::array<GamePadState,3>{{{},{},ai.movement}},
                               std::array<BorgBattleInputs,3>{{{},{},ai.attack}},false,&terrain),"advance native routed approach");
        const auto& pos=world.actors()[2].position;
        check(std::hypot(pos[0]-ally.x,pos[2]-ally.z)>=2*config.actor_radius-.01f,"ally remains solid on a detour");
        if(before<=0&&pos[2]>0){
            check(std::abs(pos[0])>620+config.actor_radius,"cross the wall's plane only outside its expanded footprint");
            passed_wall=true;
        }
        for(const auto& e:world.package_events())if(e.kind==BorgBattleEventKind::Hit&&e.actor==2){
            check(e.recipient==0,"contact still lands on the actual enemy");hit=true;
        }
    }
    check(passed_wall&&hit,"the CPU resumes the wall route after passing the ally and lands a melee hit");
}

void roster_cpu_follow_charge_cases(){
    struct Terrain final:IBattleMvpTerrain {
        std::array<StageBodyPartition,1> boxes{{{"charge-wall",-1800,100,1800,110}}};
        bool wall=true,step=false;
        std::span<const StageBodyPartition> body_partitions()const override{
            return wall?std::span<const StageBodyPartition>(boxes):std::span<const StageBodyPartition>{};
        }
        bool height(f32,f32 z,f32,f32* out)const override{if(!out)return false;*out=step&&z>100?10:0;return true;}
    } stage;
    const auto idle=definition("stationary-v1","health=400\n");
    const auto field=[&](bool follow=true,unsigned duration=20,std::string tail={}){
        return definition("grounded-v1|5|13|15","health=400\nwalk=0:1\nair=0:2\ndash=0:3\n"
            "move=charge-hit|x|10|grounded|28|0:4\nmove-charge=charge-hit|"+std::to_string(duration)+"\n"
            "field=charge-hit|3|sphere|168|168|168|30|0|50|90|150|250|150\n"
            "field-core-damage=charge-hit|enemies|165|9|6\n"+(follow?"field-follow=charge-hit|source\n":"")+tail);
    };
    BattleMvpConfig config;config.declared_actors=3;config.teams_declared=true;
    config.teams={0,1,1};config.packages={idle,idle,field()};
    config.spawn={BattlePoint{0,290},BattlePoint{700,600},BattlePoint{0,0}};
    NativeBattleMvp world;PortableRosterAi cpu;
    const auto start=[&](const IBattleMvpTerrain* terrain=nullptr){
        cpu.reset();check(world.start(config,terrain),"start charge approach fixture");
    };
    start();
    bool hit=false;
    for(unsigned n=0;n<12;++n){
        const auto ai=cpu.input(world,2);
        if(!n)check(ai.attack.pressed==BorgBattleInputs::x&&!ai.movement.l,"start an authored move-charge, not a regular dash");
        check(world.step_roster(std::array<GamePadState,3>{{{},{},ai.movement}},
                               std::array<BorgBattleInputs,3>{{{},{},ai.attack}},false),"advance genuine charge and field");
        hit|=std::any_of(world.package_events().begin(),world.package_events().end(),[](const auto& e){
            return e.actor==2&&e.kind==BorgBattleEventKind::Hit&&e.recipient==0;
        });
    }
    check(hit,"the charge carries the field into a real enemy hit");
    for(const auto& refused:{field(false),field(true,4),field(true,20,"range=charge-hit|230\n")}){
        config.packages[2]=refused;start();
        check(!cpu.input(world,2).attack.pressed,"do not extend a static field, short charge, or explicit entry range");
    }
    config.packages[2]=field();start(&stage);
    check(!cpu.input(world,2,&stage).attack.pressed,"projections do not cross a sealed partition");
    stage.wall=false;stage.step=true;start(&stage);
    check(!cpu.input(world,2,&stage).attack.pressed,"do not project a grounded field over a floor-height change");
    config.spawn[1]={0,125};start();
    check(!cpu.input(world,2).attack.pressed,"projected travel cannot pass through a solid ally");
}

void roster_cpu_field_height_cases(){
    struct Terrain final:IBattleMvpTerrain {
        std::array<StageBodyPartition,1> boxes{{{"remote-wall",1000,-6,1200,6,true}}};
        std::span<const StageBodyPartition> body_partitions()const override{return boxes;}
        bool height(f32,f32,f32,f32* out)const override{if(!out)return false;*out=0;return true;}
    } terrain;
    const auto idle=definition("stationary-v1","health=500\n");
    const std::string clips="health=500\nwalk=0:1\nair=0:2\ndash=0:3\n";
    const std::string shot="projectile=shot|bullet|1|55|0|0|0|8|45|19|ignore-floor\n";
    const std::string parked="move=parked|b|10|grounded|8|0:4\nrelease=parked|2|none|0|shot\n";
    const auto cruiser=definition("grounded-v1|0.4|13|0.4",clips+shot+parked+"cruise-boost=420|18|4|2|7|4\n");
    const auto field=[&](bool mixed=false){
        return definition("grounded-v1|8|13|14",clips+shot+
            "stock=cell|1|80\nmove=paid|b|10|grounded|14|0:4\n"
            "field=paid|3|sphere|168|168|168|14|0|50|90|150|250|150\n"
            "field-core-damage=paid|all|165|9|2\nfield-debit=paid|cell|1\n"+
            (mixed?"release=paid|2|none|0|shot\n":""));
    };
    BattleMvpConfig config;config.declared_actors=3;config.teams_declared=true;
    config.packages={cruiser,idle,field()};
    config.spawn={BattlePoint{0,0},BattlePoint{0,400},BattlePoint{0,-120}};
    config.teams={0,0,1};
    NativeBattleMvp world;PortableRosterAi cpu;
    const auto start=[&](const IBattleMvpTerrain* stage=nullptr){
        cpu.reset();
        check(world.start(config,stage)&&world.target_of(2)==0,"start field height fixture with the high rival nearest");
    };
    const auto simulate=[&](bool paid){
        start();unsigned recipients=0;
        for(unsigned n=0;n<12;++n){
            const auto ai=cpu.input(world,2);
            if(!n)check(bool(ai.attack.pressed)==paid&&ai.aim_target==255,"height check does not redirect aim");
            check(world.step_roster(std::array<GamePadState,3>{{{},{},ai.movement}},
                                   std::array<BorgBattleInputs,3>{{{},{},ai.attack}},false),"advance field height fixture");
            for(const auto& event:world.package_events())if(event.kind==BorgBattleEventKind::Hit&&event.actor==2){
                check(event.recipient<world.actor_count(),"field hits name a live roster slot");
                recipients|=1u<<event.recipient;
            }
        }
        check(world.package_action(2).stocks[0].current==(paid?0:1),"only an accepted placement pays the field's debit");
        return recipients;
    };
    check(!simulate(false),"field does not spend stock trying to reach the high cruiser");
    config.spawn[1]={135,-120};
    check(simulate(true)==2,"area still hits the lower rival without aiming at it");
    config.teams[1]=1;
    check(!simulate(false),"an allied body alone does not justify spending stock on an all-scope field");
    config.packages[2]=field(true);start();
    check(cpu.input(world,2).attack.pressed==BorgBattleInputs::b,"an independent projectile is not gated by field height");

    config.packages[0]=definition("grounded-v1|3|24|12",clips+shot+parked);
    config.packages[2]=field();start();
    for(unsigned n=0;n<12;++n){
        std::array<GamePadState,3> pads{};pads[0].b=!n;
        check(world.step_roster(pads,std::array<BorgBattleInputs,3>{},false),"ordinary hop for field planning comparison");
    }
    check(!world.actors()[0].grounded&&world.actors()[0].position[1]>165&&
          cpu.input(world,2).attack.pressed==BorgBattleInputs::b,"do not treat ordinary air travel as constant height");

    config.packages={cruiser,idle,field()};
    config.teams={0,0,1};config.local_aim_cycle=true;
    start(&terrain);
    const auto covered=cpu.input(world,2,&terrain);
    check(covered.aim_target==255&&covered.attack.pressed==BorgBattleInputs::b,"keep aim if the lower enemy is already in the field");
    config.spawn[1]={300,-120};start(&terrain);
    bool hit=false;
    for(unsigned n=0;n<70&&!hit;++n){
        const auto ai=cpu.input(world,2,&terrain);
        if(!n)check(ai.aim_target==1&&ai.movement.stick_x>0&&!ai.attack.pressed,"approach a lower enemy outside the field");
        const std::array<u8,1> hints{ai.aim_target};
        check(world.step_roster(std::array<GamePadState,3>{{{},{},ai.movement}},
            std::array<BorgBattleInputs,3>{{{},{},ai.attack}},false,&terrain,{},nullptr,nullptr,nullptr,{},
            {nullptr,0,ai.aim_target==255?std::span<const u8>{}:std::span<const u8>(hints)}),"advance native field pursuit");
        for(const auto& event:world.package_events())if(event.kind==BorgBattleEventKind::Hit&&event.actor==2){
            check(event.recipient==1,"field actually hits the lower rival");hit=true;
        }
    }
    check(hit,"field-only CPU closes the gap before paying and landing its field");
    config.packages[2]=field(true);start(&terrain);
    const auto mixed=cpu.input(world,2,&terrain);
    check(mixed.aim_target==255&&mixed.attack.pressed==BorgBattleInputs::b,"mixed field/projectile route does not change aim");
    config.packages[2]=field();
    terrain.boxes[0]={"sealed",70,-1800,76,1800,true};start(&terrain);
    const auto sealed=cpu.input(world,2,&terrain);
    check(sealed.aim_target==255&&!sealed.attack.pressed,"do not select a lower enemy across a sealed wall");
}

void roster_cpu_cruise_cases(){
    struct Terrain final:IBattleMvpTerrain {
        std::array<StageBodyPartition,1> boxes{{{"wall",-250,-6,250,6,true}}};
        std::span<const StageBodyPartition> body_partitions()const override{return boxes;}
        bool height(f32,f32,f32,f32* out)const override{if(!out)return false;*out=0;return true;}
    } terrain;
    const auto idle=definition("stationary-v1","health=400\n");
    const auto make=[](unsigned meter){
        return definition("grounded-v1|4|13|18","health=400\nwalk=0:1\nair=0:2\ndash=0:3\ncruise-boost=50|"+
            std::to_string(meter)+"|2|2|12|4\n"
            "move=wing|x|10|airborne|14|0:4\nfield=wing|2|sphere|170|170|170|14|0|40|90|150|250|150\n"
            "field-core-damage=wing|enemies|165|9|2\nfield-follow=wing|source\n");
    };
    BattleMvpConfig config;config.declared_actors=3;config.teams_declared=true;
    config.teams={0,0,1};config.packages={idle,idle,make(160)};
    config.spawn={BattlePoint{0,210},BattlePoint{155,400},BattlePoint{0,-380}};
    NativeBattleMvp world;PortableRosterAi cpu;
    const auto start=[&](const IBattleMvpTerrain* stage){cpu.reset();check(world.start(config,stage),"start CPU cruise fixture");};
    start(&terrain);
    bool detour_boost=false,crossed=false,field_hit=false;
    for(unsigned n=0;n<165&&!field_hit;++n){
        const auto ai=cpu.input(world,2,&terrain);
        const auto before=world.actors()[2].position;
        if(!n)check(!ai.movement.b&&!ai.attack.pressed&&ai.movement.stick_x,"turn onto detour before boosting");
        check(!ai.movement.l&&(!ai.movement.b||!ai.attack.pressed),"cruise uses boost rather than ground dash or a simultaneous attack");
        check(world.step_roster(std::array<GamePadState,3>{{{},{},ai.movement}},
                               std::array<BorgBattleInputs,3>{{{},{},ai.attack}},false,&terrain),"advance cruise detour");
        const auto& actor=world.actors()[2];
        check(!actor.grounded&&actor.position[1]==50,"a route keeps the existing terrain-relative cruise height");
        if(actor.position[2]>0&&before[2]<=0){
            check(std::abs(actor.position[0])>250+config.actor_radius,"cross around the wall, never above it");
            crossed=true;
        }
        detour_boost|=actor.package_boosting&&actor.package_boost_meter.value736<160&&actor.movement_speed>4;
        for(const auto& event:world.package_events())if(event.actor==2&&event.kind==BorgBattleEventKind::Hit&&event.recipient==0)
            field_hit=true;
    }
    check(detour_boost&&crossed&&field_hit,"a short-range cruiser boosts around the wall and lands its authored field");

    terrain.boxes[0].min_x=-1800;terrain.boxes[0].max_x=1800;start(&terrain);
    const auto sealed=cpu.input(world,2,&terrain);
    check(!sealed.movement.b&&!sealed.movement.l&&!sealed.attack.pressed,"sealed air route does not request boost or an attack");
    start(nullptr);
    check(cpu.input(world,2).movement.b,"clear, distant target permits the minimum boost commitment");
    config.packages[2]=make(3);start(nullptr);
    check(!cpu.input(world,2).movement.b,"CPU conserves a meter too small for the minimum commitment");
    config.packages[2]=make(160);config.spawn[2]={0,70};start(nullptr);
    const auto near=cpu.input(world,2);
    check(!near.movement.b&&near.attack.pressed==BorgBattleInputs::x,"in-range cruiser attacks instead of boosting");
}

void roster_cpu_cruise_extension_cases(){
    struct Terrain final:IBattleMvpTerrain {
        std::array<StageBodyPartition,1> boxes{{{"wall",-620,-6,620,6,true}}};
        bool step=false;
        std::span<const StageBodyPartition> body_partitions()const override{return boxes;}
        bool height(f32,f32 z,f32,f32* out)const override{if(!out)return false;*out=step&&z>30?10:0;return true;}
    } terrain;
    const auto idle=definition("stationary-v1","health=1000\n");
    const auto cruiser=definition("grounded-v1|0.4|13|15","health=1000\nwalk=0:1\nair=0:2\ndash=0:3\n"
        "cruise-boost=50|10000|1|2000|0|30\nmove=wing|x|10|airborne|28|0:4\n"
        "field=wing|3|sphere|240|240|240|30|0|40|190|90|220|120\n"
        "field-core-damage=wing|enemies|240|10|6\nfield-follow=wing|source\n");
    BattleMvpConfig config;config.declared_actors=3;config.teams_declared=true;config.local_aim_cycle=true;
    config.teams={0,0,1};config.packages={idle,idle,cruiser};
    config.spawn={BattlePoint{-240,300},BattlePoint{-240,650},BattlePoint{600,-300}};
    const auto route=battle_partition_route(terrain,config.spawn[2],config.spawn[0],config.arena_half,config.actor_radius);
    check(route.count>1&&std::hypot(route.points[0].x-600,route.points[0].z+300)<450,
          "the first corner alone is too close for this cruise minimum");
    NativeBattleMvp world;PortableRosterAi cpu;
    const auto start=[&](){cpu.reset();check(world.start(config,&terrain),"start cruise extension fixture");};
    const auto step=[&](const PortableRosterInput& ai){
        const std::array<u8,1> hints{ai.aim_target};
        check(world.step_roster(std::array<GamePadState,3>{{{},{},ai.movement}},
            std::array<BorgBattleInputs,3>{{{},{},ai.attack}},false,&terrain,{},nullptr,nullptr,nullptr,{},
            {nullptr,0,ai.aim_target==255?std::span<const u8>{}:std::span<const u8>(hints)}),"advance native cruise extension");
    };
    start();
    std::optional<unsigned> first_boost;
    GamePadState planned;
    bool passed_wall=false,hit=false;
    for(unsigned n=0;n<220&&!hit;++n){
        const auto ai=cpu.input(world,2,&terrain);
        if(!first_boost&&ai.movement.b){first_boost=n;planned=ai.movement;}
        if(first_boost&&n<*first_boost+30)
            check(ai.movement.b&&ai.movement.stick_x==planned.stick_x&&ai.movement.stick_y==planned.stick_y&&!ai.attack.pressed,
                  "hold only the checked movement input throughout the minimum boost");
        const auto before=world.actors()[2].position[2];
        step(ai);
        const auto& actor=world.actors()[2];
        check(actor.position[1]==50&&!actor.grounded,"same steady cruise height, not a wall climb");
        if(before<0&&actor.position[2]>=0){
            check(actor.position[0]>620+config.actor_radius&&first_boost&&n<*first_boost+30,
                  "the extended minimum burst clears the corner's expanded footprint");
            passed_wall=true;
        }
        for(const auto& e:world.package_events())
            hit|=e.actor==2&&e.kind==BorgBattleEventKind::Hit&&e.recipient==0;
    }
    check(first_boost&&*first_boost<40&&passed_wall&&hit,"a slow cruiser clears the corner and lands its field");

    const auto no_extension=[&](const char* reason){
        start();
        for(unsigned n=0;n<70;++n){
            const auto ai=cpu.input(world,2,&terrain);
            check(!ai.movement.b&&!ai.attack.pressed,reason);
            step(ai);
        }
    };
    terrain.step=true;
    no_extension("don't extend a boost across a floor-height change");
    terrain.step=false;config.teams[1]=1;config.spawn[1]={730,80};
    no_extension("don't extend a boost through a live ally beyond the corner");
}

void one_local_roster_cases(){
    struct Terrain final:IBattleMvpTerrain {
        std::array<StageBodyPartition,1> boxes{{{"remote",600,-6,700,6}}};
        std::span<const StageBodyPartition> body_partitions()const override{return boxes;}
        bool height(f32,f32,f32,f32* out)const override{if(!out)return false;*out=0;return true;}
    } terrain;
    const auto idle=definition("stationary-v1","health=200\n");
    const auto shooter=definition("grounded-v1|8|13|14","health=200\nwalk=0:1\nair=0:2\ndash=0:3\n"
        "projectile=shot|bullet|1|60|0|0|0|8|30|19|ignore-floor\n"
        "move=fire|b|10|any|6|0:4\nrelease=fire|2|none|0|shot\n");
    BattleMvpConfig config;config.declared_actors=4;config.teams_declared=true;config.local_aim_cycle=true;
    config.roster_local_players=1;config.teams={0,0,1,1};config.packages={idle,shooter,idle,idle};
    config.spawn={BattlePoint{-360,0},BattlePoint{0,0},BattlePoint{0,200},BattlePoint{0,440}};
    NativeBattleMvp world;
    check(world.start(config,&terrain)&&world.target_of(1)==2,"slot 1 is an allied CPU with an enemy target");
    PortableRosterAi cpu;
    const auto ai=cpu.input(world,1,&terrain);
    check(cpu.input(world,0,&terrain).attack==BorgBattleInputs{}&&ai.attack.pressed==BorgBattleInputs::b,
          "only the first slot is local; slot 1 emits its own CPU input");
    NativeBorgMovementFrame peer{};
    check(native_borg_movement_frame({0,1},&peer),"valid peer frame fixture");
    const auto tick=[&](BattleMvpLocalRosterInput control){
        std::array<GamePadState,4> pads{};pads[1]=ai.movement;
        std::array<BorgBattleInputs,4> inputs{};inputs[1]=ai.attack;
        return world.step_roster(pads,inputs,false,&terrain,{},nullptr,nullptr,nullptr,{},control);
    };
    check(!tick({&peer,0})&&!tick({nullptr,2})&&world.tick()==0,
          "a CPU slot cannot consume P2 camera or aim input");
    const std::array<u8,2> wrong_count{3,255};
    const std::array<u8,3> hints{3,255,255};
    check(!tick({nullptr,0,wrong_count})&&world.tick()==0,"CPU hint count includes slot 1 now");
    check(tick({nullptr,0,hints})&&world.aim_target_of(1)==3&&world.target_of(1)==2,
          "the first CPU aim hint belongs to slot 1 without replacing nearest contact");
    config.local_aim_cycle=false;
    check(!world.start(config,&terrain),"one-local layout requires explicit native roster scope");
}

void coop_roster_cases(){
    const auto fragile=definition("stationary-v1","health=19\n");
    const auto partner=definition("grounded-v1|8|13|14","health=19\nwalk=0:1\nair=0:2\ndash=0:3\n"
        "projectile=shot|beam|1|100|0|0|0|8|30|1|ignore-floor\n"
        "move=fire|b|10|any|4|0:4\nrelease=fire|2|none|0|shot\n");
    const auto enemy=definition("grounded-v1|8|13|14","health=200\nwalk=0:1\nair=0:2\ndash=0:3\n"
        "projectile=shot|beam|1|60|0|0|0|8|30|19|ignore-floor\n"
        "move=fire|b|10|any|4|0:4\nrelease=fire|2|none|0|shot\n");
    BattleMvpConfig config;config.declared_actors=3;config.teams_declared=true;config.local_aim_cycle=true;
    config.teams={0,0,1};config.packages={fragile,partner,enemy};
    config.spawn={BattlePoint{0,220},BattlePoint{0,420},BattlePoint{0,0}};
    NativeBattleMvp world;
    check(world.start(config)&&world.aim_target_of(0)==2&&world.aim_target_of(1)==2,
          "local allies aim at the CPU, not at each other");
    const auto opening=NativeBattleMvpView::roster_movement_frame(world);
    check(opening.forward.z<0,"co-op camera faces the other team, not the second local spawn");
    bool p1_ko=false,partner_hit=false;
    for(unsigned n=0;n<50&&world.result()==BattleMvpResult::Fighting;++n){
        std::array<BorgBattleInputs,3> inputs{};
        const auto press=[](BorgBattleInputs& input){input.pressed=input.held=BorgBattleInputs::b;};
        if(!n)press(inputs[1]);
        if(!n||n==22)press(inputs[2]);
        check(world.step_roster(std::array<GamePadState,3>{},inputs,false),"co-op battle tick");
        for(const auto& event:world.package_events())if(event.kind==BorgBattleEventKind::Hit){
            if(event.actor==1&&event.recipient==2)partner_hit=true;
            if(event.actor==2&&event.recipient==0){
                p1_ko=true;
                check(!world.actors()[0].health&&world.actors()[1].health==19&&world.result()==BattleMvpResult::Fighting,
                      "losing P1 does not end the team round or damage P2");
                check(NativeBattleMvpView::roster_movement_frame(world).forward==opening.forward,
                      "shared camera direction stays put when one local is defeated");
            }
        }
    }
    check(p1_ko&&partner_hit&&world.actors()[2].health==199&&
          !world.actors()[1].health&&world.result()==BattleMvpResult::SasukeWins,
          "partner attack passes the ally, and the CPU wins only after both locals are down");
    check(world.start(config)&&world.actors()[0].health==19&&world.actors()[1].health==19&&
          world.aim_target_of(0)==2&&world.aim_target_of(1)==2,"co-op restart restores both local fighters");
}

// Smoke-test fixture writer, never invoked by the game. Require an explicit
// owned empty root so this tool cannot overwrite a user's saved Force.
int seed(const std::filesystem::path& root){
    check(std::filesystem::is_directory(root)&&std::filesystem::is_empty(root),"fixture root must be owned and empty");
    std::string error;
    auto disk=create_account_save_storage(root.string(),false,&error);
    check(bool(disk),error.c_str());
    AccountSaveRepository repo(*disk);
    AccountSaveEntry account;
    check(repo.create_account(1,"Challenge fixture",&account)==AccountResult::Ok,"create isolated fixture account");
    for(u64 i=1;i<=4;++i){
        AccountSaveEntry entry;
        check(repo.create_save(account,i,"Force "+std::to_string(i),&entry)==AccountResult::Ok,"create isolated fixture save");
        std::unique_ptr<AccountSaveBackend> backend;
        check(repo.open(entry,&backend)==AccountResult::Ok,"select fixture save");
        PackageBorgSaveStore store(*backend);
        PackageBorgRoster roster;
        check(store.load(&roster)==PackageBorgLoad::Missing,"new fixture save has no previous package Force");
        roster.owned.push_back({11,{"example:flight-stick","Flight Stick"},i==4,
            i==3?std::optional<SavedPackageVariant>{}:SavedPackageVariant{"normal","Normal"}});
        roster.placements.push_back({11,0,7});
        if(i==2){
            roster.owned.push_back({12,{"example:jump-stick","Jump Stick"},false,SavedPackageVariant{"normal","Normal"}});
            roster.placements.push_back({12,0,19});
        }
        check(store.write(roster),"seed only the disposable package Force");
        repo.close();
    }
    return 0;
}

// Exercise the real host adapter with generated content, independently of
// the CLI. Production reuses the existing package battle loop.
int run_adapter(const std::string& save_root){
    const auto borgs=scan_borg_packages("borgs");
    PortableChallengeLaunch launch;
    const auto loaded=load_portable_challenge_launch({"example:trials","round-one",save_root,{1,1},{0}},borgs,{},&launch);
    check(bool(loaded),loaded.reason.c_str());
    headless::Bundle backends;
    const auto io=backends.get();
    RenderEngine render(*io.render);
    std::string error;
    check(render.initialize({640,480,false,true,"Challenge adapter test"},io.window,&error),error.c_str());
    NativeBattleMvpView view;
    PortableBattleHost adapter(borgs,*launch.saved_force,view,render);
    const auto accepted=adapter.admit(*launch.run);
    check(bool(accepted),accepted.reason.c_str());
    const PortableBattleBorg expected_player{"example:flight-stick","normal"},expected_enemy{"example:jump-stick","normal"};
    check(adapter.accepted()&&adapter.accepted()->players().size()==1&&adapter.accepted()->players()[0]==expected_player&&
        adapter.accepted()->request.enemies.size()==1&&adapter.accepted()->request.enemies[0]==expected_enemy&&
        view.stage_id()=="example:workshop","accepted exact stage and ordered participants");
    check(!adapter.finish(*launch.run,{PortableDuelEnd::Win,0}),"adapter cannot advance at tick zero");
    NativeBattleMvp world;
    check(world.start(adapter.config(),&view),"admit two real actors to the exact stage");
    check(view.update(world,&error),error.c_str());
    struct Origins final:IBorgBattleOrigins {
        explicit Origins(const NativeBattleMvpView& v):view(v){}
        const NativeBattleMvpView& view;
        bool socket(unsigned owner,const BattleMvpActor& actor,u32 tick,u8 move,u16 frame,const BorgBattleSocket& spec,
                    BattleMvpVec* out)const override{
            return view.package_socket(owner,actor,tick,move,frame,spec,nullptr,out);
        }
    } origins(view);
    auto end=PortableDuelEnd::Fighting;
    const auto limit=adapter.accepted()->request.tick_limit;
    for(unsigned frame=0;frame<300&&end==PortableDuelEnd::Fighting;++frame){
        const auto ai=portable_duel_enemy_input(world);
        check(world.step({GamePadState{},ai.movement},false,&view,nullptr,nullptr,nullptr,{{{},ai.attack}},&origins),
            "advance authored world with the shared socket sampler");
        check(view.update(world,&error),error.c_str());
        render.begin_frame({frame,.15f,.21f,.3f});
        render.reset_arena();
        const auto draws=view.draw(render,world,640,480,&error);
        check(render.end_frame()&&draws,error.c_str());
        end=portable_duel_end(world,limit);
    }
    const auto reported=adapter.finish(*launch.run,{end,world.tick()});
    check(end==PortableDuelEnd::Loss&&reported&&world.hits()>0&&!launch.run->completed_count(),"report correlated real AI defeat");
    check(!adapter.finish(*launch.run,{end,world.tick()}),"result cannot replay");
    render.shutdown();
    return 0;
}
} // namespace

int main(int argc,char** argv){
    if(argc==2)return seed(argv[1]);
    if(argc==3&&std::string_view(argv[1])=="--run-adapter")return run_adapter(argv[2]);
    if(argc!=1)return 2;
    saved_force_cases();
    duel_cases();
    roster_control_and_camera_cases();
    roster_cpu_attack_cases();
    roster_cpu_height_cases();
    roster_cpu_height_rival_cases();
    roster_cpu_stock_cases();
    roster_aim_cases();
    roster_cpu_aim_cases();
    roster_cpu_detour_dash_cases();
    roster_cpu_route_cost_cases();
    roster_cpu_corner_melee_cases();
    roster_cpu_body_turn_cases();
    roster_cpu_direct_dash_cases();
    roster_cpu_ally_pass_cases();
    roster_cpu_detour_ally_cases();
    roster_cpu_follow_charge_cases();
    roster_cpu_field_height_cases();
    roster_cpu_cruise_cases();
    roster_cpu_cruise_extension_cases();
    one_local_roster_cases();
    coop_roster_cases();
}
