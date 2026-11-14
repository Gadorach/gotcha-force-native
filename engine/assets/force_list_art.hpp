#pragma once
#include "engine/assets/force_editor_art.hpp"
namespace gf {
struct ForceListWidget {s16 record=-1;u16 pass=0;f32 scale=1;std::array<f32,3> position{};};
struct ForceListNumber {s16 record=-1;u8 digits=0,flags=0;std::array<f32,3> position{};};
struct ForceListFooter {u32 domain=0;std::array<s16,4> records{};std::array<f32,3> position{};};
struct ForceListArt {
    Camera camera;f32 aspect=0;u32 bank_hash=0;
    std::array<ForceListWidget,5> header;std::array<ForceListWidget,4> body;std::array<ForceListWidget,6> row,summary;
    std::array<ForceListWidget,7> tab;std::array<ForceListWidget,2> arrows;std::array<ForceListNumber,3> numbers;
    std::array<ForceListFooter,20> footer;std::array<std::array<f32,3>,4> footer_offsets;
    std::array<s16,6> variants;std::array<s16,5> rarity;std::array<s16,20> tribes;std::array<s16,21> filters;std::array<s16,3> columns,tab_labels;
    std::array<std::array<u32,2>,6> font_colours; // high then low, source styles37..42
    std::array<f32,3> name_origin{},tab_origin{},part_origin{},highlight{};std::array<std::array<f32,3>,2> part_text{};
    f32 extent=0,advance=0,row_step=0,digit_advance=0,selected_scale=0,selected_z=0,part_variant_x=0,part_rarity_x=0,part_advance=0;
    std::array<std::string,4> part_strings;
    bool load(ByteView data,std::string* error){
        *this={};auto fail=[&](){if(error)*error="force list art: GFLA extent/domain";return false;};
        if(!data.data||data.size<12||data.size>16384||data.be32(0)!=0x47464c41u||data.be32(4)!=1)return fail();
        bank_hash=data.be32(8);
        std::size_t at=12;bool ok=true;auto word=[&](){if(!data.contains(at,4)){ok=false;return u32(0);}auto v=data.be32(at);at+=4;return v;};auto half=[&](){if(!data.contains(at,2)){ok=false;return u16(0);}auto v=data.be16(at);at+=2;return v;};auto real=[&](){auto v=bits_to_f32(word());if(!std::isfinite(v)||std::abs(v)>1e6)ok=false;return v;};
        for(auto& v:camera.eye)v=real();
        for(auto& v:camera.target)v=real();
        for(auto& v:camera.up)v=real();
        camera.fov_y_degrees=real();camera.near_plane=real();camera.far_plane=real();aspect=real();
        auto widget=[&](auto& group){for(auto& w:group){w.record=std::bit_cast<s16>(half());w.pass=half();w.scale=real();for(auto& v:w.position)v=real();if(w.record<-1||w.record>=4096||w.scale<=0)ok=false;}};
        widget(header);widget(body);widget(row);widget(summary);widget(tab);widget(arrows);
        for(auto& n:numbers){n.record=std::bit_cast<s16>(half());const auto packed=half();n.digits=u8(packed>>8);n.flags=u8(packed);for(auto& v:n.position)v=real();if(n.record<0||n.record>=4096||!n.digits||n.digits>6)ok=false;}
        for(auto& f:footer){f.domain=word();for(auto& r:f.records)r=std::bit_cast<s16>(half());for(auto& v:f.position)v=real();if(f.domain>3)ok=false;}
        for(auto& p:footer_offsets)for(auto& v:p)v=real();
        auto indices=[&](auto& group){for(auto& v:group){v=std::bit_cast<s16>(half());if(v<-1||v>=4096)ok=false;}};indices(variants);indices(rarity);indices(tribes);indices(filters);indices(columns);indices(tab_labels);
        for(auto& pair:font_colours)for(auto& v:pair)v=word();
        for(auto& v:name_origin)v=real();
        extent=real();advance=real();row_step=real();digit_advance=real();selected_scale=real();selected_z=real();part_variant_x=real();part_rarity_x=real();
        for(auto& v:tab_origin)v=real();
        for(auto& v:part_origin)v=real();
        for(auto& v:highlight)v=real();
        for(auto& p:part_text)for(auto& v:p)v=real();
        part_advance=real();
        for(auto& text:part_strings){const auto n=half();if(n>512||!data.contains(at,n))return fail();text.assign(reinterpret_cast<const char*>(data.data+at),n);at+=n;}
        if(!ok||at!=data.size||!camera.basis(nullptr)||std::abs(aspect-4.f/3)>1e-4f||extent<=0||advance<=0||row_step>=0||selected_scale<=0)return fail();
        if(error)error->clear();
        return true;
    }
};
} // namespace gf
