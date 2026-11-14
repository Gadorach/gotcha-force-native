#pragma once
#include "engine/gameplay/sound_voice_lifecycle.hpp"
#include "tests/unit/sound_voice_fixture.hpp"
#include "tests/unit/sound_envelope_fixture.hpp"
namespace sound_voice_lifecycle_fixture {
inline gf::StartupSoundRing ring(unsigned events=1,gf::u16 countdown=0){
    using namespace gf;StartupSoundRing ring;ring.write=events;
    for(unsigned i=0;i<events;++i){auto& p=ring.packets[i];p.known=0x3fff;store_be32(p.bytes.data(),0x60000000u|i);p.bytes[5]=36;store_be16(p.bytes.data()+6,countdown);p.bytes[8]=127;p.bytes[9]=64;store_be16(p.bytes.data()+10,8192);p.bytes[13]=15;}
    return ring;
}
inline gf::SoundVoiceLifecycleAcquisition acquire(gf::u16 event,gf::u16 region,gf::u8 slot,gf::u32 token,gf::u64 event_generation=1,gf::u64 pass=1,gf::s16 stolen=-1,gf::u64 stolen_generation=0){return {pass,event_generation,{event,region,slot,15,token!=0,token,stolen},stolen_generation};}
inline std::vector<gf::SoundVoiceLifecycleBackendState> observations(const gf::SoundVoiceLifecycleState& s,gf::u16 backend=1){
    using namespace gf;std::vector<SoundVoiceLifecycleBackendState> result;
    for(s16 e=s.events.head;e!=-1;e=s.events.events[unsigned(e)].next12)if(!s.events.events[unsigned(e)].busy03&&!s.events.events[unsigned(e)].flag01)
        for(s16 id=s.events.events[unsigned(e)].child14;id!=-1;id=s.voices.voices[unsigned(id)].next)if(!s.voices.voices[unsigned(id)].stolen25&&s.envelopes[unsigned(id)].phase<5)result.push_back({s.callback+1,sound_voice_lifecycle_identity(s,u16(id)),backend});
    return result;
}
} // namespace sound_voice_lifecycle_fixture
