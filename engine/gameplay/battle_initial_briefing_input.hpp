#pragma once
#include "engine/gameplay/battle_initial_briefing_widget.hpp"
namespace gf {
struct BattleInitialBriefingInputResult {
    BattleInitialBriefingWidgetPrefix state;bool decrement_counter1653=false;
};
//Value-only arithmetic, not input-source or scene-dispatch authority.
inline bool battle_initial_briefing_input(const BattleInitialBriefingWidgetProfile& profile,const BattleInitialBriefingWidgetPrefix& input,u32 selected_pressed,BattleInitialBriefingInputResult* out,std::string* error){
    BattleInitialBriefingWidgetPrefix expected;
    if(!out||!battle_initial_briefing_widget_prefix(profile,3,&expected,error)||input!=expected||(selected_pressed&~0x1f70u)){
        if(error)*error="initial briefing input: exact unchanged pre-input prefix/known pressed subset required";
        return false;
    }
    BattleInitialBriefingInputResult next;next.state=input;
    next.decrement_counter1653=selected_pressed!=0;
    if(next.decrement_counter1653)++next.state.subphase19;
    next.state.awaiting_input=false;next.state.update_complete=true;*out=next;
    if(error){error->clear();}return true;
}
}
