#pragma once
#include "engine/gameplay/menu_motion/state.hpp"
namespace gf {
// Original 801CDF08, counter operation. Native selection supplies the anchor;
// original transition-mode visibility rules are outside this idle port.
inline void menu_motion_step_selector(const HsdScene::MenuMotion& config,MenuMotionState& state){
    state.selector_phase=static_cast<u16>(u32(state.selector_phase)+u32(config.selector_step));
}
} // namespace gf
