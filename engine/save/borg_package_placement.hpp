#pragma once
#include "engine/gameplay/borg_library.hpp"
#include "engine/gameplay/borg_mixed_force_view.hpp"
#include "engine/save/borg_package_save.hpp"
#include "engine/save/scoped_retail_force_record.hpp"

namespace gf {
enum class BorgPlacementResult {
    Ready,Saved,Invalid,OwnedUnavailable,Disabled,VariantUnavailable,AlreadyMember,Occupied,
    RetailUnverified,SaveUnavailable,NotConfirmed,Stale,SaveUnconfirmed,
    ForceConflict,CostUnverified,OverBudget,EditedRetail
};
// Set by the owning engine mode, never by a package or presentation script.
// The standalone arranger has no mode budget. It does not admit a battle.
struct BorgPlacementPolicy {
    std::optional<u32> budget;
    ForceCostMode cost_mode=ForceCostMode::Full;
    bool allow_edited_retail=false;
    bool operator==(const BorgPlacementPolicy&)const=default;
};
struct BorgPlacementOffer {
    OwnedPackageBorg owned;
    unsigned force=0,slot=0;
    u16 declared_cost=0;
    std::optional<std::string> retail_force_name;
    u32 before_cost=0,after_cost=0; // mode-adjusted, disabled copies excluded
    BorgPlacementPolicy policy;
    bool operator==(const BorgPlacementOffer&)const=default;
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
                                const BorgPackageCatalog& packages,std::span<const BorgCatalogSnapshot> metadata={},
                                const BorgPlacementPolicy& policy={}){
        cancel();
        if(!instance||force>=kForceCount||slot>=kForceSlots)return BorgPlacementResult::Invalid;
        Pending next;
        const auto revision=backend_.revision();
        if(!revision)return BorgPlacementResult::SaveUnavailable;
        next.revision=*revision;
        const auto loaded=store_.load(&next.before);
        if(loaded==PackageBorgLoad::Missing)return BorgPlacementResult::OwnedUnavailable;
        if(loaded!=PackageBorgLoad::Loaded)return BorgPlacementResult::SaveUnavailable;
        ForceInventory retail;
        if(!read_scoped_retail_force_record(backend_,retail_catalogue,&next.retail_record,&retail))return BorgPlacementResult::RetailUnverified;
        PackageBorgRoster candidate;
        const auto result=make_candidate(next.before,instance,force,slot,retail,packages,metadata,
            policy,&next.offer,&next.cost_basis,&candidate);
        if(backend_.revision()!=revision)return BorgPlacementResult::Stale;
        if(result==BorgPlacementResult::Ready)pending_=std::move(next);
        return result;
    }

