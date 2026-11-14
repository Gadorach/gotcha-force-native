#include <cstdio>
#include <cstdlib>
#include <limits>
#include <vector>
#include "engine/gameplay/story_event_progress.hpp"
namespace {unsigned checks=0;void check(bool ok){++checks;if(!ok)std::abort();}}
int main(){using namespace gf;
    std::vector<u8> source(kStoryEventProgressBytes);for(unsigned i=0;i<source.size();++i)source[i]=u8((i*73+19)^(i>>3));
    StoryEventProgress original;check(story_event_progress_decode_declared({source.data(),source.size()},&original));
    std::vector<u8> encoded(source.size(),0);check(story_event_progress_encode(original,encoded)&&encoded==source);
    for(unsigned id=0;id<256;++id)for(unsigned flags=0;flags<256;++flags){
        auto before=original;before.events[id][0]=u8(flags);u8 bit=255;
        check(story_event_progress_bits(before,s32(id),StoryEventFlag::Flag01,&bit)&&bit==(flags&1));
        check(story_event_progress_bits(before,s32(id),StoryEventFlag::Flag02,&bit)&&bit==(flags&2));
        //Each numeric case changes only its represented byte; full-row/full-
        //segment retention comparisons are covered separately below.
        for(unsigned operation=0;operation<3;++operation){StoryEventProgress next;check(story_event_progress_flag_plan(before,s32(id),StoryEventFlagMutation(operation),&next));
            const auto expected=operation==2?flags&253:flags|(operation==0?1:2);
            check(next.events[id][0]==expected&&next.events[id][1]==before.events[id][1]&&next.gates==before.gates);
        }
    }
    for(unsigned id=0;id<64;++id)for(unsigned flags=0;flags<256;++flags){
        auto before=original;before.gates[id][0]=u8(flags);
        for(auto flag:{StoryEventFlag::Flag01,StoryEventFlag::Flag02}){u8 bit=255;StoryEventProgress next;
            check(story_gate_progress_bits(before,s32(id),flag,&bit)&&bit==(flags&u8(flag)));
            check(story_gate_progress_flag_plan(before,s32(id),flag,&next)&&next.gates[id][0]==(flags|u8(flag))&&next.gates[id][1]==before.gates[id][1]&&next.events==before.events);
        }
    }
    for(unsigned id=0;id<256;++id)for(unsigned operation=0;operation<3;++operation){
        auto expected=original;auto& byte=expected.events[id][0];if(operation==2)byte&=253;else byte|=operation==0?1:2;
        auto next=original;check(story_event_progress_flag_plan(next,s32(id),StoryEventFlagMutation(operation),&next)&&next==expected);
    }
    for(unsigned id=0;id<64;++id)for(auto flag:{StoryEventFlag::Flag01,StoryEventFlag::Flag02}){
        auto expected=original;expected.gates[id][0]|=u8(flag);auto next=original;check(story_gate_progress_flag_plan(next,s32(id),flag,&next)&&next==expected);
    }
    for(s32 id:{std::numeric_limits<s32>::min(),-1,256,std::numeric_limits<s32>::max()}){auto next=original;u8 bit=255;
        check(!story_event_progress_bits(original,id,StoryEventFlag::Flag01,&bit)&&bit==0);
        check(!story_event_progress_flag_plan(original,id,StoryEventFlagMutation::Set01,&next)&&next==original);
    }
    for(s32 id:{std::numeric_limits<s32>::min(),-1,64,255,std::numeric_limits<s32>::max()}){auto next=original;u8 bit=255;
        check(!story_gate_progress_bits(original,id,StoryEventFlag::Flag02,&bit)&&bit==0);
        check(!story_gate_progress_flag_plan(original,id,StoryEventFlag::Flag02,&next)&&next==original);
    }
    for(unsigned flag:{0u,3u,255u}){u8 bit=255;auto next=original;
        check(!story_event_progress_bits(original,0,StoryEventFlag(flag),&bit)&&bit==0);
        check(!story_gate_progress_flag_plan(original,0,StoryEventFlag(flag),&next)&&next==original);
    }
    auto next=original;check(!story_event_progress_flag_plan(original,0,StoryEventFlagMutation(3),&next)&&next==original);
    check(!story_event_progress_bits(original,0,StoryEventFlag::Flag01,nullptr));
    check(!story_gate_progress_bits(original,0,StoryEventFlag::Flag01,nullptr));
    check(!story_event_progress_flag_plan(original,0,StoryEventFlagMutation::Set01,nullptr));
    check(!story_gate_progress_flag_plan(original,0,StoryEventFlag::Flag01,nullptr));
    check(!story_event_progress_decode_declared({source.data(),source.size()-1},&next)&&next==original);
    check(!story_event_progress_decode_declared({source.data(),0x4628},&next)&&next==original);
    check(!story_event_progress_decode_declared({nullptr,source.size()},&next)&&next==original);
    check(!story_event_progress_decode_declared({source.data(),source.size()},nullptr));
    check(!story_event_progress_encode(original,std::span<u8>(encoded.data(),encoded.size()-1))&&encoded==source);
    std::printf("Story event progress: %u checks PASS; exhaustive numeric flags/indices, not event execution/save-load authority\n",checks);
}
