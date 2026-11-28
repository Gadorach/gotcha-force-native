#include <cstdio>
#include <cstdlib>
#include <tuple>
#include "engine/gameplay/native_battle_mvp.hpp"

namespace {
using namespace gf;

void check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"KO Force: %s\n",why);std::abort();}
}

constexpr std::string_view rules=R"(GFBORG-GAMEPLAY 1
variant=normal
health=60
movement=grounded-v1|5|13|15
rig-prefix=visual/body.prefix
pose-profile=visual/body.pose
body-record=0
idle=0:0
walk=0:1
air=0:24
dash=0:2
move=opening-slash|b|10|grounded|24|1:1
range=opening-slash|150
melee=opening-slash|9|14|88|38|52|44|12
)";

std::shared_ptr<BorgGameplayProfile> package(unsigned hp,unsigned cost){
    auto p=std::make_shared<BorgGameplayProfile>();
    auto text=std::string(rules);
    if(cost)text+="ko-force-cost="+std::to_string(cost)+"\n";
    check(borg_read_gameplay_profile(text,p.get(),nullptr),"valid portable package");
    p->health=u16(hp);
    return p;
}

BattleMvpConfig duel(unsigned hp){
    BattleMvpConfig cfg;
    cfg.packages={package(60,80),package(hp,90)};
    cfg.spawn={BattlePoint{0,0},BattlePoint{0,100}};
    return cfg;
}

void step(NativeBattleMvp& world,bool press){
    const std::array<GamePadState,kBattleMvpMaxActors> pads{};
    std::array<BorgBattleInputs,kBattleMvpMaxActors> input{};
    if(press)input[0].pressed=input[0].held=BorgBattleInputs::b;
    check(world.step_roster(pads,input,false),"native dispatcher refused");
}

using Event=std::tuple<BorgBattleEventKind,u32,u32,u8,u8,u16>;
std::vector<Event> run_duel(unsigned hp,unsigned pool,bool enable){
    auto cfg=duel(hp);
    if(enable)cfg.ko_force_pools=BattleKoForcePools{100,u16(pool)};
    NativeBattleMvp world;
    check(world.start(cfg),"admit both normal costs");
    std::vector<Event> events;
    u32 hit_tick=0;
    const unsigned removed=hp==1?1:12;
    for(unsigned n=1;n<=24&&world.result()==BattleMvpResult::Fighting;++n){
        step(world,n==8);
        for(const auto& e:world.package_events()){
            events.emplace_back(e.kind,e.tick,e.object,e.actor,e.recipient,e.value);
            if(e.kind==BorgBattleEventKind::Hit){
                check(!hit_tick&&e.actor==0&&e.recipient==1&&e.value==removed,"one real admitted HP write");
                hit_tick=e.tick;
            }
        }
        if(!hit_tick&&enable)check(*world.ko_force_remaining()==*cfg.ko_force_pools,"no pre-hit debit");
    }
    check(hit_tick==17&&world.actor_count()==2&&world.actors()[1].health==hp-removed&&
          world.actors()[0].damage_credited==removed&&world.actors()[1].damage_taken==removed&&
          world.result()==(hp==1?BattleMvpResult::GRedWins:BattleMvpResult::Fighting),
          "unchanged damage, credit and authored round result");
    const auto& spent=world.ko_force_debits();
    check(!spent[0]&&!spent[2],"no cross-seat debit");
    if(!enable){
        check(!world.ko_force_remaining()&&!spent[1],"a cost alone is dormant");
    }else if(hp!=1){
        check(*world.ko_force_remaining()==*cfg.ko_force_pools&&!spent[1],"nonlethal does not debit");
    }else{
        const unsigned debit=std::min(pool,90u);
        check(spent[1]&&spent[1]->tick==hit_tick&&spent[1]->cost==90&&spent[1]->debited()==debit&&
              spent[1]->before==pool&&spent[1]->after==pool-debit&&spent[1]->team==1&&spent[1]->source==0&&
              spent[1]->exhausted()==(pool==debit)&&(*world.ko_force_remaining())[0]==100&&
              (*world.ko_force_remaining())[1]==pool-debit,
              "full or saturated KO cost on the victim's team only");
        const auto tick=world.tick();
        step(world,true);
        check(world.tick()==tick&&spent[1]->tick==hit_tick&&(*world.ko_force_remaining())[1]==pool-debit,
              "later input does not debit the dead seat again");
        check(world.start(cfg)&&!world.ko_force_debits()[1]&&*world.ko_force_remaining()==*cfg.ko_force_pools,
              "an explicit new round restores its declared Force budget");
    }
    return events;
}