    BorgPlacementResult confirm(const ForceCatalog* retail_catalogue,const BorgPackageCatalog& packages,
                                std::span<const BorgCatalogSnapshot> metadata={},const BorgPlacementPolicy& policy={}){
        if(!pending_)return BorgPlacementResult::NotConfirmed;
        const auto checked=std::move(*pending_);pending_.reset(); // one explicit confirmation, never retry
        if(backend_.revision()!=checked.revision||policy!=checked.offer.policy)return BorgPlacementResult::Stale;
        ForceInventory retail;std::vector<u8> record;
        if(!read_scoped_retail_force_record(backend_,retail_catalogue,&record,&retail)||record!=checked.retail_record)
            return BorgPlacementResult::Stale;
        PackageBorgRoster candidate;BorgPlacementOffer current;CostBasis basis;
        if(make_candidate(checked.before,checked.offer.owned.instance,checked.offer.force,checked.offer.slot,
            retail,packages,metadata,policy,&current,&basis,&candidate)!=BorgPlacementResult::Ready||
           current!=checked.offer||basis!=checked.cost_basis)
            return BorgPlacementResult::Stale;
        if(!store_.current()||backend_.revision()!=checked.revision)return BorgPlacementResult::Stale;
        if(!store_.write_scoped(candidate,backend_,checked.revision))return BorgPlacementResult::SaveUnconfirmed;
        return BorgPlacementResult::Saved;
    }
private:
    struct PackagedCost {
        BorgPackageEntry package;
        BorgCatalogVariant variant;
        std::string tribe;
        bool operator==(const PackagedCost&)const=default;
    };
    struct CostBasis {
        std::vector<PackagedCost> packaged;
        std::vector<std::pair<u8,RetailForceSlotView>> retail;
        bool operator==(const CostBasis&)const=default;
    };
    struct Pending {
        AccountSaveRevision revision;
        PackageBorgRoster before;
        std::vector<u8> retail_record;
        BorgPlacementOffer offer;
        CostBasis cost_basis;
    };
    static BorgPlacementResult make_candidate(const PackageBorgRoster& before,u64 instance,unsigned force,unsigned slot,
                                              const ForceInventory& retail,const BorgPackageCatalog& packages,
                                              std::span<const BorgCatalogSnapshot> metadata,const BorgPlacementPolicy& policy,
                                              BorgPlacementOffer* offer,CostBasis* basis,PackageBorgRoster* out){
        using Result=BorgPlacementResult;
        if(policy.cost_mode!=ForceCostMode::Full&&policy.cost_mode!=ForceCostMode::TwoThirds)return Result::Invalid;
        const auto* owned=before.find(instance);
        if(!owned)return Result::OwnedUnavailable;
        if(owned->disabled)return Result::Disabled;
        const BorgLibrary library(packages,metadata);
        const auto* exact=owned->variant?library.find(owned->identity.id,owned->variant->id):nullptr;
        const auto* snapshot=package_borg_catalog_snapshot(owned->identity.id,packages,metadata);
        const auto* selected=snapshot&&owned->variant?snapshot->profile.find(owned->variant->id):nullptr;
        if(!selected||!exact||exact->declared_cost!=selected->force_cost)return Result::VariantUnavailable;
        PackageForceView package;
        MixedBorgForceView mixed;
        if(!package_borg_force_view(before,packages,metadata,force,&package)||
           !borg_mixed_force_view(package,retail.ready()?&retail:nullptr,&mixed))return Result::Invalid;
        if(mixed.find_slot(slot))return Result::Occupied;
        if(mixed.overlaps)return Result::ForceConflict;
        for(const auto& row:mixed.slots)if(row.packaged&&row.packaged->saved.instance==instance)return Result::AlreadyMember;
        u32 total=0;
        CostBasis checked;
        checked.packaged.push_back({snapshot->package,*selected,exact->tribe});
        for(const auto& row:mixed.slots){
            if(row.retail){
                if(row.retail->value.edits.modified&&!policy.allow_edited_retail)return Result::EditedRetail;
                checked.retail.emplace_back(row.slot,*row.retail);
                total+=u32(force_cost_for_mode(s32(row.retail->declared_cost),policy.cost_mode));
            }
            if(row.packaged&&!row.packaged->saved.disabled){
                const auto& saved=row.packaged->saved;
                const auto* entry=saved.variant?library.find(saved.identity.id,saved.variant->id):nullptr;
                const auto* current=package_borg_catalog_snapshot(saved.identity.id,packages,metadata);
                const auto* variant=current&&saved.variant?current->profile.find(saved.variant->id):nullptr;
                if(!variant||!entry||!row.packaged->selectable_metadata()||entry->declared_cost!=variant->force_cost||
                   row.packaged->declared_cost!=variant->force_cost)
                    return Result::CostUnverified;
                checked.packaged.push_back({current->package,*variant,entry->tribe});
                total+=u32(force_cost_for_mode(variant->force_cost,policy.cost_mode));
            }
        }
        const auto after=total+u32(force_cost_for_mode(selected->force_cost,policy.cost_mode));
        if(policy.budget&&after>*policy.budget)return Result::OverBudget;
        auto candidate=before;
        const auto index=force*kForceSlots+slot;
        const auto at=std::lower_bound(candidate.placements.begin(),candidate.placements.end(),index,
            [](const auto& row,unsigned key){return unsigned(row.force)*kForceSlots+row.slot<key;});
        candidate.placements.insert(at,{instance,u8(force),u8(slot)});
        if(!package_borg_roster_valid(candidate))return Result::Invalid;
        *offer={*owned,force,slot,selected->force_cost,mixed.retail_source_name,total,after,policy};
        *basis=std::move(checked);
        *out=std::move(candidate);return Result::Ready;
    }
    AccountSaveBackend& backend_;
    PackageBorgSaveStore store_;
    std::optional<Pending> pending_;
};
}
