#pragma once
#include "engine/gameplay/force_specification.hpp"
namespace gf {
class ForceEditorSession;
struct BorgStatsEditBinding {};
struct BorgStatsEditEvent {u16 sound=0;bool changed=false,close=false,committed=false;u64 prior_revision=0;};
//Only an editor-session stats context can open this owner. Virtual/part rows
//cannot manufacture an editable slot. All mutations use the existing kernel.
class BorgStatsEditor {
public:
    bool current(const ForceInventory& owner)const{const auto* b=value(owner);return ready_&&b&&owner.revision()==revision_&&scope_.lock()==owner.instance_binding().lock()&&b->model==model_&&b->acquired==acquired_;}
    unsigned cursor()const{return cursor_;}unsigned first_row()const{return first_;}
    ForceSpecDomain domain()const{return domain_;}unsigned slot()const{return slot_;}
    s32 field_value(const ForceInventory& owner,unsigned field)const{
        if(!current(owner)||field>=10)return -1;
        const auto* b=&draft_;if(field==0)return b->variant;if(field==2)return b->level+1;
        const unsigned id=field==1?0:field-2;if(b->edits.values[id]>=0)return b->edits.values[id];
        if(field==1)return force_borg_cost(*owner.catalog(),*b);
        if(field>=8)return -1;
        const auto* row=data_.find(u16(b->model));return field==3?row->hp[b->variant][b->level]:row->authored[field];
    }
    bool snapshot(const ForceInventory& owner,ForceSpecificationSnapshot* out)const{return out&&current(owner)&&out->bind_impl(owner,data_,domain_,slot_,&draft_);}
    bool dirty()const{return draft_!=original_;}
    bool committed_modified(const ForceInventory& owner)const{return current(owner)&&original_.edits.modified;}
    bool step(ForceInventory& owner,u32 repeated,u32 pressed,int,BorgStatsEditEvent* out){
        if(out)*out={};
        if(!out||!current(owner))return false;
        if(pressed&0x200u){out->close=true;out->sound=123;return true;}
        if(pressed&0x100u){out->prior_revision=owner.revision();const auto result=owner.commit_stats_draft(cheats_,domain_==ForceSpecDomain::Warehouse,slot_,revision_,original_,draft_);if(result!=ForceChange::Applied&&result!=ForceChange::Unchanged)return false;out->changed=result==ForceChange::Applied;out->committed=true;out->close=true;out->sound=122;revision_=owner.revision();return true;}
        const auto before=cursor_;if(repeated&8u)cursor_=(cursor_+9)%10;else if(repeated&4u)cursor_=(cursor_+1)%10;
        if(cursor_<first_)first_=cursor_;else if(cursor_>=first_+8)first_=cursor_-7;if(cursor_!=before)out->sound=119;
        const int adjustment=repeated&1u?-1:repeated&2u?1:0;
        if(adjustment){const auto old=field_value(owner,cursor_);const auto next=old<0?(adjustment>0?1:0):old+adjustment;const auto change=force_borg_edit_value(draft_,BorgEditField(cursor_),next);
            if(change==ForceChange::Applied){
                //Returning a field to its entry value restores its original
                //override/default representation, not a spurious edited flag.
                const auto candidate=draft_;draft_=original_;const auto original_value=field_value(owner,cursor_);draft_=candidate;
                if(next==original_value){if(cursor_==0)draft_.variant=original_.variant;else if(cursor_==2)draft_.level=original_.level;else{const auto id=cursor_==1?0:cursor_-2;draft_.edits.values[id]=original_.edits.values[id];}}
                draft_.edits.modified=original_.edits.modified||draft_.variant!=original_.variant||draft_.level!=original_.level||draft_.edits.values!=original_.edits.values;out->sound=119;
            }else if(change==ForceChange::Invalid){out->sound=125;}else if(change!=ForceChange::Unchanged)return false;
        }return true;
    }
    void close(){ready_=false;scope_.reset();binding_.reset();}
private:
    bool open(const ForceInventory& owner,const ForceSpecificationCatalog& data,CheatSettings cheats,ForceSpecDomain domain,unsigned slot){
        *this={};if(!owner.ready()||!cheats.borg_editor||!data.for_catalog(*owner.catalog()))return false;domain_=domain;slot_=slot;const auto* b=value(owner);if(!b||!b->present())return false;ForceSpecificationSnapshot snapshot;if(!snapshot.bind(owner,data,domain,slot))return false;
        data_=data;cheats_=cheats;scope_=owner.instance_binding();revision_=owner.revision();model_=b->model;acquired_=b->acquired;original_=draft_=*b;cursor_=1;binding_=std::make_shared<const BorgStatsEditBinding>();ready_=true;return true;
    }
    const ForceBorg* value(const ForceInventory& owner)const{if(!owner.ready())return nullptr;if(domain_==ForceSpecDomain::Box)return slot_<kBoxBorgs?&owner.state()->box[slot_].value:nullptr;if(domain_==ForceSpecDomain::Warehouse)return slot_<kWarehouseBorgs?&owner.state()->warehouse[slot_]:nullptr;return nullptr;}
    ForceSpecificationCatalog data_;CheatSettings cheats_;std::weak_ptr<const ForceInventoryScope> scope_;u64 revision_=0;u32 acquired_=0;s16 model_=-1;unsigned slot_=0,cursor_=0,first_=0;ForceSpecDomain domain_=ForceSpecDomain::Box;bool ready_=false;
    friend class ForceEditorSession;
    friend class BorgStatsEditScene;
    std::shared_ptr<const BorgStatsEditBinding> binding_;
    ForceBorg original_,draft_;
};
} //namespace gf
