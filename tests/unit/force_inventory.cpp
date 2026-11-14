#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include "tests/unit/force_inventory_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"force inventory: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;using namespace force_inventory_fixture;const auto lib=catalog();ForceInventory inv;
    check(inv.create(lib,false),"owned empty owner");check(inv.state()->box_count==0&&inv.state()->force_count==0,"empty counts");
    for(unsigned i=0;i<3;++i)check(inv.acquire(borg(i))==ForceChange::Applied,"acquire typed instance");
    check(inv.state()->next_acquisition()==3&&inv.state()->profile[0x58+2]==1,"serials + collection relation");
    check(inv.place(0,5,0)==ForceChange::Applied&&inv.place(0,1,1)==ForceChange::Applied,"sparse force order");
    auto revision=inv.revision();check(inv.place(0,3,0)==ForceChange::AlreadyMember&&inv.revision()==revision,"one instance per force, no failed mutation");
    check(inv.rename_force(1,"DESTINATION") == ForceChange::Applied,"name empty destination");s16 copied=-1;check(inv.copy_force(0,&copied)==ForceChange::Applied&&copied==1,"first empty force copied");
    check(inv.state()->forces[1].name[0]=='D'&&inv.state()->forces[1].name[1]=='E',"copy keeps destination name");
    check(inv.state()->box[0].memberships==3&&inv.state()->box[1].memberships==3&&inv.state()->box_count==3,"copy aliases owned instances, not duplicates");
    s32 prefix=0;check(inv.budget_prefix(0,250,ForceCostMode::Full,&prefix)&&prefix==2,"exact cost admits ending slot, including prior holes");
    ForceBattleRoster roster;check(inv.battle_roster(0,250,ForceCostMode::Full,&roster)&&roster.members().size()==1&&roster.members()[0].box_slot==1&&roster.cost()==250,"battle ordered budget prefix");
    check(inv.current(roster),"current launch snapshot");check(inv.mark_seen(0,1)==ForceChange::Applied&&!inv.current(roster)&&roster.members()[0].borg.model==0x1002,"snapshot values retained, stale launch rejected");
    check(inv.finish_edit()==ForceChange::Applied,"source exit normalization");check(inv.state()->forces[0].slots[0]==1&&inv.state()->forces[0].slots[1]==0&&inv.state()->forces[0].slots[2]==-1,"stable compaction at exit only");
    check(inv.state()->forces[0].name[0]=='A'&&inv.state()->forces[0].name[21]=='A',"source-shaped unnamed force policy");
    check(inv.budget_prefix(0,249,ForceCostMode::Full,&prefix)&&prefix==0,"expensive first Borg not skipped");check(!inv.battle_roster(0,249,ForceCostMode::Full,&roster),"cannot launch zero admitted members");
    check(inv.battle_roster(0,-1,ForceCostMode::Full,&roster)&&roster.members().size()==2&&roster.source_slot_prefix()==-1,"unlimited signed sentinel");
    ForceInventory other;check(other.load_state(lib,*inv.state())&&!other.current(roster),"neighbor inventory cannot authorize snapshot");
    check(inv.swap_box(0,2)==ForceChange::Applied,"box relocation");check(inv.state()->forces[0].slots[1]==2&&inv.state()->forces[1].slots[1]==2&&inv.state()->box[2].memberships==3,"all force references follow swapped instance");
    s16 destination=-1;const auto serial=inv.state()->box[2].value.acquired;check(inv.to_warehouse(2,&destination)==ForceChange::Applied&&destination==0,"move to first warehouse hole");
    check(inv.state()->forces[0].count==1&&inv.state()->forces[1].count==1&&inv.state()->warehouse[0].acquired==serial,"warehouse transfer detaches every membership, retains instance");
    check(inv.from_warehouse(0,&destination)==ForceChange::Applied&&destination==2&&inv.state()->box[2].memberships==0&&inv.state()->box[2].value.acquired==serial,"return does not invent old force membership or acquisition");
    check(inv.delete_force(1)==ForceChange::Applied&&inv.state()->box_count==3&&inv.state()->forces[1].name[0]=='D',"delete force retains Borgs and name until exit");
    check(inv.finish_edit()==ForceChange::Applied&&inv.state()->forces[1].name[0]==0,"exit clears empty force names");
    auto seeded=std::make_unique<ForceInventoryState>(*inv.state());seeded->box[2].extension={0x12345678,0xaabbccdd,0x87654321};seeded->box[2].value.reserved13=0x81;seeded->box[2].value.reserved14=0x9234;
    check(inv.load_state(lib,*seeded)&&inv.to_warehouse(2)==ForceChange::Applied,"opaque record fields accepted and transferred");check(inv.state()->box[2].extension==seeded->box[2].extension&&inv.state()->box[2].value.reserved13==0x81&&inv.state()->warehouse[0].reserved14==0x9234,"source-unwritten tails preserved");
    check(inv.from_warehouse(0)==ForceChange::Applied&&inv.state()->box[2].extension==seeded->box[2].extension,"return retains destination box extension");
    check(inv.rename_force(0,std::string(41,'X'))==ForceChange::Invalid,"bounded source name bytes");
    auto broken=std::make_unique<ForceInventoryState>(*inv.state());broken->forces[0].slots[0]=200;revision=inv.revision();check(!inv.load_state(lib,*broken)&&inv.revision()==revision,"bad reference refused without replacing owner");
    ForceInventory full;check(full.create(lib,false),"capacity owner");
    for(unsigned i=0;i<kBoxBorgs+kWarehouseBorgs;++i)check(full.acquire(borg(i%3))==ForceChange::Applied,"box then warehouse acquisition");
    check(full.state()->box_count==200&&full.state()->warehouse_count==2000&&full.state()->next_acquisition()==2200,"capacity counts");revision=full.revision();check(full.acquire(borg())==ForceChange::Full&&full.to_warehouse(0)==ForceChange::Full&&full.from_warehouse(0)==ForceChange::Full&&full.revision()==revision,"full destinations retain all data and serials");
    const auto saved=full.state()->warehouse[3];check(full.erase_box(42)==ForceChange::Applied&&full.from_warehouse(3,&destination)==ForceChange::Applied&&destination==42&&full.state()->box[42].value.acquired==saved.acquired,"first-hole retrieval at capacity");
    for(unsigned i=0;i<kWarehouseParts;++i)check(full.acquire_part((0x1001u<<16)|((i%4)<<8)|(i%6))==ForceChange::Applied,"component storage");
    check(full.state()->part_count==1000&&full.acquire_part(0x10010000)==ForceChange::Full,"component capacity");
    auto wrap=std::make_unique<ForceInventoryState>(*full.state());store_be32(wrap->profile.data()+0x50,0xffffffffu);check(full.load_state(lib,*wrap)&&full.erase_part(17)==ForceChange::Applied&&full.acquire_part(0x10010203)==ForceChange::Applied,"explicit serial wrap");check(full.state()->parts[17].acquired==0xffffffffu&&full.state()->parts[17].component()==2&&full.state()->parts[17].variant()==3&&full.state()->next_acquisition()==0,"component and variant byte roles, modulo acquisition");
    check(ForceInventory::valid(*full.state(),lib),"final invariants");
    std::printf("force inventory: %u owned checks PASS\n",checks);
}
