#pragma once

#include <string>
#include "engine/core/types.hpp"

namespace gf {
inline constexpr unsigned kStageBodyPartitionCount=16;
// Authored full-height footprint, independent of projectile grids and floor.
struct StageBodyPartition {
    std::string id;
    f32 min_x=0,min_z=0,max_x=0,max_z=0;
    bool blocks_melee=false;
    bool operator==(const StageBodyPartition&)const=default;
};
}
