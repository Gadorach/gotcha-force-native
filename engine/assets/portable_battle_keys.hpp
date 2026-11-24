#pragma once

#include "engine/assets/borg_catalog_profile.hpp"

namespace gf {

inline constexpr std::size_t kPortableBattleRosterLimit=30;
inline constexpr u32 kPortableBattleTickLimit=216000;
inline constexpr std::string_view kPortableBattleDuelAI="duel-ai-v1";
inline constexpr std::string_view kPortableBattleDefeatOpponent="defeat-opponent";

struct PortableBattleStage {
    std::string id;
    bool operator==(const PortableBattleStage&)const=default;
};
struct PortableBattleSpawn {
    std::string id;
    bool operator==(const PortableBattleSpawn&)const=default;
};
// These are typed names, never executable profiles. A syntactically valid
// name can be listed before this adapter knows how to run it.
struct PortableBattleAI {
    std::string key;
    bool operator==(const PortableBattleAI&)const=default;
};
struct PortableBattleRules {
    std::string key;
    bool operator==(const PortableBattleRules&)const=default;
};
struct PortableBattleBorg {
    std::string package_id,variant_id;
    bool operator==(const PortableBattleBorg&)const=default;
};

inline bool portable_battle_borg_valid(const PortableBattleBorg& borg){
    return borg_package_id_valid(borg.package_id)&&borg_catalog_variant_id_valid(borg.variant_id);
}

} // namespace gf
