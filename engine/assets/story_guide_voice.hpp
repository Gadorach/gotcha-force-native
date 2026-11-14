#pragma once
#include <array>
#include <memory>
#include "engine/assets/adx.hpp"
#include "engine/core/backends.hpp"
namespace gf {
struct StoryGuideVoiceRow {u32 key=0,member=0,bytes=0,payload_hash=0,channels=0,rate=0,frames=0;};
struct StoryGuideVoiceProfile {
    std::array<StoryGuideVoiceRow,2> rows{};u32 tag=0;
    bool load(ByteView b){
        *this={};if(!b.data||b.size!=80||b.be32(0)!=0x47464756||b.be32(4)!=1||b.be16(8)!=14||b.be16(10)!=2||b.be32(12))return false;
        StoryGuideVoiceProfile p;for(unsigned i=0;i<2;++i){const auto at=16+i*32;auto& r=p.rows[i];r={b.be32(at),b.be32(at+4),b.be32(at+8),b.be32(at+12),b.be32(at+16),b.be32(at+20),b.be32(at+24)};if(r.key!=60+i||r.member!=1863+i||!r.bytes||r.bytes>(32u<<20)||!r.payload_hash||(r.channels!=1&&r.channels!=2)||r.rate<8000||r.rate>192000||!r.frames||r.frames>kMaxAdxFrames||b.be32(at+28))return false;}
        p.tag=2166136261u;for(std::size_t i=0;i<b.size;++i)p.tag=(p.tag^b.data[i])*16777619u;if(!p.tag)return false;*this=p;return true;
    }
};
//Two immutable original-guide resources. Loose native ADX files, bounded range
//I/O and the existing decoder; not a BGM cue, voice player or delivery receipt.
class StoryGuideVoiceLibrary {
public:
    bool load(ByteView profile,const std::array<std::string,2>& paths,IFileSystem& files,std::string* error){
        if(data_)return fail(error,"fresh library required");
        StoryGuideVoiceProfile p;if(!p.load(profile))return fail(error,"profile/bounded metadata");auto next=std::make_shared<Data>();next->profile=p;
        for(unsigned i=0;i<2;++i){const auto& row=p.rows[i];u64 bytes=0;std::vector<u8> file;if(paths[i].empty()||paths[i].size()>2048||!files.file_size(paths[i],&bytes)||bytes!=row.bytes||!files.read_range(paths[i],0,row.bytes,file)||file.size()!=row.bytes)return fail(error,"bounded native voice file");u32 hash=2166136261;for(auto x:file)hash=(hash^x)*16777619u;if(hash!=row.payload_hash||!adx_decode({file.data(),file.size()},&next->audio[i],error))return fail(error,"voice payload/ADX");const auto& a=next->audio[i];if(a.channels!=row.channels||a.rate!=row.rate||a.frames!=row.frames||a.loop||a.loop_start||a.loop_end)return fail(error,"declared source voice metadata");}
        data_=std::move(next);if(error)error->clear();return true;
    }
    bool ready()const{return bool(data_);}u32 tag()const{return data_?data_->profile.tag:0;}
    const AdxAudio* audio(unsigned source_key)const{return data_&&source_key>=60&&source_key<=61?&data_->audio[source_key-60]:nullptr;}
    const StoryGuideVoiceRow* row(unsigned source_key)const{return data_&&source_key>=60&&source_key<=61?&data_->profile.rows[source_key-60]:nullptr;}
    bool same_resources(const StoryGuideVoiceLibrary& other)const{return data_&&data_==other.data_;}
private:
    struct Data {StoryGuideVoiceProfile profile;std::array<AdxAudio,2> audio;};std::shared_ptr<const Data> data_;
    static bool fail(std::string* e,const char* why){if(e)*e=std::string("Story guide voice library: ")+why;return false;}
};
} //namespace gf
