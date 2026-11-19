#pragma once
#include <array>
#include <charconv>
#include <optional>
#include "engine/assets/borg_catalog_details.hpp"
#include "engine/assets/tpl.hpp"

namespace gf {
inline constexpr std::size_t kBorgCatalogProfileBytes=8192,kBorgCatalogIconBytes=128u<<10;
inline constexpr unsigned kBorgCatalogVariants=16,kBorgCatalogIconDimension=128;
struct BorgCatalogVariant {
    std::string id,label,icon;
    u16 force_cost=0;
    bool placeholder_icon()const{return icon=="placeholder";}
    bool unresolved_icon()const{return icon=="unresolved";}
};
struct BorgCatalogProfile {
    std::string description,tribe,default_variant;
    std::vector<BorgCatalogVariant> variants;
    std::optional<BorgCatalogDetails> details;
    const BorgCatalogVariant* find(std::string_view id)const{
        for(const auto& variant:variants)if(variant.id==id)return &variant;
        return nullptr;
    }
};
struct BorgCatalogCard {
    BorgIdentity identity;
    BorgCatalogProfile profile;
    // One entry per variant; null means the explicit placeholder, not a
    // decode failure. A card is presentation data, not an actor or save entry.
    std::vector<std::optional<TplImage>> icons;
};
// A parsed metadata view of one discovered manifest, independent of whether
// its editor icons or model can be loaded.
struct BorgCatalogSnapshot {
    BorgPackageEntry package;
    BorgCatalogProfile profile;
};
inline bool borg_catalog_variant_id_valid(std::string_view id){
    auto letter=[](char c){return (c>='a'&&c<='z')||(c>='0'&&c<='9');};
    if(id.empty()||id.size()>32||!letter(id.front()))return false;
    for(char c:id)if(!letter(c)&&c!='-'&&c!='_')return false;
    return true;
}
inline bool borg_catalog_read_profile(std::string_view text,BorgCatalogProfile* out){
    if(!out||text.size()>kBorgCatalogProfileBytes)return false;
    const auto header=borg_package_detail::line(text);
    const bool extended=header=="GFBORG-CATALOG 2";
    if(!extended&&header!="GFBORG-CATALOG 1")return false;
    BorgCatalogProfile next;
    borg_catalog_detail::DetailsReader details;
    while(!text.empty()){
        const auto row=borg_package_detail::line(text);
        if(row.empty()||row.starts_with('#'))continue;
        const auto equals=row.find('=');
        if(equals==std::string_view::npos)return false;
        const auto key=row.substr(0,equals),value=row.substr(equals+1);
        if(key=="variant"){
            if(next.variants.size()==kBorgCatalogVariants)return false;
            std::array<std::string_view,4> parts;
            auto rest=value;
            for(unsigned i=0;i<parts.size();++i){
                const auto pipe=rest.find('|');
                if((pipe==std::string_view::npos)!=(i+1==parts.size()))return false;
                parts[i]=rest.substr(0,pipe);
                if(pipe!=std::string_view::npos)rest.remove_prefix(pipe+1);
            }
            const auto [id,label,cost_text,icon]=parts;
            if(!borg_catalog_variant_id_valid(id)||!display_name_valid(label,64)||cost_text.empty()||cost_text.size()>5||
               next.find(id)||(icon!="placeholder"&&icon!="unresolved"&&
                   (!icon.ends_with(".tpl")||!borg_package_asset_path_valid(icon))))return false;
            u32 cost=0;const auto end=cost_text.data()+cost_text.size();
            const auto parsed=std::from_chars(cost_text.data(),end,cost);
            if(parsed.ec!=std::errc{}||parsed.ptr!=end||cost>32767)return false;
            next.variants.push_back({std::string(id),std::string(label),std::string(icon),u16(cost)});
            continue;
        }
        auto* target=key=="description"?&next.description:key=="tribe"?&next.tribe:key=="default-variant"?&next.default_variant:nullptr;
        if(!target){
            if(!extended||!details.add(key,value))return false;
            continue;
        }
        if(!target->empty()||!(key=="description"?display_name_valid(value,512):
            key=="tribe"?(value=="none"||borg_package_id_valid(value)):borg_catalog_variant_id_valid(value)))return false;
        *target=value;
    }
    if(next.description.empty()||next.tribe.empty()||!next.find(next.default_variant)||
       (extended&&!details.finish(&next.details)))return false;
    *out=std::move(next);return true;
}

// Catalogue artwork is a tightly bounded use of the shared texture decoder.
inline bool borg_catalog_decode_icon(ByteView bytes,TplImage* out,std::string* error){
    if(!out||bytes.size>kBorgCatalogIconBytes)return false;
    u32 images=0;detail::TplLayout layout;
    if(!tpl_image_count(bytes,&images,error)||images!=1||!detail::tpl_validate(bytes,0,&layout,error)||
        layout.width>kBorgCatalogIconDimension||layout.height>kBorgCatalogIconDimension)return false;
    return tpl_decode(bytes,0,out,error);
}
}
