#pragma once

#include <array>
#include <charconv>
#include <string_view>
#include "engine/core/types.hpp"

namespace gf {

inline constexpr u16 kBattleKoForceLimit=32767;
using BattleKoForcePools=std::array<u16,2>;

inline bool battle_ko_force_value_valid(unsigned n){return n&&n<=kBattleKoForceLimit;}

inline bool battle_read_ko_force_pools(std::string_view text,BattleKoForcePools* out){
    if(!out||text.size()>11)return false;
    const auto comma=text.find(',');
    if(comma==text.npos)return false;
    BattleKoForcePools next{};
    unsigned team=0;
    for(const auto field:{text.substr(0,comma),text.substr(comma+1)}){
        if(field.empty()||field.size()>5||field.front()=='0')return false;
        const auto end=field.data()+field.size();
        const auto [ptr,error]=std::from_chars(field.data(),end,next[team]);
        if(error!=std::errc{}||ptr!=end||!battle_ko_force_value_valid(next[team]))return false;
        ++team;
    }
    *out=next;return true;
}

} // namespace gf
