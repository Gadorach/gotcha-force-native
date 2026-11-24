#include <cstdio>
#include <cstdlib>
#include <limits>
#include "engine/gameplay/collision_contact_reset.hpp"
namespace {unsigned checks=0;void check(bool value,const char* why){++checks;if(!value){std::fprintf(stderr,"contact reset: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;std::string error;CollisionContactState before;
    before.retained.displacement1cc={1,2,3};before.retained.previous275=17;before.retained.timer1df=1;before.retained.timer2ae=255;before.retained.timer2af=2;before.retained.timer1e8=0;
    before.field1d9=3;before.field1da=4;before.attached1db=7;before.previous1de=8;before.field1dc=9;before.moved1dd=10;before.field280=11;before.current274=12;before.hidden270=0xfffe;before.shadow286=0xabcd;
    CollisionContactState next;check(collision_contact_reset(before,&next,&error),"declared reset");
    check(next.retained.displacement1cc==std::array<f32,3>{}&&next.previous1de==7&&!next.attached1db&&next.retained.previous275==12&&!next.current274,"ordered copies retain old attachment/status before clearing");
    check(!next.field1d9&&!next.field1da&&!next.field1dc&&!next.moved1dd&&!next.field280&&!next.hidden270&&!next.shadow286,"contact and visibility clears");
    check(!next.retained.timer1df&&next.retained.timer2ae==254&&next.retained.timer2af==1&&!next.retained.timer1e8,"byte timers decrement only when nonzero");
    check(before.attached1db==7&&before.retained.timer1df==1,"input snapshot unchanged");
    check(collision_contact_reset(next,&next,&error)&&!next.previous1de&&!next.retained.previous275&&next.retained.timer2ae==253,"numeric reset handles alias without reordering copies");
    before.retained.displacement1cc[0]=std::numeric_limits<float>::quiet_NaN();check(!collision_contact_reset(before,&next,&error),"invalid source field refuses");
    CollisionBroadphaseResult p;
    check(collision_contact_broadphase({0,0,0},1,{2,0,0},1,.1f,&p,&error)&&p.overlap&&p.distance==2&&p.radius_sum==2,"inclusive touching broadphase");
    check(collision_contact_broadphase({0,0,0},1,{2.001f,0,0},1,.1f,&p,&error)&&!p.overlap&&p.distance_used,"separated broadphase");
    check(collision_contact_broadphase({0,0,0},.1f,{0,0,0},10,.1f,&p,&error)&&!p.overlap&&!p.distance_used&&p.radii_used,"radius threshold is strict and precedes distance");
    check(!collision_contact_broadphase({0,0,0},-1,{0,0,0},10,.1f,&p,&error),"negative radius refuses rather than inventing separation");
    check(!collision_contact_broadphase({0,0,0},1,{0,0,0},10,.01f,&p,&error),"changed source threshold refuses");
    std::printf("contact reset and broadphase: %u checks PASS\n",checks);
}
