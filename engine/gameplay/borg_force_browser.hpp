#pragma once
#include "engine/gameplay/borg_mixed_force_view.hpp"
#include "engine/gameplay/gred_picker_practice_route.hpp"
#include "engine/input/input_map.hpp"

namespace gf {
enum class BorgForceBrowseChange {None,Force,Slot,Close,Practice,PracticeRoute};
// Cursor over one read-only scope; blank slots are still exact positions.
class BorgForceBrowser {
public:
    bool open(unsigned force,unsigned slot){
        if(force>=kForceCount||slot>=kForceSlots)return false;
        force_=force;slot_=slot;previous_={};practice_route_=GRedPickerPracticeRoute::Standard;return true;
    }
    unsigned force()const{return force_;}
    unsigned slot()const{return slot_;}
    GRedPickerPracticeRoute practice_route()const{return practice_route_;}
    std::string_view practice_route_name()const{return gred_picker_practice_route_name(practice_route_);}
    void sync_input(const GamePadState& pad){previous_=pad;}
    BorgForceBrowseChange step(const GamePadState& pad,bool allow_practice=false){
        const bool back=pad.b&&!previous_.b;
        const bool left=pad.dpad_left&&!previous_.dpad_left,right=pad.dpad_right&&!previous_.dpad_right;
        const bool up=pad.dpad_up&&!previous_.dpad_up,down=pad.dpad_down&&!previous_.dpad_down;
        const bool l=pad.l&&!previous_.l,r=pad.r&&!previous_.r;
        const bool practice=allow_practice&&pad.y&&!previous_.y&&!pad.b&&!pad.start;
        previous_=pad;
        if(back)return BorgForceBrowseChange::Close;
        if(left!=right){
            force_=(force_+(left?kForceCount-1:1))%kForceCount;slot_=0;
            return BorgForceBrowseChange::Force;
        }
        if(up!=down){
            slot_=(slot_+(up?kForceSlots-1:1))%kForceSlots;
            return BorgForceBrowseChange::Slot;
        }
        if(allow_practice&&l!=r){
            practice_route_=cycle_gred_picker_practice_route(practice_route_,l);
            return BorgForceBrowseChange::PracticeRoute;
        }
        if(practice)return BorgForceBrowseChange::Practice;
        return BorgForceBrowseChange::None;
    }
private:
    unsigned force_=0,slot_=0;
    GRedPickerPracticeRoute practice_route_=GRedPickerPracticeRoute::Standard;
    GamePadState previous_{};
};
}
