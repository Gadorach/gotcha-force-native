#pragma once

#include <array>
#include <charconv>
#include "engine/assets/portable_battle_keys.hpp"

namespace gf {

inline constexpr std::size_t kPortableChallengeManifestBytes=64u<<10;
inline constexpr std::size_t kPortableChallengeEncounters=64;

struct PortableChallengeEncounter {
    std::string id;
    PortableBattleStage stage;
    PortableBattleSpawn player_spawn,enemy_spawn;
    PortableBattleAI ai;
    PortableBattleRules rules;
    u32 tick_limit=0;
    std::vector<PortableBattleBorg> enemies;
};

struct PortableChallengeManifest {
    std::string id,name;
    std::vector<PortableChallengeEncounter> encounters; // Declared progression order.
    const PortableChallengeEncounter* find(std::string_view encounter_id)const{
        for(const auto& encounter:encounters)if(encounter.id==encounter_id)return &encounter;
        return nullptr;
    }
};

// Parse alone does not resolve stages or Borgs, or admit an encounter to a
// runner. A valid manifest may describe more than the first adapter supports.
inline bool portable_challenge_read_manifest(std::string_view text,PortableChallengeManifest* out){
    if(!out||text.size()>kPortableChallengeManifestBytes||borg_package_detail::line(text)!="GFCHALLENGE 1")return false;
    PortableChallengeManifest next;
    struct EnemyRow {std::string encounter_id;PortableBattleBorg borg;};
    std::vector<EnemyRow> enemies;
    while(!text.empty()){
        const auto row=borg_package_detail::line(text);
        if(row.empty())continue;
        if(row.starts_with('#')){
            // Validate UTF-8 even in comments. Tabs are allowed for comment
            // formatting, but not in manifest fields.
            auto comment=row;
            while(!comment.empty()){
                const auto tab=comment.find('\t');
                const auto part=comment.substr(0,tab);
                if(!part.empty()&&!display_name_valid(part,kPortableChallengeManifestBytes))return false;
                comment.remove_prefix(tab==std::string_view::npos?comment.size():tab+1);
            }
            continue;
        }
        const auto equals=row.find('=');
        if(equals==std::string_view::npos)return false;
        const auto key=row.substr(0,equals),value=row.substr(equals+1);
        if(key=="id"){
            if(!next.id.empty()||!borg_package_id_valid(value))return false;
            next.id=value;
        }else if(key=="name"){
            if(!next.name.empty()||!display_name_valid(value,96))return false;
            next.name=value;
        }else if(key=="encounter"){
            std::array<std::string_view,7> parts;
            if(next.encounters.size()==kPortableChallengeEncounters||
               !borg_package_detail::split_fields(value,&parts))return false;
            const auto [id,stage,player,enemy,ai,rule,ticks]=parts;
            if(!borg_catalog_variant_id_valid(id)||next.find(id)||!borg_package_id_valid(stage)||
               !borg_catalog_variant_id_valid(player)||!borg_catalog_variant_id_valid(enemy)||
               !borg_catalog_variant_id_valid(ai)||!borg_catalog_variant_id_valid(rule)||ticks.empty())return false;
            u32 tick_limit=0;
            const auto end=ticks.data()+ticks.size();
            const auto parsed=std::from_chars(ticks.data(),end,tick_limit);
            if(parsed.ec!=std::errc{}||parsed.ptr!=end||tick_limit>kPortableBattleTickLimit)return false;
            next.encounters.push_back({std::string(id),{std::string(stage)},{std::string(player)},{std::string(enemy)},
                {std::string(ai)},{std::string(rule)},tick_limit,{}});
        }else if(key=="enemy"){
            // The row has no identity of its own; repeated exact Borg types
            // are independent ordered roster positions, not duplicate IDs.
            std::array<std::string_view,3> parts;
            if(enemies.size()==kPortableChallengeEncounters*kPortableBattleRosterLimit||
               !borg_package_detail::split_fields(value,&parts))return false;
            const auto [encounter,package,variant]=parts;
            if(!borg_catalog_variant_id_valid(encounter)||!borg_package_id_valid(package)||
               !borg_catalog_variant_id_valid(variant))return false;
            enemies.push_back({std::string(encounter),{std::string(package),std::string(variant)}});
        }else return false;
    }
    if(next.id.empty()||next.name.empty()||next.encounters.empty())return false;
    for(auto& enemy:enemies){
        const auto encounter=std::find_if(next.encounters.begin(),next.encounters.end(),
            [&](const auto& row){return row.id==enemy.encounter_id;});
        if(encounter==next.encounters.end()||encounter->enemies.size()==kPortableBattleRosterLimit)return false;
        encounter->enemies.push_back(std::move(enemy.borg));
    }
    for(const auto& encounter:next.encounters)if(encounter.enemies.empty())return false;
    *out=std::move(next);
    return true;
}

struct PortableChallengeEntry {
    std::string folder;
    PortableChallengeManifest manifest;
};
enum class PortableChallengeIssueKind {Malformed,Missing,DuplicateId};
struct PortableChallengeIssue {
    std::string folder,id;
    PortableChallengeIssueKind kind;
};
enum class PortableChallengeScanStatus {Complete,NoDirectory,Unavailable,EntryBudget};
struct PortableChallengeCatalog {
    PortableChallengeScanStatus status=PortableChallengeScanStatus::Unavailable;
    std::vector<PortableChallengeEntry> discovered; // Stable ID order, not battle admission.
    std::vector<PortableChallengeIssue> issues;
    const PortableChallengeEntry* find(std::string_view id)const{
        const auto it=std::lower_bound(discovered.begin(),discovered.end(),id,
            [](const auto& entry,std::string_view key){return entry.manifest.id<key;});
        return it!=discovered.end()&&it->manifest.id==id?&*it:nullptr;
    }
};

} // namespace gf
