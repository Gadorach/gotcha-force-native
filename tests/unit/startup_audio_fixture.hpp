#pragma once
#include "engine/gameplay/startup_audio_pcm_source.hpp"
#include "tests/unit/adx_fixture.hpp"
#include "tests/unit/sound_pcm_boundary_fixture.hpp"
#include "tests/unit/startup_sound_spatial_fixture.hpp"
namespace startup_audio_fixture {
inline void le32(gf::u8* out,gf::u32 value){for(unsigned i=0;i<4;++i)out[i]=gf::u8(value>>(i*8));}
struct Files final:gf::IFileSystem {
    std::vector<gf::u8> data;unsigned reads=0;bool reject=false;
    bool read(const std::string&,std::vector<gf::u8>&)override{return false;}
    bool write(const std::string&,const gf::u8*,std::size_t)override{return false;}
    bool exists(const std::string&)override{return false;}bool erase(const std::string&)override{return false;}
    bool file_size(const std::string&,gf::u64* out)override{if(reject||!out)return false;*out=data.size();return true;}
    bool read_range(const std::string&,gf::u64 at,std::size_t count,std::vector<gf::u8>& out)override{
        ++reads;if(reject||at>data.size()||count>data.size()-at)return false;out.assign(data.begin()+std::size_t(at),data.begin()+std::size_t(at)+count);return true;
    }
};
struct Resources {
    sound_pcm_boundary_fixture::Resources sound;gf::StartupMusicProfile profile;gf::StartupMusicLibrary music;gf::StartupSoundSpatialProfile spatial;Files files;
    bool load(std::string* error){
        using namespace gf;if(!sound.load(error))return false;auto spatial_bytes=startup_sound_spatial_fixture::profile();if(!startup_sound_spatial_load({spatial_bytes.data(),spatial_bytes.size()},&spatial,error))return false;
        auto a=owned_adx::file(256,2,true),b=a;b[66]=0x43;a[84]=0x34;b[84]=0x56;
        files.data.resize(1000);le32(files.data.data(),kAfsMagic);le32(files.data.data()+4,2);
        le32(files.data.data()+8,128);le32(files.data.data()+12,u32(a.size()));le32(files.data.data()+16,512);le32(files.data.data()+20,u32(b.size()));
        std::copy(a.begin(),a.end(),files.data.begin()+128);std::copy(b.begin(),b.end(),files.data.begin()+512);
        auto stream=[&](u32 entry,u32 offset,const std::vector<u8>& payload){
            std::vector<u8> out(80);const u32 words[]={0x47464d42,1,123,2,0,1000,music_hash({files.data.data(),24}),entry,offset,u32(payload.size()),music_hash({payload.data(),payload.size()}),8000,256,0,256,1,48000,0,2,0};
            for(unsigned i=0;i<20;++i)store_be32(out.data()+i*4,words[i]);
            return out;
        };
        const auto first=stream(0,128,a),menu=stream(1,512,b);std::vector<u8> combined(200);const u32 header[]={0x47464d53,1,123,music_hash({menu.data(),menu.size()}),27,29,0,2};
        for(unsigned i=0;i<8;++i)store_be32(combined.data()+i*4,header[i]);
        std::copy(first.begin(),first.end(),combined.begin()+32);std::copy(menu.begin(),menu.end(),combined.begin()+112);store_be32(combined.data()+192,1);
        return startup_music_profile_load({combined.data(),combined.size()},{menu.data(),menu.size()},123,&profile,error)&&music.load(profile,files,"owned.afs",error);
    }
};
inline gf::StartupSoundIntent command(gf::u8 opcode,gf::u16 operand){gf::StartupSoundIntent i;i.kind=gf::StartupSoundIntentKind::MusicCommand;i.opcode=opcode;i.operand=operand;return i;}
inline bool append(gf::StartupSoundInputTimeline& timeline,const std::shared_ptr<gf::StartupSoundCaptureScope>& scope,const gf::SoundControlSnapshot& snapshot,std::vector<gf::StartupSoundIntent> commands,std::string* error){
    gf::StartupSoundCaptureFrame capture;capture.scope=scope;capture.frame=timeline.captured_frames()+1;capture.intents=std::move(commands);gf::StartupSoundCaptureContext context;context.scope=scope;context.frame=capture.frame;
    return snapshot.commands(&context.commands,error)&&timeline.append(capture,context,snapshot,error);
}
inline std::vector<gf::s16> queued(const sound_pcm_source_fixture::Sink& sink){std::vector<gf::s16> result;for(const auto& sample:sink.queued){result.push_back(sample[0]);result.push_back(sample[1]);}return result;}
inline unsigned next_cut(const gf::SoundPcmSourceSnapshot& state){
    const gf::u64 reads=161-state.buffered_position,target=state.conversion.target;
    return unsigned((reads*target-state.conversion.phase+31999)/32000);
}
}
