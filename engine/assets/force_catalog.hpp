#pragma once
// Native, owned Borg metadata exported offline. No DOL/guest pointers at runtime.
// Costs/rarities: 80066168/662C0; ordering/tribe: 800661D8/66298.
#include <algorithm>
#include <array>
#include <bit>
#include <span>
#include <string>
#include <vector>
#include "engine/core/types.hpp"
namespace gf {
struct ForceBorgMetadata {
    u16 model=0,number=0,tribe=0;
    std::array<s16,6> cost{},rarity{};
    std::array<s16,9> experience_thresholds{};
    std::string source_name; // Original encoded bytes, also used for strcmp order.
};
struct ForceCollectionPair {u16 first=0,second=0,result=0;};
class ForceCatalog {
public:
    bool assign(std::vector<ForceBorgMetadata> entries,std::vector<u16> starters,
                std::vector<ForceCollectionPair> pairs,std::string starter_name,
                std::string unnamed_name,std::string owner_name,u16 letter_byte,
                std::string* error){
        ready_=false;
        auto fail=[&](const char* why){if(error)*error=std::string("force catalog: ")+why;return false;};
        auto text=[](const std::string& x,unsigned limit){return !x.empty()&&x.size()<=limit&&x.find('\0')==std::string::npos;};
        if(entries.empty()||entries.size()>256||starters.empty()||starters.size()>200||pairs.size()>32||
           !text(starter_name,40)||!text(unnamed_name,40)||!text(owner_name,15)||letter_byte>=unnamed_name.size()||u8(unnamed_name[letter_byte])>236)return fail("counts/name domain");
        std::sort(entries.begin(),entries.end(),[](const auto& a,const auto& b){return a.model<b.model;});
        std::array<bool,256> numbers{};
        for(unsigned i=0;i<entries.size();++i){const auto& x=entries[i];
            if(x.model>=0x8000||x.number>=256||x.tribe>=32||!text(x.source_name,96)||numbers[x.number]||(i&&entries[i-1].model==x.model))return fail("model/number/tribe/name identity");
            numbers[x.number]=true;for(auto v:x.cost)if(v<0)return fail("negative source cost");
        }
        auto contains=[&](u16 id){return std::any_of(entries.begin(),entries.end(),[&](const auto& x){return x.model==id;});};
        for(auto id:starters)if(!contains(id))return fail("starter model absent");
        for(const auto& p:pairs)if(p.first==p.second||p.first==p.result||p.second==p.result||!contains(p.first)||!contains(p.second)||!contains(p.result))return fail("collection relation identity");
        entries_=std::move(entries);starters_=std::move(starters);pairs_=std::move(pairs);starter_name_=std::move(starter_name);unnamed_name_=std::move(unnamed_name);owner_name_=std::move(owner_name);letter_byte_=letter_byte;
        fingerprint_=2166136261u;
        auto byte=[&](u8 value){fingerprint_=(fingerprint_^value)*16777619u;};
        auto word=[&](u32 value){for(unsigned i=0;i<4;++i)byte(u8(value>>(24-i*8)));};
        auto name=[&](const std::string& value){word(u32(value.size()));for(unsigned char c:value)byte(c);};
        word(u32(entries_.size()));for(const auto& x:entries_){word(x.model);word(x.number);word(x.tribe);for(auto v:x.cost)word(u16(v));for(auto v:x.rarity)word(u16(v));for(auto v:x.experience_thresholds)word(u16(v));name(x.source_name);}
        word(u32(starters_.size()));for(auto id:starters_)word(id);word(u32(pairs_.size()));for(const auto& p:pairs_){word(p.first);word(p.second);word(p.result);}name(starter_name_);name(unnamed_name_);name(owner_name_);word(letter_byte_);
        ready_=true;if(error)error->clear();return true;
    }
    bool load(ByteView data,std::string* error){
        ready_=false;
        auto fail=[&](const char* why){if(error)*error=std::string("force catalog: ")+why;return false;};
        if(!data.data||data.size<32||data.size>(128u<<10)||data.be32(0)!=0x47464243u||data.be32(4)!=1||data.be16(14)||data.be32(28)!=data.size-32)return fail("GFBC header/extent");
        const auto count=data.be16(8),starters=data.be16(10),pairs=data.be16(12);
        if(!count||count>256||!starters||starters>200||pairs>32)return fail("GFBC counts");
        std::size_t at=32;std::vector<ForceBorgMetadata> records;
        for(unsigned i=0;i<count;++i){
            if(!data.contains(at,50))return fail("short metadata row");
            ForceBorgMetadata r;r.model=data.be16(at);r.number=data.be16(at+2);r.tribe=data.be16(at+4);const auto length=data.be16(at+6);
            for(unsigned k=0;k<6;++k){r.cost[k]=std::bit_cast<s16>(data.be16(at+8+k*2));r.rarity[k]=std::bit_cast<s16>(data.be16(at+20+k*2));}
            for(unsigned k=0;k<9;++k)r.experience_thresholds[k]=std::bit_cast<s16>(data.be16(at+32+k*2));
            at+=50;if(!length||length>96||!data.contains(at,length))return fail("short model name");
            r.source_name.assign(reinterpret_cast<const char*>(data.data+at),length);at+=length;records.push_back(std::move(r));
        }
        if(!data.contains(at,std::size_t(starters)*2+std::size_t(pairs)*6))return fail("short startup/collection data");
        std::vector<u16> initial;std::vector<ForceCollectionPair> relations;
        for(unsigned i=0;i<starters;++i){initial.push_back(data.be16(at));at+=2;}
        for(unsigned i=0;i<pairs;++i){relations.push_back({data.be16(at),data.be16(at+2),data.be16(at+4)});at+=6;}
        std::array<std::string,3> names;
        for(unsigned i=0;i<3;++i){const auto length=data.be16(16+i*2);if(!length||length>40||!data.contains(at,length))return fail("short default name");names[i].assign(reinterpret_cast<const char*>(data.data+at),length);at+=length;}
        if(at!=data.size)return fail("trailing catalog data");
        return assign(std::move(records),std::move(initial),std::move(relations),std::move(names[0]),std::move(names[1]),std::move(names[2]),data.be16(22),error);
    }
    bool ready()const{return ready_;}
    u32 fingerprint()const{return fingerprint_;} // Consistency key, not authentication.
    const ForceBorgMetadata* find(u16 model)const{
        if(!ready_)return nullptr;
        const auto it=std::lower_bound(entries_.begin(),entries_.end(),model,[](const auto& x,u16 id){return x.model<id;});
        return it!=entries_.end()&&it->model==model?&*it:nullptr;
    }
    std::span<const ForceBorgMetadata> entries()const{return entries_;}
    std::span<const u16> starters()const{return starters_;}
    std::span<const ForceCollectionPair> collection_pairs()const{return pairs_;}
    const std::string& starter_name()const{return starter_name_;}
    const std::string& unnamed_name()const{return unnamed_name_;}
    const std::string& owner_name()const{return owner_name_;}
    u16 letter_byte()const{return letter_byte_;}
private:
    bool ready_=false;u16 letter_byte_=0;u32 fingerprint_=0;
    std::vector<ForceBorgMetadata> entries_;std::vector<u16> starters_;std::vector<ForceCollectionPair> pairs_;
    std::string starter_name_,unnamed_name_,owner_name_;
};
} // namespace gf
