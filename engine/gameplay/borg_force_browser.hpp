#pragma once
#include "engine/gameplay/borg_mixed_force_view.hpp"
#include "engine/input/input_map.hpp"

namespace gf {
enum class BorgForceBrowseChange {None,Force,Slot,Close,Practice};
// Cursor over one read-only scope; blank slots are still exact positions.
class BorgForceBrowser {
public:
    bool open(unsigned force,unsigned slot){
        if(force>=kForceCount||slot>=kForceSlots)return false;
        force_=force;slot_=slot;previous_={};return true;
    }
    unsigned force()const{return force_;}
    unsigned slot()const{return slot_;}
    void sync_input(const GamePadState& pad){previous_=pad;}
    BorgForceBrowseChange step(const GamePadState& pad,bool allow_practice=false){
        const bool back=pad.b&&!previous_.b;
        const bool left=pad.dpad_left&&!previous_.dpad_left,right=pad.dpad_right&&!previous_.dpad_right;
        const bool up=pad.dpad_up&&!previous_.dpad_up,down=pad.dpad_down&&!previous_.dpad_down;
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
        if(practice)return BorgForceBrowseChange::Practice;
        return BorgForceBrowseChange::None;
    }
private:
    unsigned force_=0,slot_=0;
    GamePadState previous_{};
};
}
