#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
struct BattleStageSideType {u16 hp=0,collision=0,duration=0;s8 cull=0;f32 radius=0;u32 program_key=0;};
//Bounded stage2/type4/type5 constructor operands. Source programs are keys only;
//this profile does not admit their later update or destruction consumers.
class BattleStageSideProfile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;auto fail=[&](){if(error)*error="stage side profile: source stage2 constructor operands required";return false;};
        if(!b.contains(0,100)||b.size!=100||b.be32(0)!=0x47465349||b.be32(4)!=1||b.be32(8)!=100||b.be32(12))return fail();
        const std::array<u32,11> header{2,0x80087d2c,0x800818d0,0x80081980,0x800819b4,0x80081b9c,0x80081d20,0x4622f983,0x3f800000,0,0x47435000};
        for(unsigned i=0;i<header.size();++i)if(b.be32(16+4*i)!=header[i])return fail();
        const std::array<u32,10> rows{0x006400ab,0,0x444064a4,0x802d7480,0x02580000,0x006400ac,0xff000000,0x43d8748f,0x802d7540,0x02580000};
        for(unsigned i=0;i<rows.size();++i)if(b.be32(60+4*i)!=rows[i])return fail();
        for(unsigned i=0;i<2;++i){const auto at=60+20*i;types_[i]={b.be16(at),b.be16(at+2),b.be16(at+16),std::bit_cast<s8>(b.data[at+4]),b.bef32(at+8),b.be32(at+12)};}
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
    const BattleStageSideType* type(unsigned type)const{return ready_&&type>=4&&type<=5?&types_[type-4]:nullptr;}
private:std::array<BattleStageSideType,2> types_{};bool ready_=false;
};
}
