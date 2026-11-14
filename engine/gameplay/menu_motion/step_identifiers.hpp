#pragma once
#include "engine/gameplay/menu_motion/state.hpp"
namespace gf {
// Original 801CDBE0, menu-idle path: wrapping primary phase and unselected
// identifier authored-Y phase. Visibility is applied by MenuScene, not guest flags.
inline void menu_motion_step_identifiers(const HsdScene::MenuMotion& config,unsigned selected,MenuMotionState& state){
    for(const auto& b:config.bindings)if(b.item<7){
        state.gear_phase[b.item]=static_cast<u16>(u32(state.gear_phase[b.item])+u32(b.step));
        if(b.item!=selected)state.identifier_y_phase[b.item]+=config.identifier_step;
    }
}
} // namespace gf
