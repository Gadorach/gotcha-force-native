#pragma once
#include <array>
#include <bit>
#include "engine/core/types.hpp"
namespace gf {
struct SourceAdxControlProfile {
    std::array<u32,4> lengths{};std::array<std::array<f32,5>,3> gains{};std::array<s32,128> volume{};f32 full=0,half=0;bool loaded=false;
    bool valid()const{if(!loaded||lengths!=std::array<u32,4>{4,2,2,5}||full!=127||half!=.5f)return false;for(const auto& row:gains)if(row!=std::array<f32,5>{.6f,.7f,.8f,.9f,1})return false;for(unsigned i=0;i<volume.size();++i)if(volume[i]<-9600||volume[i]>0||(i&&volume[i]<volume[i-1]))return false;return volume[0]==-9600&&volume[127]==0;}
    bool load(ByteView b){*this={};if(!b.data||b.size!=608||b.be32(0)!=0x47464143||b.be32(4)!=1||b.be16(8)!=3||b.be16(10)!=4)return false;SourceAdxControlProfile p;std::size_t at=12;for(auto& n:p.lengths){n=b.be32(at);at+=4;}for(auto& row:p.gains)for(auto& v:row){v=b.bef32(at);at+=4;}for(auto& v:p.volume){v=std::bit_cast<s32>(b.be32(at));at+=4;}p.full=b.bef32(at);p.half=b.bef32(at+4);p.loaded=true;if(!p.valid())return false;*this=p;return true;}
};
} //namespace gf
