#pragma once
#include <charconv>
#include <cmath>
#include "engine/assets/borg_catalog_profile.hpp"

namespace gf {
inline constexpr std::size_t kStageManifestBytes=16u<<10;
inline constexpr std::size_t kStageSpawnCount=32;
enum class StageAsset {Model,Floor,ProjectileStage,Secondary,Tertiary,ViewFade,Count};
inline constexpr unsigned kStageAssetCount=unsigned(StageAsset::Count);
inline constexpr std::array<std::string_view,kStageAssetCount> kStageAssetKeys{
    "model","floor","projectile-stage","secondary","tertiary","view-fade"
};
inline constexpr std::array<std::size_t,kStageAssetCount> kStageAssetLimits{32u<<20,16u<<20,48,16u<<20,16u<<20,4096};

struct StageSpawnMarker {
    std::string id;
    std::array<f32,3> query{};
    bool operator==(const StageSpawnMarker&)const=default;
};
struct StagePackageManifest {
    std::string id,name;
    f32 arena_half=0;
    std::array<std::string,kStageAssetCount> asset_paths;
    std::vector<StageSpawnMarker> spawns;
    const std::string& path(StageAsset asset)const{return asset_paths[unsigned(asset)];}
    const StageSpawnMarker* spawn(std::string_view id)const{
        const auto it=std::find_if(spawns.begin(),spawns.end(),[&](const auto& marker){return marker.id==id;});
        return it==spawns.end()?nullptr:&*it;
    }
    bool operator==(const StagePackageManifest&)const=default;
};

inline bool stage_package_manifest_valid(const StagePackageManifest& manifest){
    if(!borg_package_id_valid(manifest.id)||!display_name_valid(manifest.name,96)||
       !std::isfinite(manifest.arena_half)||manifest.arena_half<=0||manifest.arena_half>10000||
       manifest.path(StageAsset::Model).empty()||manifest.path(StageAsset::Floor).empty()||
       manifest.spawns.size()<2||manifest.spawns.size()>kStageSpawnCount)return false;
    const bool projectile=!manifest.path(StageAsset::ProjectileStage).empty();
    if(projectile!=!manifest.path(StageAsset::Secondary).empty()||projectile!=!manifest.path(StageAsset::Tertiary).empty())return false;
    for(const auto& path:manifest.asset_paths)if(!path.empty()&&!borg_package_asset_path_valid(path))return false;
    for(const auto& marker:manifest.spawns)
        if(!borg_catalog_variant_id_valid(marker.id)||manifest.spawn(marker.id)!=&marker||
           !std::isfinite(marker.query[0])||std::abs(marker.query[0])>10000||
           !std::isfinite(marker.query[1])||std::abs(marker.query[1])>20000||
           !std::isfinite(marker.query[2])||std::abs(marker.query[2])>10000)return false;
    return true;
}

inline bool stage_package_read_manifest(std::string_view text,StagePackageManifest* out,std::string* error=nullptr){
    auto fail=[&](std::string reason){if(error)*error="malformed stage manifest: "+std::move(reason);return false;};
    if(!out||text.size()>kStageManifestBytes||borg_package_detail::line(text)!="GFSTAGE 1")return fail("expected GFSTAGE 1 within 16 KiB");
    const auto number=[](std::string_view token,f32* value){
        if(token.empty())return false;
        const auto [last,status]=std::from_chars(token.data(),token.data()+token.size(),*value);
        return status==std::errc{}&&last==token.data()+token.size()&&std::isfinite(*value);
    };
    StagePackageManifest next;
    bool seen_arena=false;
    for(unsigned line=2;!text.empty();++line){
        const auto row=borg_package_detail::line(text);
        if(row.empty())continue;
        const auto bad_row=[&](){return fail("invalid or duplicate row at line "+std::to_string(line));};
        if(row.starts_with('#')){
            // Reuse the existing UTF-8 scalar validator, allowing ASCII tabs in comments.
            auto comment=row;
            while(!comment.empty()){
                const auto tab=comment.find('\t');const auto part=comment.substr(0,tab);
                if(!part.empty()&&!display_name_valid(part,kStageManifestBytes))return bad_row();
                comment.remove_prefix(tab==std::string_view::npos?comment.size():tab+1);
            }
            continue;
        }
        const auto equals=row.find('=');
        if(equals==std::string_view::npos||equals+1==row.size())return bad_row();
        const auto key=row.substr(0,equals),value=row.substr(equals+1);
        if(key=="id"||key=="name"){
            auto& target=key=="id"?next.id:next.name;
            if(!target.empty()||!(key=="id"?borg_package_id_valid(value):display_name_valid(value,96)))return bad_row();
            target=value;
        }else if(key=="arena-half"){
            if(seen_arena||!number(value,&next.arena_half))return bad_row();
            seen_arena=true;
        }else if(key=="spawn"){
            std::array<std::string_view,4> fields;
            if(next.spawns.size()>=kStageSpawnCount||!borg_package_detail::split_fields(value,&fields)||
               !borg_catalog_variant_id_valid(fields[0])||next.spawn(fields[0]))return bad_row();
            StageSpawnMarker spawn;spawn.id=fields[0];
            for(unsigned i=0;i<3;++i)if(!number(fields[i+1],&spawn.query[i]))return bad_row();
            next.spawns.push_back(std::move(spawn));
        }else{
            const auto it=std::find(kStageAssetKeys.begin(),kStageAssetKeys.end(),key);
            if(it==kStageAssetKeys.end()||!borg_package_asset_path_valid(value))return bad_row();
            auto& target=next.asset_paths[std::size_t(it-kStageAssetKeys.begin())];
            if(!target.empty())return bad_row();
            target=value;
        }
    }
    if(!stage_package_manifest_valid(next))return fail("required fields, projectile group, or numeric domain");
    *out=std::move(next);if(error)error->clear();return true;
}

// Discovery is metadata-only. The optional diagnostics on a unique row do
// not erase its identity or make its assets usable.
enum class StagePackageIssue {None,Malformed,Missing,DuplicateId,Changed,Unavailable};
enum class StagePackageScanStatus {Complete,NoDirectory,Unavailable,EntryBudget};
struct StagePackageEntry {
    std::string folder;
    StagePackageManifest manifest;
    StagePackageIssue issue=StagePackageIssue::None;
    std::string reason;
};
struct StagePackageCatalog {
    StagePackageScanStatus status=StagePackageScanStatus::Unavailable;
    std::vector<StagePackageEntry> entries;
    std::vector<StagePackageEntry> issues;
    const StagePackageEntry* find(std::string_view id)const{
        const auto it=std::lower_bound(entries.begin(),entries.end(),id,
            [](const auto& entry,std::string_view key){return entry.manifest.id<key;});
        return it!=entries.end()&&it->manifest.id==id?&*it:nullptr;
    }
};
struct StagePackageSnapshot {
    StagePackageManifest manifest;
    std::array<std::vector<u8>,kStageAssetCount> bytes;
    ByteView asset(StageAsset slot)const{
        const auto& data=bytes[unsigned(slot)];return {data.data(),data.size()};
    }
    bool valid()const{
        if(!stage_package_manifest_valid(manifest))return false;
        for(unsigned i=0;i<kStageAssetCount;++i)
            if(bytes[i].empty()!=manifest.asset_paths[i].empty()||bytes[i].size()>kStageAssetLimits[i])return false;
        return true;
    }
};
} // namespace gf
