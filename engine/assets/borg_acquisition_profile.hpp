#pragma once
#include "engine/assets/borg_catalog_profile.hpp"

namespace gf {
inline constexpr std::size_t kBorgAcquisitionBytes=64u<<10;
inline constexpr unsigned kBorgAcquisitionRoutes=64,kBorgCrystalRecipes=32,kBorgCrystalRecipeParts=16;
enum class BorgAcquisitionMethod:u8 {Unresolved,StartGrant,EncounterDrop,ProgressReward,CrystalAssembly,BattleFusion,ScriptedLoan};
enum class BorgAcquisitionGrant:u8 {Unresolved,Owned,CrystalPart,BattleOnly,Loan};
enum class BorgAcquisitionRepeat:u8 {Unresolved,OncePerSave,OncePerPlay,Repeatable};
enum class BorgOwnershipPermission:u8 {Unresolved,Allowed,Blocked};

struct BorgAcquisitionRoute {
    std::string id,mode,variant,source,condition,component,retirement;
    BorgAcquisitionMethod method=BorgAcquisitionMethod::Unresolved;
    BorgAcquisitionGrant grant=BorgAcquisitionGrant::Unresolved;
    BorgAcquisitionRepeat repeat=BorgAcquisitionRepeat::Unresolved;
    BorgOwnershipPermission trade=BorgOwnershipPermission::Unresolved,discard=BorgOwnershipPermission::Unresolved;
};
struct BorgCrystalRecipe {
    std::string id,variant;
    std::vector<std::string> parts;
};
struct BorgAcquisitionProfile {
    std::vector<BorgAcquisitionRoute> routes;
    std::vector<BorgCrystalRecipe> recipes;
    const BorgCrystalRecipe* recipe(std::string_view id)const{
        for(const auto& row:recipes)if(row.id==id)return &row;
        return nullptr;
    }
};

namespace borg_acquisition_detail {
inline bool id(std::string_view value){
    return value!="none"&&value!="unresolved"&&borg_catalog_variant_id_valid(value);
}
inline bool ref(std::string_view value){
    return value=="none"||value=="unresolved"||id(value)||borg_package_id_valid(value);
}
inline bool selector(std::string_view value){
    return value=="*"||value=="unresolved"||id(value);
}
inline bool recipe_fields(std::string_view text,BorgCrystalRecipe* out){
    const auto first=text.find('|');
    if(first==std::string_view::npos||!id(text.substr(0,first)))return false;
    out->id=text.substr(0,first);text.remove_prefix(first+1);
    const auto second=text.find('|');
    if(second==std::string_view::npos||text.substr(0,second)=="*"||!selector(text.substr(0,second)))return false;
    out->variant=text.substr(0,second);text.remove_prefix(second+1);
    for(;;){
        const auto pipe=text.find('|');
        const auto part=text.substr(0,pipe);
        if(out->parts.size()==kBorgCrystalRecipeParts||!id(part)||
            std::find(out->parts.begin(),out->parts.end(),part)!=out->parts.end())return false;
        out->parts.emplace_back(part);
        if(pipe==std::string_view::npos)break;
        text.remove_prefix(pipe+1);
    }
    return out->parts.size()>=2;
}
inline bool route_valid(const BorgAcquisitionRoute& route){
    if(route.grant==BorgAcquisitionGrant::CrystalPart){
        if(route.component=="none")return false;
    }else if(route.grant!=BorgAcquisitionGrant::Unresolved&&id(route.component))return false;
    if(route.method==BorgAcquisitionMethod::BattleFusion&&route.grant!=BorgAcquisitionGrant::Unresolved&&
        route.grant!=BorgAcquisitionGrant::BattleOnly)return false;
    if(route.method==BorgAcquisitionMethod::ScriptedLoan&&route.grant!=BorgAcquisitionGrant::Unresolved&&
        route.grant!=BorgAcquisitionGrant::Loan)return false;
    if(route.method==BorgAcquisitionMethod::CrystalAssembly&&
        ((route.grant!=BorgAcquisitionGrant::Unresolved&&route.grant!=BorgAcquisitionGrant::Owned)||
         route.variant=="*"||!(route.source=="unresolved"||id(route.source))))return false;
    if((route.grant==BorgAcquisitionGrant::BattleOnly||route.grant==BorgAcquisitionGrant::Loan)&&
       (route.trade==BorgOwnershipPermission::Allowed||route.discard==BorgOwnershipPermission::Allowed))return false;
    return true;
}
}

// Reference data for package authoring. No inventory, mission, collection
// state, random drop, recipe consumption, or reward can be produced here.
inline bool borg_read_acquisition_profile(std::string_view text,BorgAcquisitionProfile* out){
    if(!out||text.size()>kBorgAcquisitionBytes||borg_package_detail::line(text)!="GFBORG-ACQUISITION 1")return false;
    using namespace borg_acquisition_detail;
    using borg_catalog_detail::label;
    constexpr std::array<std::string_view,8> fields{"source","condition","grant","component","repeat","trade","discard","retirement"};
    std::array<bool,fields.size()> seen{};
    BorgAcquisitionProfile next;bool in_route=false;
    auto complete=[&](){return !in_route||(std::all_of(seen.begin(),seen.end(),[](bool present){return present;})&&route_valid(next.routes.back()));};
    while(!text.empty()){
        const auto row=borg_package_detail::line(text);
        if(row.empty()||row.starts_with('#'))continue;
        const auto equals=row.find('=');
        if(equals==std::string_view::npos)return false;
        const auto key=row.substr(0,equals),value=row.substr(equals+1);
        if(key=="recipe"){
            BorgCrystalRecipe recipe;
            if(!complete()||next.recipes.size()==kBorgCrystalRecipes||!recipe_fields(value,&recipe)||next.recipe(recipe.id))return false;
            in_route=false;next.recipes.push_back(std::move(recipe));continue;
        }
        if(key=="route"){
            if(!complete()||next.routes.size()==kBorgAcquisitionRoutes)return false;
            std::array<std::string_view,4> parts;
            if(!borg_package_detail::split_fields(value,&parts))return false;
            const auto [name,mode,variant,method]=parts;
            BorgAcquisitionRoute route;
            if(!id(name)||!selector(mode)||!selector(variant)||
               !label(method,{"unresolved","start-grant","encounter-drop","progress-reward","crystal-assembly","battle-fusion","scripted-loan"},&route.method)||
               std::any_of(next.routes.begin(),next.routes.end(),[&](const auto& previous){return previous.id==name;}))return false;
            route.id=name;route.mode=mode;route.variant=variant;
            in_route=true;seen={};next.routes.push_back(std::move(route));continue;
        }
        if(!in_route)return false;
        const auto field=std::find(fields.begin(),fields.end(),key);
        if(field==fields.end())return false;
        const auto i=unsigned(field-fields.begin());
        if(seen[i])return false;
        seen[i]=true;auto& route=next.routes.back();
        switch(i){
        case 0:if(!ref(value))return false;route.source=value;break;
        case 1:if(!ref(value))return false;route.condition=value;break;
        case 2:if(!label(value,{"unresolved","owned","crystal-part","battle-only","loan"},&route.grant))return false;break;
        case 3:if(value!="none"&&value!="unresolved"&&!id(value))return false;route.component=value;break;
        case 4:if(!label(value,{"unresolved","once-per-save","once-per-play","repeatable"},&route.repeat))return false;break;
        case 5:if(!label(value,{"unresolved","allowed","blocked"},&route.trade))return false;break;
        case 6:if(!label(value,{"unresolved","allowed","blocked"},&route.discard))return false;break;
        case 7:if(!ref(value))return false;route.retirement=value;break;
        }
    }
    if((next.routes.empty()&&next.recipes.empty())||!complete())return false;
    for(const auto& route:next.routes)if(route.method==BorgAcquisitionMethod::CrystalAssembly&&route.source!="unresolved"){
        const auto* recipe=next.recipe(route.source);
        if(!recipe||(recipe->variant!="unresolved"&&route.variant!="unresolved"&&recipe->variant!=route.variant))return false;
    }
    *out=std::move(next);return true;
}
}
