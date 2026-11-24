#pragma once
#include "engine/assets/battle_initial_world_tail.hpp"
#include "engine/gameplay/startup_auxiliary.hpp"
namespace gf {
//C79F8 exits before any transform/history/inline writes on a zero selected
//parent channel. This is NOT startup_auxiliary_step: no update/counter increment.
inline bool battle_initial_inactive_auxiliary_world(const BattleInitialWorldTailProfile& profile,u8 channel,const std::array<u8,6>& parent_counts,const StartupAuxiliaryState& state,std::string* error){
    auto fail=[&](){if(error)*error="initial auxiliary world: fresh inline2/zero selected parent channel required; active transform unsupported";return false;};
    if(!profile.ready()||(channel!=0&&channel!=2)||parent_counts[channel]||!state.allocated||state.captured94!=0x615||state.phase!=1||state.counter||state.render_group!=1||state.cached_a8||state.previous_a9||state.matrices_valid)return fail();
    for(const auto& m:state.matrices)for(f32 v:m.m)if(f32_to_bits(v))return fail();
    for(const auto& history:state.history)for(f32 v:history)if(f32_to_bits(v))return fail();
    if(error){error->clear();}return true;
}
}
