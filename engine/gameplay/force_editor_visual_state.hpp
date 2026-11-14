#pragma once
#include "engine/gameplay/force_editor_session.hpp"
#include "engine/gameplay/force_popup_timeline.hpp"
namespace gf {
struct ForceEditorPopupState {std::optional<ForcePopupPresentation> popup;u32 context=0;};
inline bool warehouse_tool_popup_state(const ForceInventory& owner,const WarehouseTools& tools,ForceEditorPopupState* out){
    if(!out)return false;
    *out={};if(!tools.current(owner))return false;out->context=0x80000000u|tools.popup_context();
    if(tools.page()!=WarehouseToolPage::Actions)return true;
    if(!tools.selected_borg(owner)||tools.cursor()>1)return false;
    ForcePopupPresentation p;p.kind=ForcePopupKind::Warehouse;p.selected=tools.cursor();p.labels_ascii={"MOVE TO BOX","BACK"};if(owner.state()->box_count==kBoxBorgs)p.disabled|=1;out->popup=std::move(p);return true;
}
// Read-only view of the SAME interaction controllers. No second menu selection,
// inventory mutation, or inferred action based on a screenshot/layout coordinate.
inline bool force_editor_popup_state(const ForceInventory& owner,const ForceEditorSession& session,ForceEditorPopupState* out){
    if(!out)return false;
    *out={};if(!owner.ready())return false;ForcePopupPresentation p;
    if(session.view()==ForceEditorView::Grid){
        const auto& grid=session.grid();if(!grid.current(owner))return false;
        if(grid.phase()==ForceEditorGridPhase::Operations){p.kind=ForcePopupKind::Operations;p.selected=grid.menu_row();p.disabled=grid.disabled_operations(owner);}
        else if(grid.phase()==ForceEditorGridPhase::Discard){p.kind=ForcePopupKind::Erase;p.selected=grid.menu_row();}
        else return true;
    }else if(session.view()==ForceEditorView::List){
        const auto& list=session.list();if(!list.current(owner))return false;const auto domain=list.view().domain;const bool parts=domain==ForceListDomain::Components;out->context=1+unsigned(domain);p.selected=list.menu_row();p.components=parts;
        if(list.phase()==ForceEditorListPhase::Sort)p.kind=parts?ForcePopupKind::PartSort:ForcePopupKind::BorgSort;
        else if(list.phase()==ForceEditorListPhase::Tribe){p.kind=ForcePopupKind::Tribe;p.first=list.menu_first();}
        else if(list.phase()==ForceEditorListPhase::Item){p.kind=parts?ForcePopupKind::Part:ForcePopupKind::Warehouse;if(list.selected()<0)return false;if(!parts){if(owner.state()->box_count==kBoxBorgs)p.disabled|=1;const auto model=owner.state()->warehouse[unsigned(list.selected())].model;if(model==0x615||model==0x629)p.disabled|=2;}}
        else if(list.phase()==ForceEditorListPhase::Discard)p.kind=ForcePopupKind::Erase;
        else return true;
    }else return true;
    out->popup=p;return true;
}
} // namespace gf
