#pragma once
#include "engine/assets/borg_movement_pcm.hpp"
#include "engine/assets/borg_catalog_profile.hpp"
#include "engine/assets/borg_preview_profile.hpp"
#include "engine/assets/borg_variant_profile.hpp"
#include "platform/pc/borg_package_scan.hpp"

namespace gf::pc {
enum class BorgCatalogLoadIssue {Unavailable,NotDeclared,Missing,Invalid,Unsupported};
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
                         std::filesystem::path* folder,BorgCatalogProfile* out,std::string* error,
                         BorgCatalogLoadIssue* issue=nullptr){
    if(issue)*issue=BorgCatalogLoadIssue::Unavailable;
    if(package.manifest.catalog.empty()){
        if(issue)*issue=BorgCatalogLoadIssue::NotDeclared;
        if(error)*error="no catalogue profile declared";
        return false;
    }
    if(!current_folder(root,package,folder,error))return false;
    const auto& path=package.manifest.catalog;
    if(issue&&borg_package_asset_path_valid(path)){
        auto current=*folder;
        const std::filesystem::path local=path;
        for(auto it=local.begin();it!=local.end();){
            current/=*it++;
            std::error_code ec;const auto status=std::filesystem::symlink_status(current,ec);
            if(ec==std::errc::no_such_file_or_directory||status.type()==std::filesystem::file_type::not_found){
                *issue=BorgCatalogLoadIssue::Missing;break;
            }
            if(ec)break;
            if(it==local.end()||!std::filesystem::is_directory(status)){
                *issue=BorgCatalogLoadIssue::Invalid;break;
            }
        }
    }
    std::string bytes;
    const bool read=borg_package_scan_detail::read_local(*folder,path,kBorgCatalogProfileBytes,&bytes);
    if(!read||!borg_catalog_read_profile(bytes,out)){
        if(issue&&read){
            std::string_view input=bytes,header=borg_package_detail::line(input);
            constexpr std::string_view prefix="GFBORG-CATALOG ";
            if(header.starts_with(prefix)){
                header.remove_prefix(prefix.size());
                if(header!="1"&&header!="2"&&!header.empty()&&std::all_of(header.begin(),header.end(),[](char c){return c>='0'&&c<='9';}))
                    *issue=BorgCatalogLoadIssue::Unsupported;
            }
        }
        if(error)*error=issue&&*issue==BorgCatalogLoadIssue::Unsupported?
            "unsupported catalogue profile version":"missing or invalid catalogue profile";
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
inline void bind_preview_paths(const std::filesystem::path& folder,BorgCatalogSnapshot& snapshot){
    const auto& catalogue=snapshot.profile;
    const auto& manifest=snapshot.package.manifest;
    BorgVariantProfile overrides;
    std::string overrides_error;
    const bool has_overrides=catalogue.details&&
        catalogue.details->references[unsigned(BorgDefinitionFacet::VariantOverrides)].kind==BorgDefinitionReferenceKind::LocalFile;
    if(has_overrides)read_variants(folder,catalogue,&overrides,&overrides_error);
    for(const auto& variant:catalogue.variants){
        const bool base=variant.id==catalogue.default_variant;
        BorgCatalogPreviewBinding binding{variant.id,base?manifest.preview:std::string{},manifest.model,manifest.animations,overrides_error};
        const auto* row=overrides.find(variant.id);
        if(binding.error.empty()&&!base&&(!row||row->asset(BorgVariantAssetRole::Preview).kind!=BorgVariantAssetKind::LocalFile))
            binding.error="nondefault variant needs its own declared preview profile";
        if(binding.error.empty()&&row){
            auto resolve=[&](BorgVariantAssetRole role,std::string& path){
                const auto& ref=row->asset(role);
                if(ref.kind==BorgVariantAssetKind::LocalFile){path=ref.file;return true;}
                if(ref.kind==BorgVariantAssetKind::Inherit)return true;
                path.clear();return false;
            };
            const bool preview=resolve(BorgVariantAssetRole::Preview,binding.preview);
            const bool model=resolve(BorgVariantAssetRole::Model,binding.model);
            for(auto role:{BorgVariantAssetRole::Textures,BorgVariantAssetRole::Materials}){
                const auto kind=row->asset(role).kind;
                if(kind!=BorgVariantAssetKind::Inherit&&kind!=BorgVariantAssetKind::None)
                    binding.error="separate or unresolved texture and material overrides cannot be rendered in this preview";
            }
            if(binding.error.empty()&&(!preview||!model))binding.error="variant preview or model declaration is unavailable";
            if(row->asset(BorgVariantAssetRole::Animations).kind==BorgVariantAssetKind::None)binding.motions.clear();
            else if(!resolve(BorgVariantAssetRole::Animations,binding.motions)&&binding.error.empty())
                binding.error="variant motion declaration is unavailable";
        }
        if(binding.error.empty()&&binding.preview.empty())binding.error="no preview profile declared";
        snapshot.preview_bindings.push_back(std::move(binding));
    }
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
                                        std::string_view id,BorgCatalogSnapshot* out,std::string* error,
                                        BorgCatalogLoadIssue* issue=nullptr){
    if(issue)*issue=BorgCatalogLoadIssue::Unavailable;
    const auto* package=scanned.status==BorgPackageScanStatus::Complete?scanned.find(id):nullptr;
    if(!out||!package){if(error)*error="a uniquely discovered Borg ID is required";return false;}
    BorgCatalogSnapshot next;next.package=*package;
    std::filesystem::path folder;
    if(!borg_catalog_detail::read_profile(root,*package,&folder,&next.profile,error,issue))return false;
    borg_catalog_detail::bind_preview_paths(folder,next);
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
    std::string model,rig_prefix,pose_profile,motions,landing_pcm;
    static ByteView view(const std::string& data){return {reinterpret_cast<const u8*>(data.data()),data.size()};}
};
struct BorgVariantInspection {
    BorgIdentity identity;
    BorgVariantProfile profile;
};
// Enumeration may read the bounded declaration without reading model, rig,
// motion or audio bytes. Admission still rereads the declaration and assets.
inline bool read_borg_preview_declaration(const std::filesystem::path& root,const BorgCatalogSnapshot& snapshot,
                                         const BorgCatalogPreviewBinding& binding,BorgPreviewProfile* out){
    if(!out||!binding.error.empty()||!snapshot.profile.find(binding.variant))return false;
    std::filesystem::path folder;
    if(!borg_catalog_detail::current_folder(root,snapshot.package,&folder,nullptr))return false;
    std::string text;BorgPreviewProfile profile;
    if(!borg_package_scan_detail::read_local(folder,binding.preview,kBorgPreviewProfileBytes,&text)||
       !borg_read_preview_profile(text,&profile)||profile.variant!=binding.variant||
       profile.static_body!=binding.motions.empty())return false;
    *out=std::move(profile);return true;
}
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
inline bool load_borg_preview_files(const std::filesystem::path& root,const BorgCatalogSnapshot& snapshot,
                                      std::string_view variant,BorgPackagePreviewFiles* out,std::string* error,
                                      bool lab_audio=false){
    auto fail=[&](std::string_view why){if(error)*error=why;return false;};
    if(!out)return fail("preview output required");
    if(!borg_catalog_variant_id_valid(variant)||!snapshot.profile.find(variant))return fail("undeclared preview variant");
    const auto& bindings=snapshot.preview_bindings;
    const auto binding=std::find_if(bindings.begin(),bindings.end(),[&](const auto& b){return b.variant==variant;});
    if(binding==bindings.end())return fail("no pinned preview declaration for variant");
    if(!binding->error.empty())return fail(binding->error);
    std::filesystem::path folder;
    if(!borg_catalog_detail::current_folder(root,snapshot.package,&folder,error))return false;
    BorgPackagePreviewFiles next;next.identity=snapshot.package.manifest.identity;
    std::string profile;
    using borg_package_scan_detail::read_local;
    if(!read_local(folder,binding->preview,kBorgPreviewProfileBytes,&profile)||
       !borg_read_preview_profile(profile,&next.profile))return fail("missing or unsupported preview profile");
    if(next.profile.variant!=variant)return fail("preview profile does not match the selected variant");
    if(next.profile.static_body!=binding->motions.empty())return fail("preview motion declaration and static/clip binding disagree");
    if(!read_local(folder,binding->model,16u<<20,&next.model)||
        (!next.profile.static_body&&!read_local(folder,binding->motions,32u<<20,&next.motions))||
       !read_local(folder,next.profile.rig_prefix,1u<<16,&next.rig_prefix)||
        (!next.profile.whole_body&&!read_local(folder,next.profile.pose_profile,1u<<16,&next.pose_profile)))return fail("missing, linked or oversized preview asset");
    if(lab_audio&&(next.profile.lab_landing_pcm.empty()||
        !read_local(folder,next.profile.lab_landing_pcm,kBorgMovementSoundBytes,&next.landing_pcm)))
        return fail("missing exact lab landing PCM declaration or unreadable sample");
    *out=std::move(next);if(error)error->clear();return true;
}
inline bool load_borg_preview_files(const std::filesystem::path& root,const BorgPackageCatalog& scanned,
                                      std::string_view id,std::string_view requested_variant,BorgPackagePreviewFiles* out,std::string* error,
                                      bool lab_audio=false){
    BorgCatalogSnapshot snapshot;
    return load_borg_catalog_snapshot(root,scanned,id,&snapshot,error)&&
        load_borg_preview_files(root,snapshot,requested_variant.empty()?std::string_view(snapshot.profile.default_variant):requested_variant,
            out,error,lab_audio);
}
}
