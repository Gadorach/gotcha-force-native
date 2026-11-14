#pragma once
#include "engine/assets/battle_actor_levels.hpp"
#include "engine/gameplay/force_cost.hpp"
namespace gf {
struct BattleActorLevelCostWrite {u16 offset=0,value=0;bool operator==(const BattleActorLevelCostWrite&)const=default;};
struct BattleActorLevelCostPlan {std::array<s16,3> shorts1c4_1c6_1c8{};s16 cost4aa=0;std::vector<BattleActorLevelCostWrite> writes;};
//55F90's ordinary role490=0 branch ONLY. The shared two-thirds policy equals
//the source f32(3F2AAAAB)*signed-cost then truncation throughout the admitted
//nonnegative s16 catalog domain (separately checked exhaustively). No tool cost,
//rank/HP guess, peer/paired branch or RNG update is performed here.
inline bool battle_actor_level_cost(const BattleActorLevelRow& row,u8 ai3e6,u8 field486,BattleActorLevelCostPlan* out,std::string* error){
    if(out)*out={};
    if(!out||(row.model!=0x615&&row.model!=10)||row.variant||row.level>=10||!row.model_hash||row.full_cost<0||row.values[0]<0){if(error)*error="battle actor level cost: normal canonical row/cost required";return false;}
    BattleActorLevelCostPlan p;p.shorts1c4_1c6_1c8.fill(row.values[0]);using W=BattleActorLevelCostWrite;
    p.writes.push_back(W{0x1c4,std::bit_cast<u16>(row.values[0])});p.writes.push_back(W{0x1c8,std::bit_cast<u16>(row.values[0])});p.writes.push_back(W{0x1c6,std::bit_cast<u16>(row.values[0])});
    p.cost4aa=row.full_cost;p.writes.push_back(W{0x4aa,u16(p.cost4aa)});
    if(ai3e6){p.cost4aa=s16(force_cost_for_mode(row.full_cost,ForceCostMode::TwoThirds));p.writes.push_back(W{0x4aa,u16(p.cost4aa)});}
    if(field486){p.cost4aa=0;p.writes.push_back(W{0x4aa,0});}
    *out=std::move(p);if(error)error->clear();return true;
}
} // namespace gf
