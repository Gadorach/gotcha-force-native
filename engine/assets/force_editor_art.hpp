#pragma once
#include "engine/assets/force_catalog.hpp"
#include "engine/graphics/camera.hpp"
namespace gf {
struct ForceEditorArt {
    Camera camera;f32 aspect=0;
    std::array<f32,2> scroll{},cursor{},grid_origin{},icon_origin{};
    std::array<f32,3> wheel{};
    f32 member_base=0,member_radius=0,cell=0,icon_scale=0;
    std::array<f32,30> offsets{};std::array<s16,30> angles{};
    std::vector<std::pair<u16,u16>> icons;
    bool load(ByteView bytes,const ForceCatalog& catalog,std::string* error){
        *this={};auto fail=[&](){if(error)*error="force editor art: GFED data/domain mismatch";return false;};
        if(!bytes.data||bytes.size<12||bytes.be32(0)!=0x47464544u||bytes.be32(4)!=1||bytes.be32(8)!=catalog.entries().size()||bytes.size!=12+28*4+30*4+30*2+catalog.entries().size()*4)return fail();
        std::size_t at=12;auto read=[&](){const auto f=bytes.bef32(at);at+=4;return f;};
        for(auto& f:camera.eye)f=read();
        for(auto& f:camera.target)f=read();
        for(auto& f:camera.up)f=read();
        camera.fov_y_degrees=read();camera.near_plane=read();camera.far_plane=read();aspect=read();
        for(auto& f:scroll)f=read();
        for(auto& f:cursor)f=read();
        for(auto& f:wheel)f=read();
        member_base=read();member_radius=read();
        for(auto& f:grid_origin)f=read();
        cell=read();
        for(auto& f:icon_origin)f=read();
        icon_scale=read();
        for(auto& f:offsets)f=read();
        for(auto& a:angles){a=std::bit_cast<s16>(bytes.be16(at));at+=2;}
        for(std::size_t i=0;i<catalog.entries().size();++i){const auto model=bytes.be16(at),record=bytes.be16(at+2);at+=4;if(!catalog.find(model)||record>=4096)return fail();for(const auto& p:icons)if(p.first==model)return fail();icons.push_back({model,record});}
        for(std::size_t p=12;p<12+28*4+30*4;p+=4)if(!std::isfinite(bytes.bef32(p))||std::abs(bytes.bef32(p))>1e6f)return fail();
        if(!camera.basis(nullptr)||std::abs(aspect-4.f/3)>0.0001f||cell<=0||icon_scale<=0)return fail();
        if(error)error->clear();
        return true;
    }
    int icon(u16 model)const{for(const auto& p:icons)if(p.first==model)return p.second;return -1;}
};
} // namespace gf
