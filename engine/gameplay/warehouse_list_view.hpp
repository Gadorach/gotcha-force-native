#pragma once
#include "engine/gameplay/force_editor_list.hpp"
#include "engine/gameplay/warehouse_tools.hpp"
namespace gf {
struct ForceListRowValue {
    ForceBorg borg;std::string name,value_ascii;
    bool has_borg=false,fresh=false,member=false,ascii=false,disabled=false;
    s16 source_slot=-1;bool virtual_entry=false;
};
struct ForceListDetailText {std::string text;bool ascii=true;};
//Read-only, bounded visible rows for the existing list renderer. A metadata or
//virtual row never becomes a stored Borg/slot or authorizes an inventory change.
class WarehouseListView {
public:
    struct Summary {ForceListDomain domain=ForceListDomain::Components;unsigned count=0;};
    static bool build(const ForceInventory& owner,const WarehouseTools& tools,WarehouseListView* out,std::string* error){
        if(out)*out={};
        auto fail=[&](const char* why){if(error)*error=std::string("warehouse list view: ")+why;return false;};
        if(!out||!tools.current(owner))return fail("stale/foreign controller/inventory");
        WarehouseListView v;v.scope_=owner.instance_binding();v.revision_=owner.revision();v.page_=tools.page();v.cursor_=tools.cursor();
        switch(v.page_){
        case WarehouseToolPage::Models:v.summary_.count=unsigned(tools.models().size());v.title_="WAREHOUSE";v.details_={ForceListDetailText{"A VIEW BORGS"},ForceListDetailText{"Y SORT / FILTER"}};break;
        case WarehouseToolPage::Instances:v.summary_.count=unsigned(tools.instances().size());v.title_="WAREHOUSE";break;
        case WarehouseToolPage::PickerModels:v.summary_.count=unsigned(owner.catalog()->entries().size());v.title_="BORG PICKER";v.details_={ForceListDetailText{"A CHOOSE VARIANT"},ForceListDetailText{"VIRTUAL - NOT CONSUMED"}};break;
        case WarehouseToolPage::PickerVariants:v.summary_.count=6;v.title_="BORG PICKER";v.details_={ForceListDetailText{"A ADD TO WAREHOUSE"},ForceListDetailText{"ENTRY STAYS AVAILABLE"}};break;
        case WarehouseToolPage::Actions:{const auto selected=std::find(tools.instances().begin(),tools.instances().end(),u16(tools.slot()));if(selected==tools.instances().end())return fail("popup instance identity");v.cursor_=unsigned(selected-tools.instances().begin());v.summary_.count=unsigned(tools.instances().size());v.title_="WAREHOUSE";break;}
        case WarehouseToolPage::Edit:return fail("Borg Editor belongs in the stats menu, not storage");
        default:return fail("unsupported page");
        }
        //Owner correction: Warehouse uses its original GREEN source graphics;
        //only the virtual Picker keeps the orange Items presentation.
        if(v.page_==WarehouseToolPage::Models||v.page_==WarehouseToolPage::Instances||v.page_==WarehouseToolPage::Actions)v.summary_.domain=ForceListDomain::Warehouse;
        if(v.summary_.count>kWarehouseBorgs||(v.summary_.count?v.cursor_>=v.summary_.count:v.cursor_!=0))return fail("row/cursor domain");
        if((v.page_==WarehouseToolPage::PickerModels||v.page_==WarehouseToolPage::PickerVariants)&&!tools.cheats().warehouse_selector)return fail("disabled Picker");
        v.first_=v.cursor_/10*10;
        for(unsigned index=v.first_;index<std::min(v.first_+10,v.summary_.count);++index){
            ForceListRowValue row;
            if(v.page_==WarehouseToolPage::Models){row.borg.model=s16(tools.models()[index]);row.has_borg=true;}
            else if(v.page_==WarehouseToolPage::Instances||v.page_==WarehouseToolPage::Actions){const auto slot=tools.instances()[index];if(slot>=kWarehouseBorgs)return fail("warehouse slot");row.borg=owner.state()->warehouse[slot];if(!row.borg.present())return fail("missing actual instance");row.has_borg=true;row.source_slot=s16(slot);row.fresh=row.borg.fresh;}
            else if(v.page_==WarehouseToolPage::PickerModels){row.borg.model=s16(owner.catalog()->entries()[index].model);row.has_borg=row.virtual_entry=true;}
            else if(v.page_==WarehouseToolPage::PickerVariants){row.borg.model=s16(tools.model());row.borg.variant=u8(index);row.has_borg=row.virtual_entry=true;}
            else{return fail("editing fields are not a storage page");}
            if(row.has_borg){const auto* meta=owner.catalog()->find(u16(row.borg.model));if(!meta||row.borg.variant>=6||row.borg.level>=10)return fail("model/variant/level");row.name=meta->source_name;}
            v.rows_.push_back(std::move(row));
        }
        if(v.page_==WarehouseToolPage::Instances||v.page_==WarehouseToolPage::Actions){const auto* borg=tools.selected_borg(owner); //Selection is established on A, not on hover.
            if(v.summary_.count){const auto slot=tools.instances()[v.cursor_];borg=&owner.state()->warehouse[slot];}
            if(borg){v.details_[0]={"LV "+std::to_string(borg->level+1)+" COST "+std::to_string(force_borg_cost(*owner.catalog(),*borg))};v.details_[1]={(borg->edits.modified?"MODIFIED EXP ":"EXP ")+std::to_string(borg->experience)};}
        }
        if(!tools.notice().empty()){
            auto text=tools.notice();if(text.size()>48)return fail("notice budget");const auto split=text.size()>24?text.rfind(' ',24):std::string::npos;
            if(text.size()>24&&split==std::string::npos){return fail("unbreakable notice");}
            v.details_[0]={split==std::string::npos?text:text.substr(0,split)};v.details_[1]={split==std::string::npos?std::string{}:text.substr(split+1)};
            if(v.details_[0].text.size()>24||v.details_[1].text.size()>24)return fail("notice line budget");
        }
        if(!v.summary_.count)v.details_[0]={"WAREHOUSE EMPTY"};
        *out=std::move(v);if(error)error->clear();return true;
    }
    bool current(const ForceInventory& owner)const{return owner.ready()&&scope_.lock()==owner.instance_binding().lock()&&revision_==owner.revision();}
    const Summary& view()const{return summary_;}unsigned cursor()const{return cursor_;}unsigned first_row()const{return first_;}unsigned visible_rows()const{return 10;}
    u8 display_flags()const{return 2;}u32 tribes()const{return ~u32(0);}s16 selected()const{return -1;}
    const ForceListRowValue* row(unsigned index)const{return index>=first_&&index-first_<rows_.size()?&rows_[index-first_]:nullptr;}
    const std::string& title()const{return title_;}const std::array<ForceListDetailText,2>& details()const{return details_;}
    bool plain_fields()const{return false;}
    bool confirm_footer()const{return true;}
    bool stored_footer()const{return page_==WarehouseToolPage::Instances;}
private:
    Summary summary_;std::weak_ptr<const ForceInventoryScope> scope_;u64 revision_=0;WarehouseToolPage page_=WarehouseToolPage::Models;
    unsigned cursor_=0,first_=0;std::vector<ForceListRowValue> rows_;std::string title_;std::array<ForceListDetailText,2> details_;
};
} //namespace gf
