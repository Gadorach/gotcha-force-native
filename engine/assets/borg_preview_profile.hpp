#pragma once
#include <algorithm>
#include <cmath>
#include "engine/assets/borg_catalog_profile.hpp"
#include "engine/assets/borg_cruise_boost.hpp"
#include "engine/assets/borg_ground_pace.hpp"
#include "engine/assets/character_preview_mount.hpp"

namespace gf {
inline constexpr std::size_t kBorgPreviewProfileBytes=4096;
inline constexpr unsigned kBorgPreviewExtraClips=19;
inline constexpr unsigned kBorgPreviewOrigins=16;
struct BorgPreviewClip {
    std::string id;
    u8 group=0,choice=0;
    bool candidate=false; // optional inspection clip, decoded only when selected
};
struct BorgPreviewOrigin {
    std::string id;
    u8 joint=0;
    std::array<f32,3> point{};
};
struct BorgPreviewProfile {
    std::string variant,rig_prefix,pose_profile,lab_landing_pcm;
    u8 idle_group=0,idle_choice=0;
    std::vector<BorgPreviewClip> clips;
    std::vector<CharacterPreviewMount> mounts;
    std::vector<BorgPreviewOrigin> origins;
    std::optional<BorgCruiseBoost> lab_cruise;
    std::optional<u16> lab_ground_pace;
    std::array<u16,3> lab_air_dash_starts{}; // wind-up, active, recovery
    bool whole_body=false,static_body=false;
};
inline bool borg_preview_clips_valid(const BorgPreviewProfile& p){
    if((!p.mounts.empty()&&!p.whole_body)||!character_preview_mounts_valid(p.mounts))return false;
    if(!p.lab_landing_pcm.empty()&&(!p.whole_body||p.static_body||!borg_package_asset_path_valid(p.lab_landing_pcm)))return false;
    if(p.lab_cruise&&(!p.whole_body||!borg_cruise_boost_valid(*p.lab_cruise)))return false;
    if(p.lab_cruise&&!p.static_body&&(!p.mounts.empty()||!p.lab_landing_pcm.empty()||
       std::none_of(p.clips.begin(),p.clips.end(),[](const auto& c){return c.id=="cruise-entry"&&c.candidate;})))return false;
    if(p.lab_ground_pace&&(!p.whole_body||p.static_body||p.lab_cruise||!borg_lab_ground_pace_valid(*p.lab_ground_pace)||
        std::none_of(p.clips.begin(),p.clips.end(),[](const auto& c){return c.id=="ground-travel";})))return false;
    if(std::any_of(p.lab_air_dash_starts.begin(),p.lab_air_dash_starts.end(),[](u16 n){return n!=0;})){
        if(!p.whole_body||p.static_body||p.lab_cruise||
           std::any_of(p.lab_air_dash_starts.begin(),p.lab_air_dash_starts.end(),[](u16 n){return n>600;}))return false;
        for(auto role:{"ground-travel","ground-takeoff","air-continuation","landing",
                       "air-dash-wind-up","air-dash-active","air-dash-recovery"})
            if(std::none_of(p.clips.begin(),p.clips.end(),[role](const auto& c){return c.id==role&&c.candidate;}))return false;
    }
    if(p.origins.size()>kBorgPreviewOrigins)return false;
    for(unsigned i=0;i<p.origins.size();++i){
        const auto& origin=p.origins[i];
        if(origin.joint>=32||!borg_catalog_variant_id_valid(origin.id))return false;
        for(auto x:origin.point)if(!std::isfinite(x)||std::abs(x)>1e5f)return false;
        for(unsigned j=0;j<i;++j)if(p.origins[j].id==origin.id)return false;
    }
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
    const bool air_starts=header=="GFBORG-PREVIEW 9";
    const bool animated_cruise=header=="GFBORG-PREVIEW 8";
    const bool pace=header=="GFBORG-PREVIEW 7";
    const bool cruise=header=="GFBORG-PREVIEW 6";
    const bool origins=header=="GFBORG-PREVIEW 5";
    const bool lab_audio=header=="GFBORG-PREVIEW 4";
    const bool assembly=header=="GFBORG-PREVIEW 3";
    const bool extra=header=="GFBORG-PREVIEW 2"||assembly||lab_audio||origins||cruise||pace||animated_cruise||air_starts;
    if(!extra&&header!="GFBORG-PREVIEW 1")return false;
    BorgPreviewProfile next;bool format=false,idle=false,has_air_starts=false;
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
        if((key=="clip"||key=="candidate")&&extra){
            const auto separator=value.find('|');
            BorgPreviewClip clip;
            if(next.clips.size()>=kBorgPreviewExtraClips||separator==std::string_view::npos||
               !borg_catalog_variant_id_valid(value.substr(0,separator))||
               !parse_clip(value.substr(separator+1),&clip.group,&clip.choice))return false;
            clip.id=value.substr(0,separator);clip.candidate=key=="candidate";
            next.clips.push_back(std::move(clip));continue;
        }
        if(key=="mount"&&(assembly||lab_audio||origins||cruise||pace||air_starts)){
            if(next.mounts.size()>=kCharacterPreviewMounts)return false;
            auto rest=value;
            CharacterPreviewMount mount;
            for(unsigned i=0;i<5;++i){
                const auto cut=rest.find('|');
                if((i<4)!=(cut!=std::string_view::npos))return false;
                const auto field=rest.substr(0,cut);
                if(field.empty()||field.size()>5)return false;
                s32 number=0;
                const auto parsed=std::from_chars(field.data(),field.data()+field.size(),number);
                if(parsed.ec!=std::errc{}||parsed.ptr!=field.data()+field.size())return false;
                if(i<2){
                    if(number<0||number>=32)return false;
                    (i==0?mount.record:mount.bone)=u8(number);
                }else{
                    if(number< -4096||number>4096)return false;
                    mount.offset[i-2]=s16(number);
                }
                if(i<4)rest.remove_prefix(cut+1);
            }
            next.mounts.push_back(mount);continue;
        }
        if(key=="lab-landing-pcm"&&(lab_audio||origins||pace||air_starts)){
            if(!next.lab_landing_pcm.empty()||!borg_package_asset_path_valid(value))return false;
            next.lab_landing_pcm=value;continue;
        }
        if(key=="origin"&&(origins||cruise||pace||animated_cruise||air_starts)){
            if(next.origins.size()>=kBorgPreviewOrigins)return false;
            BorgPreviewOrigin origin;auto rest=value;
            for(unsigned i=0;i<5;++i){
                const auto cut=rest.find('|');
                if((i<4)!=(cut!=std::string_view::npos))return false;
                const auto field=rest.substr(0,cut);
                if(i==0){
                    if(!borg_catalog_variant_id_valid(field))return false;
                    origin.id=field;
                }else if(i==1){
                    if(!parse_number(field,32,&origin.joint))return false;
                }else{
                    if(field.empty()||field.size()>24)return false;
                    auto& number=origin.point[i-2];
                    const auto parsed=std::from_chars(field.data(),field.data()+field.size(),number);
                    if(parsed.ec!=std::errc{}||parsed.ptr!=field.data()+field.size())return false;
                }
                if(i<4)rest.remove_prefix(cut+1);
            }
            next.origins.push_back(std::move(origin));continue;
        }
        if(key=="lab-cruise"&&(cruise||animated_cruise)){
            BorgCruiseBoost spec;
            if(next.lab_cruise||!borg_read_cruise_boost(value,&spec))return false;
            next.lab_cruise=spec;continue;
        }
        if(key=="lab-ground-pace"&&(pace||air_starts)){
            u16 target=0;
            if(next.lab_ground_pace||value.empty()||value.size()>4)return false;
            const auto end=value.data()+value.size();
            const auto parsed=std::from_chars(value.data(),end,target);
            if(parsed.ec!=std::errc{}||parsed.ptr!=end||!borg_lab_ground_pace_valid(target))return false;
            next.lab_ground_pace=target;continue;
        }
        if(key=="lab-air-dash-starts"&&air_starts){
            if(has_air_starts)return false;
            auto rest=value;bool nonzero=false;
            for(unsigned i=0;i<3;++i){
                const auto cut=rest.find('|');
                if((i<2)!=(cut!=rest.npos))return false;
                const auto field=rest.substr(0,cut);u16 frame=0;
                if(field.empty()||field.size()>3)return false;
                const auto parsed=std::from_chars(field.data(),field.data()+field.size(),frame);
                if(parsed.ec!=std::errc{}||parsed.ptr!=field.data()+field.size()||frame>600)return false;
                next.lab_air_dash_starts[i]=frame;nonzero|=frame!=0;
                if(i<2)rest.remove_prefix(cut+1);
            }
            if(!nonzero)return false;
            has_air_starts=true;continue;
        }
        auto* target=key=="variant"?&next.variant:key=="rig-prefix"?&next.rig_prefix:key=="pose-profile"?&next.pose_profile:nullptr;
        if(!target||!target->empty()||!(key=="variant"?borg_catalog_variant_id_valid(value):borg_package_asset_path_valid(value)))return false;
        *target=value;
    }
    if(!format||!idle||next.variant.empty()||next.rig_prefix.empty()||
       next.whole_body!=next.pose_profile.empty()||(assembly&&next.mounts.empty())||
        (lab_audio&&next.lab_landing_pcm.empty())||(origins&&next.origins.empty())||
         (cruise&&(!next.static_body||!next.lab_cruise))||(animated_cruise&&(next.static_body||!next.lab_cruise))||
          (pace&&!next.lab_ground_pace)||(air_starts&&!has_air_starts)||!borg_preview_clips_valid(next))return false;
    *out=std::move(next);return true;
}
}
