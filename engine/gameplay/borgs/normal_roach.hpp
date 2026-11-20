#pragma once
#include "engine/gameplay/character_air_resource.hpp"
#include "engine/gameplay/character_movement_input.hpp"
#include "engine/gameplay/weapon_stock_plan.hpp"

namespace gf {
struct NormalRoachSource {
    u16 model=0;
    u8 variant=0,role48a=0;
    constexpr bool reviewed()const{return model==0x0f05&&!variant&&!role48a;}
    static constexpr u8 movement_type=1,resource_policy=0;
    static constexpr AirPointRules air_points{1,false};
    static constexpr AirMeterRules dormant_meter{AirMeterRecoveryGate::PreviousGroundedCache,10000,0,2000,{0,0,0,0}};
};
enum class RoachMovementIntent:u8 {IdleJump,FallRelaunch,GroundDash,AirDash};
struct RoachMovementRequest {
    u8 selector=0;
    u16 remaining_directions5d8=0;
    bool reset_direction57c=false;
};
// Only the reviewed leaves, after earlier actor paths decline. Both dash
// rows are unavailable; a refused request cannot consume the flick.
inline bool normal_roach_movement_request(NormalRoachSource source,RoachMovementIntent intent,u8 selector,
                                          u32 flags5e0,u8 ready588,u8 used747,bool earlier_declined,
                                          const CharacterMovementInputState& in,RoachMovementRequest* out){
    if(!out||!source.reviewed()||!earlier_declined||!NormalRoachSource::air_points.accepts(used747)||
       !character_movement_input_state_valid(in)||(in.commands5d4&~3u))return false;
    RoachMovementRequest next{selector,in.directions5d8,false};
    switch(intent){
    case RoachMovementIntent::IdleJump:
        if(selector!=0)return false;
        if(!(flags5e0&2u)&&(in.commands5d4&1u)&&(in.previous_held&0x100u)){
            next.selector=4;next.reset_direction57c=true;
        }
        break;
    case RoachMovementIntent::FallRelaunch:
        if(selector!=6)return false;
        if(!(flags5e0&2u)&&(in.commands5d4&2u)&&ready588&&NormalRoachSource::air_points.has_point(used747))next.selector=8;
        break;
    case RoachMovementIntent::GroundDash:if(selector!=0)return false;break;
    case RoachMovementIntent::AirDash:if(selector!=5&&selector!=6)return false;break;
    default:return false;
    }
    *out=next;return true;
}

inline bool normal_roach_count_entry(NormalRoachSource source,u8 selector,u8 used747,u8* out){
    if(!source.reviewed()||(selector!=5&&selector!=8))return false;
    return character_air_point_entry(NormalRoachSource::air_points,used747,selector==5?AirPointEntry::Ascent:AirPointEntry::Relaunch,out);
}
inline bool normal_roach_grounded_bookkeeping(NormalRoachSource source,const CharacterAirResourceState& in,
                                             u8 cached87,CharacterAirResourceState* out){
    return source.reviewed()&&cached87<=1&&NormalRoachSource::air_points.accepts(in.air_actions747)&&
        in.value736>=0&&in.value736<=NormalRoachSource::dormant_meter.limit&&in.delay73a>=0&&
        character_air_resource_recover(in,NormalRoachSource::dormant_meter,0,cached87,out);
}

struct RoachControlProgramSample {
    u8 lane_control1d0f=0;
    u16 behaviour_payload=0;
    u8 wait_mode=0,wait_ticks=0;
    bool binds_local_clip=false;
};
// Reviewed group-zero metadata only. Does not execute an actor or a second
// animation VM, synthesize descriptors, or equate wait ticks with clip frames.
inline bool normal_roach_control_program_sample(NormalRoachSource source,u8 group,u8 program,RoachControlProgramSample* out){
    if(!out||!source.reviewed()||group)return false;
    switch(program){
    case 0:case 2:case 31:*out={0,0,9,10,false};return true;
    case 1:*out={0,0x10,9,10,false};return true;
    case 4:case 6:case 13:case 14:*out={0,0x40,9,2,false};return true;
    case 8:*out={1,0x200,9,4,false};return true;
    default:return false;
    }
}

struct RoachBombReleasePlan {
    WeaponStockPlan stock;
    u8 next_phase540=0;
    f32 clock558=0;
    bool request_bomb=false;
    u8 firing_slot=0,object_type=0,argument=0;
};
// Upper class-owned ordinary B release. Debit is not an allocation and
// a failed allocation cannot manufacture a retirement credit.
inline bool normal_roach_bomb_release(NormalRoachSource source,u8 owner_slot,u8 phase540,
                                      const WeaponStockState& channel0,RoachBombReleasePlan* out){
    if(!out||!source.reviewed()||owner_slot>=6||phase540!=1)return false;
    RoachBombReleasePlan next;
    if(!weapon_stock_plan(channel0,WeaponStockRefillMode::ClearClockOnDebit,WeaponStockOperation::Debit,1,&next.stock))return false;
    next.next_phase540=2;next.clock558=10;
    if(next.stock.accepted){next.request_bomb=true;next.firing_slot=owner_slot;next.object_type=16;}
    *out=next;return true;
}
struct RoachBombRetirement {
    u8 object_type=0,object_phase=0,stored_owner_slot=0,current_owner_slot=0;
    bool observed_owned_retirement=false;
};
inline bool normal_roach_bomb_retirement_stock(NormalRoachSource source,const RoachBombRetirement& retirement,
                                               const WeaponStockState& channel0,s16 max_stock,WeaponStockState* out){
    return source.reviewed()&&retirement.observed_owned_retirement&&retirement.object_type==16&&retirement.object_phase==2&&
        retirement.current_owner_slot<6&&retirement.stored_owner_slot==retirement.current_owner_slot&&
        weapon_stock_return_one(channel0,max_stock,out);
}
}
