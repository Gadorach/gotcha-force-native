#pragma once
#include <compare>
#include <map>
#include "engine/gameplay/borg_package_roster.hpp"

namespace gf {
struct BorgLibraryKey {
    std::string package_id,variant_id;
    auto operator<=>(const BorgLibraryKey&)const=default;
};
enum class BorgLibraryState {
    Catalogued,CatalogNotDeclared,CatalogMissing,CatalogInvalid,CatalogUnsupported,CatalogUnavailable,
    PackageMissing,PackageIncomplete,PackageConflict,ScanUnavailable,VariantUnspecified,VariantMissing
};
inline std::string_view borg_library_state_label(BorgLibraryState state){
    switch(state){
    case BorgLibraryState::Catalogued:return "EXACT VARIANT RESOLVED";
    case BorgLibraryState::CatalogNotDeclared:return "NO CATALOGUE DECLARED";
    case BorgLibraryState::CatalogMissing:return "CATALOGUE MISSING";
    case BorgLibraryState::CatalogInvalid:return "CATALOGUE INVALID";
    case BorgLibraryState::CatalogUnsupported:return "CATALOGUE VERSION UNSUPPORTED";
    case BorgLibraryState::CatalogUnavailable:return "CATALOGUE UNAVAILABLE";
    case BorgLibraryState::PackageMissing:return "PACKAGE MISSING";
    case BorgLibraryState::PackageIncomplete:return "PACKAGE INCOMPLETE";
    case BorgLibraryState::PackageConflict:return "CONFLICTING PACKAGES";
    case BorgLibraryState::ScanUnavailable:return "PACKAGE SCAN UNAVAILABLE";
    case BorgLibraryState::VariantUnspecified:return "SAVED VARIANT UNSPECIFIED";
    case BorgLibraryState::VariantMissing:return "SAVED VARIANT MISSING";
    }
    return "UNAVAILABLE";
}
struct BorgLibraryCatalogIssue {
    std::string package_id;
    BorgLibraryState state=BorgLibraryState::CatalogUnavailable;
};
struct BorgLibraryRow {
    std::optional<BorgLibraryKey> key; // Diagnostic rows are never exact selections.
    BorgIdentity identity;
    std::string variant_label,tribe;
    std::optional<u16> declared_cost;
    BorgLibraryState state=BorgLibraryState::CatalogUnavailable;
    bool preview_declared=false,gameplay_declared=false;
};
struct BorgLibraryOwnedRow {
    OwnedPackageBorg saved; // Instance key and original saved names/label.
    BorgLibraryState state=BorgLibraryState::PackageMissing;
    std::optional<BorgLibraryRow> installed; // Exact current metadata, even if disabled.
};

// Owns a read-only projection of a catalogue scan. Neither a row with a
// gameplay declaration nor an owned instance is proof of battle admission.
class BorgLibrary {
public:
    BorgLibrary(const BorgPackageCatalog& catalog,std::span<const BorgCatalogSnapshot> metadata={},
                std::span<const BorgLibraryCatalogIssue> issues={}):scan_status_(catalog.status){
        if(scan_status_!=BorgPackageScanStatus::Complete)return;
        struct Source {
            const BorgPackageEntry* entry=nullptr;
            const BorgCatalogSnapshot* snapshot=nullptr;
            std::string_view name;
            BorgLibraryState state=BorgLibraryState::CatalogUnavailable;
            bool conflict=false,duplicate_snapshot=false;
        };
        std::map<std::string_view,Source,std::less<>> sources;
        for(const auto& entry:catalog.discovered){
            const auto& id=entry.manifest.identity;
            if(!borg_package_id_valid(id.id))continue;
            auto& source=sources[id.id];
            if(source.entry)source.conflict=true;
            source.entry=&entry;
            source.name=display_name_valid(id.name,96)?std::string_view(id.name):std::string_view(id.id);
            if(entry.manifest.catalog.empty())source.state=BorgLibraryState::CatalogNotDeclared;
        }
        for(const auto& issue:catalog.issues){
            if(issue.kind==BorgPackageIssueKind::InvalidManifest||!borg_package_id_valid(issue.id))continue;
            auto& source=sources[issue.id];
            if(issue.kind==BorgPackageIssueKind::DuplicateId||source.entry)source.conflict=true;
            else if(issue.kind==BorgPackageIssueKind::MissingAsset){
                source.state=BorgLibraryState::PackageIncomplete;
                source.name=display_name_valid(issue.name,96)?std::string_view(issue.name):std::string_view(issue.id);
            }
        }
        for(const auto& issue:issues){
            const auto at=sources.find(issue.package_id);
            if(at==sources.end()||!at->second.entry||at->second.entry->manifest.catalog.empty())continue;
            if(issue.state==BorgLibraryState::CatalogMissing||issue.state==BorgLibraryState::CatalogInvalid||
               issue.state==BorgLibraryState::CatalogUnsupported)
                at->second.state=issue.state;
        }
        for(const auto& snapshot:metadata){
            const auto at=sources.find(snapshot.package.manifest.identity.id);
            if(at==sources.end()||!at->second.entry)continue;
            auto& source=at->second;
            if(source.snapshot)source.duplicate_snapshot=true;
            source.snapshot=&snapshot;
        }
        for(const auto& [id,source]:sources){
            const auto name=source.conflict||source.name.empty()?id:source.name;
            const BorgIdentity identity{std::string(id),std::string(name)};
            const auto* snapshot=source.snapshot;
            if(source.conflict||source.state!=BorgLibraryState::CatalogUnavailable||!snapshot||source.duplicate_snapshot||
               source.entry->folder!=snapshot->package.folder||source.entry->manifest!=snapshot->package.manifest||
               !borg_package_asset_path_valid(source.entry->manifest.catalog)||!display_name_valid(source.entry->manifest.identity.name,96)||
               !valid_profile(snapshot->profile)){
                const auto state=source.conflict?BorgLibraryState::PackageConflict:
                    snapshot&&source.state==BorgLibraryState::CatalogUnavailable?BorgLibraryState::CatalogInvalid:source.state;
                rows_.push_back({{},identity,{},{},{},state});
                continue;
            }
            for(const auto& variant:snapshot->profile.variants){
                const bool base=variant.id==snapshot->profile.default_variant;
                const auto& bindings=snapshot->preview_bindings;
                const auto binding=std::find_if(bindings.begin(),bindings.end(),[&](const auto& row){return row.variant==variant.id;});
                const bool preview=binding!=bindings.end()? !binding->preview.empty():
                    bindings.empty()&&base&&!snapshot->package.manifest.preview.empty();
                rows_.push_back({BorgLibraryKey{identity.id,variant.id},identity,variant.label,snapshot->profile.tribe,
                    variant.force_cost,BorgLibraryState::Catalogued,preview,base&&!snapshot->package.manifest.gameplay.empty()});
            }
        }
    }

