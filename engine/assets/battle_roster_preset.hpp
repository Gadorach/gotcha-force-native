#pragma once

#include <charconv>
#include "engine/assets/borg_catalog_profile.hpp"

namespace gf {

inline constexpr std::size_t kBattleRosterPresetBytes=4096;
inline constexpr unsigned kBattleRosterPresetMaxActors=6;
inline constexpr u8 kBattleRosterSeriesMaxWins=5;
inline constexpr u16 kBattleRosterMaxRoundSeconds=600;

struct BattleRosterPresetFighter {
    u8 team;
    std::string borg,variant,spawn;
    bool operator==(const BattleRosterPresetFighter&)const=default;
};

struct BattleRosterPreset {
    std::string stage;
    std::vector<BattleRosterPresetFighter> fighters;
    u8 locals=2;
    u8 rounds_to_win=0; // omitted keeps the ordinary single round
    u16 round_seconds=0; // omitted leaves rounds untimed
    bool operator==(const BattleRosterPreset&)const=default;
};

// Match selection only. Exact packages, terrain, and physical placement
// are still admitted by the shared Battle Test loader.
inline bool battle_roster_read_preset(std::string_view text,BattleRosterPreset* out,std::string* error=nullptr){
    const auto fail=[&](std::string why){if(error)*error="malformed roster preset: "+std::move(why);return false;};
    if(!out||text.size()>kBattleRosterPresetBytes||borg_package_detail::line(text)!="GFROSTER 1")
        return fail("expected GFROSTER 1 within 4 KiB");
    BattleRosterPreset next;
    bool seen_locals=false;
    std::array<unsigned,2> teams{};
    for(unsigned line=2;!text.empty();++line){
        const auto row=borg_package_detail::line(text);
        if(row.empty())continue;
        const auto bad_row=[&](){return fail("invalid row at line "+std::to_string(line));};
        if(row.starts_with('#')){
            if(!display_name_valid(row,kBattleRosterPresetBytes))return bad_row();
        }else if(row.starts_with("stage=")){
            const auto id=row.substr(6);
            if(!next.stage.empty()||!borg_package_id_valid(id))return bad_row();
            next.stage=id;
        }else if(row.starts_with("locals=")){
            const auto value=row.substr(7);
            if(seen_locals||(value!="1"&&value!="2"))return bad_row();
            next.locals=u8(value.front()-'0');seen_locals=true;
        }else if(row.starts_with("rounds-to-win=")){
            const auto value=row.substr(std::string_view("rounds-to-win=").size());
            if(next.rounds_to_win||value.size()!=1||value.front()<'1'||
               value.front()>'0'+kBattleRosterSeriesMaxWins)return bad_row();
            next.rounds_to_win=u8(value.front()-'0');
        }else if(row.starts_with("round-seconds=")){
            const auto value=row.substr(std::string_view("round-seconds=").size());
            if(next.round_seconds||value.empty()||value.size()>3||value.front()=='0')return bad_row();
            u16 seconds=0;
            const auto [end,err]=std::from_chars(value.data(),value.data()+value.size(),seconds);
            if(err!=std::errc{}||end!=value.data()+value.size()||!seconds||seconds>kBattleRosterMaxRoundSeconds)return bad_row();
            next.round_seconds=seconds;
        }else if(row.starts_with("fighter=")){
            std::array<std::string_view,4> fields{};
            if(next.fighters.size()>=kBattleRosterPresetMaxActors||
               !borg_package_detail::split_fields(row.substr(8),&fields)||(fields[0]!="0"&&fields[0]!="1")||
               !borg_package_id_valid(fields[1])||!borg_catalog_variant_id_valid(fields[2])||
               !borg_catalog_variant_id_valid(fields[3]))return bad_row();
            const u8 team=u8(fields[0]=="1");
            if(next.fighters.empty()&&team!=0)return fail("P1 must be on team 0");
            if(++teams[team]>3)return fail("at most three fighters per team");
            if(std::any_of(next.fighters.begin(),next.fighters.end(),[&](const auto& fighter){return fighter.spawn==fields[3];}))
                return fail("spawn markers must be distinct");
            next.fighters.push_back({team,std::string(fields[1]),std::string(fields[2]),std::string(fields[3])});
        }else return bad_row();
    }
    if(next.stage.empty()||next.fighters.size()<3)return fail("stage and three to six fighters required");
    if(!teams[0]||!teams[1])return fail("both teams need at least one fighter");
    *out=std::move(next);if(error)error->clear();return true;
}

} // namespace gf
