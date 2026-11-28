#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/native_battle_mvp.hpp"

namespace {
using namespace gf;

const std::string shooter=R"(GFBORG-GAMEPLAY 1
variant=normal
movement=grounded-v1|8|13|14
health=180
rig-prefix=visual/prefix
pose-profile=visual/pose
idle=0:0
walk=0:1
air=0:2
dash=0:3
stock=primary|4|180
projectile=bolt|bullet|1|32|0|0|0|8|24|17|ignore-floor
move=fire|b|10|grounded|3|0:4
release=fire|1|primary|1|bolt
)";
constexpr std::string_view receiver=R"(GFBORG-GAMEPLAY 1
variant=normal
movement=stationary-v1
health=180
rig-prefix=visual/prefix
pose-profile=visual/pose
idle=0:0
)";
const std::string hold="actor-contact=bolt|stop-and-hold-slot|3\n";

void check(bool ok,const char* message){
    if(!ok){std::fprintf(stderr,"projectile contact tail: %s\n",message);std::abort();}
}
std::shared_ptr<const BorgGameplayProfile> load(std::string_view text){
    BorgGameplayProfile profile;
    std::string error;
    if(!borg_read_gameplay_profile(text,&profile,&error)){std::fprintf(stderr,"%s\n",error.c_str());std::abort();}
    return std::make_shared<const BorgGameplayProfile>(std::move(profile));
}
struct Pulse {
    u32 id=0,emitted=0,hit=0,closed=0,retired=0;
    unsigned slot=0;
};
void refusal_cases(){
    BorgGameplayProfile p;
    const auto rules=shooter+hold;
    for(const auto& bad:std::array{
        shooter+"actor-contact=bolt|stop-and-hold-slot|0\n",
        shooter+"actor-contact=bolt|stop-and-hold-slot|9\n",
        shooter+"actor-contact=other|stop-and-hold-slot|3\n",
        rules+hold,
        rules+"actor-contact=bolt|continue-once-per-actor\n",
        rules+"actor-contact=bolt|ignore\n",
        rules+"blast=bolt|actor|24|19|opponent\n",
        rules+"move=held|x|10|grounded|6|0:4\nheld-fire=held|primary|1|bolt|2|1|1|1\n",
        rules+"projectile=seed|bullet|1|20|0|0|0|8|24|5|stop-on-stage\n"
              "flight-children=seed|bolt|2|2|1\nmove=seed|x|20|grounded|6|0:5\nrelease=seed|2|none|0|seed\n",
    })check(!borg_read_gameplay_profile(bad,&p,nullptr),"conflicting, ambiguous or indirect policy accepted");
    p=*load(rules);
    p.projectiles[0].stock_return=BorgProjectileReturn::Actor;
    check(!borg_gameplay_profile_valid(p),"stock return must not be silently moved");
    p=*load(rules);
    p.moves.clear();
    check(!borg_gameplay_profile_valid(p),"unreachable declaration accepted");
}

void run_case(bool delayed,bool refused){
    auto rules=shooter+(delayed?hold:"");
    if(refused)rules+="origin=bolt|0|0|0|0|target-height\n"; // no resolver supplied
    BattleMvpConfig config;
    config.declared_actors=3;config.teams_declared=true;config.teams={0,1,1};
    const auto body=load(receiver);
    config.packages={load(rules),body,body};
    config.spawn={BattlePoint{0,0},BattlePoint{0,145},BattlePoint{0,255}};
    NativeBattleMvp world;
    check(world.start(config),"battle start");
    std::vector<Pulse> pulses;
    unsigned debits=0,refusals=0,socket_refusals=0;
    for(unsigned step=0;step<17;++step){
        // The second launch is due on the first shot's return visit. The
        // third launch is late enough to reuse the old slot.
        std::array<GamePadState,kBattleMvpMaxActors> pads{};
        std::array<BorgBattleInputs,kBattleMvpMaxActors> inputs{};
        if(step==0||step==4||step==9)inputs[0].pressed=BorgBattleInputs::b;
        check(world.step_roster(pads,inputs,false),"simulation step");
        const auto& pool=world.projectiles();
        for(const auto& e:world.package_events()){
            if(e.actor!=0)continue;
            debits+=e.kind==BorgBattleEventKind::StockSpent;
            refusals+=e.kind==BorgBattleEventKind::EmitRefused;
            socket_refusals+=e.kind==BorgBattleEventKind::SocketRefused;
            if(e.kind==BorgBattleEventKind::Emitted){
                const auto slot=std::find_if(pool.begin(),pool.end(),[&](auto& s){return s.id==e.object;});
                check(slot!=pool.end(),"emitted object in the real pool");
                pulses.push_back({.id=e.object,.emitted=e.tick,.slot=unsigned(slot-pool.begin())});
            }
            if(e.kind!=BorgBattleEventKind::Hit&&e.kind!=BorgBattleEventKind::ProjectileContactClosed&&
               e.kind!=BorgBattleEventKind::Retired)continue;
            auto pulse=std::find_if(pulses.begin(),pulses.end(),[&](auto& p){return p.id==e.object;});
            check(pulse!=pulses.end(),"contact event must refer to a live allocation identity");
            if(e.kind==BorgBattleEventKind::Hit){
                check(!pulse->hit&&e.recipient==1&&e.value==17,"one HP write on the nearer body");
                pulse->hit=e.tick;
            }else if(e.kind==BorgBattleEventKind::ProjectileContactClosed){
                check(delayed&&!pulse->closed&&pulse->hit==e.tick&&e.recipient==1&&e.value==3,
                      "closed contact does not imply a free slot");
                pulse->closed=e.tick;
                const auto& held=pool[pulse->slot];
                check(!held.active&&!held.slot_free()&&held.package_contact_return_left==3,
                      "held slot is neither active nor allocatable");
            }else{
                check(!pulse->retired&&pulse->hit&&pool[pulse->slot].slot_free(),"return frees only the matched slot");
                pulse->retired=e.tick;
            }
        }
    }
    check(debits==3,"authored debit unchanged");
    check(world.actors()[2].health==180,"no contact on second recipient of an ordinary shot");
    if(refused){
        check(pulses.empty()&&refusals==3&&socket_refusals==3&&world.actors()[1].health==180,
              "refused emission never creates a contact or held slot");
        return;
    }
    check(pulses.size()==3&&world.actors()[1].health==129&&!refusals,"all three distinct releases resolved");
    check(world.actors()[0].damage_credited==51&&world.actors()[1].damage_taken==51,
          "delayed slot return never delays or duplicates the positive damage ledger");
    check(pulses[0].id!=pulses[1].id&&pulses[0].id!=pulses[2].id,"reused slots do not carry old allocation IDs");
    for(const auto& p:pulses)
        check(p.hit&&p.retired==p.hit+(delayed?3:0)&&p.closed==(delayed?p.hit:0),"exact return visit");
    check(pulses[1].emitted==pulses[0].hit+3&&pulses[0].slot==pulses[2].slot&&
          (pulses[0].slot!=pulses[1].slot)==delayed,
          "held slot cannot be reused by an emission on its return visit");
    check(std::all_of(world.projectiles().begin(),world.projectiles().end(),[](auto& shot){return shot.slot_free();}),
          "pool is drained");
}
} // namespace

int main(){
    refusal_cases();
    run_case(false,false);
    run_case(true,false);
    run_case(true,true);
    std::puts("Native ordinary contact custody, slot reuse and publication refusal passed");
}
