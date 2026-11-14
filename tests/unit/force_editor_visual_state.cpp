#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/force_editor_visual_state.hpp"
#include "engine/core/headless_backends.hpp"
#include "tests/unit/force_inventory_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"editor visual state: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;const auto catalog=force_inventory_fixture::catalog();ForceInventory owner;check(owner.create(catalog),"owned inventory");
    check(owner.to_warehouse(1)==ForceChange::Applied&&owner.acquire_part(0x10010000)==ForceChange::Applied,"owned warehouse and part");
    ForceSupplement names;std::array<std::string,6> variants={"N","A","G","S","C","B"};std::array<std::string,10> digits;for(unsigned i=0;i<10;++i)digits[i]=std::to_string(i);std::string error;
    check(names.assign(catalog,{{0x1001,2}},"PART ",variants,digits,{"A","B"},&error),"owned supplement");
    headless::Save save;ForceInventoryStore store(save);ForceEditorSession session;check(session.open(owner,names,store),"joined session");ForceEditorPopupState view;
    auto read=[&](){check(force_editor_popup_state(owner,session,&view),"read actual controller");};
    auto step=[&](u32 repeat,u32 press){check(session.step({},repeat,press),"controller callback");read();};
    auto wait=[&](){for(unsigned i=0;i<10;++i)step(0,0);};
    read();check(!view.popup,"grid has no invented popup");step(0,0x800);check(view.popup&&view.popup->kind==ForcePopupKind::Operations&&view.popup->disabled==0x30,"operations carries actual disabled choices");
    wait();step(0,0x100);check(session.view()==ForceEditorView::List&&!view.popup&&view.context==1,"search controller entry");
    step(0,0x800);check(view.popup&&view.popup->kind==ForcePopupKind::BorgSort,"search sort presentation");wait();
    for(unsigned i=0;i<6;++i)step(4,0);
    step(0,0x100);check(view.popup&&view.popup->kind==ForcePopupKind::Tribe,"tribe submenu");wait();
    for(unsigned i=0;i<8;++i)step(4,0);
    check(view.popup->selected==8&&view.popup->first==1,"ninth tribe scrolls original eight-row window");
    for(unsigned i=0;i<13;++i)step(4,0);
    check(view.popup->selected==0&&view.popup->first==0,"tribe wrap returns top window");
    step(0,0x200);check(view.popup->kind==ForcePopupKind::BorgSort&&view.popup->first==0,"return to short menu discards scrolling view");wait();step(0,0x200);wait();step(0,0x200);
    step(0,0x20);check(view.context==2&&!view.popup,"warehouse context");step(0,0x100);check(view.popup&&view.popup->kind==ForcePopupKind::Warehouse&&view.popup->disabled==0,"warehouse operations");wait();step(4,0x100);
    check(view.popup->kind==ForcePopupKind::Erase&&view.popup->selected==1&&!view.popup->components,"warehouse discard defaults No");const auto count=owner.state()->warehouse_count;wait();step(0,0x100);check(!view.popup&&owner.state()->warehouse_count==count,"No retains warehouse item");wait();step(0,0x20);
    step(0,0x800);check(view.context==3&&view.popup->kind==ForcePopupKind::PartSort&&view.popup->components,"component-specific order menu");wait();step(0,0x200);wait();step(0,0x100);
    check(view.popup->kind==ForcePopupKind::Part&&view.popup->components,"part operation");wait();step(0,0x100);check(view.popup->kind==ForcePopupKind::Erase&&view.popup->selected==1&&view.popup->components,"part discard text and No");
    ForceInventory neighbor;check(neighbor.create(catalog)&&!force_editor_popup_state(neighbor,session,&view)&&!view.popup,"foreign owner refuses and clears output");
    check(owner.acquire(force_inventory_fixture::borg())==ForceChange::Applied&&!force_editor_popup_state(owner,session,&view)&&!view.popup,"stale list refuses instead of publishing obsolete labels");
    std::printf("editor visual state: %u checks PASS\n",checks);
}
