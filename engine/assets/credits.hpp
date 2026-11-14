#pragma once
#include "engine/assets/menu_layout.hpp"
#include "engine/gameplay/credits/timeline.hpp"
namespace gf {
struct CreditsProfile {CreditsConfig config;std::vector<u32> roots;};
inline bool credits_profile_load(ByteView file,ByteView archive,const HsdScene& source,CreditsProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("credits profile: ")+why;return false;};
    if(!out||!file.data||file.size<128||file.be32(0)!=0x47464352u||file.be32(4)!=1||file.be32(8)!=menu_asset_hash(archive))return fail("header/source mismatch");
    const u32 models=file.be32(12),rows=file.be32(16),glyphs=file.be32(20);
    if(models<2||models>128||!rows||rows>512||!glyphs||glyphs>8192||file.size!=128+std::size_t(models)*4+std::size_t(rows)*20+std::size_t(glyphs)*8)
        return fail("count/size budget");
    for(unsigned at=92;at<128;at+=4)if(file.be32(at))return fail("unsupported header flags");
    CreditsProfile result;auto& c=result.config;c.model_count=models;c.max_active=file.be32(24);c.max_updates=file.be32(28);c.entry_wait=file.be32(32);c.final_hold=file.be32(36);
    f32* values[]={&c.speed,&c.spawn_y,&c.delete_y,&c.hold_y,&c.x_base,&c.z_base,&c.x_step,&c.z_step,&c.grid_factor,&c.image_x,&c.image_z,&c.rotation_y,&c.scale};
    for(unsigned i=0;i<13;++i)*values[i]=file.bef32(40+i*4);
    if(source.joints.size()!=models||source.meshes.size()!=models||source.mesh_joint.size()!=models)return fail("source must be published single-mesh leaves");
    for(unsigned i=0;i<models;++i){const u32 root=file.be32(128+i*4);if(!root||source.joints[i].archive_offset!=root||source.joints[i].parent!=kHsdNoJoint)return fail("model root mismatch");
        if(std::find(result.roots.begin(),result.roots.end(),root)!=result.roots.end())return fail("duplicate root");
        unsigned count=0;for(u32 joint:source.mesh_joint)if(joint==i)++count;if(count!=1)return fail("ambiguous mesh root");result.roots.push_back(root);}
    std::size_t at=128+std::size_t(models)*4;c.rows.reserve(rows);c.glyphs.reserve(glyphs);
    for(unsigned i=0;i<rows;++i,at+=20)c.rows.push_back({file.be32(at),file.be32(at+4),file.be32(at+8),file.be32(at+12),file.bef32(at+16)});
    for(unsigned i=0;i<glyphs;++i,at+=8)c.glyphs.push_back({file.be32(at),file.be32(at+4)});
    if(!credits_config_valid(c)||!credits_preflight(c))return fail("unsupported/unbounded timeline");
    *out=std::move(result);return true;
}
} // namespace gf
