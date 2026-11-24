#pragma once
#include <algorithm>
#include <cmath>
#include <optional>
#include "engine/input/input_map.hpp"

namespace gf::pc {
// Explicit Battle Test controls. The source controller consumes a flick, not
// raw L/R/triggers. Leave ordinary WASD/stick deflection and Shift untouched.
inline GamePadState gred_battle_pad(GamePadState pad,bool dash_edge){
    if(dash_edge){
        float x=float(std::clamp(pad.stick_x,-128,127))+32.f*(int(pad.dpad_right)-int(pad.dpad_left));
        float y=float(std::clamp(pad.stick_y,-128,127))+32.f*(int(pad.dpad_down)-int(pad.dpad_up));
        if(x==0&&y==0)y=-32;
        const float magnitude=std::hypot(x,y);
        pad.stick_x=s32(std::lround(x*127.f/magnitude));pad.stick_y=s32(std::lround(y*127.f/magnitude));
        pad.dpad_up=pad.dpad_down=pad.dpad_left=pad.dpad_right=false;
    }
    pad.l=pad.r=pad.z=pad.trigger_l_pressed=pad.trigger_r_pressed=false;
    pad.trigger_l=pad.trigger_r=0;
    return pad;
}
// Bridge render-frame samples to practice movement ticks. Keep only the most
// recent eligible sample, so two separate edges cannot become a new chord.
class GRedPracticeInput {
public:
    void sample(const GamePadState& pad,std::optional<u8> flick,bool practicing){
        const auto direction=flick.value_or(0);
        if(!practicing||!flick)pending_.reset();
        else if((pad.b&&!jump_)||(direction&&direction!=direction_)){
            pending_=Movement{pad.b,direction?pad.stick_x:0,direction?pad.stick_y:0};
        }
        jump_=pad.b;direction_=direction;
    }
    void take(GamePadState& pad){
        if(pending_&&!pad.b&&!pad.stick_x&&!pad.stick_y&&!pad.dpad_up&&!pad.dpad_down&&!pad.dpad_left&&!pad.dpad_right){
            pad.b=pending_->jump;pad.stick_x=pending_->x;pad.stick_y=pending_->y;
        }
        pending_.reset();
    }
private:
    struct Movement {bool jump;s32 x,y;};
    std::optional<Movement> pending_;
    bool jump_=false;
    u8 direction_=0;
};
}
