#pragma once
#include <string>
#include "engine/core/types.hpp"
namespace gf {
struct ChallengeEntryProfile {
    u32 reveal_rgba=0,bgm_member=0;u16 reveal_updates=0,exit_updates=0;bool loaded=false;
    bool valid()const{return loaded&&reveal_updates&&reveal_updates<=32767&&exit_updates&&exit_updates<=32767&&bgm_member<4096;}
};
inline bool challenge_entry_profile_load(ByteView bytes,ChallengeEntryProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](){if(error)*error="Challenge entry: profile extent/domain";return false;};
    if(!out||!bytes.data||bytes.size!=32||bytes.be32(0)!=0x47464345||bytes.be32(4)!=1||bytes.be32(24)||bytes.be32(28))return fail();
    if(bytes.data[20]!=32||bytes.data[21]!=27||bytes.be16(22))return fail();
    ChallengeEntryProfile profile;profile.reveal_rgba=bytes.be32(8);profile.reveal_updates=bytes.be16(12);profile.exit_updates=bytes.be16(14);profile.bgm_member=bytes.be32(16);profile.loaded=true;
    if(!profile.valid())return fail();
    *out=profile;if(error)error->clear();return true;
}
} // namespace gf
