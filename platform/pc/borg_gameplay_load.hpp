#pragma once
#include "engine/gameplay/native_battle_mvp_view.hpp"
#include "engine/gameplay/borg_movement_audio.hpp"
#include "platform/pc/borg_practice_load.hpp"

namespace gf::pc {
struct BorgBattlePackageFiles {
    BorgIdentity identity;
    std::shared_ptr<const BorgGameplayProfile> definition;
    BorgPracticeFiles gred;
    std::array<std::string,4> body;
    BorgMovementSoundBank movement_sounds;
    BorgBattleVisualAssets visuals()const{
        if(definition&&definition->movement==BorgBattleMovement::NormalGRed){
            using A=NativeGRedPracticeAssets;const auto p=gred.views();
            return {p[A::Body],p[A::RigPrefix],p[A::PoseProfile],p[A::Bank]};
        }
        const auto view=[](const std::string& s){return ByteView{reinterpret_cast<const u8*>(s.data()),s.size()};};
        return {view(body[0]),view(body[1]),view(body[2]),view(body[3])};
    }
};
inline bool load_borg_battle_package(const std::filesystem::path& root,const BorgPackageCatalog& scanned,
                                     std::string_view id,std::string_view variant,BorgBattlePackageFiles* out,std::string* error){
    auto fail=[&](const char* why){if(error)*error=why;return false;};
    const auto* package=scanned.status==BorgPackageScanStatus::Complete?scanned.find(id):nullptr;
    if(!out||!package)return fail("a uniquely discovered Borg ID is required");
    if(package->manifest.gameplay.empty())return fail("no executable gameplay profile declared");
    std::filesystem::path folder;BorgCatalogProfile catalogue;
    if(!borg_catalog_detail::read_profile(root,*package,&folder,&catalogue,error))return false;
    if(variant.empty())variant=catalogue.default_variant;
    if(!catalogue.find(variant)||variant!=catalogue.default_variant)return fail("this version requires the exact base variant; no variant fallback");
    if(catalogue.details)for(const auto facet:{BorgDefinitionFacet::Forms,BorgDefinitionFacet::VariantOverrides})
        if(catalogue.details->references[unsigned(facet)].kind!=BorgDefinitionReferenceKind::None)
            return fail("forms and variant overrides must explicitly be absent in this API version");
    using borg_package_scan_detail::read_local;
    std::string encoded;BorgGameplayProfile definition;
    if(!read_local(folder,package->manifest.gameplay,kBorgGameplayBytes,&encoded))return fail("gameplay profile missing, linked or oversized");
    if(!borg_read_gameplay_profile(encoded,&definition,error))return false;
    if(definition.variant!=variant)return fail("gameplay profile does not match the selected variant");
    BorgBattlePackageFiles next;next.identity=package->manifest.identity;
    if(definition.movement==BorgBattleMovement::NormalGRed){
        if(!load_borg_practice_files(root,scanned,id,variant,&next.gred,error))return false;
    }else{
        const std::array<std::string_view,4> paths{package->manifest.model,definition.rig_prefix,definition.pose_profile,package->manifest.animations};
        constexpr std::array<std::size_t,4> limits{16u<<20,1u<<16,1u<<16,32u<<20};
        for(unsigned i=0;i<paths.size();++i)if(!read_local(folder,paths[i],limits[i],&next.body[i]))
            return fail("required battle model, rig or motion bank missing, linked or oversized");
    }
    for(unsigned i=0;i<definition.movement_sound.size();++i)if(!definition.movement_sound[i].empty()){
        std::string bytes;
        if(!read_local(folder,definition.movement_sound[i],kBorgMovementSoundBytes,&bytes)||
           !borg_movement_sound_decode(bytes,&next.movement_sounds[i]))
            return fail("missing or unqualified movement sound: expected bounded stereo 48 kHz s16le PCM");
    }
    next.definition=std::make_shared<const BorgGameplayProfile>(std::move(definition));
    *out=std::move(next);if(error)error->clear();return true;
}
}
