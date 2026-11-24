#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
class BattleInitialWorldDispatchProfile {
public:
    static constexpr std::array<u8,10> source_lists{0,1,2,3,6,7,8,9,10,11};
    bool load(ByteView b,std::string* error){
        ready_=false;auto fail=[&](){if(error)*error="initial world dispatch: exact89554 list order/first-frame recipe required";return false;};
        const std::array<u32,40> words{
            0x47465749,1,160,0,0x80089554,0x80029434,0x80047aa4,0x80082060,
            0x8013c144,0x8001c5ac,0x80089658,0x8008972c,10,12,83,1,
            0,1,2,3,6,7,8,9,10,11,
            0x80436290,0x80436294,0x80436280,0x80436284,0x803c6f54,0x803c6f58,
            0x803c6f5c,0x803c6f60,0x803c6f64,0x803c6f68,4,5,0x54,0x1e};
        if(!b.contains(0,160)||b.size!=160)return fail();
        for(unsigned i=0;i<words.size();++i)if(b.be32(4*i)!=words[i])return fail();
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
private:bool ready_=false;
};
}
