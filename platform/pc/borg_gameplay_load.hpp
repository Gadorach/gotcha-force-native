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
    std::string impact_archive;
    std::array<std::string,kBorgBattleMoves> field_textures;
    BorgMovementSoundBank movement_sounds;
    BorgLaunchSoundBank launch_sounds;
    BorgActionSoundBank action_sounds;
    BorgContactSoundBank contact_sounds;
    BorgImpactSoundBank impact_sounds;
    BorgReleaseSoundBank release_sounds;
    BorgAudioSlot audio()const{
        return {&movement_sounds,&launch_sounds,&release_sounds,&impact_sounds,&action_sounds,&contact_sounds};
    }
    BorgBattleVisualAssets visuals()const{
        BorgBattleVisualAssets result;
        const auto view=[](const std::string& s){return ByteView{reinterpret_cast<const u8*>(s.data()),s.size()};};
        if(definition&&definition->movement==BorgBattleMovement::NormalGRed){
            using A=NativeGRedPracticeAssets;const auto p=gred.views();
            result={p[A::Body],p[A::RigPrefix],p[A::PoseProfile],p[A::Bank]};
        }else result={view(body[0]),view(body[1]),view(body[2]),view(body[3])};
        for(unsigned i=0;i<field_textures.size();++i)result.field_texture[i]=view(field_textures[i]);
        result.impact_record_bank=view(impact_archive);
        return result;
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
        if(definition.static_body()&&!package->manifest.animations.empty())
            return fail("static body must not declare a motion bank");
        const std::array<std::string_view,4> paths{package->manifest.model,definition.rig_prefix,definition.pose_profile,package->manifest.animations};
        constexpr std::array<std::size_t,4> limits{16u<<20,1u<<16,1u<<16,32u<<20};
        for(unsigned i=0;i<(definition.static_body()?3u:4u);++i)if(!read_local(folder,paths[i],limits[i],&next.body[i]))
            return fail("required battle model, rig or motion bank missing, linked or oversized");
    }
    if(!definition.impact_record_bank.empty()&&!read_local(folder,definition.impact_record_bank,4u<<20,&next.impact_archive))
        return fail("required impact record archive missing, linked or oversized");
    for(unsigned i=0;i<definition.movement_sound.size();++i)if(!definition.movement_sound[i].empty()){
        std::string bytes;
        if(!read_local(folder,definition.movement_sound[i],kBorgMovementSoundBytes,&bytes)||
           !borg_movement_sound_decode(bytes,&next.movement_sounds[i]))
            return fail("missing or unqualified movement sound: expected bounded stereo 48 kHz s16le PCM");
    }
    for(unsigned i=0;i<definition.projectiles.size();++i)if(const auto& path=definition.projectiles[i].launch_sound;!path.empty()){
        std::string bytes;
        if(!read_local(folder,path,kBorgMovementSoundBytes,&bytes)||!borg_movement_sound_decode(bytes,&next.launch_sounds[i]))
            return fail("missing or unqualified launch sound: expected bounded stereo 48 kHz s16le PCM");
    }
    for(unsigned i=0;i<definition.moves.size();++i)if(const auto& path=definition.moves[i].action_sound;!path.empty()){
        std::string bytes;
        if(!read_local(folder,path,kBorgMovementSoundBytes,&bytes)||!borg_movement_sound_decode(bytes,&next.action_sounds[i]))
            return fail("missing or unqualified action sound: expected bounded stereo 48 kHz s16le PCM");
    }
    for(unsigned i=0;i<definition.moves.size();++i)for(unsigned band=0;band<kBorgContactSoundBands;++band)
        if(const auto& cue=definition.moves[i].contact_sounds[band];cue)
            for(unsigned sample=0;sample<cue->paths.size();++sample){
                std::string bytes;
                auto& clip=next.contact_sounds[i*kBorgContactSoundClips+2*band+sample];
                if(!read_local(folder,cue->paths[sample],kBorgMovementSoundBytes,&bytes)||!borg_movement_sound_decode(bytes,&clip))
                    return fail("missing or unqualified contact sound: expected bounded stereo 48 kHz s16le PCM");
            }
    for(unsigned i=0;i<definition.projectiles.size();++i)if(const auto& cue=definition.projectiles[i].impact_sound;cue){
        std::string bytes;
        if(!read_local(folder,cue->path,kBorgImpactSoundBytes,&bytes)||
           !borg_movement_sound_decode(bytes,&next.impact_sounds[i],kBorgImpactSoundBytes))
            return fail("missing or unqualified impact sound: expected bounded stereo 48 kHz s16le PCM");
    }
    unsigned sound_slot=0;
    for(unsigned i=0;i<definition.moves.size();++i)for(const auto& release:definition.moves[i].releases){
        if(release.debit_sound.empty())continue;
        std::string bytes;
        if(sound_slot>=next.release_sounds.size()||!read_local(folder,release.debit_sound,kBorgMovementSoundBytes,&bytes)||
           !borg_movement_sound_decode(bytes,&next.release_sounds[sound_slot].pcm))
            return fail("missing or unqualified release sound: expected bounded stereo 48 kHz s16le PCM");
        next.release_sounds[sound_slot].move=u8(i);
        next.release_sounds[sound_slot++].frame=release.frame;
    }
    for(unsigned i=0;i<definition.moves.size();++i){
        const auto& f=definition.moves[i].field;
        if(f&&!f->texture.empty()&&
           (!read_local(folder,f->texture,kBorgFieldTextureBytes,&next.field_textures[i])||
            next.field_textures[i].size()!=kBorgFieldTextureBytes))
            return fail("missing or invalid field texture: expected local 32x32 RGBA8 pixels");
    }
    next.definition=std::make_shared<const BorgGameplayProfile>(std::move(definition));
    *out=std::move(next);if(error)error->clear();return true;
}
}
