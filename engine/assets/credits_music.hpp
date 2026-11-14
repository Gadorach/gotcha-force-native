#pragma once
#include "engine/assets/menu_music.hpp"
#include "engine/audio/music_fade.hpp"
namespace gf {
struct CreditsMusicProfile {MenuMusicProfile credits;MusicFadeConfig fade;};
inline bool credits_music_profile_load(ByteView file,ByteView menu_file,u32 scene_hash,CreditsMusicProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("credits music: ")+why;return false;};
    if(!out||!file.data||!menu_file.data||menu_file.size!=80||file.size!=112||file.be32(0)!=0x47464d54u||file.be32(4)!=1||
       file.be32(8)!=music_hash(menu_file)||file.be32(24)!=0||file.be32(28)!=0)return fail("header/menu binding/target mismatch");
    CreditsMusicProfile next;next.fade={file.be32(12),file.bef32(16),file.bef32(20)};
    if(!music_fade_config_valid(next.fade))return fail("invalid scalar fade domain");
    MenuMusicProfile menu;
    if(!menu_music_profile_load(menu_file,scene_hash,&menu,error)||!menu_music_profile_load({file.data+32,80},scene_hash,&next.credits,error))return false;
    const auto& credits=next.credits;
    if(menu.entry==credits.entry||menu.archive_count!=credits.archive_count||menu.archive_size!=credits.archive_size||
       menu.directory_hash!=credits.directory_hash||menu.output_rate!=credits.output_rate)return fail("archive/route mismatch");
    *out=next;return true;
}
} // namespace gf
