#pragma once
// Owned explicit model-bound actor sound-event rows; no runtime DOL pointers.
// Sparse audited script entries only, not a fabricated whole event-table extent.
#include <memory>
#include <algorithm>
#include <vector>
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
struct ActorSoundScript {u8 id=0;std::vector<std::array<u8,8>> rows;};
class ActorSoundEventBank {
public:
    bool load(ByteView bytes,u32 model_hash,std::string* error){
        data_.reset();auto fail=[&](const char* why){if(error)*error=std::string("actor sound events: ")+why;return false;};
        if(!bytes.contains(0,36)||bytes.size>4096||bytes.be32(0)!=0x47465345u||bytes.be16(4)!=1||!bytes.be16(6)||bytes.be16(6)>16||bytes.be32(8)!=bytes.size||!model_hash||bytes.be32(12)!=model_hash||bytes.be16(18)||bytes.be32(20)||bytes.be32(24)!=0x3e4ccccdu||bytes.be32(28)!=0x3f333333u||bytes.be32(32)!=0x40000000u)return fail("GFSE/profile/model binding/scalar operands");
        auto next=std::make_shared<Data>();next->hash=model_hash;next->model=bytes.be16(16);std::size_t at=36;
        for(unsigned i=0;i<bytes.be16(6);++i){if(!bytes.contains(at,4))return fail("script descriptor");ActorSoundScript script;script.id=bytes.data[at];const unsigned count=bytes.data[at+1];if(script.id>127||!count||count>16||bytes.be16(at+2)||!bytes.contains(at+4,count*8))return fail("script identity/extent/native row budget");for(const auto& prior:next->scripts)if(prior.id==script.id)return fail("duplicate script");at+=4;
            for(unsigned row=0;row<count;++row){std::array<u8,8> value;std::copy_n(bytes.data+at,8,value.begin());at+=8;if(value[2]>1||value[3]>4||(value[3]<4&&load_be16(value.data()+4)>=384)||(value[6]==255)!=(row+1==count))return fail("supported clock/sound branch/explicit terminal");script.rows.push_back(value);}next->scripts.push_back(std::move(script));}
        if(at!=bytes.size)return fail("trailing data");
        data_=std::move(next);if(error)error->clear();return true;
    }
    bool ready()const{return bool(data_);}u32 model_hash()const{return data_?data_->hash:0;}u16 model_id()const{return data_?data_->model:0;}
    const ActorSoundScript* find(u8 id)const{if(data_)for(const auto& script:data_->scripts)if(script.id==id)return &script;return nullptr;}
private:
    struct Data {u32 hash=0;u16 model=0;std::vector<ActorSoundScript> scripts;};std::shared_ptr<const Data> data_;
};
} // namespace gf
