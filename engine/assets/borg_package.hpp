#pragma once
#include <algorithm>
#include <array>
#include <string>
#include <utility>
#include <vector>
#include "engine/core/display_name.hpp"

namespace gf {
inline constexpr std::size_t kBorgManifestBytes=8192;

// The ID is independent of folder location and display-name changes.
struct BorgIdentity {
    std::string id,name;
    bool operator==(const BorgIdentity&)const=default;
};
struct BorgPackageManifest {
    BorgIdentity identity;
    std::string model,animations,rules,animation_map;
    std::string catalog{},preview{};
    unsigned version=1;
    std::string gameplay{};
    bool operator==(const BorgPackageManifest&)const=default;
};
struct BorgPackageEntry {
    std::string folder;
    BorgPackageManifest manifest;
};
enum class BorgPackageIssueKind {InvalidManifest,MissingAsset,DuplicateId};
struct BorgPackageIssue {
    std::string folder,id;
    BorgPackageIssueKind kind;
};
enum class BorgPackageScanStatus {Complete,NoDirectory,Unavailable,EntryBudget};
struct BorgPackageCatalog {
    BorgPackageScanStatus status=BorgPackageScanStatus::Unavailable;
    std::vector<BorgPackageEntry> discovered; // Sorted by stable ID, not gameplay-ready actors.
    std::vector<BorgPackageIssue> issues;
    const BorgPackageEntry* find(std::string_view id)const{
        const auto it=std::lower_bound(discovered.begin(),discovered.end(),id,
            [](const BorgPackageEntry& entry,std::string_view key){return entry.manifest.identity.id<key;});
        return it!=discovered.end()&&it->manifest.identity.id==id?&*it:nullptr;
    }
};
inline bool borg_package_id_valid(std::string_view id){
    const auto colon=id.find(':');
    if(colon==0||colon==std::string_view::npos||colon+1==id.size()||id.size()>96||id.substr(0,colon)=="retail")return false;
    auto atom=[](std::string_view text){
        auto plain=[](char c){return (c>='a'&&c<='z')||(c>='0'&&c<='9');};
        if(!plain(text.front()))return false;
        for(char c:text)if(!plain(c)&&c!='.'&&c!='-'&&c!='_')return false;
        return true;
    };
    return atom(id.substr(0,colon))&&atom(id.substr(colon+1));
}
inline bool borg_package_asset_path_valid(std::string_view path){
    if(path.empty()||path.size()>192)return false;
    for(;;){
        const auto slash=path.find('/');
        const auto part=path.substr(0,slash);
        if(part.empty()||part=="."||part==".."||part.back()=='.')return false;
        for(char c:part)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='.'||c=='-'||c=='_'))return false;
        if(slash==std::string_view::npos)return true;
        path.remove_prefix(slash+1);
    }
}
namespace borg_package_detail {
inline std::string_view line(std::string_view& text){
    const auto newline=text.find('\n');
    auto value=text.substr(0,newline);
    text.remove_prefix(newline==std::string_view::npos?text.size():newline+1);
    if(value.ends_with('\r'))value.remove_suffix(1);
    return value;
}
template<std::size_t N> bool split_fields(std::string_view text,std::array<std::string_view,N>* out){
    for(unsigned i=0;i<N;++i){
        const auto pipe=text.find('|');
        if((pipe==std::string_view::npos)!=(i+1==N))return false;
        (*out)[i]=text.substr(0,pipe);
        if(pipe!=std::string_view::npos)text.remove_prefix(pipe+1);
    }
    return true;
}
}
// Passing the manifest parser is not model, animation, behavior, actor or
// mode validation.
inline bool borg_package_read_manifest(std::string_view text,BorgPackageManifest* out){
    if(!out||text.size()>kBorgManifestBytes)return false;
    const auto header=borg_package_detail::line(text);
    const bool capability_manifest=header=="GFBORG 2";
    if(!capability_manifest&&header!="GFBORG 1")return false;
    BorgPackageManifest next;
    next.version=capability_manifest?2:1;
    while(!text.empty()){
        const auto row=borg_package_detail::line(text);
        if(row.empty()||row.starts_with('#'))continue;
        const auto equals=row.find('=');
        if(equals==std::string_view::npos)return false;
        const auto key=row.substr(0,equals),value=row.substr(equals+1);
        auto* target=key=="id"?&next.identity.id:key=="name"?&next.identity.name:key=="model"?&next.model:
            key=="animations"?&next.animations:key=="rules"?&next.rules:key=="animation-map"?&next.animation_map:
            key=="catalog"?&next.catalog:key=="preview"?&next.preview:key=="gameplay"&&capability_manifest?&next.gameplay:nullptr;
        if(!target||!target->empty()||value.empty())return false;
        if(!(key=="id"?borg_package_id_valid(value):key=="name"?display_name_valid(value,96):borg_package_asset_path_valid(value)))return false;
        *target=value;
    }
    if(next.identity.id.empty()||next.identity.name.empty())return false;
    if(capability_manifest){
        // A catalogue-only package must not invent art or executable rules
        // to appear in the catalog. Preview and rules still need their body.
        if(next.catalog.empty()||
           (next.model.empty()&&(!next.animations.empty()||!next.preview.empty()))||
           (next.animations.empty()&&(!next.rules.empty()||!next.animation_map.empty()||
                                      (!next.model.empty()&&next.preview.empty()))))return false;
    }else if(next.model.empty()||next.animations.empty()||next.rules.empty())return false;
    *out=std::move(next);return true;
}
}
