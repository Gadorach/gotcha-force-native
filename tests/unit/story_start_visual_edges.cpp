#include <cstdio>
#include <cstdlib>
#include "tests/unit/story_start_visual_fixture.hpp"
int main(){using namespace gf;unsigned checks=0;auto check=[&](bool x){++checks;if(!x)std::abort();};
    const auto font=story_start_visual_fixture::font();const auto art=story_start_visual_fixture::art();StoryStartVisual visual;std::string error;
    check(visual.load(art,font,&error));StoryStartState state;check(story_start_open_declared({3,0,true,false},&state));u64 serial=0;
    auto tick=[&](StoryStartInput input=StoryStartInput{}){StoryStartPlan plan;check(story_start_step(state,input,{true,false},&plan,&error));state=plan.state;check(visual.step(plan,true,++serial,&error));};
    tick();while(state.page!=StoryStartPage::Menu||state.phase!=2)tick();
    tick({0,0x40004});tick({0x100,0});while(state.page!=StoryStartPage::PlayNumber||state.phase!=1)tick();
    for(unsigned i=0;i<6;++i)tick();
    check(visual.widgets()[4].phase==2&&visual.widgets()[4].frame==0);
    tick({0x100,0});
    for(unsigned i=0;i<12;++i){
        check(state.page==StoryStartPage::PlayNumber&&state.phase==2);
        check(visual.widgets()[4].phase==2&&visual.widgets()[4].frame==0);
        bool digit_blink=false;for(const auto& w:visual.widgets())if(w.source.id==8)digit_blink=w.phase==3;
        check(digit_blink);tick();
    }
    std::printf("Story label subpage guard: %u NEW checks PASS; underlying menu remains stable during play confirmation\n",checks);
}
