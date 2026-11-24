#pragma once
#include "engine/core/types.hpp"

namespace gf {
// Fixed-point native lab target. No original units or elapsed time implied.
inline constexpr bool borg_lab_ground_pace_valid(u16 pace){return pace>=42&&pace<=1042;}
}
