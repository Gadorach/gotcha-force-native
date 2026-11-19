#pragma once
#include "engine/gameplay/borg_package_roster.hpp"

namespace gf {
enum class PackageForceSlotStatus {
    MetadataReady,ScanUnavailable,MissingPackage,IncompletePackage,Conflict,VariantUnspecified,
    CatalogueUnavailable,MissingVariant
};
struct PackageForceSlotView {
    u8 slot=0;
    OwnedPackageBorg saved;
    PackageForceSlotStatus status=PackageForceSlotStatus::CatalogueUnavailable;
    std::optional<u16> declared_cost;
    bool selectable_metadata()const{return !saved.disabled&&status==PackageForceSlotStatus::MetadataReady;}
};
struct PackageForceView {
    unsigned force=0,selectable=0;
    std::vector<PackageForceSlotView> slots;
    // Only a complete, enabled package projection gets a declared total.
    // This excludes retail slots and is not a mode-specific Force budget.
    std::optional<u32> declared_package_cost;
};

// Read-only projection of the exact saved package positions. Never supplies
// a default variant, constructs actors, or consumes a mode admission token.
inline bool package_borg_force_view(const PackageBorgRoster& roster,const BorgPackageCatalog& catalog,
                                     std::span<const BorgCatalogSnapshot> metadata,unsigned force,PackageForceView* out){
    if(!out||force>=kForceCount||!package_borg_roster_valid(roster))return false;
    PackageForceView next;next.force=force;u32 total=0;
    using Status=PackageForceSlotStatus;
    for(const auto& placement:roster.placements){
        if(placement.force!=force)continue;
        const auto& owned=*roster.find(placement.instance);
        PackageForceSlotView row;row.slot=placement.slot;row.saved=owned;
        switch(package_borg_availability(catalog,owned.identity.id)){
        case PackageBorgAvailability::ScanUnavailable:row.status=Status::ScanUnavailable;break;
        case PackageBorgAvailability::Missing:row.status=Status::MissingPackage;break;
        case PackageBorgAvailability::Incomplete:row.status=Status::IncompletePackage;break;
        case PackageBorgAvailability::Conflict:row.status=Status::Conflict;break;
        case PackageBorgAvailability::MissingVariant:row.status=Status::MissingVariant;break;
        case PackageBorgAvailability::Discovered:
            if(!owned.variant){row.status=Status::VariantUnspecified;break;}
            if(const auto* snapshot=package_borg_catalog_snapshot(owned.identity.id,catalog,metadata)){
                if(const auto* variant=snapshot->profile.find(owned.variant->id)){
                    row.status=Status::MetadataReady;row.declared_cost=variant->force_cost;
                }else row.status=Status::MissingVariant;
            }
            break;
        }
        if(row.selectable_metadata()){++next.selectable;total+=*row.declared_cost;}
        next.slots.push_back(std::move(row));
    }
    if(!next.slots.empty()&&next.selectable==next.slots.size())next.declared_package_cost=total;
    *out=std::move(next);return true;
}
}
