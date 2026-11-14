#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/force_popup_timeline.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"popup timeline: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;ForcePopupTimeline timeline;ForcePopupPresentation request;
    check(timeline.step(request,3)&&timeline.visible().size()==1&&timeline.visible()[0].frame==1,"first source callback advances open request");
    for(unsigned i=2;i<=25;++i)check(timeline.step(request,3)&&timeline.visible()[0].frame==i,"open request progression");
    check(timeline.step(request,3)&&timeline.visible()[0].frame==25,"open limit retains endpoint");const auto age=timeline.visible()[0].age;
    request.selected=4;request.disabled=0xf;check(timeline.step(request,3)&&timeline.visible()[0].selected==4&&timeline.visible()[0].disabled==0xf&&timeline.visible()[0].frame==25&&timeline.visible()[0].age==age+1,"selection updates do not restart shape");
    request.kind=ForcePopupKind::Erase;request.selected=1;check(timeline.step(request,3)&&timeline.visible().size()==2&&timeline.visible()[0].frame==8&&timeline.visible()[1].frame==1,"new confirmation overlaps retiring operations");
    for(unsigned i=0;i<8;++i)check(timeline.step(request,3),"explicit close callbacks");
    check(timeline.visible().size()==2&&timeline.visible()[0].frame==0,"close zero retained for this boundary");check(timeline.step(request,3)&&timeline.visible().size()==1,"retired panel released on next callback");
    check(timeline.step(std::nullopt,3)&&timeline.visible()[0].frame==8,"return to no modal begins close");for(unsigned i=0;i<9;++i)check(timeline.step(std::nullopt,3),"no desired modal drains lifetime");check(timeline.visible().empty(),"no stale modal remains");
    request.kind=ForcePopupKind::Tribe;request.selected=8;request.first=1;check(timeline.step(request,1)&&timeline.visible()[0].frame==1,"fresh resource context starts new view");const auto previous=timeline.visible();request.first=9;check(!timeline.step(request,1)&&timeline.visible()[0].first==previous[0].first,"invalid view leaves lifetime unchanged");
    request.first=1;check(timeline.step(request,2)&&timeline.visible().size()==1&&timeline.visible()[0].frame==1,"domain replacement releases previous-context panel");
    request.kind=ForcePopupKind::Operations;request.first=request.selected=0;check(timeline.step(request,2),"same-context replacement");
    const auto closing_age=timeline.visible()[0].age;check(timeline.step(request,2)&&timeline.visible()[0].age==closing_age+1,"retiring callback continues marker counter");
    request.kind=ForcePopupKind::Erase;request.selected=2;check(!timeline.step(request,2),"invalid confirmation choice refused");
    timeline.clear();check(timeline.visible().empty(),"explicit scene teardown clears all panels");
    std::printf("popup timeline: %u owned checks PASS\n",checks);
}
