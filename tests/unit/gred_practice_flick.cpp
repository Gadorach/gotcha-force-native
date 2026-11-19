#include <cstdio>
#include <cstdlib>
#include "platform/pc/gred_practice_flick.hpp"

namespace {
void check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"practice flick: %s\n",why);std::abort();}
}
}

int main(){
    gf::pc::GRedPracticeFlick flick;
    gf::GamePadState neutral,right,left,walk;
    right.stick_x=127;left.stick_x=-127;walk.dpad_up=true;
    auto short_right=[&](){flick.sample(right,2,true);flick.sample(neutral,0,true);};
    short_right();
    auto delivered=neutral;flick.take(delivered);
    check(delivered==right,"released high deflection reaches one tick");
    delivered=neutral;flick.take(delivered);check(delivered==neutral,"no repeat");

    flick.sample(right,2,true);delivered=right;flick.take(delivered);
    flick.sample(right,2,true);flick.sample(neutral,0,true);
    delivered=neutral;flick.take(delivered);check(delivered==neutral,"already delivered held stick is not replayed on release");

    short_right();delivered=walk;flick.take(delivered);
    check(delivered==walk,"live walk direction takes priority");
    delivered=neutral;flick.take(delivered);check(delivered==neutral,"discarded flick does not become a later dash");
    short_right();delivered=left;flick.take(delivered);
    check(delivered==left,"live stick direction takes priority");

    flick.sample(right,2,true);flick.sample(left,1,true);flick.sample(neutral,0,true);
    delivered=neutral;flick.take(delivered);check(delivered==left,"latest undelivered flick wins");
    short_right();flick.sample(neutral,0,false);
    delivered=neutral;flick.take(delivered);check(delivered==neutral,"pause or restart drops pending deflection");
    flick.sample(right,2,false);flick.sample(right,2,true);flick.sample(neutral,0,true);
    delivered=neutral;flick.take(delivered);check(delivered==neutral,"holding through a stop does not produce a new edge");
    std::puts("practice flick: short, held, superseded, and reset cases PASS");
}
