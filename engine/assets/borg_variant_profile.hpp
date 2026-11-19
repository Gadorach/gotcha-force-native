#pragma once
#include "engine/assets/borg_catalog_profile.hpp"

namespace gf {
inline constexpr std::size_t kBorgVariantProfileBytes=16u<<10;
enum class BorgVariantAssetRole:u8 {Model,Animations,Preview,Textures,Materials,Stats,Movement,Moves,Collision,Effects,Acquisition,Count};
enum class BorgVariantAssetKind:u8 {Unresolved,Inherit,None,LocalFile};
enum class BorgVariantLevelKind:u8 {Unresolved,Inherit,Offset};
struct BorgVariantAsset {
    BorgVariantAssetKind kind=BorgVariantAssetKind::Unresolved;
    std::string file;
};
struct BorgVariantLevelOffset {
    BorgVariantLevelKind kind=BorgVariantLevelKind::Unresolved;
    s16 value=0;
};
struct BorgVariantOverride {
    std::string variant;
    std::array<BorgVariantAsset,unsigned(BorgVariantAssetRole::Count)> assets;
    BorgVariantLevelOffset hp,ammo;
    const BorgVariantAsset& asset(BorgVariantAssetRole role)const{return assets[unsigned(role)];}
};
struct BorgVariantProfile {
    std::vector<BorgVariantOverride> variants;
    const BorgVariantOverride* find(std::string_view id)const{
        for(const auto& row:variants)if(row.variant==id)return &row;
        return nullptr;
    }
};
namespace borg_variant_detail {
inline bool asset(std::string_view value,BorgVariantAsset* out){
    if(value=="unresolved")return true;
    if(value=="inherit"){out->kind=BorgVariantAssetKind::Inherit;return true;}
    if(value=="none"){out->kind=BorgVariantAssetKind::None;return true;}
    if(!borg_package_asset_path_valid(value))return false;
    out->kind=BorgVariantAssetKind::LocalFile;out->file=value;return true;
}
inline bool offset(std::string_view value,BorgVariantLevelOffset* out){
    if(value=="unresolved")return true;
    if(value=="inherit"){out->kind=BorgVariantLevelKind::Inherit;return true;}
    if(value.empty()||value.size()>4)return false;
    if(value.starts_with('+')){
        value.remove_prefix(1);
        if(value.empty()||value.front()<'0'||value.front()>'9')return false;
    }
    const auto end=value.data()+value.size();s32 result=0;
    const auto parsed=std::from_chars(value.data(),end,result);
    if(parsed.ec!=std::errc{}||parsed.ptr!=end||result< -255||result>255)return false;
    out->kind=BorgVariantLevelKind::Offset;out->value=s16(result);return true;
}
}
// Asset and statistic declarations only. No original colour mapping, level
// calculation, cosmetic synthesis, or gameplay variant is admitted here.
inline bool borg_read_variant_profile(std::string_view text,BorgVariantProfile* out){
    if(!out||text.size()>kBorgVariantProfileBytes||borg_package_detail::line(text)!="GFBORG-VARIANTS 1")return false;
    constexpr unsigned assets=unsigned(BorgVariantAssetRole::Count);
    constexpr std::array<std::string_view,assets+2> fields{
        "model","animations","preview","textures","materials","stats","movement","moves","collision","effects","acquisition",
        "hp-level-offset","ammo-level-offset"
    };
    std::array<bool,fields.size()> seen{};
    BorgVariantProfile next;
    auto complete=[&](){return std::all_of(seen.begin(),seen.end(),[](bool set){return set;});};
    while(!text.empty()){
        const auto row=borg_package_detail::line(text);
        if(row.empty()||row.starts_with('#'))continue;
        const auto equals=row.find('=');
        if(equals==std::string_view::npos)return false;
        const auto key=row.substr(0,equals),value=row.substr(equals+1);
        if(key=="variant"){
            if(next.variants.size()==kBorgCatalogVariants||(!next.variants.empty()&&!complete())||
                !borg_catalog_variant_id_valid(value)||value=="none"||value=="unresolved"||value=="inherit"||next.find(value))return false;
            BorgVariantOverride variant;variant.variant=value;next.variants.push_back(std::move(variant));seen={};continue;
        }
        if(next.variants.empty())return false;
        const auto field=std::find(fields.begin(),fields.end(),key);
        if(field==fields.end())return false;
        const auto i=unsigned(field-fields.begin());
        if(seen[i])return false;
        seen[i]=true;auto& variant=next.variants.back();
        if(i<assets){
            if(!borg_variant_detail::asset(value,&variant.assets[i]))return false;
        }else if(!borg_variant_detail::offset(value,i==assets?&variant.hp:&variant.ammo))return false;
    }
    if(next.variants.empty()||!complete())return false;
    *out=std::move(next);return true;
}
}
