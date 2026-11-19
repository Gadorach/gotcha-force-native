#pragma once
#include "engine/assets/borg_move_profile.hpp"
#include "platform/pc/borg_catalog_load.hpp"

namespace gf::pc {
struct BorgMoveInspection {
    BorgIdentity identity;
    BorgMoveProfile profile;
};
inline bool load_borg_move_inspection(const std::filesystem::path& root,const BorgPackageCatalog& scanned,
                                      std::string_view id,BorgMoveInspection* out,std::string* error){
    auto fail=[&](const char* why){if(error)*error=why;return false;};
    const auto* package=scanned.status==BorgPackageScanStatus::Complete?scanned.find(id):nullptr;
    if(!out||!package)return fail("a uniquely discovered Borg ID is required");
    std::filesystem::path folder;
    BorgCatalogProfile catalogue;
    if(!borg_catalog_detail::read_profile(root,*package,&folder,&catalogue,error))return false;
    if(!catalogue.details)return fail("a version 2 catalogue profile is required for move definitions");
    const auto& reference=catalogue.details->references[unsigned(BorgDefinitionFacet::Moves)];
    if(reference.kind==BorgDefinitionReferenceKind::None)return fail("package declares no move definitions");
    if(reference.kind!=BorgDefinitionReferenceKind::LocalFile)return fail("move definitions are unresolved");
    BorgMoveInspection next;next.identity=package->manifest.identity;
    std::string bytes;
    if(!borg_package_scan_detail::read_local(folder,reference.file,kBorgMoveProfileBytes,&bytes)||
       !borg_read_move_profile(bytes,&next.profile))return fail("missing or invalid move definition file");
    *out=std::move(next);if(error)error->clear();return true;
}
}
