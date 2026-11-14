#pragma once
#include <cstdlib>
#include "engine/gameplay/story_start_visual.hpp"
namespace story_start_visual_fixture {
inline gf::SourceFont font(){using namespace gf;
    std::vector<u8> tpl(120);store_be32(tpl.data(),kTplMagic);store_be32(tpl.data()+4,1);store_be32(tpl.data()+8,12);store_be32(tpl.data()+12,20);store_be16(tpl.data()+20,8);store_be16(tpl.data()+22,16);store_be32(tpl.data()+28,56);std::fill(tpl.begin()+56,tpl.begin()+88,255);
    u32 hash=2166136261u;for(auto b:tpl)hash=(hash^b)*16777619u;
    std::vector<u8> profile(28);store_be32(profile.data(),0x4746464e);store_be32(profile.data()+4,1);store_be32(profile.data()+8,hash);
    for(auto [at,v]:std::array<std::pair<unsigned,u16>,8>{{{12,2},{14,8},{16,2},{18,1},{20,0x8140},{22,0},{24,0x8260},{26,0x8261}}})store_be16(profile.data()+at,v);
    SourceFont result;std::string error;if(!result.load({profile.data(),profile.size()},{tpl.data(),tpl.size()},&error))std::abort();return result;
}
inline gf::StoryStartArt art(){using namespace gf;StoryStartArt a;a.bank_hash=123;a.camera.fov_y_degrees=35;a.camera.near_plane=10;a.camera.far_plane=40000;a.aspect=4.f/3;
    const std::array<std::vector<unsigned>,3> ids{{{0,1,2,3,4,6},{0,1,2,3,4,5,6},{7,8,9,10,11}}};
    for(unsigned g=0;g<3;++g)for(unsigned id:ids[g])a.groups[g].push_back({u16(id==11?10:id),u16(id),{float(id),float(g),-300}});
    a.cursor_y={3,-3,8,0,-8};a.angle_unit=bits_to_f32(0x38c90fdb);a.text_scale=1;a.text_advance=10;a.footer={128,330,360};a.text_reference={640,448};a.text_colours={0xff000000,0x00ff0000};
    for(auto& pair:a.descriptions)for(auto& line:pair)line=std::string("\x82\x60",2);
    return a;
}
}
