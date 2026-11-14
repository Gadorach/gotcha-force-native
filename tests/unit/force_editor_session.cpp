#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/force_editor_session.hpp"
#include "tests/unit/force_inventory_fixture.hpp"
namespace {
using namespace gf;unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"editor session: %s\n",why);std::abort();}}
struct Save final:ISaveBackend {std::vector<u8> bytes;bool fail=false;unsigned writes=0;bool load(u32,std::vector<u8>& out)override{out=bytes;return !bytes.empty();}SaveReadStatus read_slot(u32,std::vector<u8>& out)override{out=bytes;return bytes.empty()?SaveReadStatus::Missing:SaveReadStatus::Loaded;}bool store(u32 slot,const u8* p,std::size_t n)override{if(slot||fail)return false;bytes.assign(p,p+n);++writes;return true;}bool erase(u32)override{return false;}};
}
int main(){
    using namespace gf;using namespace force_inventory_fixture;const auto library=catalog();ForceInventory owner;Save backend;ForceInventoryStore store(backend);check(store.load(library,&owner)==ForceInventoryLoad::Missing&&owner.create(library),"native first load");
    ForceSupplement names;std::array<std::string,6> variants={"N","A","G","S","C","B"};std::array<std::string,10> digits;for(unsigned i=0;i<10;++i)digits[i]=std::to_string(i);std::string error;check(names.assign(library,{{0x1001,2}},"PART ",variants,digits,{"A","B"},&error),"resource");
    ForceEditorSession session;check(session.open(owner,names,store,19),"last force session");
    check(session.step({1,-1},0,0x100)&&session.held()==1&&session.step({-1,22},0,0x100)&&owner.state()->forces[19].slots[22]==1,"joined held/place");
    check(session.step({-1,22},0,0x400)&&session.view()==ForceEditorView::Spec&&session.specification()->model==0x1002&&session.step({},0,0x200)&&session.view()==ForceEditorView::Grid,"specification return");
    check(session.step({},0,0x20)&&session.view()==ForceEditorView::List&&session.list().view().domain==ForceListDomain::Warehouse,"warehouse entry");
    check(session.step({},0,0x20)&&session.list().view().domain==ForceListDomain::Components&&session.step({},0,0x20)&&session.view()==ForceEditorView::Grid,"retail R list-to-list-to-grid return");
    check(session.step({},0,0x800),"operations entry");for(unsigned i=0;i<10;++i)check(session.step({},0,0),"menu delay");
    check(session.step({},4,0x100)&&session.view()==ForceEditorView::Name,"name operation");session.name_text("OWNED NAME");session.name_backspace();session.name_text("E");
    check(session.step({},0,0x100)&&owner.state()->forces[19].name[0]=='O'&&session.view()==ForceEditorView::Grid,"name writes shared owner and returns");
    backend.fail=true;
    check(session.step({},0,0x80)&&session.view()==ForceEditorView::SaveError&&backend.writes==0&&session.notice().find("FAILED")!=std::string::npos,"Start attempts standalone save immediately and reports failure");backend.fail=false;
    check(session.step({},0,0x100)&&session.view()==ForceEditorView::Returned&&backend.writes==1&&owner.state()->forces[19].slots[0]==1,"successful retry normalizes and returns to main without another acknowledgement");
    ForceInventory reloaded;check(store.load(library,&reloaded)==ForceInventoryLoad::Loaded&&reloaded.state()->forces[19].name==owner.state()->forces[19].name&&reloaded.state()->forces[19].slots[0]==1,"actual native bytes reload edited force");
    ForceEditorReturn result;check(session.take_return(&result)&&result.saved&&result.destination==ForceEditorDestination::MainMenu&&!session.take_return(&result),"one-shot host return");
    check(session.open_for_selection(owner,names,store,19,250,ForceCostMode::Full)&&session.step({},0,0x80)&&session.view()==ForceEditorView::Selection&&backend.writes==1,"selection-entered edit returns without saving");
    check(session.step({},0,0x100)&&session.notice().find("ROSTER READY")!=std::string::npos,"battle roster handoff preview");ForceBattleRoster roster;check(session.take_battle_roster(&roster)&&roster.cost()==250&&!session.take_battle_roster(&roster),"actual roster handed out once with caller budget");
    check(session.step({},0,0x400)&&session.view()==ForceEditorView::Grid&&session.force()==19,"X selection reopens same editor force");
    std::printf("editor session: %u checks PASS\n",checks);
}
