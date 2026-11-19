#pragma once

namespace gf::pc {
// Adapt sampled input to the practice clock, not to the source controller.
// A short press can wait for one movement tick, but never survives a pause,
// restart or refusal, and never inserts a synthetic release.
class GRedPracticeJumpTap {
public:
    void sample(bool held,bool practicing){
        if(!practicing)pending_=false;
        else if(held&&!held_)pending_=true;
        held_=held;
    }
    bool take(){
        const bool jump=held_||pending_;
        pending_=false;
        return jump;
    }
private:
    bool held_=false,pending_=false;
};
}
