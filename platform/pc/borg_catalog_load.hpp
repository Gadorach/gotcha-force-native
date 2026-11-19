#pragma once
#include "engine/assets/borg_catalog_profile.hpp"
#include "engine/assets/borg_preview_profile.hpp"
#include "engine/assets/borg_variant_profile.hpp"
#include "platform/pc/borg_package_scan.hpp"

namespace gf::pc {
namespace borg_catalog_detail {
inline bool current_folder(const std::filesystem::path& root,const BorgPackageEntry& package,
                           std::filesystem::path* out,std::string* error){
    auto fail=[&](const char* why){if(error)*error=why;return false;};
    const std::filesystem::path child=package.folder;
    if(child.empty()||child.has_parent_path()||child.has_root_path()||child=="."||child=="..")return fail("invalid package directory");
    const auto folder=root/child;std::error_code ec;
    if(!std::filesystem::is_directory(std::filesystem::symlink_status(root,ec))||ec||
       !std::filesystem::is_directory(std::filesystem::symlink_status(folder,ec))||ec)
        return fail("package directory unavailable");
    BorgPackageManifest live;
    if(!borg_package_scan_detail::read(folder,&live)||live!=package.manifest)return fail("package manifest changed; rescan required");
    *out=folder;return true;
}
inline bool read_profile(const std::filesystem::path& root,const BorgPackageEntry& package,
                         std::filesystem::path* folder,BorgCatalogProfile* out,std::string* error){
    if(package.manifest.catalog.empty()){if(error)*error="no catalogue profile declared";return false;}
    if(!current_folder(root,package,folder,error))return false;
    std::string bytes;
    if(!borg_package_scan_detail::read_local(*folder,package.manifest.catalog,kBorgCatalogProfileBytes,&bytes)||
       !borg_catalog_read_profile(bytes,out)){
        if(error)*error="missing or invalid catalogue profile";
        return false;
    }
    return true;
}
inline bool read_variants(const std::filesystem::path& folder,const BorgCatalogProfile& catalogue,
                          BorgVariantProfile* out,std::string* error){
    auto fail=[&](const char* why){if(error)*error=why;return false;};
    if(!catalogue.details)return fail("a version 2 catalogue profile is required for variant overrides");
    const auto& reference=catalogue.details->references[unsigned(BorgDefinitionFacet::VariantOverrides)];
    if(reference.kind==BorgDefinitionReferenceKind::None)return fail("package declares no variant overrides");
    if(reference.kind!=BorgDefinitionReferenceKind::LocalFile)return fail("variant overrides are unresolved");
    std::string bytes;BorgVariantProfile next;
    if(!borg_package_scan_detail::read_local(folder,reference.file,kBorgVariantProfileBytes,&bytes)||
        !borg_read_variant_profile(bytes,&next))return fail("missing or invalid variant override file");
    for(const auto& row:next.variants)if(!catalogue.find(row.variant)){
        if(error)*error="override references undeclared variant "+row.variant;
        return false;
    }
    *out=std::move(next);if(error)error->clear();return true;
}
inline bool read_icon(const std::filesystem::path& folder,const BorgCatalogVariant& variant,
                      std::optional<TplImage>* out,std::string* error){
    if(variant.placeholder_icon()){out->reset();return true;}
    if(variant.unresolved_icon()){
        if(error)*error="editor icon is unresolved for variant "+variant.id;
        return false;
    }
    std::string bytes;TplImage icon;
    if(!borg_package_scan_detail::read_local(folder,variant.icon,kBorgCatalogIconBytes,&bytes)||
       !borg_catalog_decode_icon({reinterpret_cast<const u8*>(bytes.data()),bytes.size()},&icon,nullptr)){
        if(error)*error="missing, unsupported or oversized editor icon for variant "+variant.id;
        return false;
    }
    *out=std::move(icon);return true;
}
}
inline bool load_borg_catalog_snapshot(const std::filesystem::path& root,const BorgPackageCatalog& scanned,
                                       std::string_view id,BorgCatalogSnapshot* out,std::string* error){
    const auto* package=scanned.status==BorgPackageScanStatus::Complete?scanned.find(id):nullptr;
    if(!out||!package){if(error)*error="a uniquely discovered Borg ID is required";return false;}
    BorgCatalogSnapshot next;next.package=*package;
    std::filesystem::path folder;
    if(!borg_catalog_detail::read_profile(root,*package,&folder,&next.profile,error))return false;
    *out=std::move(next);if(error)error->clear();return true;
}
inline bool load_borg_variant_icon(const std::filesystem::path& root,const BorgCatalogSnapshot& snapshot,
                                   std::string_view variant_id,std::optional<TplImage>* out,std::string* error){
    const auto* variant=snapshot.profile.find(variant_id);
    if(!out||!variant){if(error)*error="a declared variant is required";return false;}
    std::filesystem::path folder;
    if(!borg_catalog_detail::current_folder(root,snapshot.package,&folder,error)||
        !borg_catalog_detail::read_icon(folder,*variant,out,error))return false;
    if(error)error->clear();
    return true;
}
// Load for inspection on demand, not for every package on startup. Broken
// catalogue or artwork data does not retroactively make a package "missing".
inline bool load_borg_catalog_card(const std::filesystem::path& root,const BorgPackageCatalog& scanned,
                                    std::string_view id,BorgCatalogCard* out,std::string* error){
    auto fail=[&](const char* why){if(error)*error=why;return false;};
    const auto* package=scanned.status==BorgPackageScanStatus::Complete?scanned.find(id):nullptr;
    if(!out||!package)return fail("a uniquely discovered Borg ID is required");
    std::filesystem::path folder;
    BorgCatalogCard next;next.identity=package->manifest.identity;
    if(!borg_catalog_detail::read_profile(root,*package,&folder,&next.profile,error))return false;
    for(const auto& variant:next.profile.variants){
        std::optional<TplImage> icon;
        if(!borg_catalog_detail::read_icon(folder,variant,&icon,error))return false;
        next.icons.push_back(std::move(icon));
    }
    *out=std::move(next);if(error)error->clear();return true;
}
struct BorgPackagePreviewFiles {
    BorgIdentity identity;
    BorgPreviewProfile profile;
    std::string model,rig_prefix,pose_profile,motions;
    static ByteView view(const std::string& data){return {reinterpret_cast<const u8*>(data.data()),data.size()};}
};
struct BorgVariantInspection {
    BorgIdentity identity;
    BorgVariantProfile profile;
};
inline bool load_borg_variant_inspection(const std::filesystem::path& root,const BorgPackageCatalog& scanned,
                                         std::string_view id,BorgVariantInspection* out,std::string* error){
    const auto* package=scanned.status==BorgPackageScanStatus::Complete?scanned.find(id):nullptr;
    if(!out||!package){if(error)*error="a uniquely discovered Borg ID is required";return false;}
    std::filesystem::path folder;BorgCatalogProfile catalogue;BorgVariantInspection next;
    next.identity=package->manifest.identity;
    if(!borg_catalog_detail::read_profile(root,*package,&folder,&catalogue,error)||
        !borg_catalog_detail::read_variants(folder,catalogue,&next.profile,error))return false;
    for(const auto& row:next.profile.variants)for(const auto& asset:row.assets){
        if(asset.kind!=BorgVariantAssetKind::LocalFile)continue;
        std::error_code ec;
        if(!borg_package_scan_detail::local_file(folder,asset.file)||!std::filesystem::file_size(folder/asset.file,ec)||ec){
            if(error)*error="missing, empty or linked override for variant "+row.variant;
            return false;
        }
    }
    *out=std::move(next);return true;
}
inline bool load_borg_preview_files(const std::filesystem::path& root,const BorgPackageCatalog& scanned,
                                    std::string_view id,std::string_view requested_variant,BorgPackagePreviewFiles* out,std::string* error){
    auto fail=[&](const char* why){if(error)*error=why;return false;};
    if(!out)return fail("preview output required");
    BorgCatalogCard card;
    if(!load_borg_catalog_card(root,scanned,id,&card,error))return false;
    const auto& package=*scanned.find(id);
    const auto& catalogue=card.profile;
    const std::string_view variant=requested_variant.empty()?std::string_view(catalogue.default_variant):requested_variant;
    if(!catalogue.find(variant))return fail("undeclared preview variant");
    std::filesystem::path folder;
    if(!borg_catalog_detail::current_folder(root,package,&folder,error))return false;
    std::string_view preview_file=package.manifest.preview,model_file=package.manifest.model,motions_file=package.manifest.animations;
    BorgVariantProfile overrides;
    const bool has_overrides=catalogue.details&&
        catalogue.details->references[unsigned(BorgDefinitionFacet::VariantOverrides)].kind==BorgDefinitionReferenceKind::LocalFile;
    if(has_overrides&&!borg_catalog_detail::read_variants(folder,catalogue,&overrides,error))return false;
    const auto* row=overrides.find(variant);
    if(variant!=catalogue.default_variant&&(!row||row->asset(BorgVariantAssetRole::Preview).kind!=BorgVariantAssetKind::LocalFile))
        return fail("nondefault variant needs its own declared preview profile");
    if(row){
        auto resolve=[&](BorgVariantAssetRole role,std::string_view* path){
            const auto& ref=row->asset(role);
            if(ref.kind==BorgVariantAssetKind::LocalFile){*path=ref.file;return true;}
            return ref.kind==BorgVariantAssetKind::Inherit;
        };
        for(auto role:{BorgVariantAssetRole::Textures,BorgVariantAssetRole::Materials}){
            const auto kind=row->asset(role).kind;
            if(kind!=BorgVariantAssetKind::Inherit&&kind!=BorgVariantAssetKind::None)
                return fail("separate or unresolved texture and material overrides cannot be rendered in this preview");
        }
        if(!resolve(BorgVariantAssetRole::Preview,&preview_file)||!resolve(BorgVariantAssetRole::Model,&model_file)||
           !resolve(BorgVariantAssetRole::Animations,&motions_file))return fail("variant preview, model or motion declaration is unavailable");
    }
    if(preview_file.empty())return fail("no preview profile declared");
    BorgPackagePreviewFiles next;next.identity=card.identity;
    std::string profile;
    using borg_package_scan_detail::read_local;
    if(!read_local(folder,preview_file,kBorgPreviewProfileBytes,&profile)||
       !borg_read_preview_profile(profile,&next.profile))return fail("missing or unsupported preview profile");
    if(next.profile.variant!=variant)return fail("preview profile does not match the selected variant");
    if(!read_local(folder,model_file,16u<<20,&next.model)||
       !read_local(folder,motions_file,32u<<20,&next.motions)||
       !read_local(folder,next.profile.rig_prefix,1u<<16,&next.rig_prefix)||
       !read_local(folder,next.profile.pose_profile,1u<<16,&next.pose_profile))return fail("missing, linked or oversized preview asset");
    *out=std::move(next);if(error)error->clear();return true;
}
}
