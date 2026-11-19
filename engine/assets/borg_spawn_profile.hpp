#pragma once
#include <cmath>
#include "engine/assets/borg_catalog_profile.hpp"

namespace gf {
inline constexpr std::size_t kBorgSpawnProfileBytes=8192;
inline constexpr unsigned kBorgSpawnRules=32;
enum class BorgSpawnAnchor:u8 {Unresolved,ModeSelected,FloorOffset,ArenaTopClearance,AbsoluteY};
enum class BorgSpawnHorizontalAnchor:u8 {Unresolved,ModeSelected,StageCenter,AbsoluteXZ};
enum class BorgSpawnValue:u8 {Unresolved,None,Number};
enum class BorgSpawnState:u8 {Unresolved,Grounded,Falling,FixedAirborne,Flying,Hovering};
enum class BorgSpawnRequirement:u8 {Unresolved,Required,Preferred};
enum class BorgSpawnObstruction:u8 {Unresolved,Refuse,ModeSelected};

struct BorgSpawnRule {
    std::string id,form,mode,variant;
    BorgSpawnAnchor anchor=BorgSpawnAnchor::Unresolved;
    BorgSpawnHorizontalAnchor horizontal_anchor=BorgSpawnHorizontalAnchor::Unresolved;
    BorgSpawnValue height_kind=BorgSpawnValue::Unresolved;
    BorgSpawnValue horizontal_kind=BorgSpawnValue::Unresolved;
    f32 height=0; // world Y coordinate, floor offset, or clearance below the arena top
    std::array<f32,2> xz{}; // world X, Z; used only by AbsoluteXZ
    BorgSpawnState initial_state=BorgSpawnState::Unresolved;
    std::optional<f32> initial_vy;
    BorgSpawnRequirement requirement=BorgSpawnRequirement::Unresolved;
    BorgSpawnObstruction on_obstruction=BorgSpawnObstruction::Unresolved;
};
struct BorgSpawnProfile {std::vector<BorgSpawnRule> rules;};

namespace borg_spawn_detail {
inline bool id(std::string_view text){
    return text!="none"&&text!="unresolved"&&borg_catalog_variant_id_valid(text);
}
inline bool scope(std::string_view text){return text=="*"||id(text);}
inline bool number(std::string_view text,f32* out){
    if(text.empty()||text.size()>24)return false;
    const auto end=text.data()+text.size();
    const auto parsed=std::from_chars(text.data(),end,*out);
    return parsed.ec==std::errc{}&&parsed.ptr==end&&std::isfinite(*out)&&std::abs(*out)<=100000.f;
}
inline bool matches(std::string_view a,std::string_view b){return a=="*"||b=="*"||a==b;}
}

// Validate declarations, not stage geometry. Never pick a spawn, transform
// coordinates, clamp to a ceiling, or mint an actor through this reader.
inline bool borg_read_spawn_profile(std::string_view text,BorgSpawnProfile* out){
    if(!out||text.size()>kBorgSpawnProfileBytes||borg_package_detail::line(text)!="GFBORG-SPAWN 1")return false;
    using namespace borg_spawn_detail;
    using borg_catalog_detail::label;
    constexpr std::array<std::string_view,8> fields{
        "anchor","height","initial-state","initial-vy","requirement","on-obstruction","horizontal-anchor","horizontal"
    };
    std::array<bool,fields.size()> seen{};
    BorgSpawnProfile next;
    auto complete=[&](){
        if(!std::all_of(seen.begin(),seen.end(),[](bool present){return present;}))return false;
        const auto& rule=next.rules.back();
        if(rule.requirement==BorgSpawnRequirement::Required&&rule.on_obstruction==BorgSpawnObstruction::ModeSelected)return false;
        if(rule.horizontal_anchor==BorgSpawnHorizontalAnchor::Unresolved){
            if(rule.horizontal_kind!=BorgSpawnValue::Unresolved)return false;
        }else if(rule.horizontal_anchor==BorgSpawnHorizontalAnchor::AbsoluteXZ){
            if(rule.horizontal_kind==BorgSpawnValue::None)return false;
        }else if(rule.horizontal_kind!=BorgSpawnValue::None)return false;
        if(rule.anchor==BorgSpawnAnchor::ModeSelected)return rule.height_kind==BorgSpawnValue::None;
        if(rule.anchor==BorgSpawnAnchor::Unresolved)return rule.height_kind==BorgSpawnValue::Unresolved;
        if(rule.height_kind==BorgSpawnValue::None)return false;
        return rule.height_kind!=BorgSpawnValue::Number||rule.anchor==BorgSpawnAnchor::AbsoluteY||rule.height>=0.f;
    };
    while(!text.empty()){
        const auto row=borg_package_detail::line(text);
        if(row.empty()||row.starts_with('#'))continue;
        const auto equals=row.find('=');
        if(equals==std::string_view::npos)return false;
        const auto key=row.substr(0,equals),value=row.substr(equals+1);
        if(key=="spawn"){
            if(next.rules.size()==kBorgSpawnRules||(!next.rules.empty()&&!complete()))return false;
            std::array<std::string_view,4> selectors;
            if(!borg_package_detail::split_fields(value,&selectors))return false;
            const auto [name,form,mode,variant]=selectors;
            if(!id(name)||!scope(form)||!scope(mode)||!scope(variant))return false;
            for(const auto& existing:next.rules)if(existing.id==name||
                (matches(existing.form,form)&&matches(existing.mode,mode)&&matches(existing.variant,variant)))return false;
            BorgSpawnRule rule;rule.id=name;rule.form=form;rule.mode=mode;rule.variant=variant;
            next.rules.push_back(std::move(rule));seen={};continue;
        }
        if(next.rules.empty())return false;
        auto& rule=next.rules.back();
        const auto field=std::find(fields.begin(),fields.end(),key);
        if(field==fields.end())return false;
        const auto i=unsigned(field-fields.begin());
        if(seen[i])return false;
        seen[i]=true;
        switch(i){
        case 0:
            if(!label(value,{"unresolved","mode-selected","floor-offset","arena-top-clearance","absolute-y"},&rule.anchor))return false;
            break;
        case 1:
            if(value=="none")rule.height_kind=BorgSpawnValue::None;
            else if(value!="unresolved"){
                rule.height_kind=BorgSpawnValue::Number;
                if(!number(value,&rule.height))return false;
            }
            break;
        case 2:
            if(!label(value,{"unresolved","grounded","falling","fixed-airborne","flying","hovering"},&rule.initial_state))return false;
            break;
        case 3:
            if(value!="unresolved"){
                f32 speed=0;if(!number(value,&speed))return false;
                rule.initial_vy=speed;
            }
            break;
        case 4:if(!label(value,{"unresolved","required","preferred"},&rule.requirement))return false;break;
        case 5:if(!label(value,{"unresolved","refuse","mode-selected"},&rule.on_obstruction))return false;break;
        case 6:
            if(!label(value,{"unresolved","mode-selected","stage-center","absolute-xz"},&rule.horizontal_anchor))return false;
            break;
        case 7:
            if(value=="none")rule.horizontal_kind=BorgSpawnValue::None;
            else if(value!="unresolved"){
                const auto pipe=value.find('|');
                if(pipe==std::string_view::npos||!number(value.substr(0,pipe),&rule.xz[0])||!number(value.substr(pipe+1),&rule.xz[1]))return false;
                rule.horizontal_kind=BorgSpawnValue::Number;
            }
            break;
        }
    }
    if(next.rules.empty()||!complete())return false;
    *out=std::move(next);return true;
}
}
