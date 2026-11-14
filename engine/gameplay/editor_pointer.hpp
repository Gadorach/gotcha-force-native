#pragma once
#include <cmath>
#include "engine/input/input_snapshot.hpp"
namespace gf {
enum class EditorPointerContext {Inactive,Main,Grid,Options,Lists};
struct EditorPointerActions {bool position=false;f32 x=0,y=0;s16 main_item=-1;u32 pressed=0;};
//Pointer edges feed the SAME A/B controllers as keyboard/gamepad input. Observe
//inactive/paused snapshots too, so held clicks cannot leak through a transition.
class EditorPointer {
public:
    EditorPointerActions update(const PointerState& pointer,unsigned window_width,unsigned window_height,
                                EditorPointerContext context,bool enabled,bool holding_borg,s16 main_hit){
        EditorPointerActions out;const bool left=(pointer.left_pressed||pointer.left)&&!left_,right=(pointer.right_pressed||pointer.right)&&!right_;
        const bool moved=!known_||pointer.x!=x_||pointer.y!=y_;left_=pointer.left;right_=pointer.right;x_=pointer.x;y_=pointer.y;known_=true;
        if(!enabled||!pointer.within_window||!window_width||!window_height||pointer.x<0||pointer.y<0||u32(pointer.x)>=window_width||u32(pointer.y)>=window_height)return out;
        out.x=f32(f64(pointer.x)*640/window_width);out.y=f32(f64(pointer.y)*480/window_height);
        if(context==EditorPointerContext::Main){if(main_hit>=0&&main_hit<7&&(moved||left)){out.main_item=main_hit;if(left)out.pressed=0x100;}}
        else if(context==EditorPointerContext::Grid){out.position=moved||left||right;if(right&&holding_borg)out.pressed=0x200;else if(left)out.pressed=0x100;}
        return out;
    }
private:
    bool left_=false,right_=false,known_=false;s32 x_=0,y_=0;
};
} //namespace gf
