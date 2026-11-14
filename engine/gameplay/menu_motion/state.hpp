#pragma once
#include <array>
#include "engine/assets/hsd.hpp"
namespace gf {
struct MenuMotionState {
    std::array<u16,7> gear_phase{};
    std::array<f32,7> identifier_y_phase{};
    u16 selector_phase=0;
};
inline f32 menu_phase_angle(u16 phase,f32 unit){return (phase<=32767?f32(phase):f32(s32(phase)-65536))*unit;}
} // namespace gf
