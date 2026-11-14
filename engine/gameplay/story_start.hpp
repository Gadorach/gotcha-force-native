#pragma once
// Story starting-menu component: 8017B1F8/B26C/B4CC and 80181A74.
// Inputs are ALREADY produced source pressed/repeat words. Dependencies below
// are explicit observations, not invented resource/fade completion receipts.
// This component requests a route; it does not load/save/init a campaign,
// play a cinematic, mutate inventory, or authorize a battle.
#include <algorithm>
#include <string>
#include "engine/core/types.hpp"

namespace gf {
enum class StoryStartAction : u8 { Continue=0, Replay=1, Special=2 };
enum class StoryStartPage : u8 {
    Resources, Menu, PlayNumber, DispatchRequested, ExitCover, MainRequested
};
struct StoryStartFacts {
    u32 completed_plays=0; // source +68; completion path saturates at 8
    u32 saved_play=0;      // source +60; zero-based selected play, not mission ID
    bool resume_valid=false; // source +28, distinct from file read success
    bool clear_pending=false; // source +48
    bool operator==(const StoryStartFacts&)const=default;
};
struct StoryStartInput { u32 pressed=0, repeated=0; };
struct StoryStartDependencies { bool resources_ready=false, fade_finished=false; };
struct StoryStartState {
    StoryStartPage page=StoryStartPage::Resources;
    StoryStartAction action=StoryStartAction::Continue;
    StoryStartFacts facts{};
    u8 phase=0; // exact local +2 phase within Menu/PlayNumber
    s16 counter=0, selection=0, play_selection=0;
    u32 selected_play=0, blink=0;
    bool resource_requested=false;
    bool operator==(const StoryStartState&)const=default;
};
struct StoryStartPlan {
    StoryStartState state{};
    bool request_resources=false, build_menu=false, build_play_numbers=false;
    bool request_cover=false; // source black, 32 updates, direction=1
    u16 sound=0; // 119/122/123 requirements, NOT delivered audio
};
inline bool story_start_facts_valid(const StoryStartFacts& facts){
    return facts.completed_plays<=8&&facts.saved_play<8;
}
inline bool story_start_special_visible(const StoryStartFacts& facts){
    return facts.completed_plays>1;
}
inline u32 story_start_play_count(StoryStartAction action,const StoryStartFacts& facts){
    //80181A74: SPECIAL excludes the not-yet-completed next play; REPLAY includes
    //it (up to 8). This is not an arbitrary stage/cinematic unlock list.
    return action==StoryStartAction::Special?std::max(facts.completed_plays,1u):
        std::min(facts.completed_plays+1,8u);
}
inline bool story_start_open_declared(const StoryStartFacts& facts,StoryStartState* out){
    if(!out||!story_start_facts_valid(facts))return false;
    StoryStartState next;next.facts=facts;next.selected_play=facts.saved_play;
    *out=next;return true;
}
namespace story_start_detail {
inline bool valid(const StoryStartState& s){
    if(!story_start_facts_valid(s.facts)||unsigned(s.action)>2||s.selected_play>=8||
       unsigned(s.page)>unsigned(StoryStartPage::MainRequested)||s.blink>=32)return false;
    if(s.action==StoryStartAction::Special&&!story_start_special_visible(s.facts))return false;
    if(s.page==StoryStartPage::Resources)return s.phase==0;
    if(s.page==StoryStartPage::Menu){
        if(s.phase>4||s.selection<0||s.selection>(story_start_special_visible(s.facts)?2:1))return false;
        if(s.phase==1&&(s.counter<1||s.counter>15))return false;
        if(s.phase==3&&(s.counter<1||s.counter>30))return false;
    }
    if(s.page==StoryStartPage::PlayNumber){
        if((s.action!=StoryStartAction::Replay&&s.action!=StoryStartAction::Special)||s.phase>3)return false;
        if(s.play_selection<0||u32(s.play_selection)>=story_start_play_count(s.action,s.facts))return false;
        if(s.phase==2&&(s.counter<1||s.counter>30))return false;
    }
    if(s.page==StoryStartPage::ExitCover&&s.phase>1)return false;
    return true;
}
inline void blink(StoryStartState& s){s.blink=(s.blink+1)&31u;}
}
inline bool story_start_step(const StoryStartState& before,const StoryStartInput& input,
                             const StoryStartDependencies& deps,StoryStartPlan* out,
                             std::string* error=nullptr){
    if(out)*out={};
    auto fail=[&](){if(error)*error="Story start: invalid declared state/output";return false;};
    if(!out||!story_start_detail::valid(before))return fail();
    StoryStartPlan next;next.state=before;auto& s=next.state;
    if(s.page==StoryStartPage::Resources){
        if(!s.resource_requested){s.resource_requested=true;next.request_resources=true;}
        //8017B1F8 advances only after actual source resource-busy becomes zero.
        if(deps.resources_ready){s.page=StoryStartPage::Menu;s.phase=0;}
    }else if(s.page==StoryStartPage::Menu){
        switch(s.phase){
        case 0:
            s.phase=1;s.counter=16;s.selection=s16(s.action);s.blink=0;
            next.build_menu=true;
            [[fallthrough]];
        case 1:
            if(--s.counter<=0)s.phase=2;
            break;
        case 2:
            //Source A precedes B, which precedes navigation. START is not A.
            if(input.pressed&0x100u){
                s.action=StoryStartAction(s.selection);s.phase=3;s.counter=30;next.sound=122;
            }else if(input.pressed&0x200u){
                s.page=StoryStartPage::ExitCover;s.phase=0;next.sound=123;
            }else if(input.repeated&0x40004u){
                const s16 count=story_start_special_visible(s.facts)?3:2;
                s.selection=s16((s.selection+1)%count);next.sound=119;
            }else if(input.repeated&0x80008u){
                const s16 count=story_start_special_visible(s.facts)?3:2;
                s.selection=s16((s.selection+count-1)%count);next.sound=119;
            }
            break;
        case 3:
            if(--s.counter<=0){
                if(s.action==StoryStartAction::Special||
                   (s.action==StoryStartAction::Replay&&s.facts.completed_plays!=0)){
                    s.page=StoryStartPage::PlayNumber;s.phase=0;s.play_selection=0;
                }else{s.phase=4;next.request_cover=true;}
            }
            break;
        case 4:
            if(deps.fade_finished)s.page=StoryStartPage::DispatchRequested;
            break;
        }
        if(s.page==StoryStartPage::Menu&&s.phase>=3)story_start_detail::blink(s);
    }else if(s.page==StoryStartPage::PlayNumber){
        switch(s.phase){
        case 0:
            s.phase=1;s.play_selection=s16(story_start_play_count(s.action,s.facts)-1);
            s.blink=0;next.build_play_numbers=true;
            [[fallthrough]];
        case 1:
            //Unlike the top menu, the play-number constructor falls through
            //to input in this SAME callback; there is no 16-update quarantine.
            if(input.pressed&0x100u){
                s.phase=2;s.counter=30;s.selected_play=u32(s.play_selection);next.sound=122;
            }else if(input.pressed&0x200u){
                s.page=StoryStartPage::Menu;s.phase=2;s.blink=0;next.sound=123;
            }else{
                s16 value=s.play_selection;
                if(input.repeated&0x40004u)value=s16(std::min(u32(value+1),story_start_play_count(s.action,s.facts)-1));
                else if(input.repeated&0x80008u)value=s16(std::max(int(value)-1,0));
                if(value!=s.play_selection){s.play_selection=value;next.sound=119;}
            }
            break;
        case 2:
            if(--s.counter<=0){s.phase=3;next.request_cover=true;}
            break;
        case 3:
            if(deps.fade_finished)s.page=StoryStartPage::DispatchRequested;
            break;
        }
        if(s.page==StoryStartPage::PlayNumber&&s.phase>=2)story_start_detail::blink(s);
    }else if(s.page==StoryStartPage::ExitCover){
        if(s.phase==0){s.phase=1;next.request_cover=true;}
        else if(deps.fade_finished)s.page=StoryStartPage::MainRequested;
    }
    *out=next;if(error)error->clear();return true;
}

enum class StoryStartRoute : u8 { Initialize, ResumeMap, SpecialMap };
struct StoryStartDispatchPlan {
    StoryStartRoute route=StoryStartRoute::Initialize;
    StoryStartFacts facts{};
    u32 selected_play=0;
    bool reset_resume_marker=false, bind_saved_player_name=false;
    bool special_fields=false;
    u32 special_unlock_or=0;
};
inline bool story_start_dispatch_plan(const StoryStartState& s,StoryStartDispatchPlan* out){
    if(out)*out={};
    if(!out||!story_start_detail::valid(s)||s.page!=StoryStartPage::DispatchRequested)return false;
    StoryStartDispatchPlan next;next.facts=s.facts;next.selected_play=s.selected_play;
    //8017B6AC: pure field/route requirements only. No constructor/reset call
    //is acknowledged by this plan, and unrelated persistent fields are untouched.
    if(s.action==StoryStartAction::Continue){
        if(next.facts.clear_pending){next.facts.clear_pending=false;next.facts.resume_valid=false;next.reset_resume_marker=true;}
        if(next.facts.resume_valid){
            next.route=StoryStartRoute::ResumeMap;next.selected_play=next.facts.saved_play;
            next.bind_saved_player_name=true;
        }
    }else if(s.action==StoryStartAction::Replay){
        next.facts.clear_pending=false;
    }else{
        next.route=StoryStartRoute::SpecialMap;next.special_fields=true;next.special_unlock_or=0x1ff6;
    }
    *out=next;return true;
}
} //namespace gf
