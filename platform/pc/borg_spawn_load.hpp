#pragma once
#include "engine/assets/borg_spawn_profile.hpp"
#include "platform/pc/borg_catalog_load.hpp"

namespace gf::pc {
struct BorgSpawnInspection {
    BorgIdentity identity;
    BorgSpawnProfile profile;
};
inline bool load_borg_spawn_inspection(const std::filesystem::path& root,const BorgPackageCatalog& scanned,
                                       std::string_view id,BorgSpawnInspection* out,std::string* error){
    auto fail=[&](const char* why){if(error)*error=why;return false;};
    const auto* package=scanned.status==BorgPackageScanStatus::Complete?scanned.find(id):nullptr;
    if(!out||!package)return fail("a uniquely discovered Borg ID is required");
    std::filesystem::path folder;
    BorgCatalogProfile catalogue;
    if(!borg_catalog_detail::read_profile(root,*package,&folder,&catalogue,error))return false;
    if(!catalogue.details)return fail("a version 2 catalogue profile is required for spawn policies");
    const auto& reference=catalogue.details->references[unsigned(BorgDefinitionFacet::Spawn)];
    if(reference.kind==BorgDefinitionReferenceKind::None)return fail("package declares no spawn override");
    if(reference.kind!=BorgDefinitionReferenceKind::LocalFile)return fail("spawn policy is unresolved");
    BorgSpawnInspection next;next.identity=package->manifest.identity;
    std::string bytes;
    if(!borg_package_scan_detail::read_local(folder,reference.file,kBorgSpawnProfileBytes,&bytes)||
       !borg_read_spawn_profile(bytes,&next.profile))return fail("missing or invalid spawn policy file");
    for(const auto& rule:next.profile.rules)
        if(rule.variant!="*"&&!catalogue.find(rule.variant))return fail("spawn policy references an undeclared variant");
    *out=std::move(next);if(error)error->clear();return true;
}
}
