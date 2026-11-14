#pragma once
#include "engine/input/input_map.hpp"
namespace gf {
// 801BEF70 retains GameCube L=0x40, R=0x20; Start is folded from0x1000
// into editor0x80. Physical labels must not be inferred from bit order.
namespace force_editor_button {
inline constexpr u32 L=0x40,R=0x20,Z=0x10,Start=0x80,A=0x100,B=0x200,X=0x400,Y=0x800;
}
inline u32 force_editor_button_bits(const GamePadState& pad){
    using namespace force_editor_button;
    return (pad.l?L:0u)|(pad.r?R:0u)|(pad.z?Z:0u)|(pad.start?Start:0u)|
           (pad.a?A:0u)|(pad.b?B:0u)|(pad.x?X:0u)|(pad.y?Y:0u);
}
} // namespace gf
