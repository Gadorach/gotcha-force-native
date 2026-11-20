#pragma once
#include "engine/gameplay/model_form_rebind.hpp"

namespace gf {
struct NormalTitanRobotSource {
    static constexpr u16 robot=0x0604,tank_form=0x0605;
    static constexpr u8 transform_class_state=2;
    u16 model=0;
    u8 variant=0;
    constexpr bool reviewed()const{return !variant&&(model==robot||model==tank_form);}
    constexpr u16 cycle_destination()const{return !reviewed()?0:model==robot?tank_form:robot;}
};

struct TitanRobotFormTraits {
    u16 data_offset=0;
    u8 movement_type=0,resource_policy=0;
    bool extra_selectors25_26=false;
    bool operator==(const TitanRobotFormTraits&)const=default;
};
inline bool normal_titan_robot_form_traits(NormalTitanRobotSource source,TitanRobotFormTraits* out){
    if(!out||!source.reviewed())return false;
    *out=source.model==NormalTitanRobotSource::robot?TitanRobotFormTraits{0,0,1,true}:TitanRobotFormTraits{432,1,1,false};
    return true;
}

struct TitanRobotCycleRequest {
    bool requested=false;
    u8 action_group=0;
};
// 69B98 conditioned X leaf only. Action arbitration can still refuse the
// group; a proposed group does not debit stock or run the class handler.
inline bool normal_titan_robot_cycle_request(NormalTitanRobotSource source,u32 commands5d4,u8 flags58e,bool reached_leaf,
                                             TitanRobotCycleRequest* out){
    if(!out||!source.reviewed()||!reached_leaf)return false;
    const bool proposed=(commands5d4&0x80u)&&!(flags58e&4u);
    *out={proposed,proposed?u8(2):u8(0)};return true;
}

enum class TitanTransformFloorReturn:u8 {Unresolved,NoContact,Contact};
struct TitanRobotCyclePreparation {
    u16 target_model548=0;
    u8 program_group=0,program=0;
    bool operator==(const TitanRobotCyclePreparation&)const=default;
};

// Both exact X descriptors select the same program and destination on either
// inspected floor return. The caller still owns class acceptance and the
// actual floor result; this is not permission to bypass its motion/contact.
inline bool normal_titan_robot_cycle_prepare(NormalTitanRobotSource source,u8 class_state,u8 substate,
                                             TitanTransformFloorReturn floor,TitanRobotCyclePreparation* out){
    if(!out||!source.reviewed()||class_state!=NormalTitanRobotSource::transform_class_state||substate||
       (floor!=TitanTransformFloorReturn::NoContact&&floor!=TitanTransformFloorReturn::Contact))return false;
    *out={source.cycle_destination(),4,0};return true;
}

// Continued X handler, after the caller observes the staged target in 548
// and the actual lower-lane completion byte. Never substitute retail 0619.
inline bool normal_titan_robot_cycle_rebind(NormalTitanRobotSource source,u16 target_model548,u8 lower_complete1cee,
                                            u8 revision3f0,ModelFormRebindPrefix* out){
    if(!out||!source.reviewed()||target_model548!=source.cycle_destination())return false;
    *out=model_form_rebind_prefix(lower_complete1cee?target_model548:0,source.variant,revision3f0);
    return true;
}

// The exact 4C4 flag-return prefix, not a rule for every hit, stagger or
// alternate class state. The caller owns entry into this callback.
inline bool normal_titan_robot_flagged_return(NormalTitanRobotSource source,u32 flags5e0,u8 revision3f0,
                                              ModelFormRebindPrefix* out){
    using S=NormalTitanRobotSource;
    if(!out||!source.reviewed()||source.model!=S::tank_form)return false;
    *out=model_form_rebind_prefix(flags5e0&0x02000000u?S::robot:0,source.variant,revision3f0);
    return true;
}
}
