#pragma once
#include <array>
#include "engine/core/types.hpp"
namespace gf {
// Native content uses stable seal IDs, not guest addresses. Columns are
// Up/Down/Left/Right. Boundary self-edges are valid; implicit wraparound is not.
struct MenuNavigationConfig {
    std::array<std::array<u8,4>,7> neighbor{};
    u8 repeat_limit=0,repeat_restart=0;
};
inline bool menu_navigation_valid(const MenuNavigationConfig& config){
    if(config.repeat_limit==255||config.repeat_restart>config.repeat_limit)return false;
    for(const auto& row:config.neighbor)for(u8 next:row)if(next>=7)return false;
    // All seven entries must remain reachable, including return paths.
    for(unsigned start=0;start<7;++start){
        std::array<bool,7> seen{};seen[start]=true;
        for(unsigned pass=0;pass<7;++pass)for(unsigned node=0;node<7;++node)if(seen[node])
            for(u8 next:config.neighbor[node])seen[next]=true;
        for(bool found:seen)if(!found)return false;
    }
    return true;
}
} // namespace gf
