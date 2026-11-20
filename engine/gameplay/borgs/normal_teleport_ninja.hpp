#pragma once
#include <array>
#include "engine/gameplay/weapon_stock_plan.hpp"

namespace gf {
struct NormalTeleportNinjaSource {
    u16 model=0;
    u8 variant=0;
    constexpr bool reviewed()const{return model==0x0005&&!variant;}
};
using TeleportPosition=std::array<f32,3>;
namespace teleport_ninja_detail {
inline bool valid(const TeleportPosition& v){
    for(f32 x:v)if(!std::isfinite(x)||std::abs(x)>1e7f)return false;
    return true;
}
}

struct TeleportSetupPlan {
    TeleportPosition displacement{},goal{};
    f32 first_clock558=0;
    u8 saved_air54a=0;
    bool operator==(const TeleportSetupPlan&)const=default;
};
// Caster history path only: if the directed gate is open, refuse rather
// than silently using history. Arms no live move or other actor's position.
inline bool normal_teleport_history_setup(NormalTeleportNinjaSource source,const TeleportPosition& position,
                                         const TeleportPosition& history5e8,u32 flags5e0,f32 directed_window144,
                                         bool has_target_snapshot,TeleportSetupPlan* out){
    using teleport_ninja_detail::valid;
    if(!out||!source.reviewed()||!std::isfinite(directed_window144)||(directed_window144>0&&has_target_snapshot)||
       !valid(position)||!valid(history5e8))return false;
    TeleportSetupPlan next;
    for(unsigned i=0;i<3;++i){
        next.displacement[i]=f32(position[i]-history5e8[i]);
        const f32 scaled=f32(50.f*next.displacement[i]);
        next.goal[i]=f32(position[i]+scaled);
    }
    if(!valid(next.displacement)||!valid(next.goal))return false;
    next.first_clock558=4;
    next.saved_air54a=u8(flags5e0&0x40u);
    *out=next;return true;
}

// Refreshed class inhibitor and scoped setup debit. An empty debit leaves
// its fields alone; it isn't an invented early exit from the original setup.
inline bool normal_teleport_stock_inhibitor(NormalTeleportNinjaSource source,s16 stock784,u8 flags58e,u8* out){
    if(!out||!source.reviewed()||stock784<0)return false;
    *out=stock784?u8(flags58e&~4u):u8(flags58e|4u);return true;
}
inline bool normal_teleport_setup_debit(NormalTeleportNinjaSource source,const WeaponStockState& channel2,
                                        WeaponStockPlan* out){
    return source.reviewed()&&weapon_stock_plan(channel2,WeaponStockRefillMode::AddOnDebit,WeaponStockOperation::Debit,1,out);
}

struct TeleportMotionProposal {
    TeleportPosition position{};
    u16 suppressed272=0;
    bool contact_due=false;
};
inline bool normal_teleport_first_step(NormalTeleportNinjaSource source,const TeleportPosition& position,
                                       const TeleportPosition& displacement,u16 field272,TeleportMotionProposal* out){
    using teleport_ninja_detail::valid;
    if(!out||!source.reviewed()||!valid(position)||!valid(displacement))return false;
    TeleportMotionProposal next;
    for(unsigned i=0;i<3;++i)next.position[i]=f32(position[i]+displacement[i]);
    if(!valid(next.position))return false;
    next.suppressed272=u16(field272|2u);next.contact_due=true;*out=next;return true;
}
// Two separately rounded scales and one add, before the source floor call.
// A bounded rate does not establish the real action clock or final root.
inline bool normal_teleport_travel_step(NormalTeleportNinjaSource source,const TeleportPosition& position,
                                        const TeleportPosition& goal,f32 rate1dc8,u16 field272,TeleportMotionProposal* out){
    using teleport_ninja_detail::valid;
    if(!out||!source.reviewed()||!valid(position)||!valid(goal)||!std::isfinite(rate1dc8)||rate1dc8<0||rate1dc8>4.f)return false;
    const f32 q=f32(0.1f*rate1dc8),remain=f32(1.f-q);
    TeleportMotionProposal next;
    for(unsigned i=0;i<3;++i){
        const f32 toward=f32(goal[i]*q),retained=f32(position[i]*remain);
        next.position[i]=f32(toward+retained);
    }
    if(!valid(next.position))return false;
    next.suppressed272=u16(field272|2u);next.contact_due=true;*out=next;return true;
}

enum class TeleportExit:u8 {Idle,Fall};
struct TeleportFinishPlan {
    u32 flags5e0=0;
    u16 suppressed272=0;
    u8 field73f=0;
    TeleportExit next=TeleportExit::Idle;
};
// Completion fragment only. Clearing the selective mask is not proof of
// registration, immunity, or any interrupted-action cleanup.
inline bool normal_teleport_finish(NormalTeleportNinjaSource source,u32 flags5e0,u16 field272,
                                   bool reached_completed_recovery,TeleportFinishPlan* out){
    if(!out||!source.reviewed()||!reached_completed_recovery)return false;
    *out={flags5e0&~3u,u16(field272&~2u),0,(flags5e0&0x40u)?TeleportExit::Fall:TeleportExit::Idle};
    return true;
}
}
