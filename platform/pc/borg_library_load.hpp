#pragma once
#include "engine/gameplay/borg_library.hpp"
#include "platform/pc/borg_catalog_load.hpp"

namespace gf::pc {
struct BorgLibraryMetadata {
    std::vector<BorgCatalogSnapshot> snapshots;
    std::vector<BorgLibraryCatalogIssue> issues;
};

// The scanner bounds directory count; each read stays inside existing
// catalogue/override limits. No icon, model or gameplay bytes are loaded.
inline BorgLibraryMetadata load_borg_library_metadata(const std::filesystem::path& root,const BorgPackageCatalog& catalog){
    BorgLibraryMetadata next;
    if(catalog.status!=BorgPackageScanStatus::Complete)return next;
    for(const auto& package:catalog.discovered){
        const auto& id=package.manifest.identity.id;
        BorgCatalogSnapshot snapshot;BorgCatalogLoadIssue issue;
        if(load_borg_catalog_snapshot(root,catalog,id,&snapshot,nullptr,&issue))next.snapshots.push_back(std::move(snapshot));
        else if(issue==BorgCatalogLoadIssue::Missing)next.issues.push_back({id,BorgLibraryState::CatalogMissing});
        else if(issue==BorgCatalogLoadIssue::Invalid)next.issues.push_back({id,BorgLibraryState::CatalogInvalid});
        else if(issue==BorgCatalogLoadIssue::Unsupported)next.issues.push_back({id,BorgLibraryState::CatalogUnsupported});
    }
    return next;
}
}
