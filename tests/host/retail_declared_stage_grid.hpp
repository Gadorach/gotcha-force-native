// One declared STIH wall, or an empty grid, through the original relocator.
#pragma once
#include "retail_stationary_contact.hpp"

namespace retail_declared_stage_grid {
using namespace retail_fixture;

inline void bind(const Module& mod,std::vector<u8>& ram,u32 stage,bool wall,float plane_x){
    const auto at=[&](u32 address){return retail_stationary_contact::at(ram,address);};
    const auto store=[&](u32 address,u32 value){put32(at(address),value);};
    const u32 files=stage+0x800,grids=stage+0x1000;
    store(0x8043B5A0-21316,stage);store(0x8043B5A0-31820,files);
    for(unsigned role=0;role<3;++role){
        const bool has_wall=wall&&role==1;
        // Separate reference lists: the original relocator edits each in place.
        const u32 file=grids+role*0x1000,table=file+40,refs=table+4*64;
        const unsigned list_bytes=has_wall?8:4;
        const u32 triangle=refs+list_bytes*64;
        constexpr std::array<u32,6> fields{500,500,8,8,u32(-1000),u32(-1000)};
        store(file,0x53544948);store(file+4,triangle+(has_wall?56u:0)-file);
        for(unsigned i=0;i<fields.size();++i)store(file+8+4*i,fields[i]);
        store(file+32,table-file-8);store(file+36,triangle-file-8);
        for(unsigned i=0;i<64;++i){
            const auto ref=refs+list_bytes*i;
            store(table+4*i,ref-file-8);
            store(ref,has_wall?0:0xFFFFFFFF);
            if(has_wall)store(ref+4,0xFFFFFFFF);
        }
        if(has_wall){
            const std::array<float,13> raw{plane_x,-200,-200,plane_x,600,-200,plane_x,-200,600,1,0,0,-plane_x};
            for(unsigned i=0;i<raw.size();++i)put_float(at(triangle+4+4*i),raw[i]);
        }
        store(files+4*role,file);
    }
    for(unsigned i=0;i<5;++i)put_float(at(stage+52+4*i),i==2||i==4?-1000.f:3000.f);
    CPUState cpu{};mod.run(cpu,ram,0x8003D4A0,65536);
    for(unsigned i=0;i<3;++i){
        const auto file=be32(at(files+4*i));
        if(be32(at(stage+28+4*i))!=file+8||be32(at(file+32))!=file+40||
           be32(at(be32(at(file+40))))!=(wall&&i==1?be32(at(file+36)):0xFFFFFFFF))
            fail("declared stage grid did not relocate through the original reader");
    }
}
} // namespace retail_declared_stage_grid
