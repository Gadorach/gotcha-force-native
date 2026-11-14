#pragma once
#include <fstream>
#include <iterator>
#include <map>
#include <functional>
#include "engine/gameplay/scene_audio_pcm_source.hpp"
#include "tests/unit/adx_fixture.hpp"
#include "tests/unit/sound_pcm_boundary_fixture.hpp"
namespace guide_voice_pcm_fixture {
using namespace gf;
inline u32 hash(std::span<const u8> b){u32 h=2166136261;for(auto x:b)h=(h^x)*16777619u;return h;}
inline ByteView view(const std::vector<u8>& b){return {b.data(),b.size()};}
inline std::vector<u8> read(const std::string& p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("voice PCM DATA read");return {std::istreambuf_iterator<char>(f),{}};}
struct Files final:IFileSystem {std::map<std::string,std::vector<u8>> data;unsigned whole=0,writes=0;bool read(const std::string&,std::vector<u8>&)override{++whole;return false;}bool file_size(const std::string& p,u64* out)override{if(!out||!data.contains(p))return false;*out=data.at(p).size();return true;}bool read_range(const std::string& p,u64 at,std::size_t n,std::vector<u8>& out)override{if(!data.contains(p)||at||n!=data.at(p).size())return false;out=data.at(p);return true;}bool write(const std::string&,const u8*,std::size_t)override{++writes;return false;}bool exists(const std::string& p)override{return data.contains(p);}bool erase(const std::string&)override{++writes;return false;}};
inline StoryGuideVoiceLibrary voices(unsigned frames=64){Files files;files.data["60"]=owned_adx::file(frames,1);files.data["61"]=owned_adx::file(frames+32,1);std::vector<u8> profile(80);store_be32(profile.data(),0x47464756);store_be32(profile.data()+4,1);store_be16(profile.data()+8,14);store_be16(profile.data()+10,2);for(unsigned i=0;i<2;++i){auto& data=files.data[i?"61":"60"];for(std::size_t at=64;at<data.size();at+=18)store_be16(data.data()+at,1000);const u32 values[]={60+i,1863+i,u32(data.size()),hash(data),1,8000,frames+i*32,0};for(unsigned k=0;k<8;++k)store_be32(profile.data()+16+i*32+k*4,values[k]);}StoryGuideVoiceLibrary library;if(!library.load(view(profile),{"60","61"},files,nullptr))throw std::runtime_error("owned mono library");return library;}
inline SourceAdxSdkGainProfile gain(){std::vector<u8> b(2462);store_be32(b.data(),0x47464147);store_be32(b.data()+4,1);store_be16(b.data()+8,963);store_be16(b.data()+10,128);store_be32(b.data()+12,u32(-903));store_be32(b.data()+16,60);store_be16(b.data()+20,0xff64);for(unsigned i=0;i<963;++i)store_be16(b.data()+24+i*2,u16(i<=903?1+32766*i/903:32767+(0xff64-32767)*(i-903)/60));for(unsigned i=0;i<128;++i)store_be32(b.data()+24+1926+i*4,u32(i<=63?-s32(i*30/63):i==64?-30:-30-s32((i-64)*874/63)));SourceAdxSdkGainProfile p;if(!p.load(view(b)))throw std::runtime_error("owned gain");return p;}
inline AdxAudio music_clip(s16 amplitude){AdxAudio a;a.rate=48000;a.channels=2;a.frames=64;a.loop=true;a.loop_start=16;a.loop_end=64;a.samples.assign(128,amplitude);return a;}
inline SceneMusicLibrary music(){SceneMusicLibrary m;if(!m.load_declared_decoded({music_clip(1000),music_clip(2000),music_clip(4000)},48000,nullptr))throw std::runtime_error("owned music");return m;}
struct Sink: sound_pcm_source_fixture::Sink {std::function<void()> hook;bool try_submit(const s16* p,std::size_t n)override{const bool result=sound_pcm_source_fixture::Sink::try_submit(p,n);if(hook){auto fn=std::move(hook);fn();}return result;}};
}
