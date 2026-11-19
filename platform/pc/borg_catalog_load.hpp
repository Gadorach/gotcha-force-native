#pragma once
#include "engine/assets/borg_catalog_profile.hpp"
#include "engine/assets/borg_preview_profile.hpp"
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
}
// Load for inspection on demand, not for every package on startup. Broken
// catalogue or artwork data does not retroactively make a package "missing".
inline bool load_borg_catalog_card(const std::filesystem::path& root,const BorgPackageCatalog& scanned,
                                    std::string_view id,BorgCatalogCard* out,std::string* error){
    auto fail=[&](const char* why){if(error)*error=why;return false;};
    const auto* package=scanned.status==BorgPackageScanStatus::Complete?scanned.find(id):nullptr;
    if(!out||!package)return fail("a uniquely discovered Borg ID is required");
    if(package->manifest.catalog.empty())return fail("no catalogue profile declared");
    std::filesystem::path folder;
    if(!borg_catalog_detail::current_folder(root,*package,&folder,error))return false;
    BorgCatalogCard next;next.identity=package->manifest.identity;
    std::string bytes;
    if(!borg_package_scan_detail::read_local(folder,package->manifest.catalog,kBorgCatalogProfileBytes,&bytes)||
        !borg_catalog_read_profile(bytes,&next.profile))return fail("missing or invalid catalogue profile");
    for(const auto& variant:next.profile.variants){
        if(variant.placeholder_icon()){next.icons.emplace_back();continue;}
        if(variant.unresolved_icon()){
            if(error)*error="editor icon is unresolved for variant "+variant.id;
            return false;
        }
        TplImage icon;
        if(!borg_package_scan_detail::read_local(folder,variant.icon,kBorgCatalogIconBytes,&bytes)||
            !borg_catalog_decode_icon({reinterpret_cast<const u8*>(bytes.data()),bytes.size()},&icon,nullptr)){
            if(error)*error="missing, unsupported or oversized editor icon for variant "+variant.id;
            return false;
        }
        next.icons.emplace_back(std::move(icon));
    }
    *out=std::move(next);if(error)error->clear();return true;
}
struct BorgPackagePreviewFiles {
    BorgIdentity identity;
    BorgPreviewProfile profile;
    std::string model,rig_prefix,pose_profile,motions;
    static ByteView view(const std::string& data){return {reinterpret_cast<const u8*>(data.data()),data.size()};}
};
inline bool load_borg_preview_files(const std::filesystem::path& root,const BorgPackageCatalog& scanned,
                                   std::string_view id,BorgPackagePreviewFiles* out,std::string* error){
    auto fail=[&](const char* why){if(error)*error=why;return false;};
    if(!out)return fail("preview output required");
    BorgCatalogCard card;
    if(!load_borg_catalog_card(root,scanned,id,&card,error))return false;
    const auto& package=*scanned.find(id);
    if(package.manifest.preview.empty())return fail("no preview profile declared");
    std::filesystem::path folder;
    if(!borg_catalog_detail::current_folder(root,package,&folder,error))return false;
    BorgPackagePreviewFiles next;next.identity=card.identity;
    std::string profile;
    using borg_package_scan_detail::read_local;
    if(!read_local(folder,package.manifest.preview,kBorgPreviewProfileBytes,&profile)||
       !borg_read_preview_profile(profile,&next.profile))return fail("missing or unsupported preview profile");
    if(next.profile.variant!=card.profile.default_variant)return fail("preview must match the declared default variant");
    if(!read_local(folder,package.manifest.model,16u<<20,&next.model)||
       !read_local(folder,package.manifest.animations,32u<<20,&next.motions)||
       !read_local(folder,next.profile.rig_prefix,1u<<16,&next.rig_prefix)||
       !read_local(folder,next.profile.pose_profile,1u<<16,&next.pose_profile))return fail("missing, linked or oversized preview asset");
    *out=std::move(next);if(error)error->clear();return true;
}
}
