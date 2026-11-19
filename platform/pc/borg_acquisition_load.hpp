#pragma once
#include "engine/assets/borg_acquisition_profile.hpp"
#include "platform/pc/borg_catalog_load.hpp"

namespace gf::pc {
struct BorgAcquisitionInspection {
    BorgIdentity identity;
    BorgAcquisitionProfile profile;
};
inline bool load_borg_acquisition_inspection(const std::filesystem::path& root,const BorgPackageCatalog& scanned,
                                             std::string_view id,BorgAcquisitionInspection* out,std::string* error){
    auto fail=[&](std::string why){if(error)*error=std::move(why);return false;};
    const auto* package=scanned.status==BorgPackageScanStatus::Complete?scanned.find(id):nullptr;
    if(!out||!package)return fail("a uniquely discovered Borg ID is required");
    std::filesystem::path folder;
    BorgCatalogProfile catalogue;
    if(!borg_catalog_detail::read_profile(root,*package,&folder,&catalogue,error))return false;
    if(!catalogue.details)return fail("a version 2 catalogue profile is required for acquisition");
    const auto& reference=catalogue.details->references[unsigned(BorgDefinitionFacet::Acquisition)];
    if(reference.kind==BorgDefinitionReferenceKind::None)return fail("package declares no acquisition routes");
    if(reference.kind!=BorgDefinitionReferenceKind::LocalFile)return fail("acquisition is unresolved");
    BorgAcquisitionInspection next;next.identity=package->manifest.identity;
    std::string bytes;
    if(!borg_package_scan_detail::read_local(folder,reference.file,kBorgAcquisitionBytes,&bytes)||
       !borg_read_acquisition_profile(bytes,&next.profile))return fail("missing or invalid acquisition definition file");
    const auto variant_declared=[&](std::string_view key){
        return key=="*"||key=="unresolved"||catalogue.find(key);
    };
    for(const auto& row:next.profile.routes)if(!variant_declared(row.variant))
        return fail("acquisition route "+row.id+" references undeclared variant "+row.variant);
    for(const auto& row:next.profile.recipes)if(!variant_declared(row.variant))
        return fail("crystal recipe "+row.id+" references undeclared variant "+row.variant);
    *out=std::move(next);if(error)error->clear();return true;
}
}
