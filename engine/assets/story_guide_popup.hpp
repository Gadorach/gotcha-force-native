#pragma once
#include "engine/graphics/camera.hpp"
#include "engine/assets/source_messages.hpp"
namespace gf {
//Source801D10E0/1668/1768 and overlay C9D4 camera/projection operands.
struct StoryGuidePopupArt {
    Camera camera;std::array<f32,3> position{};std::array<f32,9> scalars{};
    std::array<u32,3> text_y{};std::array<u16,3> records{};u32 high=0,low=0;
    u32 bank_hash=0,text_hash=0,glyph_hash=0,rgba_hash=0;bool ready=false;
    enum Scalar {Alpha,Minimum,Maximum,WidthScale,Advance,Extent,ReferenceWidth,ReferenceHeight,LineStep};
    bool load(ByteView b){
        *this={};if(!b.data||b.size!=152||b.be32(0)!=0x47464750||b.be32(4)!=1)return false;
        bank_hash=b.be32(8);text_hash=b.be32(12);glyph_hash=b.be32(16);rgba_hash=b.be32(20);if(!bank_hash||!text_hash||!glyph_hash||!rgba_hash)return false;
        std::size_t at=24;auto read=[&](){const auto f=b.bef32(at);at+=4;return f;};for(auto& v:camera.eye)v=read();for(auto& v:camera.target)v=read();for(auto& v:camera.up)v=read();camera.fov_y_degrees=read();const f32 aspect=read();camera.near_plane=read();camera.far_plane=read();for(auto& v:position)v=read();for(auto& v:scalars)v=read();for(auto& v:text_y){v=b.be32(at);at+=4;}for(auto& v:records){v=b.be16(at);at+=2;}if(b.be16(at))return false;at+=2;high=b.be32(at);low=b.be32(at+4);
        const std::array<f32,9> expected{.8f,5,20,.5f,10,24,640,448,24};if(scalars!=expected||position!=std::array<f32,3>{0,-52,-400}||text_y!=std::array<u32,3>{320,308,296}||records!=std::array<u16,3>{1,1,0}||camera.eye[0]!=0||camera.eye[1]!=0||camera.eye[2]!=0||camera.target[0]!=0||camera.target[1]!=0||camera.target[2]!=-10||camera.up[0]!=0||camera.up[1]!=1||camera.up[2]!=0||camera.fov_y_degrees!=30||aspect!=4.f/3||camera.near_plane!=10||camera.far_plane!=40000||!camera.basis(nullptr))return false;
        ready=true;return true;
    }
};
} //namespace gf
