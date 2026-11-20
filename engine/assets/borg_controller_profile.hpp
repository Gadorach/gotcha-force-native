#pragma once
#include "engine/assets/borg_catalog_profile.hpp"

namespace gf {
inline constexpr std::size_t kBorgControllerProfileBytes=16u<<10;
inline constexpr unsigned kBorgControllerDefinitions=64;

enum class BorgControllerMovement:u8 {Unresolved,None,Grounded,Hover,ContinuousFlight,FixedAerial,Other};
enum class BorgControllerContact:u8 {Unresolved,None,SolidFloor,HoverFloor,OpenFlight,Anchored,Other};
enum class BorgControllerAirResource:u8 {Unresolved,None,AirJumps,FlightMeter,BoostMeter};
enum class BorgControllerRecovery:u8 {Unresolved,None,Grounded,NotBoosting};
enum class BorgControllerBank:u8 {Unresolved,None,Local};
enum class BorgControllerPose:u8 {Unresolved,None,ClipBased,ControlOnly,ClassOwned};
enum class BorgControllerActions:u8 {Unresolved,None,ClassOwned};
enum class BorgControllerEvidence:u8 {Unresolved,Authored,SourceAudit};

// Explicit variant/form metadata, not a runtime controller. A selector family
// is an opaque comparison label, never a selector table or an inherited rule.
struct BorgControllerDefinition {
    std::string variant,form,source,selector_family;
    std::optional<u8> source_movement_type,air_jump_points;
    BorgControllerMovement movement=BorgControllerMovement::Unresolved;
    BorgControllerContact contact=BorgControllerContact::Unresolved;
    BorgControllerAirResource air_resource=BorgControllerAirResource::Unresolved;
    BorgControllerRecovery recovery=BorgControllerRecovery::Unresolved;
    BorgControllerBank motion_bank=BorgControllerBank::Unresolved;
    BorgControllerPose pose=BorgControllerPose::Unresolved;
    BorgControllerActions actions=BorgControllerActions::Unresolved;
    BorgControllerEvidence evidence=BorgControllerEvidence::Unresolved;
};

struct BorgControllerProfile {
    std::vector<BorgControllerDefinition> definitions;
    const BorgControllerDefinition* find(std::string_view variant,std::string_view form)const{
        for(const auto& row:definitions)if(row.variant==variant&&row.form==form)return &row;
        return nullptr;
    }
};

namespace borg_controller_detail {
inline constexpr std::array<std::string_view,7> movements{
    "unresolved","none","grounded","hover","continuous-flight","fixed-aerial","other"};
inline constexpr std::array<std::string_view,7> contacts{
    "unresolved","none","solid-floor","hover-floor","open-flight","anchored","other"};
inline constexpr std::array<std::string_view,5> resources{"unresolved","none","air-jumps","flight-meter","boost-meter"};
inline constexpr std::array<std::string_view,4> recoveries{"unresolved","none","grounded","not-boosting"};
inline constexpr std::array<std::string_view,3> banks{"unresolved","none","local"};
inline constexpr std::array<std::string_view,5> poses{"unresolved","none","clip-based","control-only","class-owned"};
inline constexpr std::array<std::string_view,3> actions{"unresolved","none","class-owned"};
inline constexpr std::array<std::string_view,3> evidence{"unresolved","authored","source-audit"};

template<class T,std::size_t N> bool read_label(std::string_view value,const std::array<std::string_view,N>& names,T* out){
    const auto it=std::find(names.begin(),names.end(),value);
    if(it==names.end())return false;
    *out=T(it-names.begin());
    return true;
}
inline bool id(std::string_view value){
    return borg_catalog_variant_id_valid(value)&&value!="none"&&value!="unresolved"&&value!="inherit";
}
inline bool source(std::string_view value){
    if(value=="none"||value=="unresolved")return true;
    if(value.size()<13||value.size()>15||value[6]!=':'||value[11]!='/'||!borg_catalog_detail::source_code(value.substr(7,4)))return false;
    for(char c:value.substr(0,6))if(!((c>='A'&&c<='Z')||(c>='0'&&c<='9')))return false;
    u32 variant=0;
    return borg_catalog_detail::unsigned_field(value.substr(12),255,&variant);
}
inline bool air_resource(std::string_view value,BorgControllerDefinition* row){
    if(value.starts_with("air-jumps:")){
        row->air_resource=BorgControllerAirResource::AirJumps;
        return borg_catalog_detail::optional_number(value.substr(10),255,1,&row->air_jump_points);
    }
    return value!="air-jumps"&&read_label(value,resources,&row->air_resource);
}
}

inline bool borg_read_controller_profile(std::string_view text,BorgControllerProfile* out){
    if(!out||text.size()>kBorgControllerProfileBytes||borg_package_detail::line(text)!="GFBORG-CONTROLLERS 1")return false;
    using namespace borg_controller_detail;
    constexpr std::array<std::string_view,11> fields{
        "source","source-movement-type","selector-family","movement","contact","air-resource",
        "recovery","motion-bank","pose","actions","evidence"};
    BorgControllerProfile next;std::array<bool,fields.size()> seen{};
    auto complete=[&](){
        if(next.definitions.empty())return true;
        const auto& row=next.definitions.back();
        return std::all_of(seen.begin(),seen.end(),[](bool bit){return bit;})&&
            (row.evidence!=BorgControllerEvidence::SourceAudit||(row.source!="none"&&row.source!="unresolved"))&&
            (row.air_resource!=BorgControllerAirResource::None||row.recovery==BorgControllerRecovery::None||
                row.recovery==BorgControllerRecovery::Unresolved);
    };
    while(!text.empty()){
        const auto line=borg_package_detail::line(text);
        if(line.empty()||line.starts_with('#'))continue;
        const auto equals=line.find('=');
        if(equals==std::string_view::npos)return false;
        const auto key=line.substr(0,equals),value=line.substr(equals+1);
        if(key=="controller"){
            std::array<std::string_view,2> parts;
            if(next.definitions.size()==kBorgControllerDefinitions||!complete()||!borg_package_detail::split_fields(value,&parts)||
               !id(parts[0])||!(id(parts[1])||parts[1]=="none"||parts[1]=="unresolved")||next.find(parts[0],parts[1]))return false;
            BorgControllerDefinition row;row.variant=parts[0];row.form=parts[1];
            next.definitions.push_back(std::move(row));seen={};continue;
        }
        const auto field=std::find(fields.begin(),fields.end(),key);
        if(next.definitions.empty()||field==fields.end())return false;
        const auto i=unsigned(field-fields.begin());
        if(seen[i])return false;
        seen[i]=true;
        auto& row=next.definitions.back();
        switch(i){
        case 0:if(!source(value))return false;row.source=value;break;
        case 1:if(!borg_catalog_detail::optional_number(value,255,0,&row.source_movement_type))return false;break;
        case 2:if(!(id(value)||value=="none"||value=="unresolved"))return false;row.selector_family=value;break;
        case 3:if(!read_label(value,movements,&row.movement))return false;break;
        case 4:if(!read_label(value,contacts,&row.contact))return false;break;
        case 5:if(!air_resource(value,&row))return false;break;
        case 6:if(!read_label(value,recoveries,&row.recovery))return false;break;
        case 7:if(!read_label(value,banks,&row.motion_bank))return false;break;
        case 8:if(!read_label(value,poses,&row.pose))return false;break;
        case 9:if(!read_label(value,actions,&row.actions))return false;break;
        case 10:if(!read_label(value,evidence,&row.evidence))return false;break;
        }
    }
    if(next.definitions.empty()||!complete())return false;
    *out=std::move(next);return true;
}
} // namespace gf
