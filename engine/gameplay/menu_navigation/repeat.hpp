#pragma once
#include "engine/gameplay/menu_navigation/config.hpp"
namespace gf {
struct MenuPadHistory {u32 held=0;u8 ticks=0;};
struct MenuPadEvents {u32 pressed=0,repeated=0;};
// Original 8008B7C8 repeat-counter operation, expressed as a pulse for this
// update. Caller supplies mapped digital state and keeps each port independent.
inline MenuPadEvents menu_repeat_step(MenuPadHistory& state,u32 held,const MenuNavigationConfig& config){
    MenuPadEvents events{held&~state.held,0};
    if(held==state.held){
        ++state.ticks;
        if(state.ticks>config.repeat_limit){state.ticks=config.repeat_restart;events.repeated=held;}
    }else state.ticks=0;
    state.held=held;return events;
}
} // namespace gf
