#pragma once
#include "engine/input/input_map.hpp"

namespace gf {
enum class BorgPlacementRequest {None,Cancel,Confirm};

// Controls consent only. Beginning an offer consumes the input sample that
// opened it, so a held Accept cannot proceed to the confirmation action.
class BorgPlacementConfirmation {
public:
    void begin(const GamePadState& pad){previous_=pad;confirm_selected_=false;}
    bool confirm_selected()const{return confirm_selected_;}
    BorgPlacementRequest step(const GamePadState& pad){
        const bool back=pad.b&&!previous_.b,accept=pad.a&&!previous_.a;
        const bool up=pad.dpad_up&&!previous_.dpad_up,down=pad.dpad_down&&!previous_.dpad_down;
        previous_=pad;
        if(back)return BorgPlacementRequest::Cancel;
        if(up||down){
            if(up!=down)confirm_selected_=down;
            return BorgPlacementRequest::None;
        }
        if(!accept)return BorgPlacementRequest::None;
        return confirm_selected_?BorgPlacementRequest::Confirm:BorgPlacementRequest::Cancel;
    }
private:
    bool confirm_selected_=false;
    GamePadState previous_{};
};
}
