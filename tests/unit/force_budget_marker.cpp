#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/force_budget_band.hpp"
#include "engine/gameplay/force_selection.hpp"
#include "tests/unit/force_inventory_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"budget marker: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;ForceEditorArt art;for(unsigned i=0;i<30;++i){art.offsets[i]=f32(i*4);art.angles[i]=s16(i*200);}ForceBudgetMarkerPose p;
    check(force_budget_marker_pose(art,0,0,&p)&&p.offset==58&&p.angle==2900,"boundary before first slot crosses table end");check(force_budget_marker_pose(art,2,0,&p)&&p.offset==6&&p.angle==300,"boundary after two admitted slots");check(force_budget_marker_pose(art,2,1,&p)&&p.offset==2&&p.angle==100,"turn shifts midpoint lookup");
    art.angles[17]=-32768;art.angles[18]=28672;check(force_budget_marker_pose(art,18,0,&p)&&p.angle==-2048,"original signed interpolation/store across angle wrap");
    check(!force_budget_marker_pose(art,30,0,&p)&&!force_budget_marker_pose(art,0,30,&p),"out-of-domain slot/turn refused");
    const auto lib=force_inventory_fixture::catalog();ForceInventory owner;check(owner.create(lib)&&owner.place(0,1,1)==ForceChange::Applied&&owner.place(0,2,2)==ForceChange::Applied,"force with three actual instances");
    const auto revision=owner.revision();for(auto budget:{99,100,349,350,425,999}){s32 prefix=0;check(owner.budget_prefix(0,budget,ForceCostMode::Full,&prefix),"shared admission prefix");const s32 expected=budget<100?0:budget<350?1:budget<425?2:budget==425?3:30;check(prefix==expected,"equality includes Borg and boundary follows it");ForceBattleRoster roster;const bool admitted=owner.battle_roster(0,budget,ForceCostMode::Full,&roster);check(admitted==(expected!=0),"empty admitted prefix cannot launch");if(admitted){const auto count=std::min(expected,3);check(roster.members().size()==unsigned(count)&&roster.source_slot_prefix()==prefix,"roster contains only prefix members");for(const auto& m:roster.members())check(m.force_slot<unsigned(prefix),"no over-budget Borg admitted");}
        check(owner.state()->forces[0].count==3&&owner.revision()==revision,"over-budget Borgs stay in force without mutation");}
    std::printf("budget marker: %u owned checks PASS\n",checks);
}
