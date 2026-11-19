#pragma once
#include "engine/input/input_map.hpp"

namespace gf::pc {
// Retain one sampled high-deflection edge, never a synthetic dash command.
class GRedPracticeFlick {
public:
    void sample(const GamePadState& pad,u8 direction,bool practicing){
        if(!practicing)pending_=false;
        else if(direction&&direction!=direction_){
            x_=pad.stick_x;y_=pad.stick_y;pending_=true;
        }
        direction_=direction;
    }
    void take(GamePadState& pad){
        if(pending_&&!pad.stick_x&&!pad.stick_y&&!pad.dpad_up&&!pad.dpad_down&&!pad.dpad_left&&!pad.dpad_right){
            pad.stick_x=x_;pad.stick_y=y_;
        }
        pending_=false;
    }
private:
    s32 x_=0,y_=0;
    u8 direction_=0;
    bool pending_=false;
};
}
