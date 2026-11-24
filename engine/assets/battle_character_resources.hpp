#pragma once
#include <memory>
#include "engine/assets/force_catalog.hpp"
#include "engine/assets/challenge_finalization.hpp"
namespace gf {
struct BattleCharacterResourceRow {u16 model=0,number=0;u32 member=0,bytes=0,hash=0;u16 slots=0,actor_bytes=0;u8 variants=0;};
//410BC selects NORMAL battle packages via family/local tables802D0584; this is
//not the distinct DP startup table. Profile identity is consistency, not auth.
class BattleCharacterResourceProfile {
public:
    bool load(ByteView b,const ForceCatalog& catalog,std::string* error){
        *this={};auto fail=[&](){if(error)*error="battle character resource profile: extent/catalog/domain";return false;};
        if(!catalog.ready()||!b.data||b.size<32||b.size>32+256*24||b.be32(0)!=0x4746434bu||b.be32(4)!=1||!b.be16(8)||b.be16(8)>catalog.entries().size()||b.be16(10)!=24||b.be32(12)!=b.size-32||b.size!=32+std::size_t(b.be16(8))*24)return fail();
        for(unsigned i=16;i<32;++i)if(b.data[i])return fail();
        std::vector<BattleCharacterResourceRow> rows;
        for(unsigned i=0;i<b.be16(8);++i){const unsigned at=32+i*24;BattleCharacterResourceRow r{b.be16(at),b.be16(at+2),b.be32(at+4),b.be32(at+8),b.be32(at+12),b.be16(at+16),b.be16(at+20),b.data[at+18]};const auto* meta=catalog.find(r.model);
            if(!meta||meta->number!=r.number||r.member>65535||r.bytes<96||r.bytes>(32u<<20)||r.slots<10||r.slots>32||r.variants!=6||r.actor_bytes!=432||b.data[at+19]||b.be16(at+22)||(i&&rows.back().model>=r.model))return fail();
            rows.push_back(r);}
        rows_=std::move(rows);fingerprint_=catalog.fingerprint();if(error)error->clear();return true;
    }
    bool for_catalog(const ForceCatalog& c)const{return !rows_.empty()&&c.ready()&&c.fingerprint()==fingerprint_;}
    const BattleCharacterResourceRow* find(u16 model)const{for(const auto& r:rows_)if(r.model==model)return &r;return nullptr;}
    std::span<const BattleCharacterResourceRow> rows()const{return rows_;}
private:std::vector<BattleCharacterResourceRow> rows_;u32 fingerprint_=0;
};
//Owns immutable native bytes corresponding to412C4's published slot data and an
//allocated descriptor anchor. It does NOT run419EC, decode drawable HSD/motions,
//bind class callbacks, initialize an actor, or mint a full constructor receipt.
class BattleCharacterResource {
public:
    bool load(const BattleCharacterResourceRow& row,u8 variant,ByteView bundle,std::string* error){
        *this={};auto fail=[&](const char* why){if(error)*error=std::string("battle character resource: ")+why;return false;};
        if(variant>=row.variants||row.variants!=6||row.actor_bytes!=432||bundle.size!=row.bytes||!challenge_native_bundle_valid(bundle)||bundle.be32(8)!=row.slots||row.slots<10||row.slots>32||challenge_payload_hash(bundle)!=row.hash)return fail("native identity/variant/slot extent");
        std::array<Range,4> ranges{};std::array<Range,32> blocks{};std::size_t at=16+8*row.slots;
        for(unsigned i=0;i<row.slots;++i){const std::size_t n=bundle.be32(20+8*i);blocks[i]={at,n};for(unsigned role=0;role<4;++role)if(i==(role==0?0u:role==1?2u:role==2?3u:4u+variant))ranges[role]={at,n};at+=n;}
        if(ranges[0].size<row.actor_bytes||(ranges[2].size&&ranges[2].size<64)||ranges[3].size<36)return fail("character/motion/model role extent");
        const ByteView wrapper{bundle.data+ranges[3].at,ranges[3].size};const u32 model_at=wrapper.be32(0);
        if(model_at<4||(model_at&3)||!wrapper.contains(model_at,32))return fail("declared model wrapper offset");
        const u32 model_bytes=wrapper.be32(model_at);
        if(model_bytes<32||model_bytes>(16u<<20)||!wrapper.contains(model_at,model_bytes))return fail("declared HSD extent");
        bytes_=std::make_shared<const std::vector<u8>>(bundle.data,bundle.data+bundle.size);row_=row;variant_=variant;ranges_=ranges;blocks_=blocks;model_at_=model_at;model_bytes_=model_bytes;ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}const BattleCharacterResourceRow& selection()const{return row_;}u8 variant()const{return variant_;}
    ByteView actor_data()const{return role(0,row_.actor_bytes);}ByteView battle_data()const{return role(1);}ByteView motion_bank()const{return role(2);}ByteView model_wrapper()const{return role(3);}
    //An actual empty source slot is absent DATA, not a default clip/VM capability.
    bool motion_present()const{return ready_&&ranges_[2].size!=0;}
    ByteView model_prefix()const{const auto b=role(3);return b.data?ByteView{b.data,model_at_}:ByteView{};}
    ByteView model_hsd()const{const auto b=role(3);return b.data?ByteView{b.data+model_at_,model_bytes_}:ByteView{};}
    std::size_t bytes()const{return bytes_?bytes_->size():0;}
    //Immutable decoded block extent. A zero-length block is absent source data,
    //not a destination allocation or a successfully published resource pointer.
    ByteView block(unsigned index)const{if(!ready_||!bytes_||index>=row_.slots||!blocks_[index].size)return {};const auto& r=blocks_[index];return {bytes_->data()+r.at,r.size};}
private:
    struct Range {std::size_t at=0,size=0;};
    ByteView role(unsigned index,std::size_t limit=0)const{if(!ready_||!bytes_||index>=4)return {};const auto& r=ranges_[index];return {bytes_->data()+r.at,limit?limit:r.size};}
    std::shared_ptr<const std::vector<u8>> bytes_;std::array<Range,4> ranges_{};std::array<Range,32> blocks_{};BattleCharacterResourceRow row_;u32 model_at_=0,model_bytes_=0;u8 variant_=0;bool ready_=false;
};
} // namespace gf
