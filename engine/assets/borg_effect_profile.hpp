#pragma once
#include "engine/assets/borg_move_profile.hpp"

namespace gf {
inline constexpr std::size_t kBorgEffectProfileBytes=64u<<10;
inline constexpr unsigned kBorgEffectAssets=64,kBorgEffectCues=128;
enum class BorgEffectAssetKind:u8 {Unresolved,LocalFile,SharedId};
enum class BorgVisualEffectKind:u8 {Unresolved,Sprite,Mesh,Beam,Trail,Material,Decal};
enum class BorgSoundPlayback:u8 {Unresolved,OneShot,Loop};
enum class BorgEffectAnchor:u8 {Unresolved,Actor,Projectile,Contact,World,Socket};
enum class BorgEffectLifetime:u8 {Unresolved,Event,Projectile,Owner,UntilStop,Timer};
struct BorgEffectAssetRef {
    BorgEffectAssetKind kind=BorgEffectAssetKind::Unresolved;
    std::string target;
};
struct BorgVisualEffect {
    std::string id;
    BorgVisualEffectKind kind=BorgVisualEffectKind::Unresolved;
    BorgEffectAssetRef asset;
};
struct BorgSoundEffect {
    std::string id;
    BorgSoundPlayback playback=BorgSoundPlayback::Unresolved;
    BorgEffectAssetRef asset;
};
struct BorgEffectCue {
    std::string id,event,socket,timer,stop,visual,sound;
    BorgEffectAnchor anchor=BorgEffectAnchor::Unresolved;
    BorgEffectLifetime lifetime=BorgEffectLifetime::Unresolved;
};
struct BorgEffectProfile {
    std::vector<BorgVisualEffect> visuals;
    std::vector<BorgSoundEffect> sounds;
    std::vector<BorgEffectCue> cues;

    template<class T> static const T* find(const std::vector<T>& entries,std::string_view id){
        for(const auto& entry:entries)if(entry.id==id)return &entry;
        return nullptr;
    }
};

namespace borg_effect_detail {
using borg_move_detail::id;
using borg_move_detail::cue;
using borg_move_detail::reserved;
using borg_package_detail::split_fields;
using borg_catalog_detail::label;
inline bool asset(std::string_view key,BorgEffectAssetRef* out){
    if(key=="unresolved")return true;
    if(key.starts_with("local:")&&borg_package_asset_path_valid(key.substr(6))){
        out->kind=BorgEffectAssetKind::LocalFile;out->target=key.substr(6);return true;
    }
    if(key.starts_with("shared:")&&borg_package_id_valid(key.substr(7))){
        out->kind=BorgEffectAssetKind::SharedId;out->target=key.substr(7);return true;
    }
    return false;
}
inline bool cue_fields(std::string_view text,BorgEffectCue* out){
    std::array<std::string_view,7> fields;
    if(!split_fields(text,&fields))return false;
    const auto [name,event,anchor,lifetime,stop,visual,sound]=fields;
    if(!id(name)||!(id(event)||event=="unresolved")||!cue(stop)||!cue(visual)||!cue(sound))return false;
    if(anchor.starts_with("socket:")){
        if(!id(anchor.substr(7)))return false;
        out->anchor=BorgEffectAnchor::Socket;out->socket=anchor.substr(7);
    }else if(!label(anchor,{"unresolved","actor","projectile","contact","world"},&out->anchor))return false;
    if(lifetime.starts_with("timer:")){
        if(!id(lifetime.substr(6)))return false;
        out->lifetime=BorgEffectLifetime::Timer;out->timer=lifetime.substr(6);
    }else if(!label(lifetime,{"unresolved","event","projectile","owner","until-stop"},&out->lifetime))return false;
    if(out->lifetime==BorgEffectLifetime::UntilStop&&stop=="none")return false;
    out->id=name;out->event=event;out->stop=stop;out->visual=visual;out->sound=sound;return true;
}
}

// Typed presentation data, not an effect system: no assets decoded, no
// event/timer dispatch, no playback, and no creation of projectiles or hits.
inline bool borg_read_effect_profile(std::string_view text,BorgEffectProfile* out){
    if(!out||text.size()>kBorgEffectProfileBytes||borg_package_detail::line(text)!="GFBORG-EFFECTS 1")return false;
    using namespace borg_effect_detail;
    BorgEffectProfile next;
    while(!text.empty()){
        const auto row=borg_package_detail::line(text);
        if(row.empty()||row.starts_with('#'))continue;
        const auto equals=row.find('=');
        if(equals==std::string_view::npos)return false;
        const auto key=row.substr(0,equals),value=row.substr(equals+1);
        if(key=="cue"){
            BorgEffectCue effect;
            if(next.cues.size()==kBorgEffectCues||!cue_fields(value,&effect)||BorgEffectProfile::find(next.cues,effect.id))return false;
            next.cues.push_back(std::move(effect));continue;
        }
        std::array<std::string_view,3> fields;
        if(!split_fields(value,&fields)||!id(fields[0]))return false;
        const auto [name,type,reference]=fields;
        if(key=="visual"){
            BorgVisualEffect visual;
            if(next.visuals.size()==kBorgEffectAssets||BorgEffectProfile::find(next.visuals,name)||
               !label(type,{"unresolved","sprite","mesh","beam","trail","material","decal"},&visual.kind)||
               !asset(reference,&visual.asset))return false;
            visual.id=name;next.visuals.push_back(std::move(visual));
        }else if(key=="sound"){
            BorgSoundEffect sound;
            if(next.sounds.size()==kBorgEffectAssets||BorgEffectProfile::find(next.sounds,name)||
               !label(type,{"unresolved","one-shot","loop"},&sound.playback)||!asset(reference,&sound.asset))return false;
            sound.id=name;next.sounds.push_back(std::move(sound));
        }else return false;
    }
    if(next.cues.empty())return false;
    for(const auto& effect:next.cues){
        if(!reserved(effect.visual)&&!BorgEffectProfile::find(next.visuals,effect.visual))return false;
        if(!reserved(effect.sound)){
            const auto* found=BorgEffectProfile::find(next.sounds,effect.sound);
            if(!found||(found->playback==BorgSoundPlayback::Loop&&effect.lifetime==BorgEffectLifetime::Event))return false;
        }
    }
    *out=std::move(next);return true;
}
inline bool borg_effect_move_references(const BorgEffectProfile& effects,const BorgMoveProfile& moves,std::string* error){
    for(const auto& move:moves.moves)for(const auto& phase:move.phases){
        const auto* sound=BorgEffectProfile::find(effects.sounds,phase.sound_cue);
        if((!borg_move_detail::reserved(phase.effect_cue)&&!BorgEffectProfile::find(effects.cues,phase.effect_cue))||
           (!borg_move_detail::reserved(phase.sound_cue)&&!sound)){
            if(error)*error="missing effect cue or sound on "+move.id+"/"+phase.id;
            return false;
        }
        if(sound&&sound->playback==BorgSoundPlayback::Loop){
            if(error)*error="looping sound needs a lifetime-bound effect cue on "+move.id+"/"+phase.id;
            return false;
        }
    }
    if(error)error->clear();
    return true;
}
}
