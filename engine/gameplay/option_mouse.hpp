#pragma once
#include "engine/core/types.hpp"
namespace gf {
enum class OptionMouseArea {None,Settings,More,Cheats,Details,Saved,SaveError,CheatSaveError,Credits};
struct OptionMouseCommand {OptionMouseArea area=OptionMouseArea::None;s16 row=-1,value=-1;u32 pressed=0;};
struct OptionMouseTarget {OptionMouseArea area=OptionMouseArea::None;s16 row=-1,value=-1;u32 left_pressed=0;};
inline bool option_mouse_valid(const OptionMouseCommand& c){
    if(unsigned(c.area)>unsigned(OptionMouseArea::Credits)||c.row< -1||c.value< -1||(c.pressed&~0xb00u))return false;
    if(c.area==OptionMouseArea::None)return c.row==-1&&c.value==-1&&!c.pressed;
    if(c.area==OptionMouseArea::Settings){if(c.row>=10||c.value>4||(c.value>=0&&(c.row<0||c.row>=8||((c.row==0||c.row>=4)&&c.value>1))))return false;return true;}
    if(c.area==OptionMouseArea::More)return c.row<2&&c.value==-1;
    if(c.area==OptionMouseArea::Cheats)return c.row<7&&c.value<=1&&(c.value<0||(c.row>=0&&c.row<2));
    return c.row==-1&&c.value==-1&&(c.pressed&~0x300u)==0;
}
inline OptionMouseCommand option_mouse_command(OptionMouseArea area,const OptionMouseTarget& target,bool left,bool right){
    if(area==OptionMouseArea::None)return {};
    OptionMouseCommand out;out.area=area;if(right){out.pressed=0x200;return out;}
    if(target.area!=area)return out;
    out.row=target.row;if(left){out.value=target.value;out.pressed=target.left_pressed;}return out;
}
} //namespace gf
