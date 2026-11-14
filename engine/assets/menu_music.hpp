#pragma once
#include "engine/assets/afs.hpp"
#include "engine/assets/adx.hpp"
#include "engine/core/backends.hpp"
namespace gf {
inline u32 music_hash(ByteView bytes){u32 h=2166136261u;for(std::size_t i=0;i<bytes.size;++i){h^=bytes.data[i];h*=16777619u;}return h;}
struct MenuMusicProfile {
    u32 archive_count=0,directory_hash=0,entry=0,offset=0,size=0,payload_hash=0;
    u64 archive_size=0;u32 source_rate=0,frames=0,loop_start=0,loop_end=0,output_rate=0;bool loop=false;
};
inline bool menu_music_profile_load(ByteView file,u32 scene_hash,MenuMusicProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("menu music: ")+why;return false;};
    if(!out||!file.data||file.size!=80||file.be32(0)!=0x47464d42u||file.be32(4)!=1||file.be32(8)!=scene_hash||
       file.be32(68)!=0||file.be32(72)!=2||file.be32(76)!=0)return fail("size/version/source/route mismatch");
    MenuMusicProfile c;c.archive_count=file.be32(12);c.archive_size=(u64(file.be32(16))<<32)|file.be32(20);c.directory_hash=file.be32(24);
    c.entry=file.be32(28);c.offset=file.be32(32);c.size=file.be32(36);c.payload_hash=file.be32(40);c.source_rate=file.be32(44);c.frames=file.be32(48);
    c.loop_start=file.be32(52);c.loop_end=file.be32(56);const u32 loop=file.be32(60);c.output_rate=file.be32(64);c.loop=loop!=0;
    if(!c.archive_count||c.archive_count>kMaxAfsEntries||c.entry>=c.archive_count||c.archive_size>0x100000000ull||
       !c.size||c.size>(32u<<20)||c.offset<8+u64(c.archive_count)*8||u64(c.offset)+c.size>c.archive_size||
       c.source_rate<8000||c.source_rate>192000||!c.frames||c.frames>kMaxAdxFrames||loop>1||c.output_rate!=48000||
       (c.loop&&(c.loop_start>=c.loop_end||c.loop_end>c.frames)))return fail("invalid bounded stream metadata");
    *out=c;return true;
}
// Inspect the AFS directory via range I/O and read only the qualified member.
// This does not claim validation of unselected payloads or a whole-file hash.
inline bool menu_music_load(IFileSystem& fs,const std::string& path,const MenuMusicProfile& profile,
                            AdxAudio* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("menu music: ")+why;return false;};
    if(!out||!profile.archive_count||profile.archive_count>kMaxAfsEntries||profile.entry>=profile.archive_count||
       !profile.size||profile.size>(32u<<20))return fail("invalid load target");
    u64 size=0;if(!fs.file_size(path,&size)||size!=profile.archive_size)return fail("archive size/read capability mismatch");
    const std::size_t header=8+std::size_t(profile.archive_count)*8;std::vector<u8> directory;
    if(!fs.read_range(path,0,header,directory)||directory.size()!=header||load_le32(directory.data())!=kAfsMagic||
       load_le32(directory.data()+4)!=profile.archive_count||music_hash({directory.data(),directory.size()})!=profile.directory_hash)
        return fail("AFS directory identity/read mismatch");
    for(u32 i=0;i<profile.archive_count;++i){const u32 offset=load_le32(directory.data()+8+i*8),bytes=load_le32(directory.data()+12+i*8);
        if(!bytes||offset<header||u64(offset)+bytes>size)return fail("invalid archive directory bounds");
        if(i==profile.entry&&(offset!=profile.offset||bytes!=profile.size))return fail("selected entry binding mismatch");
    }
    std::vector<u8> payload;
    if(!fs.read_range(path,profile.offset,profile.size,payload)||payload.size()!=profile.size||music_hash({payload.data(),payload.size()})!=profile.payload_hash)
        return fail("selected payload identity/read mismatch");
    AdxAudio decoded;if(!adx_decode({payload.data(),payload.size()},&decoded,error))return false;
    if(decoded.channels!=2||decoded.rate!=profile.source_rate||decoded.frames!=profile.frames||decoded.loop!=profile.loop||
       decoded.loop_start!=profile.loop_start||decoded.loop_end!=profile.loop_end)return fail("decoded metadata mismatch");
    *out=std::move(decoded);return true;
}
} // namespace gf
