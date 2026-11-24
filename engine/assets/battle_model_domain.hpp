#pragma once
#include "engine/core/types.hpp"
namespace gf {
//Versioned operand domains; this is not an actor-construction permission.
enum class BattleModelDomain:u8 {NormalPair=1,InitialAi=2};
inline bool battle_model_in_domain(BattleModelDomain domain,u16 model){
    return domain==BattleModelDomain::NormalPair?(model==0x615||model==10):
        domain==BattleModelDomain::InitialAi&&(model==0x70b||model==0x105);
}
}
