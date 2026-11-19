#pragma once
#include "engine/assets/borg_form_profile.hpp"
#include "platform/pc/borg_catalog_load.hpp"

namespace gf::pc {
struct BorgFormInspection {
    BorgIdentity identity;
    BorgFormProfile profile;
    bool checked_moves=false,checked_spawn=false;
};
inline bool load_borg_form_inspection(const std::filesystem::path& root,const BorgPackageCatalog& scanned,
                                       std::string_view id,BorgFormInspection* out,std::string* error){
    auto fail=[&](const char* why){if(error)*error=why;return false;};
    const auto* package=scanned.status==BorgPackageScanStatus::Complete?scanned.find(id):nullptr;
    if(!out||!package)return fail("a uniquely discovered Borg ID is required");
    std::filesystem::path folder;
    BorgCatalogProfile catalogue;
    if(!borg_catalog_detail::read_profile(root,*package,&folder,&catalogue,error))return false;
    if(!catalogue.details)return fail("a version 2 catalogue profile is required for forms");
    const auto& refs=catalogue.details->references;
    const auto& form_file=refs[unsigned(BorgDefinitionFacet::Forms)];
    if(form_file.kind==BorgDefinitionReferenceKind::None)return fail("package declares no form definitions");
    if(form_file.kind!=BorgDefinitionReferenceKind::LocalFile)return fail("form definitions are unresolved");
    BorgFormInspection next;next.identity=package->manifest.identity;
    using borg_package_scan_detail::read_local;
    std::string bytes;
    if(!read_local(folder,form_file.file,kBorgFormProfileBytes,&bytes)||!borg_read_form_profile(bytes,&next.profile))
        return fail("missing or invalid form definition file");
    for(const auto& form:next.profile.forms)for(const auto& asset:form.assets){
        if(asset.kind!=BorgFormAssetKind::LocalFile)continue;
        std::error_code ec;
        if(!borg_package_scan_detail::local_file(folder,asset.file)||
           !std::filesystem::file_size(folder/asset.file,ec)||ec){
            if(error)*error="missing, linked or empty override on form "+form.id;
            return false;
        }
    }
    const auto& move_file=refs[unsigned(BorgDefinitionFacet::Moves)];
    if(move_file.kind==BorgDefinitionReferenceKind::LocalFile){
        BorgMoveProfile moves;
        if(!read_local(folder,move_file.file,kBorgMoveProfileBytes,&bytes)||!borg_read_move_profile(bytes,&moves))
            return fail("missing or invalid move definition file; form references cannot be checked");
        if(!borg_form_move_references(next.profile,moves,error))return false;
        next.checked_moves=true;
    }
    const auto& spawn_file=refs[unsigned(BorgDefinitionFacet::Spawn)];
    if(spawn_file.kind==BorgDefinitionReferenceKind::LocalFile){
        BorgSpawnProfile spawn;
        if(!read_local(folder,spawn_file.file,kBorgSpawnProfileBytes,&bytes)||!borg_read_spawn_profile(bytes,&spawn))
            return fail("missing or invalid spawn policy file; form references cannot be checked");
        if(!borg_form_spawn_references(next.profile,spawn,error))return false;
        next.checked_spawn=true;
    }
    *out=std::move(next);if(error)error->clear();return true;
}
}
