#pragma once
// STIH collision-grid input. Source3D4A0 offsets are relative to file+8;
// triangle references are byte offsets into 56-byte records, NOT host pointers.
#include <array>
#include <cmath>
#include <span>
#include <string>
#include <vector>
#include "engine/core/types.hpp"
namespace gf {
struct StihTriangle {
    u32 tag=0;std::array<std::array<f32,3>,3> vertices{};std::array<f32,4> plane{};
};
struct StageGroundParameters {
    f32 bounds_min=0,bounds_max=0,cell_size=0,zero=0,secondary_normal_limit=0,high34=0,low68=0;bool declared=false;
};
inline bool stage_ground_parameters_valid(const StageGroundParameters& p){
    auto number=[](f32 v){return std::isfinite(v)&&std::abs(v)<=1e7f;};
    return p.declared&&number(p.bounds_min)&&number(p.bounds_max)&&p.bounds_min<p.bounds_max&&f32_to_bits(p.cell_size)==0x43fa0000&&f32_to_bits(p.zero)==0&&f32_to_bits(p.secondary_normal_limit)==0xbf000000&&number(p.high34)&&number(p.low68)&&p.high34>p.low68;
}
class StihGrid {
public:
    bool load(ByteView bytes,std::string* error){
        *this={};auto fail=[&](const char* why){if(error)*error=std::string("STIH: ")+why;return false;};
        if(!bytes.data||bytes.size<44||bytes.size>(16u<<20)||bytes.be32(0)!=0x53544948||bytes.be32(4)!=bytes.size)return fail("header/extent/budget");
        StihGrid next;next.cell_x_=bytes.be32(8);next.cell_z_=bytes.be32(12);next.nx_=bytes.be32(16);next.nz_=bytes.be32(20);
        next.min_x_=std::bit_cast<s32>(bytes.be32(24));next.min_z_=std::bit_cast<s32>(bytes.be32(28));
        if(!next.cell_x_||next.cell_x_>1000000||!next.cell_z_||next.cell_z_>1000000||!next.nx_||next.nx_>256||!next.nz_||next.nz_>256||std::abs(f64(next.min_x_))>1e7||std::abs(f64(next.min_z_))>1e7)return fail("grid dimensions/domain");
        const std::size_t table=std::size_t(bytes.be32(32))+8,triangles=std::size_t(bytes.be32(36))+8,cells=std::size_t(next.nx_)*next.nz_;
        if(table<40||(table&3)||!bytes.contains(table,cells*4)||triangles<table+cells*4||(triangles&3)||triangles>bytes.size||(bytes.size-triangles)%56)return fail("table/triangle extents");
        const std::size_t count=(bytes.size-triangles)/56;if(count>131072)return fail("triangle budget");next.triangles_.reserve(count);
        for(std::size_t n=0;n<count;++n){const auto at=triangles+n*56;StihTriangle t;t.tag=bytes.be32(at);
            for(unsigned v=0;v<3;++v)for(unsigned k=0;k<3;++k){const auto value=bytes.bef32(at+4+v*12+k*4);if(!number(value))return fail("triangle vertex domain");t.vertices[v][k]=value;}
            for(unsigned k=0;k<4;++k){const auto value=bytes.bef32(at+40+k*4);if(!number(value))return fail("triangle plane domain");t.plane[k]=value;}next.triangles_.push_back(t);
        }
        next.cells_.reserve(cells);std::size_t total=0;
        for(std::size_t n=0;n<cells;++n){std::size_t pointer=std::size_t(bytes.be32(table+n*4))+8;std::vector<u32> refs;
            for(unsigned budget=0;;++budget){if(budget>4096||pointer<table+cells*4||(pointer&3)||pointer>triangles||triangles-pointer<4)return fail("cell list extent/sentinel/budget");
                const u32 value=bytes.be32(pointer);pointer+=4;if(value==0xffffffffu)break;
                if(value%56||value/56>=count||++total>1048576)return fail("cell triangle reference/budget");
                refs.push_back(value/56);
            }next.cells_.push_back(std::move(refs));
        }
        next.loaded_=true;*this=std::move(next);if(error)error->clear();return true;
    }
    bool loaded()const{return loaded_;}
    u32 cells_x()const{return nx_;}u32 cells_z()const{return nz_;}u32 cell_size_x()const{return cell_x_;}u32 cell_size_z()const{return cell_z_;}
    s32 minimum_x()const{return min_x_;}s32 minimum_z()const{return min_z_;}
    std::span<const StihTriangle> triangles()const{return triangles_;}
    std::span<const u32> cell(unsigned x,unsigned z)const{return loaded_&&x<nx_&&z<nz_?std::span<const u32>(cells_[std::size_t(x)*nz_+z]):std::span<const u32>{};}
private:
    static bool number(f32 v){return std::isfinite(v)&&std::abs(v)<=1e7f;}
    bool loaded_=false;u32 cell_x_=0,cell_z_=0,nx_=0,nz_=0;s32 min_x_=0,min_z_=0;
    std::vector<StihTriangle> triangles_;std::vector<std::vector<u32>> cells_;
};
} // namespace gf
