#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <span>
#include <string>
#include <vector>
#include "engine/core/types.hpp"
namespace gf {
enum class CharacterCollisionTable {Body,Attack};
struct CharacterCollisionDescriptor {
    std::array<u8,80> source{};u8 shape=0,binding1=0;u16 flags2=0,parameter4=0,field0a=0;s16 start6=0,end8=0;std::array<f32,3> local_point{};bool declared=false;
    f32 scalar(unsigned offset)const{return offset<=76&&!(offset&3)?ByteView{source.data(),source.size()}.bef32(offset):0.f;}
};
//Normal character slot2:8AB30/8AC80 use signed header IDs and80-byte descriptors.
//Unused descriptors/tails remain RAW; only an explicitly requested descriptor is
//validated as a numeric shape input. No collider allocation/lifetime/actor ACK.
class CharacterCollisionBank {
public:
    bool load(ByteView bytes,std::string* error){
        *this={};auto fail=[&](const char* why){if(error)*error=std::string("character collision bank: ")+why;return false;};
        if(!bytes.data||bytes.size!=7840)return fail("exact normal slot2 extent required");
        auto list=[&](unsigned at,unsigned limit,unsigned entries,std::vector<u8>* out){for(unsigned i=0;i<limit;++i){const u8 id=bytes.data[at+i];if(id&0x80u)break;if(id>=entries)return false;out->push_back(id);}return true;};
        CharacterCollisionBank next;if(!list(0,32,32,&next.body_))return fail("body signed ID outside32 descriptors");
        for(unsigned group=0;group<32;++group)if(!list(0xa20+4*group,4,64,&next.attacks_[group]))return fail("attack signed ID outside64 descriptors");
        std::copy_n(bytes.data,bytes.size,next.source_.begin());next.loaded_=true;*this=std::move(next);if(error)error->clear();return true;
    }
    bool ready()const{return loaded_;}std::span<const u8> body_ids()const{return body_;}std::span<const u8> attack_ids(unsigned group)const{return loaded_&&group<32?std::span<const u8>(attacks_[group]):std::span<const u8>{};}
    ByteView source_record(CharacterCollisionTable table,unsigned index)const{if(!loaded_||(table!=CharacterCollisionTable::Body&&table!=CharacterCollisionTable::Attack)||index>=(table==CharacterCollisionTable::Body?32u:64u))return {};return {source_.data()+(table==CharacterCollisionTable::Body?0x20u:0xaa0u)+80*index,80};}
    bool descriptor(CharacterCollisionTable table,unsigned index,CharacterCollisionDescriptor* out,std::string* error)const{
        if(out)*out={};
        auto fail=[&](const char* why){if(error)*error=std::string("character collision descriptor: ")+why;return false;};const auto b=source_record(table,index);if(!out||!b.data)return fail("loaded source record required");
        CharacterCollisionDescriptor d;d.shape=b.data[0];d.binding1=b.data[1];d.flags2=b.be16(2);d.parameter4=b.be16(4);d.start6=std::bit_cast<s16>(b.be16(6));d.end8=std::bit_cast<s16>(b.be16(8));d.field0a=b.be16(10);if(d.shape>=6)return fail("source dispatch type outside six captured handlers");
        for(unsigned i=0;i<3;++i){d.local_point[i]=b.bef32(12+4*i);if(!std::isfinite(d.local_point[i])||std::abs(d.local_point[i])>1e7f)return fail("local point numeric domain");}
        std::copy_n(b.data,80,d.source.begin());d.declared=true;*out=d;if(error)error->clear();return true;
    }
private:std::array<u8,7840> source_{};std::vector<u8> body_;std::array<std::vector<u8>,32> attacks_;bool loaded_=false;
};
struct ColliderGeometryProfile {std::array<u32,8> words{};bool loaded=false;f32 value(unsigned i)const{return i<8?bits_to_f32(words[i]):0.f;}};
inline bool collider_geometry_profile_valid(const ColliderGeometryProfile& p){return p.loaded&&p.words==std::array<u32,8>{0,0x3f800000u,0x42480000u,0x3c8efa35u,0x42652ee1u,0x3fc90fdbu,0x3f060a92u,0x3c23d70au};}
inline bool collider_geometry_profile_load(ByteView b,ColliderGeometryProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](){if(error)*error="collider geometry profile: extent/source operands";return false;};if(!out||!b.data||b.size!=48||b.be32(0)!=0x47464359u||b.be32(4)!=1||b.be32(8)!=48||b.be32(12))return fail();ColliderGeometryProfile p;p.loaded=true;for(unsigned i=0;i<8;++i)p.words[i]=b.be32(16+4*i);if(!collider_geometry_profile_valid(p))return fail();*out=p;if(error)error->clear();return true;
}
} // namespace gf
