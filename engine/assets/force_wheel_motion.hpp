#pragma once
#include "engine/core/types.hpp"
#include <cmath>
#include <string>
namespace gf {
struct ForceWheelMotionProfile {
    u32 bank_hash=0;u16 turn_ticks=0;
    f32 reciprocal=0,wheel_fold=0,wheel_step=0,cost_fold=0,cost_step=0,tab_fold=0,tab_step=0,half=0,angle_unit=0;
    bool valid()const{return bank_hash&&turn_ticks==8&&reciprocal==.125f&&half==.5f&&
        std::isfinite(wheel_fold)&&wheel_fold>0&&wheel_fold<=1e6f&&std::isfinite(wheel_step)&&wheel_step>0&&wheel_step<=wheel_fold&&
        std::isfinite(cost_fold)&&cost_fold>0&&cost_fold<=1e6f&&std::isfinite(cost_step)&&cost_step>0&&cost_step<=cost_fold&&
        std::isfinite(tab_fold)&&tab_fold>0&&tab_fold<=1e6f&&std::isfinite(tab_step)&&tab_step>0&&tab_step<=tab_fold&&std::isfinite(angle_unit)&&angle_unit>0&&angle_unit<1;}
    bool load(ByteView b,std::string* error){
        *this={};if(!b.data||b.size!=52||b.be32(0)!=0x4746574du||b.be32(4)!=1||b.be16(14)){if(error)*error="wheel motion: GFWM header";return false;}
        bank_hash=b.be32(8);turn_ticks=b.be16(12);reciprocal=b.bef32(16);wheel_fold=b.bef32(20);wheel_step=b.bef32(24);cost_fold=b.bef32(28);cost_step=b.bef32(32);tab_fold=b.bef32(36);tab_step=b.bef32(40);half=b.bef32(44);angle_unit=b.bef32(48);
        if(!valid()){if(error)*error="wheel motion: unsupported operands";return false;}if(error)error->clear();return true;
    }
};
} // namespace gf
