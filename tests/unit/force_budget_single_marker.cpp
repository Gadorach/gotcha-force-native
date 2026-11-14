#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/force_budget_band.hpp"
#include "engine/gameplay/force_inventory.hpp"
#include "tests/unit/force_inventory_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"single budget marker: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;check(!force_budget_marker_slot(-1)&&!force_budget_marker_slot(31),"unlimited/invalid prefix has no marker");
    for(s32 prefix=0;prefix<=30;++prefix){const auto marker=force_budget_marker_slot(prefix);unsigned draws=0;for(unsigned slot=0;slot<30;++slot)if(marker&&slot==*marker)++draws;check(draws==1&&*marker==unsigned(prefix==30?0:prefix),"exactly one position for every limited prefix");}
    const auto lib=force_inventory_fixture::catalog();ForceInventory owner;check(owner.create(lib),"initial force");s32 prefix=0;check(owner.budget_prefix(0,350,ForceCostMode::Full,&prefix)&&prefix==30&&force_budget_marker_slot(prefix)==0,"unreached limit points after Borg30");
    check(owner.place(0,1,1)==ForceChange::Applied&&owner.budget_prefix(0,350,ForceCostMode::Full,&prefix)&&prefix==2&&force_budget_marker_slot(prefix)==2,"added Borg reaches equality and moves the single mark after Borg2");
    check(owner.place(0,2,2)==ForceChange::Applied&&owner.budget_prefix(0,350,ForceCostMode::Full,&prefix)&&prefix==2&&force_budget_marker_slot(prefix)==2&&owner.state()->forces[0].count==3,"additional excess Borg stays stored without another mark");
    std::printf("single budget marker: %u owned checks PASS\n",checks);
}
