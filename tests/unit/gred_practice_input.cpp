#include <cstdio>
#include <cstdlib>
#include "platform/pc/gred_practice_input.hpp"

namespace {
void check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"practice input: %s\n",why);std::abort();}
}
}

int main(){
    gf::pc::GRedPracticeInput input;
    gf::GamePadState neutral,jump,right,left,walk,chord,combat;
    jump.b=true;right.stick_x=127;left.stick_x=-127;walk.dpad_up=true;
    chord=right;chord.b=true;combat.a=true;
    const auto released=[&](const gf::GamePadState& pad,gf::u8 direction){
        input.sample(pad,direction,true);
        input.sample(neutral,0,true);
    };
    const auto consume=[&](gf::GamePadState live){
        input.take(live);
        return live;
    };
    check(consume(neutral)==neutral,"neutral initialization");
    released(jump,0);
    check(consume(neutral)==jump&&consume(neutral)==neutral,"short jump once");
    released(right,2);
    check(consume(neutral)==right&&consume(neutral)==neutral,"short flick once");
    released(jump,0);released(right,2);
    check(consume(neutral)==right,"a later flick does not acquire an earlier jump");
    released(right,2);released(jump,0);
    check(consume(neutral)==jump,"a later jump does not acquire an earlier flick");
    released(chord,2);
    check(consume(neutral)==chord&&consume(neutral)==neutral,"an actual sampled chord is retained once");
    released(right,2);released(left,1);
    check(consume(neutral)==left,"the latest flick wins");

    released(right,2);
    check(consume(jump)==jump&&consume(neutral)==neutral,"live jump discards a buffered flick");
    released(jump,0);
    check(consume(walk)==walk&&consume(neutral)==neutral,"live arrows discard a buffered jump");
    released(right,2);
    check(consume(left)==left,"live stick wins");
    input.sample(chord,2,true);
    check(consume(chord)==chord&&consume(chord)==chord,"held input stays held");
    input.sample(chord,2,true);input.sample(neutral,0,true);
    check(consume(neutral)==neutral,"a delivered held sample does not replay on release");

    released(jump,0);input.sample(neutral,0,false);
    check(consume(neutral)==neutral,"pause or restart clears the buffer");
    input.sample(chord,2,false);input.sample(chord,2,true);input.sample(neutral,0,true);
    check(consume(neutral)==neutral,"held controls through a stopped session do not create an edge");
    released(right,2);input.sample(combat,std::nullopt,true);
    check(consume(combat)==combat&&consume(neutral)==neutral,"unsupported samples clear pending input and remain refused by the source");
    std::puts("practice input: single-sample replay, priority and reset boundaries PASS");
}
