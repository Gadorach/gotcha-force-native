#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
struct ChallengeStageRngRow {u8 stage=0;u16 field_1e_count=0;};
//80196DAC/80087F24: private offline-exported rows. The two parameter names
//retain source offsets; this is not stage geometry/collision/music admission.
struct ChallengeStageRngProfile {
    std::array<ChallengeStageRngRow,11> rows{};
    bool valid()const{
        for(unsigned i=0;i<rows.size();++i){
            if(rows[i].stage>127||!rows[i].field_1e_count||rows[i].field_1e_count>256)return false;
            for(unsigned j=0;j<i;++j)if(rows[i].stage==rows[j].stage)return false;
        }
        return true;
    }
};
inline bool challenge_stage_rng_load(ByteView b,ChallengeStageRngProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](){if(error)*error="Challenge stage RNG: profile extent/domain";return false;};
    if(!out||!b.data||b.size!=64||b.be32(0)!=0x47464347||b.be32(4)!=1||b.be16(8)!=11||b.be16(10)!=3)return fail();
    for(unsigned i=56;i<64;++i)if(b.data[i])return fail();
    ChallengeStageRngProfile p;
    for(unsigned i=0;i<p.rows.size();++i){const unsigned at=12+4*i;if(b.data[at+1])return fail();p.rows[i]={b.data[at],b.be16(at+2)};}
    if(!p.valid())return fail();
    *out=p;if(error)error->clear();return true;
}
} // namespace gf
