// Scoped st42 STIH inlet: byte-locked source grids, copied only into guest RAM.
#pragma once
#include "retail_stationary_contact.hpp"

namespace retail_stage_grid_replay {
using namespace retail_fixture;
inline constexpr std::array<u32,3> extents{215748,126948,36484};
inline constexpr std::array<std::uint64_t,3> fingerprints{
    0x547069DE04E117C1ULL,0x6674DE9DE8754EA1ULL,0x1D83CD4A455BDC6FULL,
};
using Grids=std::array<std::vector<u8>,3>;

inline Grids input(){
    Grids out;
    for(unsigned i=0;i<out.size();++i){
        auto& bytes=out[i];
        bytes.resize(extents[i]);
        if(std::fread(bytes.data(),1,bytes.size(),stdin)!=bytes.size())fail("st42 STIH input extent");
        std::uint64_t fingerprint=14695981039346656037ULL;
        for(auto b:bytes)fingerprint=(fingerprint^b)*1099511628211ULL;
        if(fingerprint!=fingerprints[i])fail("st42 STIH input identity");
    }
    return out;
}

inline void bind(const Module& mod,std::vector<u8>& ram,u32 stage,const Grids& grids,bool empty_secondary){
    namespace contact=retail_stationary_contact;
    const auto at=[&](u32 address){return contact::at(ram,address);};
    const auto word=[&](u32 address){return be32(at(address));};
    const auto store=[&](u32 address,u32 value){put32(at(address),value);};
    constexpr u32 file_base=0x81200000,stride=0x40000,cells=42*42;
    const u32 files=stage+0x800;
    store(0x8043B5A0-21316,stage);store(0x8043B5A0-31820,files);
    for(unsigned i=0;i<grids.size();++i){
        const u32 file=file_base+i*stride;
        const auto& raw=grids[i];
        store(files+4*i,file);
        std::memcpy(at(file),raw.data(),raw.size());
        if(empty_secondary&&i==1){
            const u32 table=8+be32(raw.data()+32);
            for(unsigned j=0;j<cells;++j)store(file+8+be32(raw.data()+table+4*j),0xFFFFFFFF);
        }
    }
    // Declared stage object bounds, not a live stage constructor.
    constexpr std::array<float,5> bounds{9000,7500,-7500,7500,-7500};
    for(unsigned i=0;i<bounds.size();++i)put_float(at(stage+52+4*i),bounds[i]);
    CPUState cpu{};mod.run(cpu,ram,0x8003D4A0,65536);
    for(unsigned i=0;i<grids.size();++i){
        const u32 file=file_base+i*stride;
        const auto& raw=grids[i];
        const u32 table=8+be32(raw.data()+32),triangles=8+be32(raw.data()+36);
        if(word(stage+28+4*i)!=file+8||word(stage+40+4*i)!=raw.size()||
           word(file+32)!=file+table||word(file+36)!=file+triangles||
           std::memcmp(at(file+triangles),raw.data()+triangles,raw.size()-triangles))
            fail("st42 relocation changed a grid extent or triangle bytes");
        for(unsigned j=0;j<cells;++j){
            const u32 list=8+be32(raw.data()+table+4*j);
            if(word(file+table+4*j)!=file+list)fail("st42 relocation changed a cell list");
            for(u32 p=list;p<triangles;p+=4){
                const auto entry=empty_secondary&&i==1?0xFFFFFFFF:be32(raw.data()+p);
                if(word(file+p)!=(entry==0xFFFFFFFF?entry:file+triangles+entry))
                    fail("st42 relocation changed a triangle reference");
                if(entry==0xFFFFFFFF)break;
            }
        }
    }
}
} // namespace retail_stage_grid_replay
