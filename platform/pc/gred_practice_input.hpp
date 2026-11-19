#pragma once
#include <optional>
#include "engine/input/input_map.hpp"

namespace gf::pc {
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
