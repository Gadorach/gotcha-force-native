#pragma once
#include <algorithm>
#include <cmath>
#include "engine/core/types.hpp"
namespace gf {
enum class MenuCursorPhase {Free,Expanding,Snapped,Shrinking,Digital};
struct MenuCursorPose {f32 x=0,y=0,diameter=48;MenuCursorPhase phase=MenuCursorPhase::Free;s16 item=-1;};
//Native mouse animation policy, not original game timing or an actor completion.
//Pointer position is ignored while latched; the host supplies the item's source
//anchor until its actual footprint/release zone is left. Reversal starts at the
//currently displayed pose, without resetting radius or teleporting the centre.
class MenuCursorTransition {
public:
    bool configure(f32 free_diameter,f32 duration){if(!std::isfinite(free_diameter)||free_diameter<=0||free_diameter>128||!std::isfinite(duration)||duration<=0||duration>1)return false;*this={};free_=free_diameter;duration_=duration;return true;}
    bool digital(f32 x,f32 y,f32 diameter){if(!valid(x,y,diameter))return false;pose_={x,y,diameter,MenuCursorPhase::Digital,-1};known_=true;elapsed_=0;return true;}
    bool step(f32 pointer_x,f32 pointer_y,s16 item,f32 anchor_x,f32 anchor_y,f32 normal_diameter,f32 seconds){
        if(!valid(pointer_x,pointer_y,normal_diameter)||!std::isfinite(seconds)||seconds<0||seconds>1||item< -1||item>=7||(item>=0&&!valid(anchor_x,anchor_y,normal_diameter)))return false;
        if(!known_){pose_={pointer_x,pointer_y,free_,MenuCursorPhase::Free,-1};known_=true;}
        const f32 target_x=item>=0?anchor_x:pointer_x,target_y=item>=0?anchor_y:pointer_y,target_diameter=item>=0?normal_diameter:free_;
        const bool entering=item>=0&&(pose_.item!=item||pose_.phase==MenuCursorPhase::Digital),leaving=item<0&&(pose_.item>=0||pose_.phase==MenuCursorPhase::Digital);
        if(entering||leaving){from_=pose_;elapsed_=0;pose_.phase=entering?MenuCursorPhase::Expanding:MenuCursorPhase::Shrinking;pose_.item=item;}
        if(pose_.phase==MenuCursorPhase::Free){pose_={pointer_x,pointer_y,free_,MenuCursorPhase::Free,-1};return true;}
        if(pose_.phase==MenuCursorPhase::Snapped){pose_={anchor_x,anchor_y,normal_diameter,MenuCursorPhase::Snapped,item};return true;}
        elapsed_=std::min(duration_,elapsed_+seconds);const f32 t=elapsed_/duration_,ease=t*t*(3-2*t);pose_.x=from_.x+(target_x-from_.x)*ease;pose_.y=from_.y+(target_y-from_.y)*ease;pose_.diameter=from_.diameter+(target_diameter-from_.diameter)*ease;
        if(elapsed_==duration_)pose_={target_x,target_y,target_diameter,item>=0?MenuCursorPhase::Snapped:MenuCursorPhase::Free,item};
        return true;
    }
    const MenuCursorPose& pose()const{return pose_;}s16 latched_item()const{return pose_.item;}
private:
    bool valid(f32 x,f32 y,f32 diameter)const{return std::isfinite(x)&&std::isfinite(y)&&std::isfinite(diameter)&&x>=0&&y>=0&&x<640&&y<480&&diameter>=free_&&diameter<=256;}
    MenuCursorPose pose_,from_;f32 free_=48,duration_=.14f,elapsed_=0;bool known_=false;
};
} //namespace gf
