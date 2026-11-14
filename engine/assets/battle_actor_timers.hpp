#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
//Bounded table slices for21B9C/22744/227B0/23234/232A0. The eighteen signed-byte
//selectors and sixteen rows are native safe extents, not proof of all callers.
class BattleActorTimerProfile {
public:
    bool load(ByteView b,std::string* error){
        *this={};auto fail=[&](){if(error)*error="battle actor timer profile: extent/version/domain";return false;};
        if(!b.data||b.size!=23072||b.be32(0)!=0x47464254u||b.be32(4)!=1||b.be32(8)!=5||b.be32(12)!=18||b.be32(16)!=16||b.be32(20)!=8||b.be32(24)!=23040||b.be32(28))return fail();
        for(unsigned i=0;i<values_.size();++i)values_[i]=std::bit_cast<s16>(b.be16(32+2*i));
        loaded_=true;if(error)error->clear();return true;
    }
    bool ready()const{return loaded_;}
    bool value(unsigned table,unsigned selector,unsigned level,unsigned choice,s16* out)const{
        if(!out||!loaded_||table>=5||selector>=18||level>=16||choice>=8)return false;
        *out=values_[((table*18+selector)*16+level)*8+choice];return true;
    }
private:std::array<s16,5*18*16*8> values_{};bool loaded_=false;
};
} // namespace gf
