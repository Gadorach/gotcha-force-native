#pragma once
#include "engine/assets/battle_initial_gred_children.hpp"
#include "engine/gameplay/startup_attachment_state.hpp"
namespace gf {
//Fresh source-bound slot3 entry only. No scene/actor/world authority comes from
//this numeric adapter; the bootstrap separately owns the actual parent/child.
inline bool battle_initial_gred_child_state(const BattleInitialGRedChildrenProfile& profile,unsigned index,u8 actual_slot,const StartupAttachmentParent& parent,StartupAttachmentUpdate* out,std::string* error){
    if(out)*out={};
    if(!out||!profile.ready()||actual_slot!=3||index>=2||parent.model_id!=0x615||parent.retire662||parent.alternate_ae||parent.selector3ec){if(error)*error="initial G-Red state: sealed fresh normal parent at actual slot3 required";return false;}
    return startup_attachment_state_detail::step(*profile.attachments(),index,actual_slot,5,parent,{},out,error);
}
}
