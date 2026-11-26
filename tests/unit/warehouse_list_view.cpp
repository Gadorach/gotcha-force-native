#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/force_list_scene.hpp"
#include "engine/gameplay/force_editor_visual_state.hpp"
#include "tests/unit/editor_review_corrections_fixture.hpp"
namespace {using namespace gf;using namespace editor_review_corrections_fixture;unsigned checks=0;std::string error;
void check(bool x,const char* why){++checks;if(!x){std::fprintf(stderr,"shared warehouse list: %s (%s)\n",why,error.c_str());std::abort();}}
}
int main(){using namespace gf;using namespace editor_review_corrections_fixture;Fixture f;check(f.load(),"owned catalog/inventory/spec data");
    const std::array<u8,5> variants={2,0,0,1,0};const std::array<u32,5> experience={0,100,300,200,300};
    for(unsigned i=0;i<variants.size();++i){auto borg=force_inventory_fixture::borg(0,variants[i]);borg.experience=experience[i];check(f.owner.acquire(borg)==ForceChange::Applied&&f.owner.to_warehouse(3)==ForceChange::Applied,"actual stored warehouse instances, not merged model copies");}
    WarehouseTools tools;WarehouseToolEvent event;WarehouseListView view;check(tools.open(f.owner,f.data,{true,true})&&WarehouseListView::build(f.owner,tools,&view,&error),"model grouping read-only view");const auto revision=f.owner.revision();
    check(view.current(f.owner)&&view.view().domain==ForceListDomain::Warehouse&&view.view().count==1&&view.visible_rows()==10&&view.first_row()==0&&view.title()=="WAREHOUSE","same Items ten-row presentation, explicit new title");
    check(view.row(0)&&view.row(0)->has_borg&&view.row(0)->source_slot==-1&&!view.row(0)->virtual_entry&&view.row(0)->borg.model==0x1001&&!view.row(1),"model metadata row is not an actual warehouse slot");
    check(tools.step(f.owner,0,0x100,0,&event)&&WarehouseListView::build(f.owner,tools,&view,&error),"model→actual instance rows");
    const std::array<s16,5> order={2,4,1,3,0};
    for(unsigned i=0;i<order.size();++i){const auto* row=view.row(i);check(row&&row->source_slot==order[i]&&!row->virtual_entry&&row->has_borg&&row->borg.variant==variants[unsigned(order[i])]&&row->borg.experience==experience[unsigned(order[i])],"variant primary normal-first, EXP descending within, stable ties retain identity");}
    check(f.owner.revision()==revision&&view.details()[0].text=="LV 1 COST 100"&&view.details()[1].text=="EXP 300","drawing data is read-only and selected detail is actual instance data");
    ForceEditorPopupState popup;check(tools.step(f.owner,0,0x100,0,&event)&&WarehouseListView::build(f.owner,tools,&view,&error)&&warehouse_tool_popup_state(f.owner,tools,&popup)&&popup.popup&&popup.popup->labels_ascii==std::vector<std::string>{"MOVE TO BOX","BACK"}&&view.view().count==5&&view.row(0)->source_slot==2,"storage actions are a two-entry popup over actual Borg rows, no editor");
    const auto before_adjust=f.owner.revision();check(tools.step(f.owner,0,0,1,&event)&&f.owner.revision()==before_adjust&&tools.page()==WarehouseToolPage::Actions,"adjustment cannot edit through storage actions");
    while(tools.wait_updates()){check(tools.step(f.owner,0,0,0,&event),"source-style popup opening input wait");}
    check(tools.step(f.owner,4,0,0,&event)&&tools.step(f.owner,0,0x100,0,&event)&&WarehouseListView::build(f.owner,tools,&view,&error)&&tools.page()==WarehouseToolPage::Instances,"second storage action is Back even with editing cheat enabled");
    const auto old=view;check(f.owner.edit_borg({true,true},true,2,f.owner.revision(),BorgEditField::Cost,101)==ForceChange::Applied&&!old.current(f.owner)&&f.owner.state()->warehouse[2].edits.modified,"underlying inventory edit kernel remains separate and invalidates old row revision");
    check(tools.open(f.owner,f.data,{true,true})&&tools.step(f.owner,0,0x100,0,&event)&&WarehouseListView::build(f.owner,tools,&view,&error)&&view.details()[0].text=="LV 1 COST 101"&&view.details()[1].text=="MODIFIED EXP 300","storage can display existing sticky edits without offering editing");
    WarehouseTools picker;check(picker.open_picker(f.owner,f.data,{true,true})&&WarehouseListView::build(f.owner,picker,&view,&error)&&view.title()=="BORG PICKER"&&view.view().count==3,"virtual model picker source catalog order");
    for(unsigned i=0;i<3;++i)check(view.row(i)&&view.row(i)->virtual_entry&&view.row(i)->source_slot==-1&&view.row(i)->borg.model==s16(f.catalog.entries()[i].model),"virtual model rows never fabricate stored slot/provenance");
    check(picker.step(f.owner,0,0x100,0,&event)&&WarehouseListView::build(f.owner,picker,&view,&error)&&view.view().count==6,"six explicit variant rows");for(unsigned i=0;i<6;++i)check(view.row(i)->virtual_entry&&view.row(i)->borg.variant==i,"normal and special variant badges retain source order");
    const auto count=f.owner.state()->warehouse_count;check(picker.step(f.owner,0,0x100,0,&event)&&WarehouseListView::build(f.owner,picker,&view,&error)&&view.view().count==6&&view.cursor()==0&&f.owner.state()->warehouse_count==count+1,"successful Picker action grants real item but does not consume virtual row");
    check(picker.step(f.owner,0,0x100,0,&event)&&WarehouseListView::build(f.owner,picker,&view,&error)&&f.owner.state()->warehouse_count==count+2&&view.row(0)->source_slot==-1,"same virtual entry remains reusable");
    Fixture neighbor;check(neighbor.load()&&!view.current(neighbor.owner)&&!WarehouseListView::build(neighbor.owner,picker,&view,&error)&&!view.current(f.owner),"same-catalog foreign owner refuses/clears view");
    Fixture empty;WarehouseTools empty_tools;check(empty.load()&&empty_tools.open(empty.owner,empty.data,{false,false})&&WarehouseListView::build(empty.owner,empty_tools,&view,&error)&&view.view().count==0&&!view.row(0)&&view.details()[0].text=="WAREHOUSE EMPTY","empty warehouse is explicit, no invented row");
    check(empty.owner.rename_force(0,"CHANGED")==ForceChange::Applied&&!WarehouseListView::build(empty.owner,empty_tools,&view,&error),"stale inventory cannot be rendered as a current tool view");
    std::printf("shared Items-style warehouse row views: %u checks PASS; identity/order/virtual/edit/guards\n",checks);return 0;
}
