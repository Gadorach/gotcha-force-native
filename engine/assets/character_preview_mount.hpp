#pragma once
#include <array>
#include <span>
#include "engine/core/types.hpp"

namespace gf {
inline constexpr unsigned kCharacterPreviewMounts=16;
// Static geometry from the same model bank, mounted on a logical pose slot.
// No actor, independent animation, effects or runtime selectors are implied.
struct CharacterPreviewMount {
    u8 record=0,bone=0;
    std::array<s16,3> offset{};
};
inline bool character_preview_mounts_valid(std::span<const CharacterPreviewMount> mounts){
    if(mounts.size()>kCharacterPreviewMounts)return false;
    for(unsigned i=0;i<mounts.size();++i){
        const auto& m=mounts[i];
        if(!m.record||m.record>=32||m.bone>=32)return false;
        for(auto v:m.offset)if(v< -4096||v>4096)return false;
        for(unsigned j=0;j<i;++j)if(mounts[j].record==m.record&&mounts[j].bone==m.bone)return false;
    }
    return true;
}
}
