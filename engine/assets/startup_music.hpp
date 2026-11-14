#pragma once
// Sparse source script-index -> explicit ADX member bindings. This does not infer
// the full source table, original delivery latency or full-actor admission.
#include <array>
#include <memory>
#include "engine/assets/menu_music.hpp"
namespace gf {
namespace startup_music_detail {
struct Bindings {std::array<s16,2> indices{};std::array<MenuMusicProfile,2> streams;};
}
class StartupMusicProfile {
public:
    bool ready()const{return bool(data_);}
    bool same_bindings(const StartupMusicProfile& other)const{return data_&&data_==other.data_;}
    bool contains_member(u32 member)const{return data_&&(data_->streams[0].entry==member||data_->streams[1].entry==member);}
    const MenuMusicProfile* stream(unsigned slot)const{return data_&&slot<2?&data_->streams[slot]:nullptr;}
    const MenuMusicProfile* resolve(s16 index)const{
        if(data_)for(unsigned i=0;i<2;++i)if(data_->indices[i]==index)return &data_->streams[i];
        return nullptr;
    }
private:
    friend bool startup_music_profile_load(ByteView,ByteView,u32,StartupMusicProfile*,std::string*);
    std::shared_ptr<const startup_music_detail::Bindings> data_;
};
inline bool startup_music_profile_load(ByteView file,ByteView menu_file,u32 scene_hash,StartupMusicProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("startup music profile: ")+why;return false;};
    if(!out||!file.data||file.size!=200||!menu_file.data||menu_file.size!=80||file.be32(0)!=0x47464d53u||
       file.be32(4)!=1||file.be32(8)!=scene_hash||file.be32(12)!=music_hash(menu_file)||
       file.be32(24)!=0||file.be32(28)!=2||file.be32(192)!=1||file.be32(196)!=0)
        return fail("header/menu/source-route/pause-lane mismatch");
    const auto first=file.be32(16),second=file.be32(20);
    if(first>32767||second>32767||first==second||!std::equal(menu_file.data,menu_file.data+80,file.data+112))
        return fail("two distinct signed source indices and exact parent menu profile required");
    auto next=std::make_shared<startup_music_detail::Bindings>();
    next->indices={s16(first),s16(second)};
    if(!menu_music_profile_load({file.data+32,80},scene_hash,&next->streams[0],error)||
       !menu_music_profile_load({file.data+112,80},scene_hash,&next->streams[1],error))return false;
    const auto& a=next->streams[0];const auto& b=next->streams[1];
    if(a.entry==b.entry||a.archive_count!=b.archive_count||a.archive_size!=b.archive_size||
       a.directory_hash!=b.directory_hash||a.output_rate!=b.output_rate)return fail("distinct same-archive stream bindings required");
    out->data_=std::move(next);if(error)error->clear();return true;
}
} // namespace gf
