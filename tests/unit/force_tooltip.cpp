#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/force_tooltip.hpp"
#include "tests/unit/force_inventory_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"force tooltip: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;using namespace force_inventory_fixture;const auto lib=catalog();ForceInventory owner;check(owner.create(lib),"owned inventory");ForceTooltipArt art;art.lifetime=10;art.split_x=-10;art.split_z=-220;ForceTooltip tip;check(tip.bind(owner,art),"owner binding");
    check(tip.step(owner,0,-1,{0,-1},{-10,-220},true)&&!tip.visible(owner),"current-force membership box badge does not expose a duplicate card");
    check(tip.step(owner,0,-1,{-1,0},{-10,-220},true)&&tip.visible(owner)&&tip.visible(owner)->model==0x1001&&tip.visible(owner)->cost==100&&tip.visible(owner)->level==1&&tip.visible(owner)->corner==0,"member card and strict corner thresholds");
    for(unsigned corner=0;corner<4;++corner){const std::array<f32,2> cursor={(corner&2)?0.f:-10.f,(corner&1)?-221.f:-220.f};check(tip.step(owner,0,-1,{1,-1},cursor,true)&&tip.visible(owner)->corner==corner&&tip.visible(owner)->model==0x1002&&tip.visible(owner)->cost==250,"four original placement quadrants");}
    const auto before=*tip.visible(owner);for(unsigned i=0;i<50;++i)check(tip.visible(owner)->remaining==before.remaining,"read/draw observation never advances lifetime");
    for(unsigned i=1;i<10;++i)check(tip.step(owner,0,-1,{150,-1},{100,100},true)&&tip.visible(owner)&&tip.visible(owner)->remaining==10-i&&tip.visible(owner)->corner==before.corner,"empty hover retains last snapshot/position while counting down");
    check(tip.step(owner,0,-1,{150,-1},{100,100},true)&&!tip.visible(owner),"tenth missing-focus callback hides card");
    check(tip.step(owner,0,0,{1,-1},{0,0},true)&&tip.visible(owner)->model==0x1001,"held Borg overrides hovered Borg");check(tip.step(owner,1,-1,{0,-1},{0,0},true)&&tip.visible(owner)->model==0x1001,"same box Borg available on different force");
    auto special=borg(2,5);special.level=9;check(owner.acquire(special)==ForceChange::Applied&&tip.step(owner,0,-1,{3,-1},{0,0},true)&&tip.visible(owner)->variant==5&&tip.visible(owner)->level==10&&tip.visible(owner)->cost==125,"variant-specific cost and stored level+1");
    check(tip.step(owner,0,-1,{3,-1},{0,0},false)&&!tip.visible(owner),"leaving grid clears card immediately");
    check(!tip.step(owner,0,199,{}, {0,0},true)&&!tip.visible(owner),"empty held reference refuses");check(!tip.step(owner,20,-1,{}, {0,0},true)&&!tip.visible(owner),"force index domain");check(!tip.step(owner,0,-1,{0,0},{0,0},true)&&!tip.visible(owner),"ambiguous hit refuses");check(!tip.step(owner,0,-1,{1,-1},{NAN,0},true)&&!tip.visible(owner),"nonfinite cursor refuses");
    ForceInventory other;check(other.create(lib)&&!tip.step(other,0,-1,{1,-1},{0,0},true)&&!tip.visible(owner),"foreign owner cannot retain previous snapshot");check(owner.create(lib)&&!tip.step(owner,0,-1,{1,-1},{0,0},true),"same object reload invalidates old binding");
    std::printf("force tooltip: %u owned checks PASS\n",checks);
}
