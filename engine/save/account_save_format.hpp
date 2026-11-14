#pragma once
#include <array>
#include "engine/core/account_save_storage.hpp"
#include "engine/save/save_engine.hpp"

namespace gf {

struct AccountSaveDescription {
    AccountSaveKey key;std::string name;bool retired=false;
    bool operator==(const AccountSaveDescription&)const=default;
};
struct AccountSaveRecords {std::array<std::vector<u8>,3> records;};

namespace account_save_detail {
inline constexpr u8 empty_byte=0;
//Native metadata policy, not a source-game name limit. Strict UTF-8, no control
//characters. The UI separately checks whether its font can display a name.
inline bool name_valid(const std::string& name){
    if(name.empty()||name.size()>64)return false;
    for(std::size_t i=0;i<name.size();){
        const auto c=u8(name[i++]);u32 code=0,minimum=0;unsigned continuation=0;
        if(c<0x80)code=c;
        else if(c>=0xc2&&c<=0xdf){code=c&0x1fu;minimum=0x80;continuation=1;}
        else if(c>=0xe0&&c<=0xef){code=c&0xfu;minimum=0x800;continuation=2;}
        else if(c>=0xf0&&c<=0xf4){code=c&7u;minimum=0x10000;continuation=3;}
        else return false;
        if(continuation>name.size()-i)return false;
        for(unsigned n=0;n<continuation;++n){const auto next=u8(name[i++]);if((next&0xc0u)!=0x80u)return false;code=(code<<6)|(next&0x3fu);}
        if(code<minimum||code>0x10ffffu||(code>=0xd800&&code<=0xdfff)||code<32||(code>=0x7f&&code<=0x9f))return false;
    }return true;
}
//Reuse the existing SaveEngine framing, never maintain a second checksum/codec.
struct Framing final:ISaveBackend {
    std::vector<u8> bytes;
    bool load(u32,std::vector<u8>&)override{return false;}
    bool store(u32,const u8* data,std::size_t size)override{if(!data||size>kAccountObjectBytes)return false;bytes.assign(data,data+size);return true;}
    bool erase(u32)override{return false;}
};
inline bool framed(const std::vector<u8>& bytes){Framing sink;SaveEngine codec(sink);std::vector<u8> empty;SaveReader reader(empty);return codec.read_record(bytes,reader);}
inline bool allowed(AccountSaveKey key,unsigned slot){return key.save?(slot==0||slot==2):slot==1;}
}

inline bool account_description_encode(const AccountSaveDescription& value,std::vector<u8>* out){
    if(!out)return false;
    out->clear();if(!value.key.valid()||!account_save_detail::name_valid(value.name))return false;
    SaveWriter writer;writer.u32_field(0x47464143u);writer.u32_field(1);writer.u64_field(value.key.account);writer.u64_field(value.key.save);writer.u32_field(value.retired?1:0);writer.blob(reinterpret_cast<const u8*>(value.name.data()),value.name.size());
    account_save_detail::Framing sink;SaveEngine codec(sink);if(!codec.write(0,writer)||sink.bytes.size()>kAccountMetadataBytes)return false;*out=std::move(sink.bytes);return true;
}
inline bool account_description_decode(const std::vector<u8>& bytes,AccountSaveKey expected,AccountSaveDescription* out){
    if(out)*out={};
    if(!out||!expected.valid()||bytes.size()>kAccountMetadataBytes)return false;
    account_save_detail::Framing sink;SaveEngine codec(sink);std::vector<u8> empty;SaveReader reader(empty);
    if(!codec.read_record(bytes,reader)||reader.u32_field()!=0x47464143u||reader.u32_field()!=1)return false;
    AccountSaveDescription next;next.key={reader.u64_field(),reader.u64_field()};const auto flags=reader.u32_field();const auto name=reader.blob();
    if(!reader.consumed()||next.key!=expected||flags>1||name.empty()||name.size()>64)return false;
    next.name.assign(reinterpret_cast<const char*>(name.data()),name.size());next.retired=flags!=0;if(!account_save_detail::name_valid(next.name))return false;*out=std::move(next);return true;
}
inline bool account_records_encode(AccountSaveKey key,const AccountSaveRecords& value,std::vector<u8>* out){
    if(!out)return false;
    out->clear();if(!key.valid())return false;
    SaveWriter writer;writer.u32_field(0x47464152u);writer.u32_field(1);writer.u64_field(key.account);writer.u64_field(key.save);
    std::size_t total=0;for(unsigned slot=0;slot<3;++slot){const auto& record=value.records[slot];if((!account_save_detail::allowed(key,slot)&&!record.empty())||record.size()>kAccountObjectBytes-total||(!record.empty()&&!account_save_detail::framed(record)))return false;total+=record.size();writer.blob(record.empty()?&account_save_detail::empty_byte:record.data(),record.size());}
    account_save_detail::Framing sink;SaveEngine codec(sink);if(!codec.write(0,writer)||sink.bytes.size()>kAccountObjectBytes)return false;*out=std::move(sink.bytes);return true;
}
inline bool account_records_decode(const std::vector<u8>& bytes,AccountSaveKey expected,AccountSaveRecords* out){
    if(out)*out={};
    if(!out||!expected.valid()||bytes.size()>kAccountObjectBytes)return false;
    account_save_detail::Framing sink;SaveEngine codec(sink);std::vector<u8> empty;SaveReader reader(empty);
    if(!codec.read_record(bytes,reader)||reader.u32_field()!=0x47464152u||reader.u32_field()!=1||reader.u64_field()!=expected.account||reader.u64_field()!=expected.save)return false;
    AccountSaveRecords next;for(unsigned slot=0;slot<3;++slot){next.records[slot]=reader.blob();if((!account_save_detail::allowed(expected,slot)&&!next.records[slot].empty())||(!next.records[slot].empty()&&!account_save_detail::framed(next.records[slot])))return false;}
    if(!reader.consumed())return false;
    *out=std::move(next);return true;
}

} //namespace gf
