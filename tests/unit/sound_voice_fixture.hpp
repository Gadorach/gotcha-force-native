#pragma once
#include "engine/gameplay/sound_voice_allocate.hpp"
namespace sound_voice_fixture {
struct Bank {
    std::vector<gf::u8> tones,header,payload;
    explicit Bank(unsigned regions=2){using namespace gf;tones.resize(32);std::memcpy(tones.data(),"TSBD",4);auto little=[&](unsigned at,u32 value){for(unsigned k=0;k<4;++k)tones[at+k]=u8(value>>(8*k));};little(4,32);little(8,256);little(12,1);tones[19]=36;tones[31]=255;const unsigned samples=std::max(1u,regions),prog_size=28+regions*14,sample_at=32+prog_size,dsp_at=sample_at+16+samples*12;header.resize(dsp_at+116);auto word=[&](unsigned at,u32 value){store_be32(header.data()+at,value);};std::memcpy(header.data(),"Head",4);word(4,32);word(8,1);word(12,u32(header.size()));word(16,8);word(20,32);word(24,sample_at);word(28,dsp_at);
        auto chunk=[&](unsigned at,const char* tag,unsigned size,unsigned count){std::memcpy(header.data()+at,tag,4);word(at+4,size);word(at+8,count-1);word(at+12,0xffffffff);};chunk(32,"Prog",prog_size,1);chunk(sample_at,"Smpl",16+samples*12,samples);chunk(dsp_at,"Dspi",116,1);word(48,20);header[52]=u8(regions);header[53]=255;header[54]=127;header[55]=64;
        for(unsigned i=0;i<regions;++i){const unsigned r=60+i*14;header[r]=15;header[r+1]=255;header[r+2]=header[r+3]=36;store_be16(header.data()+r+4,200);store_be16(header.data()+r+6,300);header[r+8]=127;header[r+9]=64;store_be16(header.data()+r+12,u16(i));}
        for(unsigned i=0;i<samples;++i){const unsigned s=sample_at+16+i*12;word(s,0xfe07e03f);header[s+5]=u8(36+i);header[s+6]=127;header[s+7]=64;}
        const unsigned h=dsp_at+20;word(h,14);word(h+4,16);word(h+8,22050);word(h+16,2);word(h+20,15);word(h+24,2);payload.assign(8,0x11);payload[0]=0;
    }
    gf::SoundVoiceBankInput input(gf::u8 id=0)const{return {id,{tones.data(),tones.size()},{header.data(),header.size()},{payload.data(),payload.size()}};}
};
inline std::vector<gf::u8> profile(){using namespace gf;std::vector<u8> b(56);store_be32(b.data(),0x47465649);store_be16(b.data()+4,1);store_be16(b.data()+6,56);const std::array<f32,6> values={65536,32000,127,255,8191,8192};for(unsigned i=0;i<6;++i)store_be_f32(b.data()+8+i*4,values[i]);store_be32(b.data()+32,100);store_be32(b.data()+36,32767);store_be32(b.data()+40,0x40000000);store_be32(b.data()+48,0x4092c000);return b;}
inline gf::StartupSoundEventPool event_pool(){auto p=gf::startup_sound_event_pool_fresh();--p.free_count;p.head=p.tail=0;auto& e=p.events[0];e.allocated=true;e.current04=e.requested08=0x60;e.program38=0;e.key39=36;e.volume28=127;e.pan29=64;e.pitch2a=8192;e.dirty30=0xffff;return p;}
inline void add_event(gf::StartupSoundEventPool& p){using namespace gf;const auto id=p.free[--p.free_count];auto& e=p.events[id];e.allocated=true;e.previous0e=p.tail;p.events[unsigned(p.tail)].next12=s16(id);p.tail=s16(id);e.current04=e.requested08=0x60;e.key39=36;e.volume28=127;e.pan29=64;e.pitch2a=8192;e.dirty30=0xffff;}
} // namespace sound_voice_fixture
