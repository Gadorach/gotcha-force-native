#include <cstdio>
#include <cstdlib>
#include "platform/pc/gred_practice_jump_tap.hpp"

namespace {
void check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"practice jump tap: %s\n",why);std::abort();}
}
}

int main(){
    gf::pc::GRedPracticeJumpTap tap;
    check(!tap.take(),"neutral at initialization");
    tap.sample(true,true);
    tap.sample(false,true);
    check(tap.take()&&!tap.take(),"short tap reaches exactly one tick");

    tap.sample(true,true);
    check(tap.take()&&tap.take(),"holding still supplies normal held input");
    tap.sample(false,true);
    check(!tap.take(),"release after an already delivered press");

    tap.sample(true,true);
    tap.sample(false,false);
    tap.sample(false,true);
    check(!tap.take(),"pause or restart drops a pending tap");
    tap.sample(true,false);
    tap.sample(false,false);
    tap.sample(false,true);
    check(!tap.take(),"a tap entirely while stopped cannot replay");

    tap.sample(true,true);
    tap.sample(false,true);
    tap.sample(true,true);
    tap.sample(false,true);
    check(tap.take()&&!tap.take(),"multiple between-tick taps coalesce without injecting releases");
    std::puts("practice jump tap: sampled edges, held input and reset boundaries PASS");
}
