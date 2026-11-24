#pragma once

#include <array>
#include "engine/core/types.hpp"

namespace gf {
constexpr unsigned kBorgTrailingChainJoints=8;
struct BorgTrailingChainSpec {
    std::array<u8,kBorgTrailingChainJoints> bones{};
    u8 count=0;
};

inline bool borg_trailing_chain_valid(const BorgTrailingChainSpec& spec){
    if(spec.count<3||spec.count>spec.bones.size())return false;
    u32 seen=1;
    for(unsigned i=0;i<spec.count;++i){
        const unsigned b=spec.bones[i];
        if(b>=32||(seen&(u32(1)<<b)))return false;
        seen|=u32(1)<<b;
    }
    return true;
}
} // namespace gf
