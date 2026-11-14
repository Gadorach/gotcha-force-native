#pragma once
#include <array>
#include "engine/assets/menu_layout.hpp"
namespace gf {
struct OptionWidgetTemplate {
    u8 kind=0,id=0,count=0;
    u32 primary=0,secondary=0;
    f32 position[3]={},rotation[3]={},secondary_offset[3]={};
};
struct OptionWidgetProfile {
    std::array<u32,23> roots{};
    std::array<f32,10> row_y{};
    std::array<std::array<u8,2>,5> group_rows{};
    std::array<u8,7> value_rows{};
    std::array<std::array<u8,2>,2> cursor_rows{};
    u32 selected_colour=0,normal_colour=0;
    f32 angle_unit=0;s32 cursor_step=0;u8 repeat_limit=0,repeat_restart=0;
    std::array<OptionWidgetTemplate,23> widgets{};
};
inline bool option_widget_profile_load(ByteView file,ByteView archive,const HsdScene& scene,
                                       OptionWidgetProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("option widgets: ")+why;return false;};
    if(!out||!file.data||file.size!=1368||file.be32(0)!=0x47464f57u||file.be32(4)!=1||
       file.be32(8)!=menu_asset_hash(archive)||file.be32(12)!=23||file.be32(16)!=23||file.be32(28)!=0)
        return fail("size/version/source/count mismatch");
    OptionWidgetProfile c;c.angle_unit=file.bef32(20);c.cursor_step=std::bit_cast<s32>(file.be32(24));
    if(!std::isfinite(c.angle_unit)||std::fabs(c.angle_unit*65536-6.28318530718f)>0.001f||
       c.cursor_step< -32768||c.cursor_step>32767)return fail("invalid rotation domain");
    if(scene.joints.size()!=23||scene.meshes.size()!=23||scene.mesh_joint.size()!=23)return fail("source must be 23 published leaf meshes");
    for(unsigned i=0;i<23;++i){c.roots[i]=file.be32(32+i*4);
        if(!c.roots[i]||scene.joints[i].archive_offset!=c.roots[i]||scene.joints[i].parent!=kHsdNoJoint)return fail("root binding mismatch");
        for(unsigned j=0;j<i;++j)if(c.roots[j]==c.roots[i])return fail("duplicate model root");
        unsigned count=0;for(u32 joint:scene.mesh_joint)if(joint==i)++count;
        if(count!=1)return fail("ambiguous leaf mesh");
    }
    for(unsigned i=0;i<10;++i){c.row_y[i]=file.bef32(124+i*4);
        if(!std::isfinite(c.row_y[i])||std::fabs(c.row_y[i])>1e6f||(i&&c.row_y[i]>=c.row_y[i-1]))return fail("invalid row positions");}
    for(unsigned i=0;i<5;++i)for(unsigned k=0;k<2;++k){const u32 row=file.be32(164+(i*2+k)*4);if(row>9)return fail("invalid group row");c.group_rows[i][k]=u8(row);}
    for(const auto& range:c.group_rows)if(range[0]>range[1])return fail("reversed group range");
    for(unsigned i=0;i<7;++i){const u32 row=file.be32(204+i*4);if(row>9)return fail("invalid value row");c.value_rows[i]=u8(row);}
    for(unsigned i=0;i<2;++i)for(unsigned k=0;k<2;++k){const u32 row=file.be32(232+(i*2+k)*4);if(row>9)return fail("invalid cursor row");c.cursor_rows[i][k]=u8(row);}
    for(const auto& range:c.cursor_rows)if(range[0]>range[1])return fail("reversed cursor range");
    c.selected_colour=file.be32(248);c.normal_colour=file.be32(252);
    if((c.selected_colour&255)!=255||(c.normal_colour&255)!=255)return fail("unsupported label alpha");
    const u32 limit=file.be32(256),restart=file.be32(260);if(limit>=255||restart>limit)return fail("invalid repeat domain");
    c.repeat_limit=u8(limit);c.repeat_restart=u8(restart);
    std::array<std::array<bool,7>,6> seen{};const unsigned counts[6]={1,5,2,3,7,5};
    for(unsigned i=0;i<23;++i){const std::size_t at=264+i*48;auto& w=c.widgets[i];
        w.kind=file.data[at];w.id=file.data[at+1];w.count=file.data[at+2];
        if(w.kind>=6||w.id>=counts[w.kind]||seen[w.kind][w.id]||file.data[at+3])return fail("invalid/duplicate widget identity");
        seen[w.kind][w.id]=true;w.primary=file.be32(at+4);w.secondary=file.be32(at+8);
        const auto has_root=[&](u32 root){return std::find(c.roots.begin(),c.roots.end(),root)!=c.roots.end();};
        const unsigned expected=(w.kind==5||(w.kind==1&&w.id==0)||(w.kind==4&&w.id>=3))?2:1;
        if(w.count!=expected||!has_root(w.primary)||(w.count==2?!has_root(w.secondary):w.secondary!=0))return fail("invalid instance model/count");
        for(unsigned k=0;k<3;++k){w.position[k]=file.bef32(at+12+k*4);w.rotation[k]=file.bef32(at+24+k*4);w.secondary_offset[k]=file.bef32(at+36+k*4);
            if(!std::isfinite(w.position[k])||!std::isfinite(w.rotation[k])||!std::isfinite(w.secondary_offset[k])||
               std::fabs(w.position[k])>1e6f||std::fabs(w.secondary_offset[k])>1e6f||std::fabs(w.rotation[k])>7)return fail("invalid widget transform");}
    }
    *out=c;return true;
}
} // namespace gf
