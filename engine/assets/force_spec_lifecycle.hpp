#pragma once
#include <array>
#include <cmath>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
struct ForceSpecLifecycleProfile {
    u32 bank_hash=0;std::array<f32,64> weights{};f32 total=0,start_z=0,distance=0;u16 fade_callbacks=0,release_callbacks=0,slide_callbacks=0;
    bool valid()const{
        if(!bank_hash||start_z!=-200||distance!=200||fade_callbacks!=16||release_callbacks!=4||slide_callbacks!=16)return false;
        f32 sum=0;for(auto w:weights){if(!std::isfinite(w)||w<=0||w>1e6f)return false;sum=f32(sum+w);}
        return std::isfinite(sum)&&sum>0&&sum==total;
    }
    bool load(ByteView b,u32 expected_bank,std::string* error){
        *this={};auto fail=[&](){*this={};if(error)*error="spec lifecycle: GFSL extent/source binding";return false;};
        if(!b.data||b.size!=284||b.be32(0)!=0x4746534cu||b.be32(4)!=1||b.be32(8)!=expected_bank||b.be16(282))return fail();
        bank_hash=expected_bank;for(unsigned i=0;i<64;++i){weights[i]=b.bef32(12+i*4);total=f32(total+weights[i]);}
        start_z=b.bef32(268);distance=b.bef32(272);fade_callbacks=b.be16(276);release_callbacks=b.be16(278);slide_callbacks=b.be16(280);
        if(!valid())return fail();
        if(error)error->clear();
        return true;
    }
};
} // namespace gf
