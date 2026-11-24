#pragma once
#include "engine/core/types.hpp"
#include <string>
namespace gf {
//4394C→439A0(1): unitall_mdl.arc, resource slot1. Source offsets are
//provenance only; no native arena or host-pointer-as-guest-word representation.
class BattleCommonUiProfile {
public:
    bool load(ByteView b,std::string* error){
        *this={};
        if(!b.data||b.size!=40||b.be32(0)!=0x47465549||b.be32(4)!=1||b.be32(8)!=40||b.be32(12)!=2497||b.be32(16)<32||b.be32(16)>(8u<<20)||!b.be32(20)||b.be32(24)!=1||b.be32(28)!=0x944400||b.be32(32)!=0x700000||b.be32(36)){if(error)*error="battle common UI: source recipe/extent";return false;}
        bytes_=b.be32(16);hash_=b.be32(20);ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}u32 bytes()const{return bytes_;}u32 hash()const{return hash_;}
private:u32 bytes_=0,hash_=0;bool ready_=false;
};
} // namespace gf
