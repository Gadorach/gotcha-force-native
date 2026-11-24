#pragma once
#include "engine/assets/battle_initial_actor_order.hpp"
#include "engine/gameplay/scene_physical_links.hpp"
#include "engine/gameplay/source_byte_rng.hpp"
#include <optional>
namespace gf {
struct BattleInitialFactoryOrderFields {
    f32 field204=0,field208=0,field664=0;
    bool operator==(const BattleInitialFactoryOrderFields&)const=default;
};
struct BattleInitialActorOrderState {
    //These are53B30's actual global lists/counts. Unknown update-array tails are
    //not zeroed: source resets only its count, then writes the admitted prefix.
    std::array<std::array<ScenePhysicalLink,6>,4> teams{};
    std::array<u8,4> team_counts{};std::array<std::optional<ScenePhysicalLink>,6> updates{};
    u8 count=0,start=0; s8 step=-1;std::array<u8,2> draws{};
    std::array<u8,6> scan{};
    //Values written to factory cells; after handoff the actor owns current fields.
    std::array<BattleInitialFactoryOrderFields,6> factory_writes{};
    bool operator==(const BattleInitialActorOrderState&)const=default;
};
struct BattleInitialActorOrderPlan {BattleInitialActorOrderState state;SourceByteRngState after;};
//Declared numeric adapter only. Native admission verifies every unclaimed
//physical factory cell and the same live party RNG owner.
inline bool battle_initial_actor_order(const BattleInitialActorOrderProfile& profile,const std::array<u8,6>& active,SourceByteRngState before,BattleInitialActorOrderPlan* out,std::string* error){
    if(out)*out={};
    if(!out||!profile.ready()){if(error)*error="initial actor order: source profile/destination required";return false;}
    for(u8 value:active)if(value>1){if(error)*error="initial actor order: factory activity domain";return false;}
    BattleInitialActorOrderPlan next;next.after=before;
    for(auto& draw:next.state.draws)draw=source_byte_rng_step(next.after);
    next.state.start=u8((next.state.draws[0]>>2)%6);next.state.step=(next.state.draws[1]&8)?1:-1;
    int slot=next.state.start;
    for(unsigned i=0;i<6;++i){
        next.state.scan[i]=u8(slot);next.state.factory_writes[unsigned(slot)].field664=1;
        //Factory18=0: no phase1 team insertion or data+98 read. Factory1DB,
        //4A0/4A1/4A4 are zero: dependency ranks tie and source appends in scan order.
        if(active[unsigned(slot)])next.state.updates[next.state.count++]=ScenePhysicalLink{0,slot};
        slot=(slot+next.state.step+6)%6;
    }
    *out=next;if(error)error->clear();return true;
}
}
