#pragma once
#include "engine/assets/borg_catalog_profile.hpp"

namespace gf {
inline constexpr std::size_t kBorgFusionProfileBytes=64u<<10;
inline constexpr unsigned kBorgFusions=32;
enum class BorgFusionResultUse:u8 {Unresolved,FusionOnly,Independent};

struct BorgFusionParty {
    std::string package,form,variant;
};
struct BorgFusionDefinition {
    std::string id;
    BorgFusionParty first,second,result;
    BorgFusionResultUse result_use=BorgFusionResultUse::Unresolved;
    std::string trigger,guard,synchronize,commit,interrupt,separation,duration;
    std::array<std::string,3> entry,exit; // first participant, second participant, result
    std::array<std::string,2> transfer; // combine, separate
};
struct BorgFusionProfile {std::vector<BorgFusionDefinition> fusions;};

namespace borg_fusion_detail {
inline bool metadata_id(std::string_view value){
    return value!="none"&&value!="unresolved"&&borg_catalog_variant_id_valid(value);
}
inline bool cue(std::string_view value){return value=="none"||value=="unresolved"||metadata_id(value);}
inline bool scoped_key(std::string_view value,bool participant){
    return value=="unresolved"||(participant&&value=="*")||metadata_id(value);
}
inline bool party(std::string_view line,bool participant,BorgFusionParty* out){
    std::array<std::string_view,3> fields;
    if(!borg_package_detail::split_fields(line,&fields))return false;
    const auto [package,form,variant]=fields;
    if(package=="unresolved"){
        if(form!="unresolved"||variant!="unresolved")return false;
    }else if(!borg_package_id_valid(package)||!scoped_key(form,participant)||!scoped_key(variant,participant))return false;
    *out={std::string(package),std::string(form),std::string(variant)};return true;
}
template<std::size_t N> bool cues(std::string_view line,std::array<std::string,N>* out){
    std::array<std::string_view,N> fields;
    if(!borg_package_detail::split_fields(line,&fields))return false;
    for(unsigned i=0;i<N;++i){
        if(!cue(fields[i]))return false;
        (*out)[i]=fields[i];
    }
    return true;
}
}

// Battle fusion metadata only, independent of inventory crystal assembly.
// Nothing here creates actors, admits a result to a picker, or transfers state.
inline bool borg_read_fusion_profile(std::string_view text,BorgFusionProfile* out){
    if(!out||text.size()>kBorgFusionProfileBytes||borg_package_detail::line(text)!="GFBORG-FUSIONS 1")return false;
    using namespace borg_fusion_detail;
    constexpr std::array<std::string_view,14> fields{
        "first","second","result","result-use","trigger","guard","synchronize","commit","interrupt","separation",
        "duration","entry","exit","transfer"
    };
    std::array<bool,fields.size()> seen{};
    BorgFusionProfile next;
    auto complete=[&](){
        if(!std::all_of(seen.begin(),seen.end(),[](bool present){return present;}))return false;
        const auto& rule=next.fusions.back();
        // This version declares a separate result package. Two roles can
        // represent the same Borg type, but must be distinct live instances
        // before a future runtime can even consider a fusion.
        return rule.result.package=="unresolved"||
            (rule.result.package!=rule.first.package&&rule.result.package!=rule.second.package);
    };
    while(!text.empty()){
        const auto row=borg_package_detail::line(text);
        if(row.empty()||row.starts_with('#'))continue;
        const auto equals=row.find('=');
        if(equals==std::string_view::npos)return false;
        const auto key=row.substr(0,equals),value=row.substr(equals+1);
        if(key=="fusion"){
            if(next.fusions.size()==kBorgFusions||(!next.fusions.empty()&&!complete())||!borg_package_id_valid(value)||
                std::any_of(next.fusions.begin(),next.fusions.end(),[&](const auto& rule){return rule.id==value;}))return false;
            BorgFusionDefinition rule;rule.id=value;
            next.fusions.push_back(std::move(rule));seen={};continue;
        }
        if(next.fusions.empty())return false;
        const auto field=std::find(fields.begin(),fields.end(),key);
        if(field==fields.end())return false;
        const auto i=unsigned(field-fields.begin());
        if(seen[i])return false;
        seen[i]=true;auto& rule=next.fusions.back();
        switch(i){
        case 0:if(!party(value,true,&rule.first))return false;break;
        case 1:if(!party(value,true,&rule.second))return false;break;
        case 2:if(!party(value,false,&rule.result))return false;break;
        case 3:
            if(!borg_catalog_detail::label(value,{"unresolved","fusion-only","independent"},&rule.result_use))return false;
            break;
        case 11:if(!cues(value,&rule.entry))return false;break;
        case 12:if(!cues(value,&rule.exit))return false;break;
        case 13:if(!cues(value,&rule.transfer))return false;break;
        default:{
            if(!cue(value))return false;
            std::string* refs[]{&rule.trigger,&rule.guard,&rule.synchronize,&rule.commit,&rule.interrupt,&rule.separation,&rule.duration};
            *refs[i-4]=value;break;
        }
        }
    }
    if(next.fusions.empty()||!complete())return false;
    *out=std::move(next);return true;
}
}
