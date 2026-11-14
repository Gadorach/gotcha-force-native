#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/force_editor_session.hpp"
#include "engine/core/headless_backends.hpp"
#include "tests/unit/force_inventory_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"editor budget policy: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;const auto lib=force_inventory_fixture::catalog();ForceInventory owner;check(owner.create(lib)&&owner.place(0,1,1)==ForceChange::Applied,"two-instance force");s32 total=0,prefix=0;check(owner.total_cost(0,ForceCostMode::Full,&total)&&total==350,"full display total");check(owner.total_cost(0,ForceCostMode::TwoThirds,&total)&&total==232,"per-Borg integer policy, not rounding the aggregate");
    for(auto mode:{ForceCostMode::Full,ForceCostMode::TwoThirds})for(s32 budget:{65,66,100,231,232,350,351}){check(owner.total_cost(0,mode,&total)&&owner.budget_prefix(0,budget,mode,&prefix),"same cost kernel for display and boundary");ForceBattleRoster roster;const bool launched=owner.battle_roster(0,budget,mode,&roster);check(launched==(prefix!=0),"boundary predicts launch availability");if(launched){check(roster.source_slot_prefix()==prefix&&roster.cost()<=u32(budget),"roster obeys same boundary");if(prefix>=2)check(roster.cost()==u32(total),"all-member launch equals displayed total");}}
    ForceSupplement names;std::array<std::string,6> variants={"N","A","G","S","C","B"};std::array<std::string,10> digits;for(unsigned i=0;i<10;++i)digits[i]=std::to_string(i);std::string error;check(names.assign(lib,{},"PART ",variants,digits,{"A"},&error),"supplement");headless::Save save;ForceInventoryStore store(save);ForceEditorSession editor;
    check(editor.open_for_selection(owner,names,store,0,232,ForceCostMode::TwoThirds)&&editor.cost_mode()==ForceCostMode::TwoThirds&&editor.energy_limit()==232,"caller cost mode reaches presentation");check(editor.step({},0,0x80)&&editor.step({},0,0x100),"return to matching selection context");ForceBattleRoster roster;check(editor.take_battle_roster(&roster)&&roster.members().size()==2&&roster.cost()==232,"retained caller mode produces actual roster");
    check(editor.open(owner,names,store)&&editor.cost_mode()==ForceCostMode::Full&&editor.energy_limit()==-1,"standalone resets caller policy");check(!owner.total_cost(20,ForceCostMode::Full,&total)&&total==0&&!owner.total_cost(0,ForceCostMode(99),&total),"invalid total request clears output");
    std::printf("editor budget policy: %u owned checks PASS\n",checks);
}
