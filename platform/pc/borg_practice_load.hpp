#pragma once
#include "engine/assets/borg_practice_profile.hpp"
#include "platform/pc/borg_catalog_load.hpp"

namespace gf::pc {
struct BorgPracticeFiles {
    BorgIdentity identity;
    std::string variant;
    std::array<std::string,NativeGRedPracticeAssets::Count> bytes;
    NativeGRedPracticeAssets views()const{
        NativeGRedPracticeAssets assets;
        for(unsigned i=0;i<bytes.size();++i)assets.bytes[i]={
            reinterpret_cast<const u8*>(bytes[i].data()),bytes[i].size()};
        return assets;
    }
};

// Capture only the exact requested normal base variant. Do not walk the
// preview/icon paths, inherit any override, or fall back to loose Borg files.
inline bool load_borg_practice_files(const std::filesystem::path& root,const BorgPackageCatalog& scanned,
                                     std::string_view id,std::string_view variant,BorgPracticeFiles* out,std::string* error){
    using Assets=NativeGRedPracticeAssets;
    auto fail=[&](std::string why){if(error)*error=std::move(why);return false;};
    const auto* package=scanned.status==BorgPackageScanStatus::Complete?scanned.find(id):nullptr;
    if(!package||!out)return fail("a uniquely discovered package is required");
    if(variant!="normal")return fail("practice requires the explicitly selected normal variant");
    if(!package->manifest.animation_map.empty())return fail("a separate animation map is not supported in practice");
    std::filesystem::path folder;BorgCatalogProfile catalogue;
    if(!borg_catalog_detail::read_profile(root,*package,&folder,&catalogue,error))return false;
    if(catalogue.default_variant!=variant||!catalogue.find(variant))return fail("the requested normal variant must be declared as the base");
    if(catalogue.details&&catalogue.details->references[unsigned(BorgDefinitionFacet::VariantOverrides)].kind!=
       BorgDefinitionReferenceKind::None)return fail("practice requires no variant overrides");
    BorgPracticeProfile profile;std::string rules;
    using borg_package_scan_detail::read_local;
    if(!read_local(folder,package->manifest.rules,kBorgPracticeProfileBytes,&rules)||
       !borg_read_normal_gred_practice_profile(rules,&profile)||profile.variant!=variant)
        return fail("the package does not declare qualified normal G-Red practice rules");
    profile.paths[Assets::Body]=package->manifest.model;profile.paths[Assets::Bank]=package->manifest.animations;
    BorgPracticeFiles next;next.identity=package->manifest.identity;next.variant=variant;
    for(unsigned i=0;i<Assets::Count;++i)
        if(!read_local(folder,profile.paths[i],Assets::sizes[i],&next.bytes[i])||next.bytes[i].size()!=Assets::sizes[i])
            return fail("unavailable or wrong-sized practice asset: "+std::string(Assets::roles[i]));
    if(!next.views().qualified())return fail("practice assets do not match the qualified normal G-Red content");
    *out=std::move(next);if(error)error->clear();return true;
}
}
