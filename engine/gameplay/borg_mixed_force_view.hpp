#pragma once
#include "engine/gameplay/borg_package_force_view.hpp"

namespace gf {
struct RetailForceSlotView {
    u16 box_slot=0,collection_number=0;
    ForceBorg value;
    std::string source_name; // original encoding, not necessarily UTF-8
    u32 declared_cost=0;
};
struct MixedBorgForceSlotView {
    u8 slot=0;
    std::optional<RetailForceSlotView> retail;
    std::optional<PackageForceSlotView> packaged;
    bool overlap()const{return retail&&packaged;}
};
struct MixedBorgForceView {
    unsigned force=0,overlaps=0;
    bool retail_record_present=false;
    std::vector<MixedBorgForceSlotView> slots;
    // Inspection only. Not a cost checked against any mode's rules.
    std::optional<u32> declared_cost;
    const MixedBorgForceSlotView* find_slot(unsigned slot)const{
        const auto it=std::lower_bound(slots.begin(),slots.end(),slot,[](const auto& row,unsigned index){return row.slot<index;});
        return it!=slots.end()&&it->slot==slot?&*it:nullptr;
    }
};

// Combine two already-read views of one selected save. Null retail is for
// a proven absent retail record, never an unreadable or mismatched one.
// Retain both claims on overlap; nothing here admits a member to battle.
inline bool borg_mixed_force_view(const PackageForceView& packages,const ForceInventory* retail,MixedBorgForceView* out){
    if(!out||packages.force>=kForceCount||packages.slots.size()>kForceSlots||(retail&&!retail->ready()))return false;
    MixedBorgForceView next;next.force=packages.force;next.retail_record_present=retail!=nullptr;
    std::array<const PackageForceSlotView*,kForceSlots> package_slots{};
    for(const auto& row:packages.slots){
        if(row.slot>=kForceSlots||package_slots[row.slot])return false;
        package_slots[row.slot]=&row;
    }
    const auto* original=retail?&retail->state()->forces[packages.force]:nullptr;
    bool complete=true;u32 total=0;
    for(unsigned i=0;i<kForceSlots;++i){
        const auto* pkg=package_slots[i];const s16 box_slot=original?original->slots[i]:-1;
        if(!pkg&&box_slot<0)continue;
        MixedBorgForceSlotView row;row.slot=u8(i);
        if(pkg){
            row.packaged=*pkg;
            if(!pkg->selectable_metadata()||!pkg->declared_cost)complete=false;
            else total+=*pkg->declared_cost;
        }
        if(box_slot>=0){
            if(unsigned(box_slot)>=kBoxBorgs)return false;
            const auto& value=retail->state()->box[unsigned(box_slot)].value;
            const auto* meta=retail->catalog()->find(u16(value.model));
            if(!value.present()||!meta)return false;
            const auto cost=force_borg_cost(*retail->catalog(),value);
            if(cost<0)return false;
            row.retail=RetailForceSlotView{u16(box_slot),meta->number,value,meta->source_name,u32(cost)};
            total+=u32(cost);
        }
        if(row.overlap()){++next.overlaps;complete=false;}
        next.slots.push_back(std::move(row));
    }
    if(complete&&!next.slots.empty())next.declared_cost=total;
    *out=std::move(next);return true;
}
}
