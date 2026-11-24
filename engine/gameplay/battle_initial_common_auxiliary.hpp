#pragma once
#include "engine/assets/battle_initial_common_auxiliary.hpp"
#include "engine/gameplay/startup_auxiliary.hpp"
namespace gf {
inline bool battle_initial_common_auxiliary_state(const BattleInitialCommonAuxiliaryProfile& profile,u8 actual_slot,const StartupAuxiliaryState& input,StartupAuxiliaryState* out,std::string* error){
    if(out)*out={};
    if(!out||!profile.ready()||actual_slot!=3||!input.allocated||input.captured94!=0x615||input.phase||input.counter||input.render_group||input.cached_a8||input.previous_a9||input.matrices_valid){if(error)*error="initial common auxiliary: actual slot3/fresh inline-two constructor state required";return false;}
    auto next=input;startup_auxiliary_detail::initialize_inline_two(next);
    //The fresh class2 buffer was cleared by the allocator. These are storage
    //words, not identity/current worlds; C79F8 has not written either matrix.
    for(auto& matrix:next.matrices)std::fill_n(matrix.m,16,0.0f);
    *out=next;if(error)error->clear();return true;
}
}
