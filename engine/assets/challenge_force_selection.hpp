#pragma once
#include "engine/core/types.hpp"
#include <string>
namespace gf {
struct ChallengeForceSelectionProfile {u16 opening=0,closing=0;bool valid()const{return opening&&opening<=1024&&closing&&closing<=1024;}};
inline bool challenge_force_selection_load(ByteView b,ChallengeForceSelectionProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](){if(error)*error="Challenge Force Selection: profile extent/domain";return false;};
    if(!out||!b.data||b.size!=32||b.be32(0)!=0x47464653||b.be32(4)!=1||b.be16(12)!=1||b.be16(14)!=2||b.be16(16)!=0x200||b.be16(18)!=0x400||b.be16(20)!=0x180)return fail();
    for(unsigned i=22;i<32;++i)if(b.data[i])return fail();
    ChallengeForceSelectionProfile p{b.be16(8),b.be16(10)};if(!p.valid())return fail();*out=p;if(error)error->clear();return true;
}
} // namespace gf
