#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/force_tooltip.hpp"
#include "tests/unit/force_inventory_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"bottom tooltip: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;const auto lib=force_inventory_fixture::catalog();ForceInventory owner;check(owner.create(lib),"inventory");ForceTooltipArt art;art.lifetime=10;art.split_x=-10;art.split_z=-220;ForceTooltip tooltip;
    for(f32 width:{640.f,960.f}){check(tooltip.bind_bottom(owner,art,width),"explicit bottom placement");for(f32 y:{0.f,240.f,480.f})for(f32 x:{0.f,width*.5f-.5f,width*.5f,width*.5f+.5f,width}){check(tooltip.step(owner,0,-1,{1,-1},{x,y},true),"hover at screen coordinates");check(tooltip.visible(owner)&&tooltip.visible(owner)->corner==(x<width*.5f?1u:3u),"opposite horizontal side; always bottom regardless of vertical position");}}
    check(tooltip.bind_bottom(owner,art,640)&&tooltip.step(owner,0,1,{}, {319,0},true)&&tooltip.visible(owner)->corner==1,"held Borg left of midpoint");check(tooltip.step(owner,0,1,{}, {320,479},true)&&tooltip.visible(owner)->corner==3,"half-screen boundary switches to bottom left");
    check(tooltip.step(owner,0,-1,{199,-1},{0,0},true)&&tooltip.visible(owner)->remaining==9&&tooltip.visible(owner)->corner==3,"no target retains last bottom corner during linger");
    check(!tooltip.bind_bottom(owner,art,0)&&!tooltip.visible(owner)&&!tooltip.bind_bottom(owner,art,NAN),"invalid viewport clears owner");
    std::printf("bottom tooltip: %u owned checks PASS\n",checks);
}
