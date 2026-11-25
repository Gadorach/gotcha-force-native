#include <cstdio>
#include <cstdlib>
#include <limits>
#include "engine/gameplay/native_battle_mvp.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"native battle MVP: %s\n",why);std::abort();}}
struct Terrain:gf::IBattleMvpTerrain {
    bool missing=false;float wall=10000;
    bool height(float x,float,float,float* out)const override{if(missing)return false;*out=x>wall?500.f:0.f;return true;}
};}
int main(){
    using namespace gf;std::array<GamePadState,2> pads{};NativeBattleMvp duel;
    check(!duel.step(pads),"no gameplay before spawn");
    check(duel.start()&&duel.ready(),"spawn initialized duel");
    check(duel.actors()[0].model==0x615&&duel.actors()[1].model==0x000a,"fixed normal G-Red and Sasuke identities");
    check(duel.actors()[0].health==100&&duel.actors()[1].health==100,"real independent health state");
    const auto spawn=duel.actors()[0].position;
    pads[0].stick_x=127;check(duel.step(pads,false)&&duel.actors()[0].position[0]>spawn[0]&&duel.actors()[0].action==BattleMvpAction::Move,"movement owns position/action state");
    pads={};pads[0].b=true;check(duel.step(pads,false)&&!duel.actors()[0].grounded&&duel.actors()[0].position[1]>0,"jump starts vertical motion");
    pads={};for(unsigned i=0;i<100;++i)check(duel.step(pads,false),"jump tick");
    check(duel.actors()[0].grounded&&duel.actors()[0].position[1]==0,"gravity lands on terrain");
    const auto tick=duel.tick();pads[0].start=true;check(duel.step(pads,false)&&duel.paused()&&duel.tick()==tick,"pause without physics tick");
    check(duel.step(pads,false)&&duel.paused(),"held pause does not toggle twice");pads={};check(duel.step(pads,false),"release pause");pads[0].start=true;check(duel.step(pads,false)&&!duel.paused(),"unpause");pads={};
    check(duel.start(),"fresh weapon test");
    pads[0].a=true;check(duel.step(pads,false)&&duel.shots_spawned()==1,"G-Red spawns a beam entity");
    bool beam=false;for(const auto& p:duel.projectiles())if(p.active)beam|=p.weapon==BattleMvpWeapon::EnergyBeam&&p.owner==0&&p.id!=0;check(beam,"beam has owner/id/velocity/lifetime");
    for(unsigned i=0;i<240&&duel.result()==BattleMvpResult::Fighting;++i)check(duel.step(pads,false),"shooting tick");
    check(duel.result()==BattleMvpResult::GRedWins&&duel.actors()[1].health==0&&duel.hits()>0,"swept shots deal damage and end duel");
    const auto stopped=duel.tick();check(duel.step(pads,false)&&duel.tick()==stopped,"won duel does not keep damaging actors");
    pads={};pads[0].y=true;check(duel.step(pads,false)&&duel.result()==BattleMvpResult::Fighting&&duel.actors()[1].health==100,"restart initializes a new playable round");
    check(duel.step(pads,false)&&duel.tick()==1,"held restart does not reset every tick");
    check(duel.start(),"fresh Sasuke test");pads={};pads[1].a=true;pads[1].x=true;check(duel.step(pads,false)&&duel.shots_spawned()==4,"Sasuke spawns three shuriken and a bullet");
    unsigned shuriken=0,bullets=0;for(const auto& p:duel.projectiles())if(p.active){shuriken+=p.weapon==BattleMvpWeapon::Shuriken;bullets+=p.weapon==BattleMvpWeapon::Bullet;check(p.owner==1,"second actor owns its weapons");}
    check(shuriken==3&&bullets==1,"weapon kinds remain distinct runtime entities");
    check(duel.start(),"fresh bomb test");pads={};pads[0].x=true;std::array<std::optional<BattleMvpVec>,2> muzzle{};
    muzzle[0]=BattleMvpVec{duel.actors()[1].position[0],-20,duel.actors()[1].position[2]};
    check(duel.step(pads,false,nullptr,&muzzle),"pose-derived muzzle seam accepted");pads={};
    for(unsigned i=0;i<75;++i)check(duel.step(pads,false),"bomb fuse/gravity tick");
    check(duel.actors()[1].health==65,"bomb fuse causes radial damage");
    check(duel.actors()[0].health==100,"initial MVP weapons exclude their owner");
    check(duel.start(),"fresh AI test");const auto ai_start=duel.actors()[1].position;pads={};
    for(unsigned i=0;i<120;++i)check(duel.step(pads,true),"AI tick");
    check(duel.actors()[1].position!=ai_start&&duel.shots_spawned()>0,"basic opponent moves and attacks");
    Terrain terrain;check(duel.start({},&terrain),"ground adapter spawn");const auto before=duel.tick();terrain.missing=true;
    check(!duel.step(pads,false,&terrain)&&duel.tick()==before,"missing terrain is not guessed floor0");
    terrain.missing=false;terrain.wall=duel.actors()[0].position[0]+1;pads[0].stick_x=127;
    check(duel.step(pads,false,&terrain)&&duel.actors()[0].position[0]<terrain.wall,"excessive terrain step blocks movement");
    BattleMvpConfig close;close.spawn={BattlePoint{-100,0},BattlePoint{100,0}};check(duel.start(close),"body collision test spawn");pads={};pads[0].stick_x=127;pads[1].stick_x=-128;
    for(unsigned i=0;i<100;++i)check(duel.step(pads,false),"body movement tick");
    check(duel.actors()[1].position[0]-duel.actors()[0].position[0]>=2*close.actor_radius-.01f,"Borg bodies cannot walk through each other");
    BattleMvpConfig bad;bad.gravity=0;check(!duel.start(bad),"invalid prototype tuning refuses");
    muzzle[0]=BattleMvpVec{std::numeric_limits<f32>::quiet_NaN(),0,0};check(!duel.step({},false,nullptr,&muzzle),"invalid weapon origin refuses");
    check(battle_health_cells(2000,2000,20)==20&&battle_health_cells(0,2000,20)==0,
          "health bar fills and empties with its own maximum");
    check(battle_health_cells(1000,2000,20)==10,"health bar halves at half health");
    check(battle_health_cells(500,0,20)==0,"zero maximum reports an empty bar");
    check(battle_health_cells(9999,100,20)==20,"health bar never overfills");
    const auto idle_gauge=battle_charge_gauge(0,10,40);
    check(idle_gauge.total==0&&idle_gauge.elapsed==0,"no charge window reports no gauge");
    const auto mid_gauge=battle_charge_gauge(20,10,22);
    check(mid_gauge.total==20&&mid_gauge.elapsed==12,"charge gauge counts from the started tick");
    const auto full_gauge=battle_charge_gauge(20,10,99);
    check(full_gauge.elapsed==20,"charge gauge clamps at its total");
    const auto early_gauge=battle_charge_gauge(20,10,4);
    check(early_gauge.elapsed==0,"charge gauge does not run backwards");
    std::printf("native MVP duel movement/jump/AI/beam/bullet/shuriken/bomb/damage/restart/charge gauge: %u checks PASS\n",checks);
}
