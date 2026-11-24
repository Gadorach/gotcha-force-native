#pragma once
#include "engine/assets/battle_initial_briefing_widget.hpp"
namespace gf {
struct BattleInitialBriefingWidgetPrefix {
    u8 phase18=0,render82=0,subphase19=0;s16 timer1c=0,pitch72=0,yaw74=0,rate7e=0,rate80=0;
    f32 material_time44=0,material_step4c=0;
    u64 generation=0;bool applied=false,awaiting_input=false,update_complete=false;
    bool operator==(const BattleInitialBriefingWidgetPrefix&)const=default;
};
inline bool battle_initial_briefing_widget_prefix(const BattleInitialBriefingWidgetProfile& profile,u8 argument,BattleInitialBriefingWidgetPrefix* out,std::string* error){
    if(!profile.ready()||argument!=3||!out){if(error)*error="initial briefing widget: exact fresh type4/argument3 prefix required";return false;}
    BattleInitialBriefingWidgetPrefix next;next.phase18=1;next.render82=0x12;
    next.timer1c=s16(60*argument-1);next.material_step4c=bits_to_f32(0x3dcccccd);
    //The two material requests use the OLD time44=0. The increment follows
    //both AnimAll calls; it is not the selector time or a player tick.
    next.material_time44=f32(next.material_time44+next.material_step4c);
    if(next.material_time44>bits_to_f32(0x426c0000))next.material_time44=f32(next.material_time44-bits_to_f32(0x426c0000));
    next.rate7e=0xa0;next.rate80=0x80;next.pitch72=next.rate7e;next.yaw74=next.rate80;
    next.generation=1;next.applied=next.awaiting_input=true;*out=next;
    if(error){error->clear();}return true;
}
}
