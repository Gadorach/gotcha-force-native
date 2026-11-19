#pragma once
#include "engine/assets/borg_package.hpp"
#include "engine/assets/character_program.hpp"

namespace gf {
// The first package-driven practice path uses the already-qualified normal
// G-Red body, rig and controller. These are content checks, not signatures
// or a generic package actor/behavior interface.
struct NativeGRedPracticeAssets {
    enum Role : unsigned {Body,Bank,RigPrefix,PoseProfile,Programs,PoseHeight,MovementInput,MovementMotion,Count};
    static constexpr std::array<std::size_t,Count> sizes{193095,355264,256,188,2476,28,192,376};
    static constexpr std::array<u32,Count> hashes{
        0xe2b4439au,0x276b4e9eu,0x5e71a4a7u,0x5b1ea8feu,0x638570ffu,0x0c0b1313u,0x9e40085cu,0x409f4accu
    };
    static constexpr std::array<std::string_view,Count> roles{
        "model","animations","rig-prefix","pose-profile","locomotion","pose-height","movement-input","movement-motion"
    };
    std::array<ByteView,Count> bytes{};
    ByteView operator[](Role role)const{return bytes[role];}
    bool qualified()const{
        for(unsigned i=0;i<Count;++i)
            if(!bytes[i].data||bytes[i].size!=sizes[i]||character_program_model_hash(bytes[i])!=hashes[i])return false;
        return true;
    }
};
inline constexpr std::size_t kBorgPracticeProfileBytes=4096;
struct BorgPracticeProfile {
    std::string variant;
    // Body and bank are declared only by the manifest, not repeated here.
    std::array<std::string,NativeGRedPracticeAssets::Count> paths;
};
inline bool borg_read_normal_gred_practice_profile(std::string_view text,BorgPracticeProfile* out){
    using Assets=NativeGRedPracticeAssets;
    if(!out||text.size()>kBorgPracticeProfileBytes||borg_package_detail::line(text)!="GFBORG-PRACTICE 1")return false;
    BorgPracticeProfile next;bool controller=false;
    while(!text.empty()){
        const auto row=borg_package_detail::line(text);
        if(row.empty()||row.starts_with('#'))continue;
        const auto equals=row.find('=');
        if(equals==std::string_view::npos)return false;
        const auto key=row.substr(0,equals),value=row.substr(equals+1);
        if(key=="controller"){
            if(controller||value!="normal-gred-gfmm9")return false;
            controller=true;
        }else if(key=="variant"){
            if(!next.variant.empty()||value!="normal")return false;
            next.variant=value;
        }else{
            const auto first=Assets::roles.begin()+Assets::RigPrefix;
            const auto slot=std::find(first,Assets::roles.end(),key);
            if(slot==Assets::roles.end()||!borg_package_asset_path_valid(value))return false;
            auto& path=next.paths[std::size_t(slot-Assets::roles.begin())];
            if(!path.empty())return false;
            path=value;
        }
    }
    if(!controller||next.variant.empty()||std::any_of(next.paths.begin()+Assets::RigPrefix,next.paths.end(),
        [](const auto& path){return path.empty();}))return false;
    *out=std::move(next);return true;
}
}
