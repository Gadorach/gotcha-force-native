#pragma once
#include <array>
#include <cmath>
#include "engine/assets/source_font.hpp"
#include "engine/graphics/camera.hpp"
namespace gf {
struct StoryStartWidget {u16 record=0,id=0;std::array<f32,3> position{};};
struct StoryStartArt {
    u32 bank_hash=0;Camera camera;f32 aspect=0;
    std::array<std::vector<StoryStartWidget>,3> groups;
    std::array<f32,5> cursor_y{};f32 angle_unit=0,text_scale=0,text_advance=0;
    std::array<f32,3> footer{};std::array<f32,2> text_reference{};
    std::array<u32,2> text_colours{}; //source KONST(high), then TEV0(low)
    std::array<std::array<std::string,2>,3> descriptions;
    bool valid(const SourceFont& font)const{
        if(!font.ready()||!bank_hash||groups[0].size()!=6||groups[1].size()!=7||groups[2].size()!=5||!camera.basis(nullptr))return false;
        if(camera.fov_y_degrees!=35||camera.near_plane!=10||camera.far_plane!=40000||!std::isfinite(aspect)||std::abs(aspect-4.f/3)>1e-6f)return false;
        constexpr std::array<f32,3> eye{0,0,0},target{0,0,-1},up{0,1,0};
        for(unsigned i=0;i<3;++i)if(camera.eye[i]!=eye[i]||camera.target[i]!=target[i]||camera.up[i]!=up[i])return false;
        const std::array<std::vector<unsigned>,3> expected{{{0,1,2,3,4,6},{0,1,2,3,4,5,6},{7,8,9,10,11}}};
        for(unsigned g=0;g<3;++g)for(unsigned i=0;i<groups[g].size();++i){const auto& w=groups[g][i];
            if(w.id!=expected[g][i]||w.record!=(w.id==11?10:w.id))return false;
            for(f32 v:w.position)if(!std::isfinite(v)||std::abs(v)>10000)return false;
        }
        for(f32 v:cursor_y)if(!std::isfinite(v)||std::abs(v)>10000)return false;
        if(angle_unit!=bits_to_f32(0x38c90fdb)||text_scale!=1||text_advance!=10||footer!=std::array<f32,3>{128,330,360}||text_reference!=std::array<f32,2>{640,448})return false;
        for(const auto& pair:descriptions)for(const auto& line:pair){if(line.size()>512||line.size()%2)return false;
            for(unsigned at=0;at<line.size();at+=2){const u16 code=u16(u16(u8(line[at]))<<8|u8(line[at+1]));if(code!=font.blank()&&font.glyph(code)<0)return false;}}
        return true;
    }
    bool load(ByteView b,const SourceFont& font,std::string* error){
        *this={};auto fail=[&](){*this={};if(error)*error="Story start art: schema/source domain/font mismatch";return false;};
        if(!b.data||b.size<72||b.size>8192||b.be32(0)!=0x47465353u||b.be32(4)!=1)return fail();
        bank_hash=b.be32(8);std::size_t at=12;bool ok=true;
        auto word=[&](){if(!b.contains(at,4)){ok=false;return u32(0);}const u32 v=b.be32(at);at+=4;return v;};
        auto half=[&](){if(!b.contains(at,2)){ok=false;return u16(0);}const u16 v=b.be16(at);at+=2;return v;};
        auto real=[&](){return bits_to_f32(word());};
        for(auto& v:camera.eye)v=real();
        for(auto& v:camera.target)v=real();
        for(auto& v:camera.up)v=real();
        camera.fov_y_degrees=real();camera.near_plane=real();camera.far_plane=real();aspect=real();
        std::array<unsigned,3> counts{};for(auto& n:counts)n=half();if(half()||counts!=std::array<unsigned,3>{6,7,5})return fail();
        for(unsigned g=0;g<3;++g)for(unsigned i=0;i<counts[g];++i){StoryStartWidget w;const auto record=word(),id=word();if(record>10||id>11)return fail();w.record=u16(record);w.id=u16(id);for(auto& v:w.position)v=real();groups[g].push_back(w);}
        for(auto& v:cursor_y)v=real();
        angle_unit=real();text_scale=real();text_advance=real();for(auto& v:footer)v=real();for(auto& v:text_reference)v=real();for(auto& c:text_colours)c=word();
        for(auto& pair:descriptions)for(auto& line:pair){const auto n=half();if(!b.contains(at,n)||n>512)return fail();line.assign(reinterpret_cast<const char*>(b.data+at),n);at+=n;}
        if(!ok||at!=b.size||!valid(font))return fail();
        if(error)error->clear();
        return true;
    }
};
} //namespace gf
