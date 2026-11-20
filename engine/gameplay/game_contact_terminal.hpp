#pragma once
#include <array>
#include <cmath>
#include "engine/core/types.hpp"

namespace gf {
enum class GameImpactProbeResult:u8 {Unresolved,Clear,Contact};
enum class GameImpactQuery:u8 {None,Floor,Scene};
struct GameContactImpactInput {
    bool observed_post_class=false;
    u8 role493=0,lifecycle18=0,phase19=0;
    f32 entry_hold720=0;
    u8 contact1d9=0,response1dd=0;
    GameImpactProbeResult floor=GameImpactProbeResult::Unresolved,scene=GameImpactProbeResult::Unresolved;
    bool observed_matrix_translation=false;
    std::array<f32,3> matrix_translation{};
};
struct GameContactImpactPlan {
    GameImpactQuery next_query=GameImpactQuery::None;
    bool request_area_object=false,write_pending_terminal=false;
    u8 owner_slot=0,object_type=0,argument=0;
    s16 hp1c6=0;
    u8 pending663=0;
    u16 mask272=0;
    std::array<f32,3> request_position{};
    bool operator==(const GameContactImpactPlan&)const=default;
};
namespace game_contact_terminal_detail {
enum class Actor:u8 {DeathIcbm,DeathBomb};
// Shared fictional actor post-class gate. Impact takes observed contact
// sources in order; this neither queries geometry nor allocates an object.
// Terminal writes do not depend on the later area allocation succeeding.
inline bool impact(Actor actor,u8 slot,const GameContactImpactInput& in,GameContactImpactPlan* out){
    using P=GameImpactProbeResult;
    if(!out||slot>=6||(actor!=Actor::DeathIcbm&&actor!=Actor::DeathBomb)||!in.observed_post_class||!std::isfinite(in.entry_hold720)||
       in.floor>P::Contact||in.scene>P::Contact)return false;
    GameContactImpactPlan next;
    if(in.role493||in.lifecycle18!=1||std::bit_cast<s8>(in.phase19)<=1||in.entry_hold720>0){
        *out=next;return true;
    }
    if(!in.contact1d9&&!in.response1dd&&in.floor!=P::Contact){
        if(in.floor==P::Unresolved){next.next_query=GameImpactQuery::Floor;*out=next;return true;}
        if(in.scene==P::Unresolved)next.next_query=GameImpactQuery::Scene;
        if(in.scene!=P::Contact){*out=next;return true;}
    }
    if(!in.observed_matrix_translation)return false;
    for(f32 x:in.matrix_translation)if(!std::isfinite(x)||std::abs(x)>1e7f)return false;
    next.request_area_object=next.write_pending_terminal=true;
    next.owner_slot=slot;
    next.object_type=actor==Actor::DeathIcbm?73:5;
    next.argument=actor==Actor::DeathIcbm?0:17;
    next.hp1c6=1;next.pending663=1;next.mask272=255;
    next.request_position=in.matrix_translation;
    *out=next;return true;
}
}

struct GameContactDefeatPlan {
    bool request_self_defeat=false;
    u8 slot=0,received1da=0;
    s16 hp1c6=0;
    bool operator==(const GameContactDefeatPlan&)const=default;
};
namespace game_contact_terminal_detail {
inline bool consume_terminal(u8 slot,u8 lifecycle18,u8 pending663,u8 received1da,bool observed_active_slot_pass,
                             GameContactDefeatPlan* out){
    if(!out||slot>=6||!observed_active_slot_pass)return false;
    *out=lifecycle18==1&&pending663?GameContactDefeatPlan{true,slot,u8(received1da|1u),0}:GameContactDefeatPlan{};
    return true;
}
}
}
