#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/force_editor_session.hpp"
#include "engine/gameplay/force_editor_buttons.hpp"
#include "tests/unit/force_inventory_fixture.hpp"
namespace {
using namespace gf;unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"retail editor controls: %s\n",why);std::abort();}}
struct Save final:ISaveBackend {bool load(u32,std::vector<u8>&)override{return false;}bool store(u32,const u8*,std::size_t)override{return false;}bool erase(u32)override{return false;}};
}
int main(){
    using namespace gf;using namespace force_inventory_fixture;const auto lib=catalog();ForceInventory owner;check(owner.create(lib),"owned inventory");ForceSupplement names;std::array<std::string,6> variants={"N","A","G","S","C","B"};std::array<std::string,10> digits;for(unsigned i=0;i<10;++i)digits[i]=std::to_string(i);std::string error;check(names.assign(lib,{},"PART ",variants,digits,{"A"},&error),"owned resource");Save backend;ForceInventoryStore store(backend);ForceEditorSession editor;check(editor.open(owner,names,store),"joined editor");
    GamePadState l,r;l.l=true;r.r=true;check(force_editor_button_bits(l)==0x40&&force_editor_button_bits(r)==0x20,"physical L/R source bits");
    const auto revision=owner.revision();
    for(unsigned repeat=0;repeat<3;++repeat){
        check(editor.step({},0,force_editor_button_bits(l))&&editor.view()==ForceEditorView::List&&editor.list().view().domain==ForceListDomain::Components,"retail L EDIT -> ITEM LIST");
        check(editor.step({},0,force_editor_button_bits(l))&&editor.view()==ForceEditorView::List&&editor.list().view().domain==ForceListDomain::Warehouse,"retail L ITEM LIST -> WAREHOUSE");
        check(editor.step({},0,force_editor_button_bits(l))&&editor.view()==ForceEditorView::Grid,"retail L WAREHOUSE -> EDIT");
        check(editor.step({},0,force_editor_button_bits(r))&&editor.view()==ForceEditorView::List&&editor.list().view().domain==ForceListDomain::Warehouse,"retail R EDIT -> WAREHOUSE");
        check(editor.step({},0,force_editor_button_bits(r))&&editor.view()==ForceEditorView::List&&editor.list().view().domain==ForceListDomain::Components,"retail R WAREHOUSE -> ITEM LIST");
        check(editor.step({},0,force_editor_button_bits(r))&&editor.view()==ForceEditorView::Grid,"retail R ITEM LIST -> EDIT");
    }
    check(owner.revision()==revision,"mode cycling is read-only");
    GamePadState both;both.l=both.r=true;check(editor.step({},0,force_editor_button_bits(both))&&editor.list().view().domain==ForceListDomain::Components,"source L priority over simultaneous R");
    GamePadState all;all.a=all.b=all.x=all.y=all.l=all.r=all.z=all.start=true;check(force_editor_button_bits(all)==0xff0,"ordinary action buttons and folded Start");
    check(editor.step({},0,0x200)&&editor.view()==ForceEditorView::List,"idle B in ITEM LIST stays there");
    std::printf("retail editor controls: %u checks PASS\n",checks);
}
