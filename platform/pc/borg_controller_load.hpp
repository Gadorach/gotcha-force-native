#pragma once
#include "engine/assets/borg_controller_profile.hpp"
#include "engine/assets/borg_form_profile.hpp"
#include "platform/pc/borg_catalog_load.hpp"

namespace gf::pc {
struct BorgControllerInspection {
    BorgIdentity identity;
    BorgControllerProfile profile;
};

inline bool load_borg_controller_inspection(const std::filesystem::path& root,const BorgPackageCatalog& scanned,
                                            std::string_view id,BorgControllerInspection* out,std::string* error){
    auto fail=[&](const char* why){if(error)*error=why;return false;};
    const auto* package=scanned.status==BorgPackageScanStatus::Complete?scanned.find(id):nullptr;
    if(!out||!package)return fail("a uniquely discovered Borg ID is required");
    std::filesystem::path folder;
    BorgCatalogProfile catalogue;
    if(!borg_catalog_detail::read_profile(root,*package,&folder,&catalogue,error))return false;
    if(!catalogue.details)return fail("a version 2 catalogue profile is required for controllers");
    const auto& refs=catalogue.details->references;
    const auto& file=refs[unsigned(BorgDefinitionFacet::Controllers)];
    if(file.kind==BorgDefinitionReferenceKind::None)return fail("package declares no controller profiles");
    if(file.kind!=BorgDefinitionReferenceKind::LocalFile)return fail("controller profiles are unresolved");
    using borg_package_scan_detail::read_local;
    BorgControllerInspection next;next.identity=package->manifest.identity;
    std::string bytes;
    if(!read_local(folder,file.file,kBorgControllerProfileBytes,&bytes)||!borg_read_controller_profile(bytes,&next.profile))
        return fail("missing or invalid controller profile");
    const auto& form_file=refs[unsigned(BorgDefinitionFacet::Forms)];
    BorgFormProfile forms;
    if(form_file.kind==BorgDefinitionReferenceKind::LocalFile&&
       (!read_local(folder,form_file.file,kBorgFormProfileBytes,&bytes)||!borg_read_form_profile(bytes,&forms)))
        return fail("missing or invalid form definitions; controller scope cannot be checked");
    for(const auto& row:next.profile.definitions){
        if(!catalogue.find(row.variant)){
            if(error)*error="controller references undeclared variant "+row.variant;
            return false;
        }
        if(row.form=="unresolved")continue;
        if(row.form=="none"){
            if(form_file.kind!=BorgDefinitionReferenceKind::None)return fail("form-less controller requires forms=none");
        }else if(!forms.find(row.form)){
            if(error)*error="controller references undeclared form "+row.form;
            return false;
        }
    }
    *out=std::move(next);if(error)error->clear();return true;
}
}
