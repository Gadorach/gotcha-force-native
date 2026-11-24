#pragma once
#include "engine/core/types.hpp"
#include <string>
#include "engine/assets/battle_model_domain.hpp"
namespace gf {
//Two qualified normal4F0 tables, selector0 ONLY. No arbitrary AI/control table,
//program bytes, motion request or runtime class-state authority.
class BattleActorInitialControlsProfile {
public:
    bool load(ByteView b,std::string* error){
        *this={};auto fail=[&](){if(error)*error="battle initial controls: source class/table/selector extent";return false;};
        if(!b.contains(0,32)||b.size!=32||b.be32(0)!=0x47464943||(b.be32(4)!=1&&b.be32(4)!=2)||b.be32(8)!=32||!b.be32(12)||!battle_model_in_domain(BattleModelDomain(b.be32(4)),b.be16(20))||b.be16(22)||b.data[24]!=1||b.data[25]!=1)return fail();
        const auto model=b.be16(20);if(b.be32(16)!=(model==10?0x802d3ff8u:model==0x615?0x80365cf8u:model==0x70b?0x80373a38u:0x8036fe80u))return fail();
        for(unsigned i=26;i<32;++i)if(b.data[i])return fail();
        model_=model;hash_=b.be32(12);key_=b.be32(16);ready_=true;if(error)error->clear();return true;
    }
    bool matches(u16 model,u32 hash,u32 key)const{return ready_&&model==model_&&hash==hash_&&key==key_;}
    u8 primary()const{return ready_?1:0;}u8 secondary()const{return ready_?1:0;}
private:u16 model_=0;u32 hash_=0,key_=0;bool ready_=false;
};
}
