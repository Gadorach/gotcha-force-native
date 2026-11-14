#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/force_search.hpp"
#include "engine/save/force_inventory.hpp"
#include "tests/unit/force_inventory_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"force search: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;using namespace force_inventory_fixture;auto library=catalog();ForceInventory owner;check(owner.create(library,false),"owned owner");
    check(owner.acquire(borg(0))==ForceChange::Applied&&owner.acquire(borg(0))==ForceChange::Applied&&owner.acquire(borg(2))==ForceChange::Applied,"three sort entries");
    ForceSortSpec cost;for(auto& t:cost)t.key=ForceSortKey::Cost;ForceSearchList list;
    check(force_search(owner,ForceListDomain::Box,~u32(0),cost,nullptr,&list)&&list.count==3,"cost view");
    check(list.slots[0]==2&&list.slots[1]==1&&list.slots[2]==0,"source interchange sort reverses full ties in this case; not stable_sort");
    check(list.slots.size()==200&&list.slots.back()==-1&&list.current(owner),"source tail and native owner binding");
    const auto revision=owner.revision();auto before=std::make_unique<ForceInventoryImages>();check(force_inventory_encode(*owner.state(),library,before.get()),"before image");
    for(auto& t:cost)t.descending=true;
    check(force_search(owner,ForceListDomain::Box,~u32(0),cost,nullptr,&list)&&list.slots[0]==0&&list.slots[1]==1&&list.slots[2]==2,"descending direction");
    check(force_search(owner,ForceListDomain::Box,1,cost,nullptr,&list)&&list.count==2&&list.slots[0]==0&&list.slots[1]==1,"tribe mask");
    check(force_search(owner,ForceListDomain::Box,0,cost,nullptr,&list)&&list.count==0&&list.slots[0]==-1,"empty filter");
    auto after=std::make_unique<ForceInventoryImages>();check(force_inventory_encode(*owner.state(),library,after.get())&&before->box==after->box&&before->warehouse==after->warehouse&&before->profile==after->profile&&owner.revision()==revision,"search does not mutate records or source order");
    check(owner.mark_seen(0,0)==ForceChange::Applied&&!list.current(owner),"mutation invalidates index view");
    check(owner.to_warehouse(2)==ForceChange::Applied&&force_search(owner,ForceListDomain::Warehouse,~u32(0),cost,nullptr,&list)&&list.count==1&&list.slots[0]==0&&list.slots.size()==2000,"warehouse view uses warehouse slots");
    check(owner.acquire_part((0x1001u<<16)|(1u<<8)|3u)==ForceChange::Applied&&owner.acquire_part((0x1001u<<16)|5u)==ForceChange::Applied,"part records");
    ForceSortSpec numeric;for(auto& t:numeric)t.key=ForceSortKey::Number;
    check(force_search(owner,ForceListDomain::Components,~u32(0),numeric,nullptr,&list)&&list.count==2&&list.slots[0]==0,"part number key: number then variant then component");
    ForceSupplement names;std::array<std::string,6> variants={"N","A","G","S","C","B"};std::array<std::string,10> digits;for(unsigned i=0;i<10;++i)digits[i]=std::to_string(i);std::string error;
    check(names.assign(library,{{0x1001,2}},"PART ",variants,digits,{"A","B"},&error),"owned name format");std::string name;check(names.part_name(library,0x1001,3,1,&name)&&name=="PART S001B","part string includes original ordinal+1 and actual byte roles");
    ForceSortSpec by_name;for(auto& t:by_name)t.key=ForceSortKey::Name;
    check(!force_search(owner,ForceListDomain::Components,~u32(0),by_name,nullptr,&list)&&!list.current(owner),"missing part-name resource refuses");
    check(force_search(owner,ForceListDomain::Components,~u32(0),by_name,&names,&list)&&list.slots[0]==1&&list.slots[1]==0,"encoded part-name sort differs from variant-number order");
    ForceInventory neighbor;check(neighbor.load_state(library,*owner.state())&&!list.current(neighbor),"same data in foreign owner does not authorize index view");
    auto invalid=by_name;invalid[0].key=ForceSortKey(7);check(!force_search(owner,ForceListDomain::Box,~u32(0),invalid,nullptr,&list),"unknown key refuses");
    // Acquisition sorting is signed32, matching the original comparator.
    auto state=std::make_unique<ForceInventoryState>(*owner.state());state->box[0].value.acquired=0x80000000u;state->box[1].value.acquired=1;check(owner.load_state(library,*state),"owned signed-counter state");ForceSortSpec acquired;for(auto& t:acquired)t.key=ForceSortKey::Acquired;
    check(force_search(owner,ForceListDomain::Box,~u32(0),acquired,nullptr,&list)&&list.slots[0]==0,"signed acquisition order");
    std::printf("force search: %u owned checks PASS\n",checks);
}
