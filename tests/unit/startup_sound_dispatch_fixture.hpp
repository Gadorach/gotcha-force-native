#pragma once
#include "engine/gameplay/startup_sound_dispatch.hpp"
namespace startup_sound_dispatch_fixture {
inline gf::StartupSoundPacket packet(gf::u8 opcode=0x60,gf::u8 bank=0,gf::u8 tone=40,gf::u8 group=5,gf::u8 flags=1,gf::u8 priority=100){
    using namespace gf;StartupSoundPacket p;p.known=0x3fff;
    const u32 word=u32(opcode)<<24|u32(bank&127u)<<16|u32(tone&127u)<<8|u32(group&127u)|((flags&1u)?128u:0u)|((flags&2u)?0x8000u:0u)|((flags&4u)?0x800000u:0u);
    store_be32(p.bytes.data(),word);p.bytes[4]=1;p.bytes[5]=27;p.bytes[8]=90;p.bytes[9]=64;store_be16(p.bytes.data()+10,8192);p.bytes[13]=priority;return p;
}
inline gf::StartupSoundRing ring(std::span<const gf::StartupSoundPacket> packets,unsigned start=0){
    gf::StartupSoundRing r;r.read=r.write=start;for(const auto& p:packets){r.packets[r.write]=p;r.write=(r.write+1u)%48u;}return r;
}
inline void mask(gf::StartupSoundPacket& p,gf::u16 known){p.known=known;for(unsigned i=0;i<16;++i)if(!(known&(1u<<i)))p.bytes[i]=0;}
}
