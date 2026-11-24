#pragma once
#include "engine/core/types.hpp"
#include <array>
#include <string>
namespace gf {
//Only the first547DC entry,742=0/741=1, before appearance-completion effects.
class BattleActorSpawnEntryProfile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;auto fail=[&](){if(error)*error="battle spawn entry: source caller/operand recipe";return false;};
        if(!b.contains(0,80)||b.size!=80||b.be32(0)!=0x47465356||b.be32(4)!=1||b.be32(8)!=80||b.be32(12))return fail();
        const std::array<u32,16> words{0x80054070,0x8005475c,0x802d3274,0x800547dc,0x800061a8,0x80045204,0x800465fc,0x80006284,0x41a00000,0x3dcccccd,0x3f666666,0x46800000,0x41980000,0x3f800000,0x802c347c,0};
        for(unsigned i=0;i<words.size();++i)if(b.be32(16+4*i)!=words[i])return fail();
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
    f32 duration()const{return 20;}f32 narrow()const{return bits_to_f32(0x3dcccccd);}f32 gain()const{return bits_to_f32(0x3f666666);}f32 angle_scale()const{return 16384;}f32 vertical_factor()const{return 19;}
    u32 colour_key()const{return 0x802c347c;}
private:bool ready_=false;
};
}
