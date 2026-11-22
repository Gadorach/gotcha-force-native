#pragma once
#include "engine/assets/borg_catalog_profile.hpp"

namespace gf {
inline constexpr std::size_t kBorgPreviewProfileBytes=4096;
inline constexpr unsigned kBorgPreviewExtraClips=7;
struct BorgPreviewClip {
    std::string id;
    u8 group=0,choice=0;
};
struct BorgPreviewProfile {
    std::string variant,rig_prefix,pose_profile;
    u8 idle_group=0,idle_choice=0;
    std::vector<BorgPreviewClip> clips;
    bool whole_body=false,static_body=false;
};
inline bool borg_preview_clips_valid(const BorgPreviewProfile& p){
    if(p.static_body)return p.whole_body&&p.clips.empty()&&p.idle_group==0&&p.idle_choice==0;
    if(p.idle_group>=16||p.idle_choice>=128||p.clips.size()>kBorgPreviewExtraClips)return false;
    for(std::size_t i=0;i<p.clips.size();++i){
        const auto& c=p.clips[i];
        if(c.id=="idle"||!borg_catalog_variant_id_valid(c.id)||c.group>=16||c.choice>=128)return false;
        for(std::size_t j=0;j<i;++j)if(p.clips[j].id==c.id)return false;
    }
    return true;
}
// Explicit raw clip bindings for one exact variant. No source programs or rules.
inline bool borg_read_preview_profile(std::string_view text,BorgPreviewProfile* out){
    if(!out||text.size()>kBorgPreviewProfileBytes)return false;
    const auto header=borg_package_detail::line(text);
    const bool extra=header=="GFBORG-PREVIEW 2";
    if(!extra&&header!="GFBORG-PREVIEW 1")return false;
    BorgPreviewProfile next;bool format=false,idle=false;
    auto parse_number=[](std::string_view text,u32 limit,u8* out){
        if(text.empty()||text.size()>3)return false;
        const auto end=text.data()+text.size();u32 value=0;
        const auto r=std::from_chars(text.data(),end,value);
        if(r.ec!=std::errc{}||r.ptr!=end||value>=limit)return false;
        *out=u8(value);return true;
    };
    auto parse_clip=[&](std::string_view value,u8* group,u8* choice){
        const auto colon=value.find(':');
        return colon!=std::string_view::npos&&parse_number(value.substr(0,colon),16,group)&&
            parse_number(value.substr(colon+1),128,choice);
    };
    while(!text.empty()){
        const auto row=borg_package_detail::line(text);
        if(row.empty()||row.starts_with('#'))continue;
        const auto equals=row.find('=');
        if(equals==std::string_view::npos)return false;
        const auto key=row.substr(0,equals),value=row.substr(equals+1);
        if(key=="model-format"){
            if(format||(value!="hsd-body-v1"&&(!extra||value!="hsd-whole-body-v1")))return false;
            next.whole_body=value=="hsd-whole-body-v1";
            format=true;continue;
        }
        if(key=="idle"){
            if(idle)return false;
            if(extra&&value=="none")next.static_body=true;
            else if(!parse_clip(value,&next.idle_group,&next.idle_choice))return false;
            idle=true;continue;
        }
        if(key=="clip"&&extra){
            const auto separator=value.find('|');
            BorgPreviewClip clip;
            if(next.clips.size()>=kBorgPreviewExtraClips||separator==std::string_view::npos||
               !borg_catalog_variant_id_valid(value.substr(0,separator))||
               !parse_clip(value.substr(separator+1),&clip.group,&clip.choice))return false;
            clip.id=value.substr(0,separator);next.clips.push_back(std::move(clip));continue;
        }
        auto* target=key=="variant"?&next.variant:key=="rig-prefix"?&next.rig_prefix:key=="pose-profile"?&next.pose_profile:nullptr;
        if(!target||!target->empty()||!(key=="variant"?borg_catalog_variant_id_valid(value):borg_package_asset_path_valid(value)))return false;
        *target=value;
    }
    if(!format||!idle||next.variant.empty()||next.rig_prefix.empty()||
       next.whole_body!=next.pose_profile.empty()||!borg_preview_clips_valid(next))return false;
    *out=std::move(next);return true;
}
}
