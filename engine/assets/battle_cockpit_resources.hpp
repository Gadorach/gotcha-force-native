#pragma once
#include "engine/core/types.hpp"
#include <array>
#include <string>
namespace gf {
struct BattleCockpitResourceRow {u32 mode=0,member=0,bytes=0,hash=0;};
//42520 selects ckpt00/01/02. Mode3 aliases row1 in the source; the current
//Challenge producer supplies only0/1. This metadata is byte identity, not HUD UI.
class BattleCockpitResourceProfile {
public:
    bool load(ByteView b,std::string* error){
        *this={};auto fail=[&](){if(error)*error="battle cockpit profile: header/selection/extent";return false;};
        if(!b.data||b.size!=64||b.be32(0)!=0x47464350||b.be32(4)!=1||b.be32(8)!=3||b.be32(12)!=48)return fail();
        for(unsigned i=0;i<3;++i){const unsigned at=16+16*i;auto& r=rows_[i];r={b.be32(at),b.be32(at+4),b.be32(at+8),b.be32(at+12)};
            if(r.mode!=i||r.member!=2707+i||r.bytes<32||r.bytes>(4u<<20)||!r.hash)return fail();}
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
    const BattleCockpitResourceRow* find(unsigned mode)const{return ready_&&mode<4?&rows_[mode==3?1:mode]:nullptr;}
private:std::array<BattleCockpitResourceRow,3> rows_{};bool ready_=false;
};
} // namespace gf
