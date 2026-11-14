#pragma once
#include "engine/assets/force_name_art.hpp"
namespace force_name_fixture {
inline gf::SourceFont font(){
    using namespace gf;std::vector<u8> tpl(120);store_be32(tpl.data(),kTplMagic);store_be32(tpl.data()+4,1);store_be32(tpl.data()+8,12);store_be32(tpl.data()+12,20);store_be16(tpl.data()+20,8);store_be16(tpl.data()+22,16);store_be32(tpl.data()+28,56);std::fill(tpl.begin()+56,tpl.begin()+88,255);
    u32 hash=2166136261u;for(auto b:tpl)hash=(hash^b)*16777619u;std::vector<u8> p(28);store_be32(p.data(),0x4746464e);store_be32(p.data()+4,1);store_be32(p.data()+8,hash);
    for(auto [at,v]:std::array<std::pair<unsigned,u16>,8>{{{12,2},{14,8},{16,2},{18,1},{20,0x8140},{22,0},{24,0x8260},{26,0x8261}}})store_be16(p.data()+at,v);
    SourceFont result;std::string error;if(!result.load({p.data(),p.size()},{tpl.data(),tpl.size()},&error))std::abort();return result;
}
inline gf::ForceNameArt art(){
    using namespace gf;ForceNameArt a;a.bank_hash=1;a.maximum=20;a.cursor_code=0x8261;a.widgets={ForceNameWidget{0,2,{0,0,-10}}};a.keys.fill(0x8260);a.keys[1]=0x8261;a.keys[2]=0x8140;a.ascii['A']=0x8260;a.ascii['B']=0x8261;a.ascii[' ']=0x8140;
    a.weights.fill(1);a.total=64;a.slide=1500;a.title_start=10;a.title_delta=-9.15f;a.title_y=1.15f;a.title_pulse=.15f;a.fade_step=.0625f;a.underline_divisor=15;a.cell=8;a.name_scale=1.25f;a.preview_scale=1.5f;a.angle_unit=.0000958738f;a.screen_width=640;a.screen_height=448;a.name_advance=19;a.name_x=133;a.name_y=90;a.preview_x=9;a.preview_y=-1;a.sounds={119,122,123,125};return a;
}
inline std::vector<gf::u8> profile(const gf::ForceNameArt& a){
    using namespace gf;std::vector<u8> b;auto word=[&](u32 v){const auto i=b.size();b.resize(i+4);store_be32(b.data()+i,v);};auto half=[&](u16 v){const auto i=b.size();b.resize(i+2);store_be16(b.data()+i,v);};auto real=[&](f32 v){word(f32_to_bits(v));};
    word(0x47464e4b);word(1);word(a.bank_hash);half(a.maximum);half(u16(a.widgets.size()));half(15);half(7);
    for(const auto& w:a.widgets){word(w.record);word(w.type);for(auto v:w.position)real(v);}
    for(auto c:a.keys)half(c);
    for(auto c:a.ascii)half(c);
    for(auto v:a.x)real(v);
    for(auto v:a.y)real(v);
    for(auto v:a.screen_x)half(u16(v));
    for(auto v:a.screen_y)half(u16(v));
    for(auto v:a.weights)real(v);
    for(auto v:{a.slide,a.title_start,a.title_delta,a.title_y,a.title_pulse,a.fade_step,a.underline_divisor,a.cell,a.name_scale,a.preview_scale,a.angle_unit,a.screen_width,a.screen_height,a.name_advance,a.name_x,a.name_y,a.preview_x,a.preview_y})real(v);
    for(auto pair:a.colours)for(auto v:pair)word(v);
    half(a.cursor_code);half(0);word(a.dim_colour);for(auto s:a.sounds)half(s);return b;
}
}
