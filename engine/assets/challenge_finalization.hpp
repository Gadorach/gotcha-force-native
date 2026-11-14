#pragma once
#include <array>
#include <vector>
#include <string>
#include <algorithm>
#include "engine/core/types.hpp"
namespace gf {
struct ChallengeStagePayloadSpec {u32 id=0xffffffffu,size=0,hash=0;};
struct ChallengeStagePayloadRow {u8 stage=0,field_1d=0,field_1e=0;std::array<ChallengeStagePayloadSpec,2> payloads;};
//Offline source table -> bounded, already-decoded native payload bundles. A data
//receipt is NOT stage geometry/collision/actor/DMA readiness or file authenticity.
class ChallengeFinalizationProfile {
public:
    bool load(ByteView b,std::string* error){
        *this={};auto fail=[&](){if(error)*error="Challenge finalization: profile extent/domain";return false;};
        if(!b.data||b.size<32||b.size>32+132*32||b.be32(0)!=0x47464342||b.be32(4)!=1||!b.be16(8)||b.be16(8)>132||b.be16(10)!=32||b.be32(12)!=b.size-32||b.size!=32+std::size_t(b.be16(8))*32||!b.be16(16)||b.be16(16)>1024||!b.be16(18)||b.be16(18)>32767)return fail();
        for(unsigned i=20;i<32;++i)if(b.data[i])return fail();
        std::vector<ChallengeStagePayloadRow> rows;
        for(unsigned i=0;i<b.be16(8);++i){const unsigned at=32+i*32;ChallengeStagePayloadRow row;row.stage=b.data[at];row.field_1d=b.data[at+1];row.field_1e=b.data[at+2];if(b.data[at+3]||b.be32(at+28)||row.stage>127||row.field_1d>=3||row.field_1e>16)return fail();
            for(unsigned j=0;j<2;++j){const unsigned p=at+4+12*j;row.payloads[j]={b.be32(p),b.be32(p+4),b.be32(p+8)};const auto& x=row.payloads[j];if(j==1&&x.id==0xffffffffu){if(x.size||x.hash)return fail();}else if(x.id>65535||x.size<24||x.size>(32u<<20))return fail();}
            if(std::any_of(rows.begin(),rows.end(),[&](const auto& x){return x.stage==row.stage&&x.field_1d==row.field_1d&&x.field_1e==row.field_1e;}))return fail();
            rows.push_back(row);
        }
        rows_=std::move(rows);wait_=b.be16(16);cover_=b.be16(18);if(error)error->clear();return true;
    }
    bool ready()const{return !rows_.empty();}u16 wait()const{return wait_;}u16 cover()const{return cover_;}
    const ChallengeStagePayloadRow* find(u8 stage,u8 a,u8 b)const{const auto x=std::find_if(rows_.begin(),rows_.end(),[&](const auto& row){return row.stage==stage&&row.field_1d==a&&row.field_1e==b;});return x==rows_.end()?nullptr:&*x;}
private:
    std::vector<ChallengeStagePayloadRow> rows_;u16 wait_=0,cover_=0;
};
inline u32 challenge_payload_hash(ByteView b){u32 h=2166136261u;for(std::size_t i=0;i<b.size;++i)h=(h^b.data[i])*16777619u;return h;}
inline bool challenge_native_bundle_valid(ByteView b){
    if(!b.data||b.size<24||b.size>(32u<<20)||b.be32(0)!=0x47464e42||b.be32(4)!=1||!b.be32(8)||b.be32(8)>511||b.be32(12)!=b.size-16||!b.contains(16,std::size_t(b.be32(8))*8))return false;
    std::size_t at=16+std::size_t(b.be32(8))*8;for(unsigned i=0;i<b.be32(8);++i){const auto n=b.be32(20+8*i);if(b.be32(16+8*i)!=i||!b.contains(at,n))return false;at+=n;}return at==b.size;
}
} // namespace gf
