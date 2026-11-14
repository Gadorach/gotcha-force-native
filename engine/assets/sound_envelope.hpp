#pragma once
// EF390's shared envelope-rate lookup. Loose private export, no runtime DOL.
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
class SoundEnvelopeProfile {
public:
    bool load(ByteView bytes,std::string* error){
        ready_=false;rates_={};
        auto fail=[&](const char* why){if(error)*error=std::string("sound envelope profile: ")+why;return false;};
        if(!bytes.data||bytes.size!=528||bytes.be32(0)!=0x47465645u||bytes.be16(4)!=1||bytes.be16(6)!=528||bytes.be32(8)!=0x4d000000u||bytes.be32(12)!=0x3c800000u)return fail("GFVE header/source scalar operands");
        std::array<u32,128> rates;
        for(unsigned i=0;i<rates.size();++i){rates[i]=bytes.be32(16+i*4);if(rates[i]>0x08000000u)return fail("rate exceeds bounded source arithmetic domain");}
        rates_=rates;ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
    u32 rate(unsigned index)const{return ready_&&index<rates_.size()?rates_[index]:0;}
private:
    std::array<u32,128> rates_{};bool ready_=false;
};
} // namespace gf
