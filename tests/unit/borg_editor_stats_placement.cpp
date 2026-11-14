#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/warehouse_list_view.hpp"
#include "engine/gameplay/force_editor_visual_state.hpp"
#include "tests/unit/editor_review_corrections_fixture.hpp"
namespace {using namespace gf;using namespace editor_review_corrections_fixture;unsigned checks=0;std::string error;void check(bool x,const char* why){++checks;if(!x){std::fprintf(stderr,"stats-only Borg Editor placement: %s (%s)\n",why,error.c_str());std::abort();}}}
int main(){using namespace gf;using namespace editor_review_corrections_fixture;
    for(bool editing_enabled:{false,true}){Fixture f;check(f.load()&&f.owner.acquire(force_inventory_fixture::borg())==ForceChange::Applied&&f.owner.to_warehouse(3)==ForceChange::Applied,"actual stored Borg fixture");WarehouseTools tools;WarehouseToolEvent event;WarehouseListView view;check(tools.open(f.owner,f.data,{true,editing_enabled})&&tools.step(f.owner,0,0x100,0,&event)&&tools.step(f.owner,0,0x100,0,&event)&&WarehouseListView::build(f.owner,tools,&view,&error),"warehouse instance actions");
        ForceEditorPopupState popup;check(tools.page()==WarehouseToolPage::Actions&&warehouse_tool_popup_state(f.owner,tools,&popup)&&popup.popup&&popup.popup->labels_ascii==std::vector<std::string>{"MOVE TO BOX","BACK"}&&view.view().count==1&&view.row(0)->source_slot==0,"Warehouse popup exposes no Borg Editor regardless of cheat flag");const auto revision=f.owner.revision();
        for(unsigned tick=0;tick<20;++tick){check(tools.step(f.owner,0,0,tick%2?1:-1,&event)&&f.owner.revision()==revision&&!f.owner.state()->warehouse[0].edits.modified&&tools.page()!=WarehouseToolPage::Edit,"plus/minus cannot edit from storage pages");}
        check(tools.step(f.owner,4,0,0,&event)&&tools.step(f.owner,0,0x100,0,&event)&&tools.page()==WarehouseToolPage::Instances,"Back cannot open the former edit page");
        check(tools.open_picker(f.owner,f.data,{true,editing_enabled}),"Picker fixture");
        for(unsigned phase=0;phase<2;++phase){check(WarehouseListView::build(f.owner,tools,&view,&error),"virtual picker view");for(unsigned index=view.first_row();index<std::min(view.first_row()+10,view.view().count);++index){const auto* row=view.row(index);check(row&&row->virtual_entry&&row->source_slot==-1&&row->name!="BORG EDITOR"&&row->value_ascii.empty(),"virtual rows cannot offer stored-instance editing");}const auto before=f.owner.revision();check(tools.step(f.owner,0,0,1,&event)&&f.owner.revision()==before,"virtual entry adjustment does nothing");if(!phase)check(tools.step(f.owner,0,0x100,0,&event),"variant page, not editor");}
    }
    Fixture f;ForceEditorSession session;check(f.load()&&session.open(f.owner,f.names,f.store,19)&&session.configure_tools({true,true},f.data)&&session.configure_specification(f.data)&&session.step({0,-1},0,0x400)&&session.view()==ForceEditorView::Spec,"actual Borg still has a dedicated stats context");const auto request=session.spec_request();check(request&&request->snapshot().current(f.owner)&&request->snapshot().value(f.owner)->model==0x1001&&!session.complete_specification({}),"stats request stays actual-instance/revision bound; no editor completion fabricated");
    std::printf("Borg Editor storage-placement removal: %u checks PASS; stats integration still separate\n",checks);return 0;
}
