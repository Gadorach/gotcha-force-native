#pragma once
#include "engine/assets/borg_package.hpp"
#include "engine/gameplay/force_inventory.hpp"

namespace gf {
struct OwnedPackageBorg {
    u64 instance=0;
    BorgIdentity identity;
    bool disabled=false;
    bool operator==(const OwnedPackageBorg&)const=default;
};
struct PackageBorgPlacement {
    u64 instance=0;
    u8 force=0,slot=0;
    bool operator==(const PackageBorgPlacement&)const=default;
};
// Only packaged entries and their reserved positions, not a second Force
// inventory. Mode readiness and conflicts with retail slots are checked
// by the eventual mixed-roster owner, not by this persistence boundary.
struct PackageBorgRoster {
    std::vector<OwnedPackageBorg> owned; // Ordered by nonzero instance ID.
    std::vector<PackageBorgPlacement> placements; // Ordered by force, then slot.
    bool operator==(const PackageBorgRoster&)const=default;
    const OwnedPackageBorg* find(u64 instance)const{
        const auto it=std::lower_bound(owned.begin(),owned.end(),instance,
            [](const auto& value,u64 id){return value.instance<id;});
        return it!=owned.end()&&it->instance==instance?&*it:nullptr;
    }
};
inline bool package_borg_roster_valid(const PackageBorgRoster& roster){
    if(roster.owned.size()>kBoxBorgs+kWarehouseBorgs||roster.placements.size()>kForceCount*kForceSlots)return false;
    u64 previous_id=0;
    for(const auto& borg:roster.owned){
        if(borg.instance<=previous_id||!borg_package_id_valid(borg.identity.id)||!display_name_valid(borg.identity.name,96))return false;
        previous_id=borg.instance;
    }
    int previous_slot=-1;
    std::array<u32,kBoxBorgs+kWarehouseBorgs> used{};
    static_assert(kForceCount<=32);
    for(const auto& place:roster.placements){
        const int order=int(place.force)*int(kForceSlots)+place.slot;
        if(place.force>=kForceCount||place.slot>=kForceSlots||order<=previous_slot)return false;
        const auto* borg=roster.find(place.instance);
        if(!borg)return false;
        auto& mask=used[std::size_t(borg-roster.owned.data())];
        const auto bit=u32{1}<<place.force;
        if(mask&bit)return false;
        mask|=bit;previous_slot=order;
    }
    return true;
}
enum class PackageBorgAvailability {Discovered,Missing,Incomplete,Conflict,ScanUnavailable};

inline PackageBorgAvailability package_borg_availability(const BorgPackageCatalog& catalog,std::string_view id){
    using State=PackageBorgAvailability;
    if(catalog.status!=BorgPackageScanStatus::Complete&&catalog.status!=BorgPackageScanStatus::NoDirectory)
        return State::ScanUnavailable;
    if(catalog.find(id))return State::Discovered;
    for(const auto& issue:catalog.issues)if(issue.id==id){
        if(issue.kind==BorgPackageIssueKind::DuplicateId)return State::Conflict;
        if(issue.kind==BorgPackageIssueKind::MissingAsset)return State::Incomplete;
    }
    return State::Missing;
}
struct MissingPackageBorg {
    OwnedPackageBorg saved;
    PackageBorgAvailability reason=PackageBorgAvailability::Missing;
    std::vector<PackageBorgPlacement> affected;
    bool operator==(const MissingPackageBorg&)const=default;
};
inline bool package_borg_missing(const PackageBorgRoster& roster,const BorgPackageCatalog& catalog,
                                 std::vector<MissingPackageBorg>* out){
    if(!out||!package_borg_roster_valid(roster)||
        (catalog.status!=BorgPackageScanStatus::Complete&&catalog.status!=BorgPackageScanStatus::NoDirectory))return false;
    std::vector<MissingPackageBorg> next;
    for(const auto& borg:roster.owned){
        const auto reason=package_borg_availability(catalog,borg.identity.id);
        if(reason==PackageBorgAvailability::Discovered)continue;
        MissingPackageBorg missing{borg,reason,{}};
        for(const auto& place:roster.placements)if(place.instance==borg.instance)missing.affected.push_back(place);
        next.push_back(std::move(missing));
    }
    *out=std::move(next);return true;
}

enum class MissingPackageChoice {DisableInForces,DeleteFromSave};
// Build a candidate only after an explicit choice on the still-current notice.
// Neither this function nor generating a notice writes a save or package.
inline bool package_borg_resolve_missing(const PackageBorgRoster& roster,const BorgPackageCatalog& catalog,
                                         const MissingPackageBorg& notice,MissingPackageChoice choice,PackageBorgRoster* out){
    if(!out||(notice.reason!=PackageBorgAvailability::Missing&&notice.reason!=PackageBorgAvailability::Incomplete))return false;
    std::vector<MissingPackageBorg> current;
    if(!package_borg_missing(roster,catalog,&current)||std::find(current.begin(),current.end(),notice)==current.end())return false;
    auto next=roster;const auto instance=notice.saved.instance;
    if(choice==MissingPackageChoice::DisableInForces){
        for(auto& borg:next.owned)if(borg.instance==instance)borg.disabled=true;
    }else if(choice==MissingPackageChoice::DeleteFromSave){
        std::erase_if(next.placements,[&](const auto& place){return place.instance==instance;});
        std::erase_if(next.owned,[&](const auto& borg){return borg.instance==instance;});
    }else return false;
    *out=std::move(next);return true;
}
// Clearing the saved disable flag is not actor admission. Loaders and modes
// still need to validate the discovered package before using it.
inline bool package_borg_enable_restored(const PackageBorgRoster& roster,const BorgPackageCatalog& catalog,
                                          const OwnedPackageBorg& expected,PackageBorgRoster* out){
    if(!out||!expected.disabled||!package_borg_roster_valid(roster)||
        package_borg_availability(catalog,expected.identity.id)!=PackageBorgAvailability::Discovered)return false;
    const auto* saved=roster.find(expected.instance);
    if(!saved||*saved!=expected)return false;
    auto next=roster;next.owned[std::size_t(saved-roster.owned.data())].disabled=false;
    *out=std::move(next);return true;
}
}
