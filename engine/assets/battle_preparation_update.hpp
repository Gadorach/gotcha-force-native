#pragma once
#include "engine/core/types.hpp"
#include <array>
#include <string>
namespace gf {
struct BattlePreparationCall {u32 key=0,argument=0;u8 arguments=0;bool operator==(const BattlePreparationCall&)const=default;};
class BattlePreparationUpdateProfile {
public:
    bool load(ByteView b,std::string* error){
        *this={};auto fail=[&](){if(error)*error="battle preparation update: source recipe/table domain";return false;};
        if(!b.data||b.size!=272||b.be32(0)!=0x47465055||b.be32(4)!=1||b.be32(8)!=272||b.data[12]!=3||b.data[13]!=12||b.data[14]!=7||b.data[15]!=1||b.be16(16)!=32||b.be16(18)||b.be16(20)!=0xac||b.be16(22)!=0xb0||b.be32(24)!=0x8002844c||b.be32(28)!=0x800295e0)return fail();
        const std::array<u32,3> handlers{0x80029648,0x80029688,0x800296e0};for(unsigned i=0;i<3;++i)if(b.be32(32+4*i)!=handlers[i])return fail();
        const std::array<BattlePreparationCall,19> expected{{{0x80089a94,40,1},{0x80089898,57,1},{0x80087cf0,0,0},{0x80089554,0,0},{0x8008a28c,0,0},{0x80089aec,0,0},{0x8002bb14,0,0},{0x80036e24,0,0},{0x8008972c,0,0},{0x80089658,0,0},{0x80089898,59,1},{0x80040b94,0,0},{0x8008a16c,0,0},{0x80089cd0,40,1},{0x80089c00,57,1},{0x80087814,0,0},{0x8008c0dc,0x80089eb4,1},{0x8008c104,0x80089f28,1},{0x8008c12c,0x80089f9c,1}}};
        for(unsigned i=0;i<19;++i){const unsigned at=44+12*i;BattlePreparationCall row{b.be32(at),b.be32(at+8),b.data[at+4]};if(b.data[at+5]||b.be16(at+6)||row!=expected[i])return fail();calls_[i]=row;}
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}u8 initial_scene45()const{return ready_?1:0;}u16 fade_updates()const{return ready_?32:0;}
    const BattlePreparationCall* call(unsigned index)const{return ready_&&index<19?&calls_[index]:nullptr;}
private:std::array<BattlePreparationCall,19> calls_{};bool ready_=false;
};
} // namespace gf
