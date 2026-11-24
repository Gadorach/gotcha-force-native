#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
//Player2's E99C8/E9914 constants, OPTION gain row and E91D4 command lengths.
//A request profile does not identify an ADX stream or a playing backend handle.
class MusicRequestProfile {
public:
    bool load(ByteView b,std::string* error){
        *this={};auto fail=[&](){if(error)*error="music request profile: source recipe";return false;};
        if(!b.data||b.size!=64||b.be32(0)!=0x47464d51||b.be32(4)!=1||b.be32(8)!=64||b.be32(12)!=64||b.be32(16)!=4||b.be32(20)!=2||b.be32(24)!=7||b.be32(28)!=0x42fe0000||b.be32(32)||b.be32(56)!=0x2c||b.be32(60))return fail();
        constexpr std::array<u32,5> bits{0x3f19999a,0x3f333333,0x3f4ccccd,0x3f666666,0x3f800000};
        for(unsigned i=0;i<5;++i){if(b.be32(36+4*i)!=bits[i])return fail();gains_[i]=b.bef32(36+4*i);}
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
    f32 gain(unsigned index)const{return ready_&&index<5?gains_[index]:0;}
    s32 battle_member()const{return ready_?7:-1;}
private:std::array<f32,5> gains_{};bool ready_=false;
};
} // namespace gf
