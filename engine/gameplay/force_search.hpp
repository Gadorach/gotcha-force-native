#pragma once
// 801CD060 filter and 801CD1B4 pairwise interchange sort. Sorting a view never
// reorders inventory storage. Original order among full ties is not stable.
#include "engine/gameplay/force_inventory.hpp"
#include "engine/assets/force_supplement.hpp"
namespace gf {
enum class ForceListDomain:u8 {Box,Warehouse,Components};
enum class ForceSortKey:u8 {Storage,Number,Acquired,LevelExperience,Cost,Rarity,Name};
struct ForceSortTerm {ForceSortKey key=ForceSortKey::Storage;bool descending=false;};
using ForceSortSpec=std::array<ForceSortTerm,4>;
struct ForceSearchList {
    ForceListDomain domain=ForceListDomain::Box;std::vector<s16> slots;unsigned count=0;u64 revision=0;u32 tribes=0;
    std::weak_ptr<const ForceInventoryScope> scope;
    bool current(const ForceInventory& owner)const{return owner.ready()&&scope.lock()==owner.instance_binding().lock()&&revision==owner.revision();}
};
inline bool force_search(const ForceInventory& owner,ForceListDomain domain,u32 tribes,
                         const ForceSortSpec& sort,const ForceSupplement* names,ForceSearchList* out,
                         const ForceSearchList* previous_order=nullptr){
    if(!out)return false;
    *out={};if(!owner.ready()||unsigned(domain)>2)return false;
    bool need_name=false;for(const auto& term:sort){if(unsigned(term.key)>6)return false;need_name|=term.key==ForceSortKey::Name;}
    if(domain==ForceListDomain::Components&&need_name&&(!names||!names->for_catalog(*owner.catalog())))return false;
    const auto& s=*owner.state();const auto& catalog=*owner.catalog();
    const unsigned capacity=domain==ForceListDomain::Box?kBoxBorgs:domain==ForceListDomain::Warehouse?kWarehouseBorgs:kWarehouseParts;
    struct Keys {std::array<s32,6> values{};std::string name;};
    std::vector<Keys> keys(capacity);ForceSearchList result;result.domain=domain;result.tribes=tribes;result.slots.assign(capacity,-1);result.scope=owner.instance_binding();result.revision=owner.revision();
    for(unsigned i=0;i<capacity;++i){const bool part=domain==ForceListDomain::Components;ForceBorg b;
        if(part){const auto& p=s.parts[i];if(!p.present())continue;b.model=s16(p.model());b.variant=p.variant();b.acquired=p.acquired;}
        else{b=domain==ForceListDomain::Box?s.box[i].value:s.warehouse[i];if(!b.present())continue;}
        const auto* metadata=catalog.find(u16(b.model));if(!metadata)return false;
        if(!(tribes&(u32(1)<<metadata->tribe)))continue;
        auto& key=keys[i];key.values[0]=s32(i);key.values[2]=std::bit_cast<s32>(b.acquired);
        if(part){
            const auto component=std::bit_cast<s8>(s.parts[i].component());
            key.values[1]=std::bit_cast<s32>(u32(s32(component))|(u32(b.variant)<<8)|(u32(metadata->number)<<16));
            key.values[5]=metadata->rarity[b.variant];
            if(need_name&&!names->part_name(catalog,u16(b.model),b.variant,s.parts[i].component(),&key.name))return false;
        }else{
            key.values[1]=s32((u32(metadata->number)<<8)|b.variant);
            key.values[3]=std::bit_cast<s32>((u32(b.level)<<27)|(b.experience&0x07ffffffu));
            key.values[4]=force_borg_cost(*owner.catalog(),b);key.values[5]=metadata->rarity[b.variant];
            if(need_name)key.name=metadata->source_name;
        }
        result.slots[result.count++]=s16(i);
    }
    // Sort-menu changes operate on the CURRENT permutation. Rebuilding from
    // storage here would change full-tie order in this nonstable source sort.
    if(previous_order){
        if(!previous_order->current(owner)||previous_order->domain!=domain||previous_order->tribes!=tribes||previous_order->count!=result.count||previous_order->slots.size()!=capacity)return false;
        std::vector<bool> eligible(capacity,false);
        for(unsigned i=0;i<result.count;++i)eligible[unsigned(result.slots[i])]=true;
        for(unsigned i=0;i<capacity;++i){const auto slot=previous_order->slots[i];if(i>=result.count){if(slot!=-1)return false;continue;}if(slot<0||unsigned(slot)>=capacity||!eligible[unsigned(slot)])return false;eligible[unsigned(slot)]=false;}
        result.slots=previous_order->slots;
    }
    auto compare_names=[](const std::string& a,const std::string& b){
        for(unsigned i=0;i<std::min(a.size(),b.size());++i){const auto x=u8(a[i]),y=u8(b[i]);if(x!=y)return x<y?-1:1;}
        return a.size()==b.size()?0:a.size()<b.size()?-1:1;
    };
    for(unsigned i=0;i<result.count;++i)for(unsigned j=i+1;j<result.count;++j){
        const auto& a=keys[unsigned(result.slots[i])];const auto& b=keys[unsigned(result.slots[j])];
        for(const auto& term:sort){const auto k=unsigned(term.key);const int difference=k==6?compare_names(a.name,b.name):a.values[k]==b.values[k]?0:a.values[k]<b.values[k]?-1:1;
            if(difference){if(term.descending?difference<0:difference>0)std::swap(result.slots[i],result.slots[j]);break;}}
    }
    *out=std::move(result);return true;
}
inline bool force_resort(const ForceInventory& owner,const ForceSortSpec& sort,const ForceSupplement* names,ForceSearchList* view){
    if(!view)return false;
    const auto previous=*view;
    return force_search(owner,previous.domain,previous.tribes,sort,names,view,&previous);
}
} // namespace gf
