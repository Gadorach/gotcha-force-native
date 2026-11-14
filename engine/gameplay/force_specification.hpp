#pragma once
#include "engine/assets/force_specification.hpp"
#include "engine/gameplay/force_inventory.hpp"
namespace gf {
enum class ForceSpecDomain {Box,Warehouse};
// Source editor801BA4F8 supplies the600-byte profile rooted at803BFE20.
//801DD298 reads803BFE78[number], i.e. profile+58+number. Existing inventory
// acquisition owns these persistent collection flags; do not substitute a scan
// of current force membership or silently assume every model is unlocked.
inline ForceSpecMoveAccess force_spec_move_access(const ForceInventory& owner,u16 model){
    if(!owner.ready())return ForceSpecMoveAccess::Unknown;
    const auto* metadata=owner.catalog()->find(model);if(!metadata)return ForceSpecMoveAccess::Unknown;
    return owner.state()->profile[0x58+metadata->number]?ForceSpecMoveAccess::Available:ForceSpecMoveAccess::Unavailable;
}
// Read-only native snapshot of one stored instance plus authored UI fields.
// It does not initialize/acknowledge the separate3D actor or its state callback.
class ForceSpecificationSnapshot {
public:
    bool bind(const ForceInventory& owner,const ForceSpecificationCatalog& data,ForceSpecDomain domain,unsigned slot){
        return bind_impl(owner,data,domain,slot,nullptr);
    }
private:
    friend class BorgStatsEditor;
    bool bind_impl(const ForceInventory& owner,const ForceSpecificationCatalog& data,ForceSpecDomain domain,unsigned slot,const ForceBorg* draft){
        *this={};if(!owner.ready()||!data.for_catalog(*owner.catalog()))return false;
        const ForceBorg* value=nullptr;
        if(domain==ForceSpecDomain::Box){if(slot>=kBoxBorgs)return false;value=&owner.state()->box[slot].value;}
        else if(domain==ForceSpecDomain::Warehouse){if(slot>=kWarehouseBorgs)return false;value=&owner.state()->warehouse[slot];}
        else return false;
        if(draft){if(value->model!=draft->model||value->acquired!=draft->acquired)return false;value=draft;}
        if(!value->present()||!data.authored_view(*owner.catalog(),u16(value->model),value->variant,value->level,force_spec_move_access(owner,u16(value->model)),&view_))return false;
        if(value->edits.values[0]>=0)view_.cost=value->edits.values[0];
        if(value->edits.values[1]>=0)view_.hp=value->edits.values[1];
        for(unsigned i=0;i<4;++i)if(value->edits.values[2+i]>=0)view_.ratings[i]=u8(value->edits.values[2+i]);
        value_=*value;scope_=owner.instance_binding();revision_=owner.revision();ready_=true;return true;
    }
public:
    bool current(const ForceInventory& owner)const{return ready_&&owner.ready()&&revision_==owner.revision()&&!scope_.expired()&&scope_.lock()==owner.instance_binding().lock();}
    const ForceSpecAuthoredView* view(const ForceInventory& owner)const{return current(owner)?&view_:nullptr;}
    const ForceBorg* value(const ForceInventory& owner)const{return current(owner)?&value_:nullptr;}
private:
    bool ready_=false;u64 revision_=0;std::weak_ptr<const ForceInventoryScope> scope_;ForceBorg value_;ForceSpecAuthoredView view_;
};
} // namespace gf
