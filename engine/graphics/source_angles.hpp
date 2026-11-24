#pragma once
#include <cmath>
#include "engine/core/types.hpp"
namespace gf {
//45204/45238: signed16 PSQ input, single-precision radians product, double
//sin/cos call and final frsp. Numeric host model, not a hardware/FPSCR claim.
inline f32 source_angle_sin(s16 angle){return f32(std::sin(f64(f32(f32(angle)*bits_to_f32(0x38c90fdb)))));}
inline f32 source_angle_cos(s16 angle){return f32(std::cos(f64(f32(f32(angle)*bits_to_f32(0x38c90fdb)))));}
inline f32 source_angle_tan(s16 angle){return f32(std::tan(f64(f32(f32(angle)*bits_to_f32(0x38c90fdb)))));}
}
