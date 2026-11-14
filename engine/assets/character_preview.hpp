#pragma once
#include "engine/core/types.hpp"
#include <array>
#include <cmath>
#include <string>
namespace gf {
// GFPV source58244/583CC/1DD6B0 operands for the two reviewed normal models.
struct CharacterPreviewProfile {
    u32 scene_hash=0;std::array<u16,2> models{};u16 pitch=0,yaw=0;
    std::array<f32,3> position{};f32 one=0,radius=0,near_plane=0,far_plane=0,fov=0,aspect=0,angle_unit=0;bool loaded=false;
};
inline bool character_preview_profile_valid(const CharacterPreviewProfile& p){
    return p.loaded&&p.scene_hash&&p.models==std::array<u16,2>{0x615,10}&&p.pitch==0x800&&p.yaw==0&&p.position==std::array<f32,3>{-60,15,0}&&p.one==1&&f32_to_bits(p.radius)==0x439e199au&&p.near_plane==10&&p.far_plane==40000&&p.fov==41&&p.aspect==640.f/480&&f32_to_bits(p.angle_unit)==0x38c90fdbu;
}
inline bool character_preview_load(ByteView b,u32 scene_hash,CharacterPreviewProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](){if(error)*error="character preview: GFPV source/scene binding";return false;};
    if(!out||!b.data||b.size!=60||b.be32(0)!=0x47465056u||b.be32(4)!=1||b.be32(8)!=scene_hash)return fail();
    CharacterPreviewProfile p;p.scene_hash=scene_hash;p.models={b.be16(12),b.be16(14)};p.pitch=b.be16(16);p.yaw=b.be16(18);for(unsigned i=0;i<3;++i)p.position[i]=b.bef32(20+i*4);
    p.one=b.bef32(32);p.radius=b.bef32(36);p.near_plane=b.bef32(40);p.far_plane=b.bef32(44);p.fov=b.bef32(48);p.aspect=b.bef32(52);p.angle_unit=b.bef32(56);p.loaded=true;
    if(!character_preview_profile_valid(p))return fail();
    *out=p;if(error)error->clear();return true;
}
} // namespace gf
