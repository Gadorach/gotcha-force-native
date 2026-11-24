#pragma once

#include <span>
#include "engine/core/types.hpp"

namespace gf {
// Explicitly bound native presentation rule for authored zero normals.
// The original model is not rewritten and other material refusals stay closed.
struct BorgZeroReflection {
    u32 pobj=0;
    u16 count=0;
};
inline constexpr unsigned kBorgZeroReflectionMeshes=4;

inline bool borg_zero_reflection_valid(const BorgZeroReflection& policy){
    return policy.pobj&&policy.pobj<(4u<<20)&&!(policy.pobj&3)&&policy.count&&policy.count<=64;
}
inline bool borg_zero_reflections_valid(std::span<const BorgZeroReflection> rows){
    if(rows.size()>kBorgZeroReflectionMeshes)return false;
    for(unsigned i=0;i<rows.size();++i){
        if(!borg_zero_reflection_valid(rows[i]))return false;
        for(unsigned j=0;j<i;++j)if(rows[j].pobj==rows[i].pobj)return false;
    }
    return true;
}
} // namespace gf
