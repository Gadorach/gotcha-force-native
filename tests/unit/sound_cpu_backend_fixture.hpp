#pragma once
#include "engine/gameplay/sound_cpu_backend.hpp"
#include "tests/unit/sound_voice_parameter_fixture.hpp"
namespace sound_cpu_backend_fixture {
inline sound_voice_fixture::Bank bank(bool loop=false){auto b=sound_voice_fixture::Bank();const auto h=gf::load_be32(b.header.data()+28)+20;gf::store_be32(b.header.data()+h+8,32000);gf::store_be16(b.header.data()+h+62,12);b.payload[0]=12;if(loop){gf::store_be16(b.header.data()+h+12,1);gf::store_be16(b.header.data()+h+68,12);}return b;}
inline gf::SoundVoiceLifecycleIdentity owner(unsigned slot,gf::u64 generation){return {0,gf::u16(slot%64),1,generation,0};}
inline gf::SoundVoiceParameterPacket packet(const gf::SoundCpuLease& lease,gf::u64 callback,bool start=true){gf::SoundVoiceParameterPacket p;p.identity=lease.identity;p.callback=callback;p.publish=true;p.start=start;p.pitch_write=true;p.ve_volume=32767;p.mix[0]=p.mix[2]=32768;p.src_q16=65536;return p;}
} // namespace sound_cpu_backend_fixture
