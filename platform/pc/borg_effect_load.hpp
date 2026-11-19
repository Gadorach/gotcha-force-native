#pragma once
#include "engine/assets/borg_effect_profile.hpp"
#include "platform/pc/borg_catalog_load.hpp"

namespace gf::pc {
struct BorgEffectInspection {
    BorgIdentity identity;
    BorgEffectProfile profile;
    bool checked_move_references=false;
};
inline bool load_borg_effect_inspection(const std::filesystem::path& root,const BorgPackageCatalog& scanned,
                                        std::string_view id,BorgEffectInspection* out,std::string* error){
    auto fail=[&](const char* why){if(error)*error=why;return false;};
    const auto* package=scanned.status==BorgPackageScanStatus::Complete?scanned.find(id):nullptr;
    if(!out||!package)return fail("a uniquely discovered Borg ID is required");
    std::filesystem::path folder;
    BorgCatalogProfile catalogue;
    if(!borg_catalog_detail::read_profile(root,*package,&folder,&catalogue,error))return false;
    if(!catalogue.details)return fail("a version 2 catalogue profile is required for effects");
    const auto& effect_file=catalogue.details->references[unsigned(BorgDefinitionFacet::Effects)];
    if(effect_file.kind==BorgDefinitionReferenceKind::None)return fail("package declares no effects");
    if(effect_file.kind!=BorgDefinitionReferenceKind::LocalFile)return fail("effects are unresolved");
    BorgEffectInspection next;next.identity=package->manifest.identity;
    using borg_package_scan_detail::read_local;
    std::string bytes;
    if(!read_local(folder,effect_file.file,kBorgEffectProfileBytes,&bytes)||!borg_read_effect_profile(bytes,&next.profile))
        return fail("missing or invalid effect definition file");
    const auto check_assets=[&](const auto& group){
        for(const auto& entry:group){
            const auto& ref=entry.asset;
            if(ref.kind!=BorgEffectAssetKind::LocalFile)continue;
            std::error_code ec;
            if(!borg_package_scan_detail::local_file(folder,ref.target)||
               !std::filesystem::file_size(folder/ref.target,ec)||ec){
                if(error)*error="missing, linked or empty local asset for "+entry.id;
                return false;
            }
        }
        return true;
    };
    if(!check_assets(next.profile.visuals)||!check_assets(next.profile.sounds))return false;
    const auto& move_file=catalogue.details->references[unsigned(BorgDefinitionFacet::Moves)];
    if(move_file.kind==BorgDefinitionReferenceKind::LocalFile){
        BorgMoveProfile moves;
        if(!read_local(folder,move_file.file,kBorgMoveProfileBytes,&bytes)||!borg_read_move_profile(bytes,&moves))
            return fail("missing or invalid move definition file; effect references cannot be checked");
        if(!borg_effect_move_references(next.profile,moves,error))return false;
        next.checked_move_references=true;
    }
    *out=std::move(next);if(error)error->clear();return true;
}
}
