#pragma once
#include "engine/gameplay/editor_pointer.hpp"
namespace gf {
struct EditorMouseActions:EditorPointerActions {bool cursor=false,snapped=false,left_click=false,right_click=false;};
//Explicit owner mouse policy. Raw digital activity is observed BEFORE merging
//mouse A/B, so a mouse click cannot masquerade as a keyboard/controller override.
//Legacy EditorPointer and its default callers remain unchanged.
class EditorMousePointer {
public:
    EditorMouseActions update(const PointerState& pointer,unsigned width,unsigned height,EditorPointerContext context,bool interactive,bool holding,s16 main_hit,bool digital_active,u64 scope=0){
        EditorMouseActions out;const bool left=(pointer.left||pointer.left_pressed)&&!left_,right=(pointer.right||pointer.right_pressed)&&!right_;
        const bool moved=!known_||pointer.x!=x_||pointer.y!=y_,entered=pointer.within_window&&!within_;
        const bool changed=!known_||context!=context_||interactive!=interactive_||scope!=scope_;left_=pointer.left;right_=pointer.right;x_=pointer.x;y_=pointer.y;known_=true;within_=pointer.within_window;context_=context;interactive_=interactive;scope_=scope;
        const bool valid=pointer.within_window&&width&&height&&pointer.x>=0&&pointer.y>=0&&u32(pointer.x)<width&&u32(pointer.y)<height;
        if(digital_active)mouse_=false;else if(valid&&(moved||entered||left||right))mouse_=true;
        if(!valid||!mouse_)return out;
        out.x=f32(f64(pointer.x)*640/width);out.y=f32(f64(pointer.y)*480/height);out.cursor=true;
        if(!interactive||digital_active)return out;
        out.left_click=left&&!changed;out.right_click=right&&!changed;
        if(context==EditorPointerContext::Main){if(main_hit>=0&&main_hit<7){out.snapped=true;out.main_item=main_hit;if(left&&!changed)out.pressed=0x100;}}
        else if(context==EditorPointerContext::Grid){out.position=moved||left||right||entered;if(!changed){if(right&&holding)out.pressed=0x200;else if(left)out.pressed=0x100;}}
        return out;
    }
    bool mouse_mode()const{return mouse_;}
private:
    bool left_=false,right_=false,known_=false,within_=false,mouse_=false,interactive_=false;s32 x_=0,y_=0;u64 scope_=0;EditorPointerContext context_=EditorPointerContext::Inactive;
};
} //namespace gf
