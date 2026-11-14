#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/force_editor_list.hpp"
#include "engine/gameplay/force_editor_grid.hpp"
#include "tests/unit/force_inventory_fixture.hpp"
namespace {
unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"force editor list: %s\n",why);std::abort();}}
gf::ForceSupplement names(const gf::ForceCatalog& lib){gf::ForceSupplement out;std::array<std::string,6> variants={"N","A","G","S","C","B"};std::array<std::string,10> digits;for(unsigned i=0;i<10;++i)digits[i]=std::to_string(i);std::string error;check(out.assign(lib,{{0x1001,2}},"PART ",variants,digits,{"A","B","C"},&error),"owned names");return out;}
}
int main(){
    using namespace gf;using namespace force_inventory_fixture;auto lib=catalog();const auto supplement=names(lib);ForceInventory owner;check(owner.create(lib,false),"empty inventory");
    for(unsigned i=0;i<30;++i)check(owner.acquire(borg(i%3))==ForceChange::Applied&&owner.to_warehouse(0)==ForceChange::Applied,"warehouse instances");
    ForceEditorList list;ForceEditorListEvents event;check(list.open(owner,ForceListDomain::Warehouse,supplement),"warehouse list");
    auto step=[&](u32 repeat,u32 pressed){return list.step(owner,repeat,pressed,&event);};
    auto wait=[&](){while(list.wait_updates())if(!step(0,0))return false;return true;};
    check(step(1,0)&&list.cursor()==29&&list.first_row()==17&&event.sounds[0]==119,"page-left at first wraps to final thirteen rows");
    check(step(2,0)&&list.cursor()==0&&list.first_row()==0,"page-right at end wraps to start");
    check(step(2,0)&&list.cursor()==13&&list.first_row()==13,"page-right advances thirteen");
    check(step(0xc,0)&&list.cursor()==12&&list.first_row()==12,"Up has priority over Down");
    check(step(0,0x200)&&event.request==ForceEditorListRequest::None,"warehouse idle B is ignored");
    check(step(4,0x100)&&list.phase()==ForceEditorListPhase::Item&&event.sound_count==2&&event.sounds[0]==119&&event.sounds[1]==122,"navigation before item action");
    check(wait()&&step(0,0x100)&&owner.state()->box_count==1&&owner.state()->warehouse_count==29&&list.view().count==29,"return selected warehouse Borg to box and rebuild filtered list");
    check(wait()&&step(0,0x100)&&wait()&&step(4,0x100)&&list.phase()==ForceEditorListPhase::Discard&&list.menu_row()==1,"warehouse erase defaults No");
    check(wait()&&step(0,0x100)&&owner.state()->warehouse_count==29,"default-No confirmation keeps record");
    check(wait()&&step(0,0x100)&&wait()&&step(4,0x100)&&wait()&&step(8,0x100)&&owner.state()->warehouse_count==28,"explicit Yes deletes selected record");
    check(wait()&&step(0,0x800)&&wait()&&step(0,0x100)&&list.sort()[0].key==ForceSortKey::LevelExperience&&list.sort()[0].descending,"source sort menu first term");
    check(wait()&&step(0,0x800)&&wait()&&step(0,0x100)&&!list.sort()[0].descending,"select same primary toggles direction");
    check(wait()&&step(0,0x800)&&wait(),"filter menu entry");for(unsigned i=0;i<6;++i)check(step(4,0),"filter menu navigation");
    check(step(0,0x100)&&list.phase()==ForceEditorListPhase::Tribe&&wait()&&step(4,0x100)&&list.tribes()==1&&list.cursor()==0,"tribe filter resets list cursor and rebuilds");
    check(wait()&&step(0,0x60)&&event.request==ForceEditorListRequest::Grid,"L before R from warehouse");
    check(list.resume(owner)&&step(0,0x10)&&list.display_flags()==0,"display toggle source flag");
    check(step(0,0x80)&&event.request==ForceEditorListRequest::Exit,"warehouse Start exits");
    ForceInventory parts;check(parts.create(lib,false),"parts owner");for(unsigned i=0;i<24;++i)check(parts.acquire_part((0x1001u<<16)|((i%2)<<8)|(i%6))==ForceChange::Applied,"part instances");
    check(list.open(parts,ForceListDomain::Components,supplement)&&list.visible_rows()==10,"parts show ten rows");
    check(list.step(parts,2,0,&event)&&list.cursor()==10&&list.first_row()==10,"parts page-right advances ten");
    check(list.step(parts,0,0x10,&event)&&list.display_flags()==0,"parts toggle bit two");
    check(list.step(parts,0,0x40,&event)&&event.request==ForceEditorListRequest::Warehouse,"parts L goes to warehouse");
    check(list.resume(parts)&&list.step(parts,0,0x20,&event)&&event.request==ForceEditorListRequest::Grid,"parts R goes to grid");
    // A current list is re-sorted from its current permutation, not storage order.
    ForceInventory ties;check(ties.create(lib,false),"tie owner");for(unsigned model:{0u,0u,2u})check(ties.acquire(borg(model))==ForceChange::Applied,"tie instances");
    auto state=std::make_unique<ForceInventoryState>(*ties.state());for(unsigned i=0;i<3;++i)state->box[i].value.acquired=0;check(ties.load_state(lib,*state),"duplicate serial comparison fixture");
    ForceSortSpec cost;for(auto& term:cost)term.key=ForceSortKey::Cost;ForceSearchList view;check(force_search(ties,ForceListDomain::Box,~u32(0),cost,nullptr,&view)&&view.slots[0]==2&&view.slots[1]==1&&view.slots[2]==0,"initial interchange permutation");
    ForceSortSpec acquired;for(auto& term:acquired)term.key=ForceSortKey::Acquired;check(force_resort(ties,acquired,nullptr,&view)&&view.slots[0]==2&&view.slots[1]==1&&view.slots[2]==0,"all equal re-sort preserves current permutation");
    view.slots[1]=view.slots[0];check(!force_resort(ties,acquired,nullptr,&view)&&!view.current(ties),"forged duplicate index view rejected");
    // Search returns a carry to the same grid, rather than copying a Borg.
    ForceEditorGrid grid;ForceEditorEvents grid_event;check(grid.open(ties,19)&&grid.step(ties,{},0,0x800,&grid_event),"force20 opens operations");
    while(grid.wait_updates())check(grid.step(ties,{},0,0,&grid_event),"operation wait");
    check(grid.step(ties,{},0,0x100,&grid_event)&&grid_event.request==ForceEditorRequest::Search,"search request");
    check(list.open(ties,ForceListDomain::Box,supplement,19)&&list.step(ties,0,0x100,&event)&&event.request==ForceEditorListRequest::Picked,"search picks valid box record");
    check(grid.accept_search_pick(ties,unsigned(event.slot))&&grid.held()==event.slot&&grid.step(ties,{-1,29},0,0x100,&grid_event)&&ties.state()->forces[19].count==1&&ties.state()->box_count==3,"search-to-last-force-slot uses shared membership and no duplication");
    check(!grid.accept_search_pick(ties,200),"invalid search carry refuses");
    ForceInventory empty;check(empty.create(lib,false)&&list.open(empty,ForceListDomain::Components,supplement)&&list.step(empty,0xf,0x100,&event)&&list.selected()==-1&&list.cursor()==0&&event.sound_count==0,"empty component list never manufactures selection");
    check(list.step(empty,0,0x400,&event)&&event.sounds[0]==125,"empty specification request rejects");
    check(empty.acquire_part(0x10010000)==ForceChange::Applied&&!list.step(empty,0,0x100,&event),"mutation invalidates existing list instead of stale index use");
    std::printf("force editor list: %u owned checks PASS\n",checks);
}
