#pragma once
//GFMI1: qualified normal G-Red input tables/resource operands. Not animation,
//input-device mapping, collision, physics or actor/frame readiness.
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
struct CharacterMovementCommand {
    u16 kind=0,flags=0,window=0,pulse=0;std::array<u16,2> masks{};
};
struct CharacterMovementInputProfile {
    u16 model=0;u32 model_hash=0,held_mask=0;
    u8 movement_type=0,resource_policy=0,raw_resource_policy=0;
    u8 low_threshold=0,high_threshold=0,axis_window=0,other_window=0,direction_window=0,release_window=0;
    std::array<u8,8> octants{};std::array<s8,16> nibble_indices{};
    std::array<CharacterMovementCommand,10> commands{};
    s16 resource_limit=0,recovery_delay=0,recovery_increment=0;std::array<s16,4> costs{};
    bool loaded=false;
};
inline bool character_movement_input_profile_valid(const CharacterMovementInputProfile& p){
    if(!p.loaded||!p.model_hash||!p.held_mask||!p.low_threshold||p.high_threshold<p.low_threshold||p.high_threshold>127||
       !p.axis_window||p.axis_window>127||!p.other_window||p.other_window>127||
       !p.direction_window||p.direction_window>127||!p.release_window||p.release_window>127)return false;
    for(auto value:p.octants)if(!value||value>15)return false;
    for(auto value:p.nibble_indices)if(value< -1||value>8)return false;
    for(const auto& c:p.commands)if(c.kind!=1||(c.flags&~31u)||c.window>127||!c.pulse||c.pulse>127||!c.masks[0]||c.masks[1])return false;
    if(p.resource_limit<1||p.recovery_delay<0||p.recovery_increment<0)return false;
    for(auto cost:p.costs)if(cost<0)return false;
    return true;
}
inline bool character_movement_input_load(ByteView b,CharacterMovementInputProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](){if(error)*error="movement input: qualified normal G-Red GFMI profile required";return false;};
    if(!out||!b.data||b.size!=192||b.be32(0)!=0x47464d49u||b.be16(4)!=1||b.be16(6)!=0x615||
       b.be32(8)!=0xe2b4439au||b.be32(12)!=192||b.data[19]||b.data[26]!=10||b.data[27]||b.be16(190))return fail();
    u32 hash=2166136261u;for(std::size_t i=0;i<b.size;++i)hash=(hash^b.data[i])*16777619u;
    if(hash!=0x9e40085cu)return fail(); //reviewed source revision/tables/normal resource payload
    CharacterMovementInputProfile p;p.model=b.be16(6);p.model_hash=b.be32(8);p.held_mask=b.be32(28);
    p.movement_type=b.data[16];p.resource_policy=b.data[17];p.raw_resource_policy=b.data[18];
    p.low_threshold=b.data[20];p.high_threshold=b.data[21];p.axis_window=b.data[22];p.other_window=b.data[23];p.direction_window=b.data[24];p.release_window=b.data[25];
    for(unsigned i=0;i<8;++i)p.octants[i]=b.data[32+i];
    for(unsigned i=0;i<16;++i)p.nibble_indices[i]=std::bit_cast<s8>(b.data[40+i]);
    for(unsigned i=0;i<10;++i){const auto at=56+i*12;auto& c=p.commands[i];c.kind=b.be16(at);c.flags=b.be16(at+2);c.window=b.be16(at+4);c.pulse=b.be16(at+6);c.masks={b.be16(at+8),b.be16(at+10)};}
    p.resource_limit=std::bit_cast<s16>(b.be16(176));p.recovery_delay=std::bit_cast<s16>(b.be16(178));p.recovery_increment=std::bit_cast<s16>(b.be16(180));
    for(unsigned i=0;i<4;++i)p.costs[i]=std::bit_cast<s16>(b.be16(182+i*2));
    p.loaded=true;if(!character_movement_input_profile_valid(p)||p.movement_type||p.resource_policy!=1)return fail();
    *out=p;if(error)error->clear();return true;
}
}
