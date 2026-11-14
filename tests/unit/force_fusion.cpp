#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/force_fusion.hpp"
#include "engine/save/force_inventory.hpp"
#include "tests/unit/force_inventory_fixture.hpp"
namespace {
unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"force fusion: %s\n",why);std::abort();}}
gf::ForceSupplement supplement(const gf::ForceCatalog& lib){
    gf::ForceSupplement out;std::array<std::string,6> variants={"N","A","G","S","C","B"};
    std::array<std::string,10> digits;for(unsigned i=0;i<10;++i)digits[i]=std::to_string(i);std::string error;
    check(out.assign(lib,{{0x1002,3},{0x1001,2}},"PART ",variants,digits,{"A","B","C"},&error),"owned recipe order");return out;
}
}
int main(){
    using namespace gf;using namespace force_inventory_fixture;const auto lib=catalog();const auto recipes=supplement(lib);ForceInventory inv;
    check(inv.create(lib,false),"empty owner");ForceFusionPlan plan;ForceFusionResult result;
    check(ForceFusion::first(inv,recipes,&plan)==ForceFusionStatus::Incomplete,"no invented components");
    for(u32 key:{0x10010102u,0x10010102u,0x10010003u,0x10010002u})check(inv.acquire_part(key)==ForceChange::Applied,"component fixtures");
    check(ForceFusion::prepare(inv,recipes,0x1001,3,&plan)==ForceFusionStatus::Incomplete,"variants stay separate");
    check(ForceFusion::first(inv,recipes,&plan)==ForceFusionStatus::Ready&&plan.model()==0x1001&&plan.variant()==2,"first complete variant");
    check(plan.components().size()==2&&plan.components()[0]==0&&plan.components()[1]==3,"first distinct slots in storage order, not component-number order");
    ForceInventory neighbor;check(neighbor.load_state(lib,*inv.state())&&ForceFusion::commit(neighbor,plan)==ForceFusionStatus::Stale,"foreign plan refused");
    const auto snapshot=plan;const auto serial=inv.state()->next_acquisition();
    check(ForceFusion::commit(inv,plan,&result)==ForceFusionStatus::Ready&&result.slot==0&&!result.warehouse,"grant to box first");
    check(result.borg.model==0x1001&&result.borg.variant==2&&result.borg.level==0&&result.borg.experience==0&&result.borg.acquired==serial&&result.borg.fresh==1,"new Borg source defaults and single serial");
    check(inv.state()->part_count==2&&inv.state()->parts[1].present()&&inv.state()->parts[2].present()&&!inv.state()->parts[0].present()&&!inv.state()->parts[3].present(),"only chosen duplicate set consumed");
    check(inv.state()->profile[0x58]==4&&inv.state()->profile[0x158]==4,"ordinary acquisition collection flags");
    check(ForceFusion::commit(inv,snapshot,&result)==ForceFusionStatus::Stale&&result.slot==-1,"no repeated completion");
    check(inv.acquire_part(0x10010002u)==ForceChange::Applied&&ForceFusion::first(inv,recipes,&plan)==ForceFusionStatus::Ready,"duplicate set can complete later");
    check(inv.rename_force(0,"CHANGED")==ForceChange::Applied&&ForceFusion::commit(inv,plan)==ForceFusionStatus::Stale,"edit invalidates pending completion");
    for(u32 key:{0x10020201u,0x10020001u,0x10020101u})check(inv.acquire_part(key)==ForceChange::Applied,"earlier recipe set");
    check(ForceFusion::first(inv,recipes,&plan)==ForceFusionStatus::Ready&&plan.model()==0x1002&&plan.variant()==1,"recipe table order precedes model number");
    check(inv.acquire_part(0x10020301u)==ForceChange::Applied&&ForceFusion::prepare(inv,recipes,0x1002,1,&plan)==ForceFusionStatus::Incomplete,"extra component bit prevents exact-mask match");
    check(ForceFusion::prepare(inv,recipes,0x1003,0,&plan)==ForceFusionStatus::Incomplete,"absent model defaults to one, not a fusion recipe");
    auto state=std::make_unique<ForceInventoryState>(*inv.state());
    for(auto& b:state->box){b.value=borg();b.memberships=0;}state->box_count=200;
    // Keep a hole with opaque destination bytes in otherwise full storage.
    for(auto& b:state->warehouse)b=borg();
    state->warehouse_count=2000;
    check(inv.load_state(lib,*state),"full destinations");const auto revision=inv.revision();
    check(ForceFusion::first(inv,recipes,&plan)==ForceFusionStatus::Full&&inv.revision()==revision,"full storage refuses before any consumption");
    state->warehouse[42].model=-1;state->warehouse[42].reserved13=0xc7;state->warehouse[42].reserved14=0x1234;--state->warehouse_count;
    state->parts[0].reserved={0x81,0x82,0x83};store_be32(state->profile.data()+0x50,0xffffffffu);
    check(inv.load_state(lib,*state)&&ForceFusion::prepare(inv,recipes,0x1001,2,&plan)==ForceFusionStatus::Ready,"warehouse capacity available");
    check(ForceFusion::commit(inv,plan,&result)==ForceFusionStatus::Ready&&result.warehouse&&result.slot==42,"first warehouse hole used when box full");
    check(result.borg.reserved13==0xc7&&result.borg.reserved14==0x1234&&result.borg.acquired==0xffffffffu&&inv.state()->next_acquisition()==0,"destination tails and serial wrap preserved");
    check(inv.state()->parts[0].reserved==std::array<u8,3>{0x81,0x82,0x83}&&inv.state()->parts[0].key==0xffffffffu,"consumption retains source-unwritten bytes");
    check(ForceInventory::valid(*inv.state(),lib),"joint final invariants");
    std::printf("force fusion: %u owned checks PASS\n",checks);
}
