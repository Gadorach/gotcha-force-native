#pragma once
#include "engine/gameplay/force_specification.hpp"
namespace gf {
enum class WarehouseToolPage {Models,Instances,Actions,PickerModels,PickerVariants,Edit};
struct WarehouseToolEvent {bool close=false,legacy_list=false;u16 sound=0;bool specification=false,sort_menu=false;s16 slot=-1;};
// Owner-requested grouped view. Slots remain the actual inventory identities;
// sorting never reorders or merges the stored Borgs.
class WarehouseTools {
public:
    bool open(const ForceInventory& owner,const ForceSpecificationCatalog& data,CheatSettings cheats){
        *this={};if(!owner.ready()||!data.for_catalog(*owner.catalog()))return false;
        data_=data;cheats_=cheats;scope_=owner.instance_binding();revision_=owner.revision();ready_=true;rebuild(owner);return true;
    }
    bool open_picker(const ForceInventory& owner,const ForceSpecificationCatalog& data,CheatSettings cheats){
        if(!cheats.warehouse_selector||!open(owner,data,cheats))return false;
        picker_root_=true;page_=WarehouseToolPage::PickerModels;return true;
    }
    bool current(const ForceInventory& owner)const{return ready_&&owner.ready()&&scope_.lock()==owner.instance_binding().lock()&&revision_==owner.revision();}
    WarehouseToolPage page()const{return page_;}unsigned cursor()const{return cursor_;}u16 model()const{return model_;}s16 slot()const{return slot_;}
    unsigned wait_updates()const{return wait_;}
    u32 popup_context()const{return popup_context_;}
    std::span<const u16> models()const{return models_;}std::span<const u16> instances()const{return instances_;}
    const std::string& notice()const{return notice_;}CheatSettings cheats()const{return cheats_;}
    const ForceBorg* selected_borg(const ForceInventory& owner)const{return current(owner)&&slot_>=0?&owner.state()->warehouse[unsigned(slot_)]:nullptr;}
    s32 edit_value(const ForceInventory& owner,unsigned row)const{
        const auto* b=selected_borg(owner);if(!b||row>9)return -1;
        if(row==0)return b->variant;
        if(row==2)return b->level+1;
        const unsigned index=row==1?0:row-2;if(b->edits.values[index]>=0)return b->edits.values[index];
        if(row==1)return force_borg_cost(*owner.catalog(),*b);
        if(row>=8)return -1;
        const auto* data=data_.find(u16(b->model));return row==3?data->hp[b->variant][b->level]:data->authored[row];
    }
    // Native P=0x1000 opens the virtual picker; +/- use separate native actions.
    bool step(ForceInventory& owner,u32 repeated,u32 pressed,int adjustment,WarehouseToolEvent* out,bool source_access=false,s16 pointed_row=-1){
        if(!out)return false;
        *out={};if(!current(owner)||adjustment< -1||adjustment>1||page_==WarehouseToolPage::Edit)return false;
        if(wait_){--wait_;return true;}
        if((pressed&0x800)&&page_==WarehouseToolPage::Models){out->legacy_list=true;out->sort_menu=source_access;out->sound=122;return true;}
        if((pressed&0x1000)&&cheats_.warehouse_selector&&page_==WarehouseToolPage::Models){page_=WarehouseToolPage::PickerModels;cursor_=0;notice_.clear();return true;}
        const unsigned count=page_==WarehouseToolPage::Models?unsigned(models_.size()):page_==WarehouseToolPage::Instances?unsigned(instances_.size()):page_==WarehouseToolPage::PickerModels?unsigned(owner.catalog()->entries().size()):page_==WarehouseToolPage::PickerVariants?6:2;
        if(pointed_row< -1||(pointed_row>=0&&unsigned(pointed_row)>=count))return false;
        const auto before=cursor_;if(pointed_row>=0)cursor_=unsigned(pointed_row);if(count){if(repeated&8)cursor_=(cursor_+count-1)%count;else if(repeated&4)cursor_=(cursor_+1)%count;else if(page_!=WarehouseToolPage::Actions&&(repeated&1))cursor_=cursor_>10?cursor_-10:0;else if(page_!=WarehouseToolPage::Actions&&(repeated&2))cursor_=std::min(cursor_+10,count-1);}
        if(cursor_!=before)out->sound=119;
        //Explicit stored-list integration. A grouped model or virtual Picker row
        //has no instance authority. Y is the original sorting menu, not actions.
        if(source_access&&page_==WarehouseToolPage::Instances&&count&&(pressed&0xc00u)){
            out->slot=s16(instances_[cursor_]);out->specification=(pressed&0x400u)!=0;out->legacy_list=out->sort_menu=!out->specification;out->sound=122;return true;
        }
        if(pressed&0x200){out->sound=123;notice_.clear();if(page_==WarehouseToolPage::Models||(picker_root_&&page_==WarehouseToolPage::PickerModels))out->close=true;else if(page_==WarehouseToolPage::Actions){return_to_instances(owner);}else if(page_==WarehouseToolPage::PickerVariants){page_=WarehouseToolPage::PickerModels;cursor_=0;}else{page_=WarehouseToolPage::Models;cursor_=0;rebuild(owner);}return true;}
        if(!(pressed&0x100)||!count)return true;
        if(page_==WarehouseToolPage::Instances&&popup_context_==0x7fffffffu)return false;
        out->sound=122;
        if(page_==WarehouseToolPage::Models){model_=models_[cursor_];page_=WarehouseToolPage::Instances;cursor_=0;rebuild_instances(owner);}
        else if(page_==WarehouseToolPage::Instances){slot_=s16(instances_[cursor_]);page_=WarehouseToolPage::Actions;cursor_=0;wait_=10;++popup_context_;}
        else if(page_==WarehouseToolPage::PickerModels){model_=owner.catalog()->entries()[cursor_].model;page_=WarehouseToolPage::PickerVariants;cursor_=0;}
        else if(page_==WarehouseToolPage::PickerVariants){const auto result=owner.pick_warehouse(cheats_,model_,u8(cursor_));if(result==ForceChange::Applied){revision_=owner.revision();notice_="ADDED TO WAREHOUSE - PICK AGAIN";}else{notice_="WAREHOUSE FULL";out->sound=125;}}
        else if(cursor_==0){const auto selected=std::find(instances_.begin(),instances_.end(),u16(slot_));if(selected==instances_.end())return false;const unsigned prior_instance=unsigned(selected-instances_.begin());const auto group=std::find(models_.begin(),models_.end(),model_);if(group==models_.end())return false;const unsigned prior_group=unsigned(group-models_.begin());const auto result=owner.from_warehouse(unsigned(slot_));if(result==ForceChange::Applied){revision_=owner.revision();slot_=-1;rebuild_instances(owner);
                //Stay in this Borg type while ANY variant remains. Preserve the
                //nearest valid ordinal after removal under the existing stable
                //variant-primary/EXP ordering, not a guessed model re-selection.
                if(!instances_.empty()){page_=WarehouseToolPage::Instances;cursor_=std::min(prior_instance,unsigned(instances_.size()-1));slot_=s16(instances_[cursor_]);}
                else{page_=WarehouseToolPage::Models;rebuild(owner);cursor_=models_.empty()?0:std::min(prior_group,unsigned(models_.size()-1));}
                notice_="MOVED TO BOX";wait_=10;
            }else{notice_="BOX FULL";out->sound=125;}}
        else{return_to_instances(owner);}
        return true;
    }
private:
    bool resume_stats_selection(const ForceInventory& owner,s16 slot){
        if(!ready_||!owner.ready()||scope_.lock()!=owner.instance_binding().lock()||page_!=WarehouseToolPage::Instances||slot<0||unsigned(slot)>=kWarehouseBorgs||owner.state()->warehouse[unsigned(slot)].model!=s16(model_))return false;
        revision_=owner.revision();rebuild_instances(owner);const auto found=std::find(instances_.begin(),instances_.end(),u16(slot));if(found==instances_.end())return false;cursor_=unsigned(found-instances_.begin());slot_=slot;return true;
    }
    friend class ForceEditorSession;
    void return_to_instances(const ForceInventory& owner){page_=WarehouseToolPage::Instances;rebuild_instances(owner);const auto selected=std::find(instances_.begin(),instances_.end(),u16(slot_));if(selected!=instances_.end())cursor_=unsigned(selected-instances_.begin());wait_=10;}
    void rebuild(const ForceInventory& owner){models_.clear();for(const auto& entry:owner.catalog()->entries())for(const auto& b:owner.state()->warehouse)if(b.present()&&b.model==s16(entry.model)){models_.push_back(entry.model);break;}}
    void rebuild_instances(const ForceInventory& owner){instances_.clear();for(unsigned i=0;i<kWarehouseBorgs;++i)if(owner.state()->warehouse[i].model==s16(model_))instances_.push_back(u16(i));
        std::stable_sort(instances_.begin(),instances_.end(),[&](u16 a,u16 b){const auto& x=owner.state()->warehouse[a];const auto& y=owner.state()->warehouse[b];return x.variant!=y.variant?x.variant<y.variant:x.experience>y.experience;});cursor_=0;
    }
    ForceSpecificationCatalog data_;CheatSettings cheats_;std::weak_ptr<const ForceInventoryScope> scope_;u64 revision_=0;bool ready_=false,picker_root_=false;
    WarehouseToolPage page_=WarehouseToolPage::Models;unsigned cursor_=0,wait_=0;u32 popup_context_=0;u16 model_=0;s16 slot_=-1;std::vector<u16> models_,instances_;std::string notice_;
};
} // namespace gf
