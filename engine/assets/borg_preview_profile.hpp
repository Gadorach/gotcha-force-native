#pragma once
#include "engine/assets/borg_catalog_profile.hpp"

namespace gf {
inline constexpr std::size_t kBorgPreviewProfileBytes=4096;
struct BorgPreviewProfile {
    std::string variant,rig_prefix,pose_profile;
    u8 idle_group=0,idle_choice=0;
};
// An explicit raw idle mapping for one default variant in the currently
// supported HSD body format. This does not load source programs or rules.
inline bool borg_read_preview_profile(std::string_view text,BorgPreviewProfile* out){
    if(!out||text.size()>kBorgPreviewProfileBytes||borg_package_detail::line(text)!="GFBORG-PREVIEW 1")return false;
    BorgPreviewProfile next;bool format=false,idle=false;
    auto parse_number=[](std::string_view text,u32 limit,u8* out){
        if(text.empty()||text.size()>3)return false;
        const auto end=text.data()+text.size();u32 value=0;
        const auto r=std::from_chars(text.data(),end,value);
        if(r.ec!=std::errc{}||r.ptr!=end||value>=limit)return false;
        *out=u8(value);return true;
    };
    while(!text.empty()){
        const auto row=borg_package_detail::line(text);
        if(row.empty()||row.starts_with('#'))continue;
        const auto equals=row.find('=');
        if(equals==std::string_view::npos)return false;
        const auto key=row.substr(0,equals),value=row.substr(equals+1);
        if(key=="model-format"){
            if(format||value!="hsd-body-v1")return false;
            format=true;continue;
        }
        if(key=="idle"){
            const auto colon=value.find(':');
            if(idle||colon==std::string_view::npos||!parse_number(value.substr(0,colon),16,&next.idle_group)||
                !parse_number(value.substr(colon+1),128,&next.idle_choice))return false;
            idle=true;continue;
        }
        auto* target=key=="variant"?&next.variant:key=="rig-prefix"?&next.rig_prefix:key=="pose-profile"?&next.pose_profile:nullptr;
        if(!target||!target->empty()||!(key=="variant"?borg_catalog_variant_id_valid(value):borg_package_asset_path_valid(value)))return false;
        *target=value;
    }
    if(!format||!idle||next.variant.empty()||next.rig_prefix.empty()||next.pose_profile.empty())return false;
    *out=std::move(next);return true;
}
}
