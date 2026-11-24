#pragma once
#include "engine/assets/battle_initial_post_world.hpp"
namespace gf {
enum class BattleInitialPostWorldPhase:u8 {CounterAdvanced,CollidersInitialized,ContactPrefix,AwaitVisibility,UiWorlds,AwaitActorLate};
//Inspection values only. The scene's private entry identity/counter/revisions,
//completed world traversal and current actors retain continuation authority.
struct BattleInitialPostWorldState {
    u32 before=0,after=0;u64 generation=0,scene_revision=0,entry_revision=0;
    u8 common66=0;BattleInitialPostWorldPhase phase=BattleInitialPostWorldPhase::CounterAdvanced;
    u32 next_source()const{
        switch(phase){case BattleInitialPostWorldPhase::CounterAdvanced:return 0x8008a28c;case BattleInitialPostWorldPhase::CollidersInitialized:return 0x8002bb14;case BattleInitialPostWorldPhase::ContactPrefix:return 0x8002d35c;case BattleInitialPostWorldPhase::AwaitVisibility:return 0x80036e24;case BattleInitialPostWorldPhase::UiWorlds:return 0x80089658;case BattleInitialPostWorldPhase::AwaitActorLate:return 0x80053a94;}return 0;
    }
};
//294D0..294D8 numerical model only, not an entry/scene tick grant. Unsigned
//addition retains addi/stw's low32 bits without signed host overflow.
inline bool battle_initial_post_world_counter(const BattleInitialPostWorldProfile& profile,u32 before,u32* after,std::string* error){
    if(!profile.ready()||!after){if(error)*error="initial post-world counter: source profile/output required";return false;}
    *after=before+u32(1);if(error){error->clear();}return true;
}
}
