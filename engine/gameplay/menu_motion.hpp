#pragma once
// Portable menu-idle controller, with individual semantic ports below.
// Steps/angle units come from archive-bound native content, not guessed speeds.
#include "engine/gameplay/menu_motion/model_matrix.hpp"
#include "engine/gameplay/menu_motion/caption_matrix.hpp"
#include "engine/gameplay/menu_motion/step_identifiers.hpp"
#include "engine/gameplay/menu_motion/step_selector.hpp"
namespace gf {
inline void menu_motion_step(const HsdScene::MenuMotion& config,unsigned selected,MenuMotionState& state){
    menu_motion_step_identifiers(config,selected,state);
    menu_motion_step_selector(config,state);
}
} // namespace gf
