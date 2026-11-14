#pragma once
#include "engine/core/types.hpp"
#include <string>
namespace gf {
struct ChallengeSetupPagesProfile {
    u16 players_open=0,players_close=0,box_open=0,load_delay=0,box_close=0;
    bool valid()const{return players_open&&players_open<=1024&&players_close&&players_close<=1024&&box_open&&box_open<=1024&&load_delay&&load_delay<=1024&&box_close&&box_close<=1024;}
};
inline bool challenge_setup_pages_load(ByteView bytes,ChallengeSetupPagesProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](){if(error)*error="Challenge setup pages: profile extent/domain";return false;};
    if(!out||!bytes.data||bytes.size!=32||bytes.be32(0)!=0x47465053||bytes.be32(4)!=1)return fail();
    for(unsigned i=18;i<32;++i)if(bytes.data[i])return fail();
    ChallengeSetupPagesProfile p{bytes.be16(8),bytes.be16(10),bytes.be16(12),bytes.be16(14),bytes.be16(16)};
    if(!p.valid())return fail();
    *out=p;if(error)error->clear();return true;
}
} // namespace gf
