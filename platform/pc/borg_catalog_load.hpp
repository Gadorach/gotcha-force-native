#pragma once
#include "engine/assets/borg_catalog_profile.hpp"
#include "platform/pc/borg_package_scan.hpp"

namespace gf::pc {
// Load for inspection on demand, not for every package on startup. Broken
// catalogue or artwork data does not retroactively make a package "missing".
inline bool load_borg_catalog_card(const std::filesystem::path& root,const BorgPackageCatalog& scanned,
                                    std::string_view id,BorgCatalogCard* out,std::string* error){
    auto fail=[&](const char* why){if(error)*error=why;return false;};
    const auto* package=scanned.status==BorgPackageScanStatus::Complete?scanned.find(id):nullptr;
    if(!out||!package)return fail("a uniquely discovered Borg ID is required");
    if(package->manifest.catalog.empty())return fail("no catalogue profile declared");
    const std::filesystem::path child=package->folder;
    if(child.empty()||child.has_parent_path()||child.has_root_path()||child=="."||child=="..")return fail("invalid package directory");
    const auto folder=root/child;std::error_code ec;
    if(!std::filesystem::is_directory(std::filesystem::symlink_status(root,ec))||ec||
       !std::filesystem::is_directory(std::filesystem::symlink_status(folder,ec))||ec)
        return fail("package directory unavailable");
    BorgPackageManifest live;
    if(!borg_package_scan_detail::read(folder,&live)||live!=package->manifest)return fail("package manifest changed; rescan required");
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
}
