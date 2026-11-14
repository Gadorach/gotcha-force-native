#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/story_start.hpp"
namespace {
using namespace gf;
unsigned checks=0;
void check(bool yes,const char* why){++checks;if(!yes){std::fprintf(stderr,"Story start: %s\n",why);std::abort();}}
StoryStartPlan step(StoryStartState& s,StoryStartInput in={},StoryStartDependencies deps={}){
    StoryStartPlan p;std::string error;
    check(story_start_step(s,in,deps,&p,&error),"valid step");s=p.state;return p;
}
void activate(StoryStartState& s,const StoryStartFacts& f){
    check(story_start_open_declared(f,&s),"fresh declared facts");
    check(step(s).request_resources,"request resource once");
    check(!step(s).request_resources&&s.page==StoryStartPage::Resources,"no fake ready/repeated request");
    step(s,{}, {true,false});check(s.page==StoryStartPage::Menu&&s.phase==0,"ready does not construct early");
    for(unsigned i=0;i<16;++i){const auto p=step(s,{0x100,0x40004});check(p.build_menu==(i==0)&&s.selection==0,"opening discards input; one build");}
    check(s.phase==2&&s.counter==0,"16 callbacks including constructor; active NEXT input");
}
}
int main(){using namespace gf;
    for(u32 plays=0;plays<=8;++plays){
        StoryStartFacts facts{plays,0,true,false};StoryStartState s;activate(s,facts);
        const unsigned count=plays>1?3:2;
        for(unsigned i=0;i<6;++i){auto p=step(s,{0,0x40004});check(s.selection==s16((i+1)%count)&&p.sound==119,"source menu down wraps enabled slots");}
        step(s,{0,0x80008});check(s.selection==s16(count-1),"source up skips disabled slots");
        auto p=step(s,{0x300,0x40004});check(s.phase==3&&s.action==StoryStartAction(count-1)&&p.sound==122,"A wins B/navigation");
        for(unsigned i=0;i<29;++i){p=step(s);check(s.page==StoryStartPage::Menu&&!p.request_cover,"30 confirm callbacks, no early route");}
        p=step(s);
        const bool numbers=count==3||plays!=0;
        check((s.page==StoryStartPage::PlayNumber)==numbers,"replay-zero skips play numbers; special always selects");
        if(numbers){
            p=step(s);const unsigned n=story_start_play_count(s.action,facts);
            check(p.build_play_numbers&&s.play_selection==s16(n-1),"default highest source-available play");
            p=step(s,{0,0x40004});check(s.play_selection==s16(n-1)&&p.sound==0,"clamped down silent");
            for(unsigned j=1;j<n;++j)step(s,{0,0x80008});
            p=step(s,{0,0x80008});check(s.play_selection==0&&p.sound==0,"clamped up silent");
            p=step(s,{0x200,0x40004});check(s.page==StoryStartPage::Menu&&s.phase==2&&p.sound==123,"B restores SAME menu active; no opening/resources reset");
            step(s,{0x100,0});for(unsigned i=0;i<30;++i)step(s);
            p=step(s,{0x100,0});check(p.build_play_numbers&&s.phase==2&&p.sound==122,"play constructor processes same-callback A");
            for(unsigned i=0;i<29;++i)check(!step(s).request_cover,"play confirm does not cover early");
            check(step(s).request_cover,"play confirm30 starts cover32");
        }else check(p.request_cover,"direct continue/replay starts cover32");
        for(unsigned i=0;i<4;++i){step(s);check(s.page!=StoryStartPage::DispatchRequested,"no fake fade completion");}
        step(s,{}, {false,true});check(s.page==StoryStartPage::DispatchRequested,"real declared fade observation gates dispatch request");
        const auto frozen=s;step(s,{0x300,0x40004},{true,true});check(s==frozen,"terminal request is stable, no implicit re-entry");
    }
    for(unsigned action=0;action<3;++action)for(unsigned resume=0;resume<2;++resume)for(unsigned clear=0;clear<2;++clear){
        StoryStartState s;s.page=StoryStartPage::DispatchRequested;s.action=StoryStartAction(action);
        s.facts={3,2,bool(resume),bool(clear)};s.selected_play=1;StoryStartDispatchPlan p;
        check(story_start_dispatch_plan(s,&p),"declared dispatch candidate");
        const auto route=action==2?StoryStartRoute::SpecialMap:action==0&&resume&&!clear?StoryStartRoute::ResumeMap:StoryStartRoute::Initialize;
        check(p.route==route,"three routes preserve source clear/resume decision");
        check(p.selected_play==(route==StoryStartRoute::ResumeMap?2u:1u),"continue restores saved play, other choices retain selected play");
        check(p.special_fields==(action==2)&&p.special_unlock_or==(action==2?0x1ff6u:0u),"special flags separate from persistent completion count");
        check(p.facts.completed_plays==3&&p.facts.saved_play==2,"planning does not grant progression or change saved play");
    }
    StoryStartState s;activate(s,{2,0,true,false});auto p=step(s,{0x1000,0});check(s.phase==2&&p.sound==0,"START does not act as A");
    p=step(s,{0x200,0x40004});check(s.page==StoryStartPage::ExitCover&&!p.request_cover,"B changes subpage, exit cover begins NEXT callback");
    check(step(s).request_cover,"exit cover request once");check(!step(s).request_cover&&s.page==StoryStartPage::ExitCover,"exit waits without resource/fade guessing");
    step(s,{}, {false,true});check(s.page==StoryStartPage::MainRequested,"main return remains a request");
    StoryStartPlan rejected;p=step(s);auto bad=s;bad.facts.completed_plays=9;
    check(!story_start_step(bad,{}, {},&rejected)&&!rejected.request_cover&&!rejected.request_resources,"bad progression invalidates whole output");
    bad=s;bad.selected_play=8;check(!story_start_step(bad,{}, {},&rejected),"out-of-domain play refuses");
    bad=s;bad.blink=32;check(!story_start_step(bad,{}, {},&rejected),"bad source blink refuses");
    check(!story_start_step(s,{}, {},nullptr),"missing output refuses");
    check(!story_start_open_declared({0,8,false,false},&s),"invalid saved play refuses");
    check(!story_start_open_declared({},nullptr),"open requires output");
    std::printf("Story start component: %u checks PASS; declared inputs/readiness, no save/render/battle/audio ACK\n",checks);
}
