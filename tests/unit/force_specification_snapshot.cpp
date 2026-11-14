#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/force_specification.hpp"
#include "tests/unit/force_inventory_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"spec snapshot: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;auto catalog=force_inventory_fixture::catalog();std::vector<u8> b(26+138*catalog.entries().size());store_be32(b.data(),0x47465344);store_be32(b.data()+4,1);store_be32(b.data()+8,catalog.fingerprint());store_be16(b.data()+12,u16(catalog.entries().size()));store_be16(b.data()+14,138);
    for(unsigned i=0;i<catalog.entries().size();++i){const auto at=26+i*138;store_be16(b.data()+at,catalog.entries()[i].model);store_be16(b.data()+at+2,catalog.entries()[i].number);std::fill_n(b.data()+at+13,5,255);b[at+13]=1;for(unsigned k=0;k<60;++k)store_be16(b.data()+at+18+k*2,u16(100+k));}
    ForceSpecificationCatalog data;std::string error;check(data.load({b.data(),b.size()},catalog,&error),"owned display resource");ForceInventory owner;check(owner.create(catalog),"inventory");
    const auto initial_revision=owner.revision();ForceSpecificationSnapshot snapshot;check(snapshot.bind(owner,data,ForceSpecDomain::Box,1)&&snapshot.current(owner),"bind actual box instance");check(snapshot.view(owner)->model==0x1002&&snapshot.view(owner)->page_mask==7&&snapshot.value(owner)->acquired==owner.state()->box[1].value.acquired&&owner.revision()==initial_revision,"read-only identity and persistent access");
    ForceInventory neighbor;check(neighbor.create(catalog)&&!snapshot.current(neighbor)&&!snapshot.view(neighbor),"same-data foreign inventory refuses");
    check(owner.to_warehouse(1)==ForceChange::Applied&&!snapshot.current(owner)&&!snapshot.value(owner),"transfer invalidates old revision");check(snapshot.bind(owner,data,ForceSpecDomain::Warehouse,0)&&snapshot.value(owner)->model==0x1002,"same instance warehouse rebinding");
    check(!snapshot.bind(owner,data,ForceSpecDomain::Box,1)&&!snapshot.current(owner),"empty source slot clears snapshot");check(!snapshot.bind(owner,data,ForceSpecDomain::Box,kBoxBorgs)&&!snapshot.bind(owner,data,ForceSpecDomain::Warehouse,kWarehouseBorgs),"slot bounds");
    check(owner.erase_warehouse(0)==ForceChange::Applied&&force_spec_move_access(owner,0x1002)==ForceSpecMoveAccess::Available,"historical collection flag survives last current instance removal");
    auto state=*owner.state();state.profile[0x58]=0;ForceInventory declared;check(declared.load_state(catalog,state),"explicit collection fixture");check(force_spec_move_access(declared,0x1001)==ForceSpecMoveAccess::Unavailable&&snapshot.bind(declared,data,ForceSpecDomain::Box,0)&&snapshot.view(declared)->page_mask==3,"stored instance does not fabricate unlock over explicit profile");
    auto fresh=force_inventory_fixture::borg(0,5);check(declared.acquire(fresh)==ForceChange::Applied&&force_spec_move_access(declared,0x1001)==ForceSpecMoveAccess::Available&&!snapshot.current(declared),"actual acquisition updates availability and invalidates snapshot");
    check(snapshot.bind(declared,data,ForceSpecDomain::Box,1)&&snapshot.view(declared)->variant==5&&snapshot.view(declared)->hp==150,"variant instance readout");
    check(declared.load_state(catalog,*declared.state())&&!snapshot.current(declared),"reload retires instance scope");check(force_spec_move_access(declared,0xffff)==ForceSpecMoveAccess::Unknown,"unknown model not unlocked");
    std::printf("specification inventory snapshot: %u owned checks PASS\n",checks);
}
