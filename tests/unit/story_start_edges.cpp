#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/story_start.hpp"
int main(){using namespace gf;unsigned checks=0;
    auto check=[&](bool yes){++checks;if(!yes)std::abort();};
    StoryStartState state;StoryStartPlan out;std::string error;
    check(story_start_open_declared({2,0,true,false},&state));
    state.action=StoryStartAction::Special;
    check(story_start_step(state,{}, {},&out,&error));
    for(unsigned completed=0;completed<2;++completed){
        auto bad=state;bad.facts.completed_plays=completed;out.request_resources=true;
        check(!story_start_step(bad,{}, {},&out,&error));
        check(!out.request_resources&&!out.request_cover&&out.sound==0&&!error.empty());
        StoryStartDispatchPlan dispatch;bad.page=StoryStartPage::DispatchRequested;
        check(!story_start_dispatch_plan(bad,&dispatch));
    }
    state.action=StoryStartAction::Continue;state.page=StoryStartPage::ExitCover;
    for(unsigned phase=0;phase<6;++phase){state.phase=u8(phase);
        check(story_start_step(state,{}, {},&out,&error)==(phase<2));
        if(phase>=2)check(!out.request_cover&&!out.request_resources);
    }
    std::printf("Story start new guard edges: %u checks PASS; no prior normal test replay\n",checks);
}
