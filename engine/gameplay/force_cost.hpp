#pragma once
#include "engine/core/types.hpp"
namespace gf {
enum class ForceCostMode {Full,TwoThirds};
//Shared existing inventory cost policy. Callers validate the mode and bounded
//catalog/edit value; AI uses authored variant0, never a tool override.
inline s32 force_cost_for_mode(s32 value,ForceCostMode mode){return mode==ForceCostMode::TwoThirds?value*2/3:value;}
} // namespace gf
