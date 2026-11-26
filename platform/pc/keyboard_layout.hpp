#pragma once
// Shared desktop keyboard layout. The device layer reads scancodes and fills a
// KeyboardPad; this pure mapper applies it to one raw port so the layout and
// the axis/button merge can be tested without SDL.
#include "engine/input/input_snapshot.hpp"
namespace gf::pc {
// Device bit layout shared by the SDL backend and this pure mapper.
enum DeviceButton : u32 {
    kButtonA = 1u << 0,
    kButtonB = 1u << 1,
    kButtonX = 1u << 2,
    kButtonY = 1u << 3,
    kButtonStart = 1u << 4,
    kButtonDpadUp = 1u << 5,
    kButtonDpadDown = 1u << 6,
    kButtonDpadLeft = 1u << 7,
    kButtonDpadRight = 1u << 8,
    kButtonL = 1u << 9,
    kButtonR = 1u << 10,
    kButtonZ = 1u << 11,
};
struct KeyboardPad {
    bool dpad_up=false,dpad_down=false,dpad_left=false,dpad_right=false;
    bool axis_up=false,axis_down=false,axis_left=false,axis_right=false;
    bool a=false,b=false,x=false,y=false,start=false,l=false,r=false,z=false;
};
inline void map_keyboard_pad(const KeyboardPad& keys,RawInputState* pad){
    if(!pad)return;
    if(keys.start)pad->buttons|=kButtonStart;
    if(keys.a)pad->buttons|=kButtonA;
    if(keys.b)pad->buttons|=kButtonB;
    if(keys.x)pad->buttons|=kButtonX;
    if(keys.y)pad->buttons|=kButtonY;
    if(keys.dpad_up)pad->buttons|=kButtonDpadUp;
    if(keys.dpad_down)pad->buttons|=kButtonDpadDown;
    if(keys.dpad_left)pad->buttons|=kButtonDpadLeft;
    if(keys.dpad_right)pad->buttons|=kButtonDpadRight;
    if(keys.l)pad->buttons|=kButtonL;
    if(keys.r)pad->buttons|=kButtonR;
    if(keys.z)pad->buttons|=kButtonZ;
    if(keys.axis_up)pad->axis[1]=-20000;
    if(keys.axis_down)pad->axis[1]=20000;
    if(keys.axis_left)pad->axis[0]=-20000;
    if(keys.axis_right)pad->axis[0]=20000;
}
} // namespace gf::pc
