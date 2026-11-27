#pragma once

#include <algorithm>
#include <array>
#include <charconv>
#include <span>
#include <string_view>
#include "engine/core/types.hpp"

namespace gf {

inline constexpr unsigned kPackageBattleActorCapacity=6;
inline constexpr unsigned kBorgDamageSettings=32,kBorgDamageRanks=8;

struct BattleMvpDamageIndices {
    std::array<u8,2> settings{};
    std::array<u8,kPackageBattleActorCapacity> ranks{};
    u8 rank_count=0;
    std::array<u8,kPackageBattleActorCapacity> banks{}; // optional role-table bank per actor
    u8 bank_count=0; // omitted keeps the authored team-keyed policy
    bool valid(unsigned count)const{
        return count>=2&&rank_count==count&&count<=ranks.size()&&
            std::all_of(settings.begin(),settings.end(),[](u8 i){return i<kBorgDamageSettings;})&&
            std::all_of(ranks.begin(),ranks.begin()+count,[](u8 i){return i<kBorgDamageRanks;})&&
            (!bank_count||(bank_count==count&&
             std::all_of(banks.begin(),banks.begin()+count,[](u8 i){return i<2;})));
    }
    u8 bank(unsigned slot,u8 team)const{return bank_count?banks[slot]:team;}
    bool operator==(const BattleMvpDamageIndices&)const=default;
};

// Settings follow the two teams; ranks and optional banks are in actor-slot
// order. The caller still validates the count against its roster and the
// engine still admits the associated package tables.
inline bool battle_read_damage_indices(std::string_view encoded,BattleMvpDamageIndices* out){
    constexpr auto max_length=5+4*kPackageBattleActorCapacity;
    if(!out||encoded.size()>max_length)return false;
    const auto divider=encoded.find(':');
    if(divider==encoded.npos)return false;
    const auto bank_divider=encoded.find(':',divider+1);
    const auto ranks=encoded.substr(divider+1,bank_divider==encoded.npos?encoded.npos:bank_divider-divider-1);
    const auto row=[](std::string_view value,std::span<u8> cells,unsigned limit)->u8{
        u8 n=0;
        while(!value.empty()){
            const auto comma=value.find(',');
            const auto cell=value.substr(0,comma);
            if(cell.empty()||n==cells.size())return 0;
            unsigned index=0;
            const auto end=cell.data()+cell.size();
            const auto parsed=std::from_chars(cell.data(),end,index);
            if(parsed.ec!=std::errc{}||parsed.ptr!=end||index>=limit)return 0;
            cells[n++]=u8(index);
            if(comma==value.npos)return n;
            value.remove_prefix(comma+1);
        }
        return 0;
    };
    BattleMvpDamageIndices next;
    if(row(encoded.substr(0,divider),next.settings,kBorgDamageSettings)!=next.settings.size())return false;
    next.rank_count=row(ranks,next.ranks,kBorgDamageRanks);
    if(next.rank_count<2)return false;
    if(bank_divider!=encoded.npos){
        next.bank_count=row(encoded.substr(bank_divider+1),next.banks,2);
        if(next.bank_count!=next.rank_count)return false;
    }
    *out=next;
    return true;
}

} // namespace gf
