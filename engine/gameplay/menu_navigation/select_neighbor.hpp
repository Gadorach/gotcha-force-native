#pragma once
#include "engine/gameplay/menu_navigation/config.hpp"
namespace gf {
// Original 800C0454: first matching direction wins, Up > Down > Left > Right.
// Adjacency is loaded from archive-bound content; no guessed screen geometry.
inline unsigned menu_select_neighbor(const MenuNavigationConfig& config,unsigned selected,u32 directions){
    if(selected>=7)return selected;
    unsigned column=4;
    if(directions&8u)column=0;
    else if(directions&4u)column=1;
    else if(directions&1u)column=2;
    else if(directions&2u)column=3;
    return column<4?config.neighbor[selected][column]:selected;
}
} // namespace gf
