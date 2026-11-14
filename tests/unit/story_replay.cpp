#include <cstdio>
#include <cstdlib>
#include <limits>
#include "engine/gameplay/story_replay.hpp"
int main(){using namespace gf;unsigned checks=0;
    auto check=[&](bool yes){++checks;if(!yes)std::abort();};
    for(unsigned index=0;index<8;++index){
        StoryReplayContext replay;check(StoryReplayContext::from_declared_index(index,&replay));
        check(replay.valid()&&replay.index()==index&&replay.event_flag()==(1u<<index)&&replay.displayed_play()==index+1);
        for(unsigned flags=0;flags<256;++flags){bool matched=false;
            check(story_event_replay_mask_matches(replay,u8(flags),&matched));
            check(matched==((flags&(1u<<index))!=0));
        }
        for(unsigned event=0;event<256;++event){
            StoryReplayHistory history;for(unsigned i=0;i<256;++i)history.bytes[i]=u8((i*37+index*11)^0x5a);
            const auto before=history;StoryReplayHistory next;
            check(story_replay_history_mark_plan(history,replay,event,&next));
            const unsigned at=index*32+event/8;
            for(unsigned i=0;i<256;++i)check(next.bytes[i]==(i==at?u8(before.bytes[i]|(1u<<(event&7))):before.bytes[i]));
            bool seen=false;check(story_replay_history_seen(next,replay,event,&seen)&&seen);
            check(history==before);
        }
    }
    //Total completion8 must not override a current REPLAY1 or a saved Continue2.
    StoryStartState state;state.page=StoryStartPage::DispatchRequested;state.action=StoryStartAction::Continue;
    state.facts={8,2,true,false};state.selected_play=7;StoryStartDispatchPlan start;
    check(story_start_dispatch_plan(state,&start));StoryReplayContext current;
    check(story_replay_context_for_start_candidate(start,&current)&&current.index()==2&&current.event_flag()==4);
    state.action=StoryStartAction::Replay;state.selected_play=1;check(story_start_dispatch_plan(state,&start));
    check(story_replay_context_for_start_candidate(start,&current)&&current.index()==1&&current.event_flag()==2);
    //Same event completion remains separately recorded for each running play.
    StoryReplayHistory history;check(story_replay_history_mark_plan(history,current,254,&history));
    StoryReplayContext other;check(StoryReplayContext::from_declared_index(2,&other));bool seen=true;
    check(story_replay_history_seen(history,other,254,&seen)&&!seen);
    StoryReplayHistory reloaded;check(story_replay_history_decode({history.bytes.data(),history.bytes.size()},&reloaded)&&reloaded==history);
    const auto saved=history;
    check(!story_replay_history_mark_plan(history,other,256,&history)&&history==saved);
    check(!story_replay_history_mark_plan(history,other,std::numeric_limits<u32>::max(),&history)&&history==saved);
    check(!story_replay_history_seen(history,{},0,&seen)&&!seen);
    check(!story_replay_history_decode({history.bytes.data(),255},&history)&&history==saved);
    check(!StoryReplayContext::from_declared_index(8,&current)&&!current.valid());
    check(!StoryReplayContext::from_declared_index(std::numeric_limits<u32>::max(),&current)&&!current.valid());
    check(!StoryReplayContext::from_declared_index(0,nullptr));
    check(!story_event_replay_mask_matches({},255,&seen)&&!seen);
    std::printf("Story current replay/event-history: %u checks PASS; numeric masks/history, not battle/drop-rate or save-owner ACK\n",checks);
}