    std::span<const BorgLibraryRow> rows()const{return rows_;}
    const BorgLibraryRow* find(std::string_view package_id,std::string_view variant_id)const{
        if(!borg_package_id_valid(package_id)||!borg_catalog_variant_id_valid(variant_id))return nullptr;
        for(auto it=package_begin(package_id);it!=rows_.end()&&it->identity.id==package_id;++it)
            if(it->key&&it->key->variant_id==variant_id)return &*it;
        return nullptr;
    }
    const BorgLibraryRow* find(const BorgLibraryKey& key)const{return find(key.package_id,key.variant_id);}
    std::optional<std::vector<BorgLibraryOwnedRow>> project_owned(const PackageBorgRoster& roster)const{
        if(!package_borg_roster_valid(roster))return {};
        std::vector<BorgLibraryOwnedRow> rows;rows.reserve(roster.owned.size());
        for(const auto& saved:roster.owned){
            BorgLibraryOwnedRow row{saved,BorgLibraryState::PackageMissing,{}};
            const auto package=package_begin(saved.identity.id);
            if(scan_status_!=BorgPackageScanStatus::Complete&&scan_status_!=BorgPackageScanStatus::NoDirectory)
                row.state=BorgLibraryState::ScanUnavailable;
            else if(package!=rows_.end()&&package->identity.id==saved.identity.id){
                if(!package->key)row.state=package->state;
                else if(!saved.variant)row.state=BorgLibraryState::VariantUnspecified;
                else if(const auto* exact=find(saved.identity.id,saved.variant->id)){
                    row.state=exact->state;
                    row.installed=*exact;
                }else row.state=BorgLibraryState::VariantMissing;
            }
            rows.push_back(std::move(row));
        }
        return rows;
    }
private:
    static bool valid_profile(const BorgCatalogProfile& profile){
        if(!display_name_valid(profile.description,512)||(profile.tribe!="none"&&!borg_package_id_valid(profile.tribe))||
           profile.variants.size()>kBorgCatalogVariants||!profile.find(profile.default_variant))return false;
        for(const auto& row:profile.variants){
            if(!borg_catalog_variant_id_valid(row.id)||profile.find(row.id)!=&row||!display_name_valid(row.label,64)||
               row.force_cost>32767||(!row.placeholder_icon()&&!row.unresolved_icon()&&
                   (!row.icon.ends_with(".tpl")||!borg_package_asset_path_valid(row.icon))))return false;
        }
        return true;
    }
    std::vector<BorgLibraryRow>::const_iterator package_begin(std::string_view id)const{
        return std::lower_bound(rows_.begin(),rows_.end(),id,
            [](const auto& row,std::string_view key){return row.identity.id<key;});
    }
    BorgPackageScanStatus scan_status_;
    std::vector<BorgLibraryRow> rows_;
};
}
