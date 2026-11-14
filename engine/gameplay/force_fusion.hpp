#pragma once
#include "engine/gameplay/force_inventory.hpp"
#include "engine/assets/force_supplement.hpp"
namespace gf {
enum class ForceFusionStatus {Ready,Incomplete,Full,Invalid,Stale,NotReady};
class ForceFusionPlan {
public:
    u16 model()const{return model_;}u8 variant()const{return variant_;}
    std::span<const u16> components()const{return {slots_.data(),count_};}
private:
    friend class ForceFusion;
    std::weak_ptr<const ForceInventoryScope> scope_;u64 revision_=0;
    std::shared_ptr<const ForceSupplement> recipe_;u16 model_=0;u8 variant_=0,count_=0;
    std::array<u16,7> slots_{};
};
struct ForceFusionResult {
    bool warehouse=false;s16 slot=-1;ForceBorg borg;
};
class ForceFusion {
public:
    // 801CEE38: scan physical part slots, taking the FIRST occurrence of each
    // distinct component. A different variant never contributes to this set.
    static ForceFusionStatus prepare(const ForceInventory& owner,const ForceSupplement& recipes,
                                     u16 model,u8 variant,ForceFusionPlan* out){
        if(!out)return ForceFusionStatus::Invalid;
        *out={};if(!owner.ready())return ForceFusionStatus::NotReady;
        if(!recipes.for_catalog(*owner.catalog())||!owner.catalog()->find(model)||variant>=6)return ForceFusionStatus::Invalid;
        const auto& state=*owner.state();
        // Callers 80180908/80095E5C check destination capacity BEFORE completion.
        if(state.box_count==kBoxBorgs&&state.warehouse_count==kWarehouseBorgs)return ForceFusionStatus::Full;
        const unsigned required=recipes.required(model);if(required<2)return ForceFusionStatus::Incomplete;
        u32 mask=0;unsigned count=0;ForceFusionPlan plan;
        for(unsigned i=0;i<kWarehouseParts;++i){const auto& p=state.parts[i];
            if(!p.present()||p.model()!=model||p.variant()!=variant)continue;
            // Unqualified oversized source scratch/shift domains are refused.
            if(p.component()>=32)return ForceFusionStatus::Invalid;
            const u32 bit=u32(1)<<p.component();if(mask&bit)continue;
            mask|=bit;if(count<plan.slots_.size())plan.slots_[count]=u16(i);++count;
        }
        if(mask!=(u32(1)<<required)-1||count!=required)return ForceFusionStatus::Incomplete;
        plan.scope_=owner.instance_binding();plan.revision_=owner.revision();
        plan.recipe_=std::make_shared<const ForceSupplement>(recipes);plan.model_=model;plan.variant_=variant;plan.count_=u8(count);
        *out=std::move(plan);return ForceFusionStatus::Ready;
    }
    // 80180908: source recipe-table order, then variants 0..5. One completion at
    // a time, so a caller can present its acquisition before asking for the next.
    static ForceFusionStatus first(const ForceInventory& owner,const ForceSupplement& recipes,ForceFusionPlan* out){
        if(!out)return ForceFusionStatus::Invalid;
        *out={};if(!owner.ready())return ForceFusionStatus::NotReady;
        if(!recipes.for_catalog(*owner.catalog()))return ForceFusionStatus::Invalid;
        if(owner.state()->box_count==kBoxBorgs&&owner.state()->warehouse_count==kWarehouseBorgs)return ForceFusionStatus::Full;
        for(const auto& r:recipes.recipes())if(r.components>1)for(u8 variant=0;variant<6;++variant){
            const auto status=prepare(owner,recipes,r.model,variant,out);if(status!=ForceFusionStatus::Incomplete)return status;
        }
        return ForceFusionStatus::Incomplete;
    }
    // 801CF03C grant-before-consume, with native all-or-nothing publication.
    // The immutable plan is scoped to one inventory revision, not a global list
    // of scratch indices that could consume a later replacement or neighbor.
    static ForceFusionStatus commit(ForceInventory& owner,const ForceFusionPlan& plan,ForceFusionResult* out=nullptr){
        if(out)*out={};
        if(!owner.ready())return ForceFusionStatus::NotReady;
        if(!plan.recipe_||plan.scope_.lock()!=owner.instance_binding().lock()||plan.revision_!=owner.revision())return ForceFusionStatus::Stale;
        ForceFusionPlan checked;const auto status=prepare(owner,*plan.recipe_,plan.model_,plan.variant_,&checked);
        if(status!=ForceFusionStatus::Ready)return status;
        if(plan.count_!=checked.count_||plan.slots_!=checked.slots_)return ForceFusionStatus::Invalid;
        ForceFusionResult result;
        const auto applied=owner.change([&](auto& s){
            result.warehouse=s.box_count==kBoxBorgs;
            if(result.warehouse){for(unsigned i=0;i<kWarehouseBorgs;++i)if(!s.warehouse[i].present()){result.slot=s16(i);break;}}
            else{for(unsigned i=0;i<kBoxBorgs;++i)if(!s.box[i].value.present()){result.slot=s16(i);break;}}
            ForceBorg value;value.model=s16(plan.model_);value.variant=plan.variant_;
            const auto grant=ForceInventory::acquire_into(s,*owner.catalog_,value);
            if(grant!=ForceChange::Applied)return grant;
            for(auto slot:plan.components())ForceInventory::clear(s.parts[slot]);
            result.borg=result.warehouse?s.warehouse[unsigned(result.slot)]:s.box[unsigned(result.slot)].value;
            return ForceChange::Applied;
        });
        if(applied!=ForceChange::Applied)return applied==ForceChange::Full?ForceFusionStatus::Full:ForceFusionStatus::Invalid;
        if(out)*out=result;
        return ForceFusionStatus::Ready;
    }
};
} // namespace gf