void roster(unsigned pool){
    auto cfg=duel(1);
    cfg.declared_actors=3;cfg.teams_declared=true;cfg.teams={0,1,1};
    cfg.actor_radius=30;
    cfg.spawn={BattlePoint{0,0},BattlePoint{-32,100},BattlePoint{32,104}};
    cfg.packages[2]=package(1,70);
    cfg.ko_force_pools=BattleKoForcePools{200,u16(pool)};
    NativeBattleMvp world;
    check(world.start(cfg),"three loaded authored slots");
    bool first=false;
    unsigned hits=0;
    for(unsigned n=1;n<=80&&world.result()==BattleMvpResult::Fighting;++n){
        step(world,n==8||n==40);
        for(const auto& e:world.package_events())hits+=e.kind==BorgBattleEventKind::Hit;
        if(!first&&hits){
            first=true;
            check(hits==1&&world.result()==BattleMvpResult::Fighting&&world.actor_count()==3,
                  "Force exhaustion alone may not end the authored round");
            const unsigned defeated=world.ko_force_debits()[1]?1:2;
            const auto& debit=*world.ko_force_debits()[defeated];
            check(debit.cost==(defeated==1?90:70)&&debit.exhausted()==(pool==40)&&world.actors()[3-defeated].health==1,
                  "first debit uses the exact defeated seat's cost");
        }
    }
    const auto& done=world.ko_force_debits();
    check(hits==2&&done[1]&&done[2]&&done[1]->cost==90&&done[2]->cost==70&&
          unsigned(done[1]->debited())+done[2]->debited()==std::min(pool,160u)&&
          *world.ko_force_remaining()==BattleKoForcePools{200,u16(pool-std::min(pool,160u))}&&
          world.result()==BattleMvpResult::GRedWins,
          "separate defeats debit the shared pool without underflow or replacement");
    if(pool==40)check(done[1]->exhausted()!=done[2]->exhausted(),"crossing zero is recorded once");
}

void ally_is_not_admitted(){
    auto cfg=duel(1);
    cfg.declared_actors=3;cfg.teams_declared=true;cfg.teams={0,0,1};cfg.actor_radius=30;
    cfg.spawn={BattlePoint{0,0},BattlePoint{-32,100},BattlePoint{32,104}};
    cfg.packages[2]=package(60,70);
    cfg.ko_force_pools=BattleKoForcePools{90,90};
    NativeBattleMvp world;
    check(world.start(cfg),"admit authored ally control");
    for(unsigned n=1;n<=24;++n)step(world,n==8);
    check(world.actors()[1].health==1&&world.actors()[2].health==48&&
          *world.ko_force_remaining()==*cfg.ko_force_pools&&
          !world.ko_force_debits()[1]&&!world.ko_force_debits()[2],"cost opt-in does not admit enemy-only melee against allies");
}

void refuse(){
    BattleKoForcePools parsed{19,20};
    for(auto s:{"","7","1,","0,1","1,0","01,1","1, 2","+1,2","1,2,3","32768,1","999999999,1"})
        check(!battle_read_ko_force_pools(s,&parsed)&&parsed==BattleKoForcePools{19,20},"pool parser must fail closed");
    check(battle_read_ko_force_pools("1,32767",&parsed)&&parsed==BattleKoForcePools{1,32767},"pool parser limits");
    for(auto bad:{"ko-force-cost=0\n","ko-force-cost=32768\n","ko-force-cost=1\nko-force-cost=2\n",
                  "ko-force-cost=90\ndamage-kind=1\n"}){
        BorgGameplayProfile p;
        check(!borg_read_gameplay_profile(std::string(rules)+bad,&p,nullptr),"invalid KO cost row parsed");
    }
    auto alternate=std::string(rules);
    alternate.replace(alternate.find("variant=normal"),14,"variant=alternate");
    BorgGameplayProfile out;
    check(!borg_read_gameplay_profile(alternate+"ko-force-cost=90\n",&out,nullptr),"alternate named profile refused");
    NativeBattleMvp world;
    for(unsigned i=0;i<7;++i){
        auto cfg=duel(1);
        cfg.ko_force_pools=BattleKoForcePools{90,90};
        if(i==0)cfg.packages[1]=package(1,0);
        if(i==1)cfg.team_force[0]=90;
        if(i==2)cfg.force_policy[1].enabled=true;
        if(i==3)(*cfg.ko_force_pools)[1]=0;
        if(i==4)cfg.team_ledger[0].flag109=true;
        if(i==5){cfg.teams_declared=true;cfg.teams={0,0};}
        if(i==6)cfg.actor_flag1ea[1]=true;
        check(!world.start(cfg)&&!world.ready(),"invalid or mixed accounting scope admitted");
    }
}
} // namespace

int main(){
    refuse();
    check(run_duel(60,90,true)==run_duel(60,90,false),"nonlethal authored events unchanged");
    const auto baseline=run_duel(1,90,false);
    for(unsigned pool:{180u,90u,40u})
        check(run_duel(1,pool,true)==baseline,"lethal authored events unchanged");
    roster(180);roster(40);ally_is_not_admitted();
    std::puts("Native KO Force matches scoped debit; damage, admission and round results unchanged");
}
