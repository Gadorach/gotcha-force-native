#pragma once
#include "engine/save/account_save_repository.hpp"
#include "engine/save/borg_package_save.hpp"
#include "engine/save/force_inventory.hpp"

namespace gf {
enum class BorgPlacementResult {
    Ready,Saved,Invalid,OwnedUnavailable,Disabled,VariantUnavailable,AlreadyMember,Occupied,
    RetailUnverified,SaveUnavailable,NotConfirmed,Stale,SaveUnconfirmed
};
struct BorgPlacementOffer {
    OwnedPackageBorg owned;
    unsigned force=0,slot=0;
    u16 declared_cost=0;
};

// One explicit placement of an already owned package instance. This edits the
// package roster only; it never acquires a Borg, replaces a position, or
// changes the retail inventory. The selected account backend guards both
// records with the same save revision, including at the atomic replacement.
class BorgPackagePlacementSession {
public:
    explicit BorgPackagePlacementSession(AccountSaveBackend& backend):backend_(backend),store_(backend){}
    BorgPackagePlacementSession(const BorgPackagePlacementSession&)=delete;
    BorgPackagePlacementSession& operator=(const BorgPackagePlacementSession&)=delete;
    const BorgPlacementOffer* offer()const{return pending_?&pending_->offer:nullptr;}
    void cancel(){pending_.reset();}

    BorgPlacementResult prepare(u64 instance,unsigned force,unsigned slot,const ForceCatalog* retail_catalogue,
                                const BorgPackageCatalog& packages,std::span<const BorgCatalogSnapshot> metadata={}){
        cancel();
        if(!instance||force>=kForceCount||slot>=kForceSlots)return BorgPlacementResult::Invalid;
        Pending next;
        const auto loaded=store_.load(&next.before);
        if(loaded==PackageBorgLoad::Missing)return BorgPlacementResult::OwnedUnavailable;
        if(loaded!=PackageBorgLoad::Loaded)return BorgPlacementResult::SaveUnavailable;
        ForceInventory retail;
        if(!read_retail(retail_catalogue,&next.retail_record,&retail))return BorgPlacementResult::RetailUnverified;
        PackageBorgRoster candidate;
        const auto result=make_candidate(next.before,instance,force,slot,retail,packages,metadata,
            &next.offer,&next.package,&next.variant,&candidate);
        if(result==BorgPlacementResult::Ready)pending_=std::move(next);
        return result;
    }

    BorgPlacementResult confirm(const ForceCatalog* retail_catalogue,const BorgPackageCatalog& packages,
                                std::span<const BorgCatalogSnapshot> metadata={}){
        if(!pending_)return BorgPlacementResult::NotConfirmed;
        const auto checked=std::move(*pending_);pending_.reset(); // one explicit confirmation, never retry
        ForceInventory retail;std::vector<u8> record;
        if(!read_retail(retail_catalogue,&record,&retail)||record!=checked.retail_record)return BorgPlacementResult::Stale;
        PackageBorgRoster candidate;BorgPlacementOffer current;BorgPackageEntry package;BorgCatalogVariant variant;
        if(make_candidate(checked.before,checked.offer.owned.instance,checked.offer.force,checked.offer.slot,
            retail,packages,metadata,&current,&package,&variant,&candidate)!=BorgPlacementResult::Ready||
           package.folder!=checked.package.folder||package.manifest!=checked.package.manifest||
           variant.id!=checked.variant.id||variant.label!=checked.variant.label||
           variant.icon!=checked.variant.icon||variant.force_cost!=checked.variant.force_cost)
            return BorgPlacementResult::Stale;
        if(!store_.current())return BorgPlacementResult::Stale;
        if(!store_.write(candidate))return BorgPlacementResult::SaveUnconfirmed;
        return BorgPlacementResult::Saved;
    }
private:
    struct Pending {
        PackageBorgRoster before;
        std::vector<u8> retail_record;
        BorgPlacementOffer offer;
        BorgPackageEntry package;
        BorgCatalogVariant variant;
    };
    bool read_retail(const ForceCatalog* catalogue,std::vector<u8>* record,ForceInventory* out){
        const auto state=backend_.read_slot(0,*record);
        if(state==SaveReadStatus::Missing){record->clear();return true;}
        if(state!=SaveReadStatus::Loaded||record->empty()||record->size()>kAccountObjectBytes||!catalogue)return false;
        ForceInventoryStore reader(backend_);
        if(reader.load(*catalogue,out)!=ForceInventoryLoad::Loaded)return false;
        std::vector<u8> checked;
        return backend_.read_slot(0,checked)==SaveReadStatus::Loaded&&checked==*record;
    }
    static BorgPlacementResult make_candidate(const PackageBorgRoster& before,u64 instance,unsigned force,unsigned slot,
                                             const ForceInventory& retail,const BorgPackageCatalog& packages,
                                             std::span<const BorgCatalogSnapshot> metadata,BorgPlacementOffer* offer,
                                             BorgPackageEntry* package,BorgCatalogVariant* variant,PackageBorgRoster* out){
        const auto* owned=before.find(instance);
        if(!owned)return BorgPlacementResult::OwnedUnavailable;
        if(owned->disabled)return BorgPlacementResult::Disabled;
        const auto* snapshot=package_borg_catalog_snapshot(owned->identity.id,packages,metadata);
        const auto* selected=snapshot&&owned->variant?snapshot->profile.find(owned->variant->id):nullptr;
        if(!selected)return BorgPlacementResult::VariantUnavailable;
        if(retail.ready()&&retail.state()->forces[force].slots[slot]>=0)return BorgPlacementResult::Occupied;
        bool already_member=false;
        for(const auto& row:before.placements)if(row.force==force){
            if(row.slot==slot)return BorgPlacementResult::Occupied;
            if(row.instance==instance)already_member=true;
        }
        if(already_member)return BorgPlacementResult::AlreadyMember;
        auto candidate=before;
        const auto index=force*kForceSlots+slot;
        const auto at=std::lower_bound(candidate.placements.begin(),candidate.placements.end(),index,
            [](const auto& row,unsigned key){return unsigned(row.force)*kForceSlots+row.slot<key;});
        candidate.placements.insert(at,{instance,u8(force),u8(slot)});
        if(!package_borg_roster_valid(candidate))return BorgPlacementResult::Invalid;
        *offer={*owned,force,slot,selected->force_cost};*package=snapshot->package;*variant=*selected;
        *out=std::move(candidate);return BorgPlacementResult::Ready;
    }
    AccountSaveBackend& backend_;
    PackageBorgSaveStore store_;
    std::optional<Pending> pending_;
};
}
