#pragma once
#include "engine/gameplay/stage_ground.hpp"
namespace stage_ground_fixture {
inline gf::StageGroundParameters parameters(){return {-7500,7500,500,0,-.5f,5000,-1540,true};}
inline gf::StihTriangle triangle(float y,bool ceiling=false){
    gf::StihTriangle t;t.tag=0xcccccccc;t.vertices={{{-1000,y,-1000},{-1000,y,1000},{1000,y,-1000}}};t.plane={0,1,0,-y};
    if(ceiling){std::swap(t.vertices[1],t.vertices[2]);t.plane={0,-1,0,y};}return t;
}
inline std::vector<gf::u8> bytes(const std::vector<gf::StihTriangle>& triangles,gf::u32 nx=4,gf::u32 nz=4){
    using namespace gf;const u32 table=40,list=table+nx*nz*4,data=list+u32(triangles.size()+1)*4;std::vector<u8> b(data+triangles.size()*56);
    store_be32(b.data(),0x53544948);store_be32(b.data()+4,b.size());store_be32(b.data()+8,500);store_be32(b.data()+12,500);store_be32(b.data()+16,nx);store_be32(b.data()+20,nz);store_be32(b.data()+24,u32(-1000));store_be32(b.data()+28,u32(-1000));store_be32(b.data()+32,table-8);store_be32(b.data()+36,data-8);
    for(unsigned n=0;n<nx*nz;++n)store_be32(b.data()+table+n*4,list-8);
    for(unsigned n=0;n<triangles.size();++n){store_be32(b.data()+list+n*4,n*56);const auto& t=triangles[n];const auto at=data+n*56;store_be32(b.data()+at,t.tag);for(unsigned v=0;v<3;++v)for(unsigned k=0;k<3;++k)store_be_f32(b.data()+at+4+v*12+k*4,t.vertices[v][k]);for(unsigned k=0;k<4;++k)store_be_f32(b.data()+at+40+k*4,t.plane[k]);}
    store_be32(b.data()+list+triangles.size()*4,0xffffffff);return b;
}
} // namespace stage_ground_fixture
