#pragma once
#include <array>
#include <cmath>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
//GFMM1..9: numeric normal-G-Red operands, not a controller or actor admission.
struct CharacterMovementProfile {
    u16 model=0;u32 model_hash=0,class_flags=0;u8 movement_type=0;
    std::array<f32,6> walk_speeds{};std::array<u16,5> turn_counts{};
    std::array<u8,8> direction_classes{},cardinal_headings{},octant_headings{};
    std::array<f32,3> rank{},turn_gains{},limits{}; //rank B8/1DD0/5F8; limits678/67C/680
    std::array<f32,5> dimensions{},dash_steering{}; //class44/48/4C/50/54; data120..130
    std::array<f32,12> air{}; //data44..70
    std::array<std::array<f32,4>,3> ground_dash{}; //speed/change/minimum/timer
    std::array<u32,4> held_dash_masks{};f32 dash_body_gain=0,dash_angle_factor=0,dash_near_delta=0;
    f32 zero=0,epsilon=0,special_min_vertical=0,angle_unit=0;bool loaded=false;
    f32 walk_angle_divisor=0,stop_damping_base=0,stop_damping_gain=0;bool walk_operands=false;
    std::array<f32,4> air_damping{};s16 ascent_pitch=0;u8 air_ready_delay=0;bool air_operands=false;
    f32 air_dash_windup_factor=0,air_dash_contact_delay=0;bool air_dash_operands=false;
    std::array<s16,2> landing_angles{};f32 contact_motion_epsilon=0;bool surface_operands=false;
    f32 steep_damping=0;bool steep_operands=false;
    f32 relaunch_floor_damping=0;bool relaunch_floor_operands=false;
    f32 ground_loss_zero=0,ground_loss_contact_delay=0;bool ground_loss_operands=false;
    f32 dash_recovery_damping=0;bool ordered_dash_operands=false;
};
inline bool character_movement_profile_valid(const CharacterMovementProfile& p){
    if(!p.loaded||!p.model_hash||p.zero!=0||!std::isfinite(p.epsilon)||p.epsilon<=0||p.epsilon>1||
       !std::isfinite(p.special_min_vertical)||p.angle_unit!=bits_to_f32(0x38c90fdbu)||
       !std::isfinite(p.dash_body_gain)||std::abs(p.dash_body_gain)>1||!std::isfinite(p.dash_angle_factor)||std::abs(p.dash_angle_factor)>65536||
       !std::isfinite(p.dash_near_delta)||std::abs(p.dash_near_delta)>1e5f)return false;
    auto numbers=[](const auto& values){for(auto value:values)if(!std::isfinite(value)||std::abs(value)>1e5f)return false;return true;};
    if(!numbers(p.walk_speeds)||!numbers(p.rank)||!numbers(p.dimensions)||!numbers(p.turn_gains)||!numbers(p.air)||!numbers(p.limits)||!numbers(p.dash_steering))return false;
    for(const auto& row:p.ground_dash)if(!numbers(row))return false;
    for(auto value:p.rank)if(value<=0)return false;
    for(auto value:p.dimensions)if(value<=0)return false;
    for(auto value:p.walk_speeds)if(value<0)return false;
    if(p.walk_operands&&(!std::isfinite(p.walk_angle_divisor)||p.walk_angle_divisor<=0||
       !std::isfinite(p.stop_damping_base)||std::abs(p.stop_damping_base)>2||
       !std::isfinite(p.stop_damping_gain)||std::abs(p.stop_damping_gain)>2))return false;
    if(p.air_operands&&(!p.walk_operands||!numbers(p.air_damping)||p.ascent_pitch<0||p.ascent_pitch>16384))return false;
    if(p.air_dash_operands&&(!p.air_operands||!std::isfinite(p.air_dash_windup_factor)||p.air_dash_windup_factor<0||p.air_dash_windup_factor>1||
       !std::isfinite(p.air_dash_contact_delay)||p.air_dash_contact_delay<0||p.air_dash_contact_delay>1e5f))return false;
    if(p.surface_operands&&(!p.air_dash_operands||!std::isfinite(p.contact_motion_epsilon)||p.contact_motion_epsilon<=0||p.contact_motion_epsilon>1))return false;
    if(p.surface_operands)for(auto angle:p.landing_angles)if(angle<0||angle>16384)return false;
    if(p.steep_operands&&(!p.surface_operands||!std::isfinite(p.steep_damping)||p.steep_damping<=0||p.steep_damping>1))return false;
    if(p.relaunch_floor_operands&&(!p.steep_operands||!std::isfinite(p.relaunch_floor_damping)||p.relaunch_floor_damping<=0||p.relaunch_floor_damping>1))return false;
    if(p.ground_loss_operands&&(!p.relaunch_floor_operands||p.ground_loss_zero!=0||
        !std::isfinite(p.ground_loss_contact_delay)||p.ground_loss_contact_delay<=0||p.ground_loss_contact_delay>1e5f))return false;
    if(p.ordered_dash_operands&&(!p.ground_loss_operands||!std::isfinite(p.dash_recovery_damping)||
       p.dash_recovery_damping<=0||p.dash_recovery_damping>1))return false;
    for(auto value:p.direction_classes)if(value>3)return false;
    return p.limits[0]>=0&&p.limits[2]<=p.limits[1];
}
inline bool character_movement_load(ByteView b,CharacterMovementProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](){if(error)*error="movement operands: qualified normal G-Red GFMM required";return false;};
    if(!out||!b.data||b.size<320||b.be32(0)!=0x47464d4du||b.be16(4)<1||b.be16(4)>9||
       b.size!=(b.be16(4)==9?376u:b.be16(4)==8?372u:b.be16(4)==7?364u:b.be16(4)==6?360u:b.be16(4)==5?356u:b.be16(4)==4?348u:b.be16(4)==3?340u:320u)||b.be16(6)!=0x615||b.be32(8)!=0xe2b4439au||b.be32(12)!=b.size)return fail();
    u32 hash=2166136261u;for(std::size_t i=0;i<b.size;++i)hash=(hash^b.data[i])*16777619u;
    if(hash!=(b.be16(4)==1?0x4e61230bu:b.be16(4)==2?0x0abec001u:b.be16(4)==3?0x474a31f2u:b.be16(4)==4?0x379246edu:b.be16(4)==5?0x001b5555u:b.be16(4)==6?0x91ebe47du:b.be16(4)==7?0x77c3d18eu:b.be16(4)==8?0x9525c46eu:0x409f4accu))return fail();
    CharacterMovementProfile p;p.model=b.be16(6);p.model_hash=b.be32(8);p.class_flags=b.be32(16);p.movement_type=b.data[20];
    for(unsigned i=0;i<6;++i)p.walk_speeds[i]=b.bef32(32+i*4);
    for(unsigned i=0;i<5;++i){p.turn_counts[i]=b.be16(56+i*2);p.dimensions[i]=b.bef32(104+i*4);p.dash_steering[i]=b.bef32(260+i*4);}
    for(unsigned i=0;i<8;++i){p.direction_classes[i]=b.data[66+i];p.cardinal_headings[i]=b.data[74+i];p.octant_headings[i]=b.data[82+i];}
    for(unsigned i=0;i<3;++i){p.rank[i]=b.bef32(92+i*4);p.turn_gains[i]=b.bef32(140+i*4);p.limits[i]=b.bef32(200+i*4);for(unsigned j=0;j<4;++j)p.ground_dash[i][j]=b.bef32(212+i*16+j*4);}
    for(unsigned i=0;i<12;++i)p.air[i]=b.bef32(152+i*4);
    p.zero=b.bef32(124);p.epsilon=b.bef32(128);p.special_min_vertical=b.bef32(132);p.angle_unit=b.bef32(136);p.loaded=true;
    for(unsigned i=0;i<4;++i)p.held_dash_masks[i]=b.be32(280+i*4);
    p.dash_body_gain=b.bef32(296);p.dash_angle_factor=b.bef32(300);p.dash_near_delta=b.bef32(304);
    if(b.be16(4)>=2){p.walk_operands=true;p.walk_angle_divisor=b.bef32(308);p.stop_damping_base=b.bef32(312);p.stop_damping_gain=b.bef32(316);}
    if(b.be16(4)>=3){p.air_operands=true;for(unsigned i=0;i<4;++i)p.air_damping[i]=b.bef32(320+i*4);p.ascent_pitch=std::bit_cast<s16>(b.be16(336));p.air_ready_delay=b.data[338];}
    if(b.be16(4)>=4){p.air_dash_operands=true;p.air_dash_windup_factor=b.bef32(340);p.air_dash_contact_delay=b.bef32(344);}
    if(b.be16(4)>=5){p.surface_operands=true;p.landing_angles={std::bit_cast<s16>(b.be16(348)),std::bit_cast<s16>(b.be16(350))};p.contact_motion_epsilon=b.bef32(352);}
    if(b.be16(4)>=6){p.steep_operands=true;p.steep_damping=b.bef32(356);}
    if(b.be16(4)>=7){p.relaunch_floor_operands=true;p.relaunch_floor_damping=b.bef32(360);}
    if(b.be16(4)>=8){p.ground_loss_operands=true;p.ground_loss_zero=b.bef32(364);p.ground_loss_contact_delay=b.bef32(368);}
    if(b.be16(4)>=9){p.ordered_dash_operands=true;p.dash_recovery_damping=b.bef32(372);}
    if(!character_movement_profile_valid(p)||p.movement_type||p.class_flags!=12)return fail();
    *out=p;if(error)error->clear();return true;
}
}
