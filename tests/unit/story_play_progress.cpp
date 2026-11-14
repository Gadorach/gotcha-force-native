#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/story_play_progress.hpp"
int main(){using namespace gf;unsigned checks=0;
    auto check=[&](bool value){++checks;if(!value)std::abort();};
    for(u32 play=0;play<8;++play)for(u32 completed=0;completed<=8;++completed)for(u32 mask=0;mask<256;++mask){
        const StoryPlayProgress before{play,completed,mask,0xa5a50000u|(~mask&255u),false};StoryPlayProgress after;
        check(story_play_completion_plan(before,&after));
        const u32 expected_play=play==7?7:play+1;
        const u32 expected_completed=play==7?8:(play+1>completed?completed+1:completed);
        check(after.selected_play==expected_play&&after.completed_plays==expected_completed);
        check(after.clear_pending&&after.current_unlocks==mask&&after.accumulated_unlocks==0xa5a500ffu);
        check(before.selected_play==play&&!before.clear_pending);
    }
    StoryPlayProgress output{4,5,6,7,true};const auto saved=output;
    check(!story_play_completion_plan({8,0,0,0,false},&output)&&output==saved);
    check(!story_play_completion_plan({0,9,0,0,false},&output)&&output==saved);
    check(!story_play_completion_plan({},nullptr));
    std::printf("Story ending progression component: %u checks PASS (numeric declared states, not completed playthroughs)\n",checks);
}
