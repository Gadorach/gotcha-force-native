#pragma once
#include "tests/unit/startup_actor_frame_fixture.hpp"
#include "engine/gameplay/startup_actor_attachments.hpp"
namespace startup_attachments_fixture {
inline std::vector<gf::u8> profile(gf::u32 hash,bool red=true){
    using namespace gf;const unsigned modes=red?1:3;std::vector<u8> b(36+2*(8+24*modes));store_be32(b.data(),0x47464154);store_be16(b.data()+4,1);store_be16(b.data()+6,2);store_be32(b.data()+8,b.size());store_be32(b.data()+12,hash);store_be16(b.data()+16,red?0x615:10);b[18]=red;b[19]=u8(modes);store_be_f32(b.data()+24,1);store_be_f32(b.data()+32,.1f);
    for(unsigned i=0;i<2;++i){const auto at=36+i*(8+24*modes);b[at]=u8(red?24+i:6+i);b[at+1]=red?0x41:0x4b;store_be16(b.data()+at+2,u16(i+1));b[at+4]=red?u8(i+1):0;
        for(unsigned mode=0;mode<modes;++mode){const auto pos=at+8+mode*24;b[pos]=u8(i);store_be_f32(b.data()+pos+8,float(2*i+mode));store_be_f32(b.data()+pos+12,1);}
    }
    return b;
}
struct Fixture {
    startup_actor_frame_fixture::Fixture frame;std::vector<gf::u8> attachments;
    bool load(std::string* error){
        if(!frame.load(error))return false;
        // Preserve the owned two-bone body, add two distinct rigid published
        // record roots. The root SRT is intentionally NOT the supplied world.
        auto old=frame.motion.model;const unsigned data_size=gf::load_be32(old.data()+4),relocs=gf::load_be32(old.data()+8);std::vector<gf::u8> block(old.begin()+32,old.begin()+32+data_size);
        std::vector<gf::u32> fields;for(unsigned i=0;i<relocs;++i)fields.push_back(gf::load_be32(old.data()+32+data_size+i*4));
        auto ptr=[&](unsigned at,unsigned value){gf::store_be32(block.data()+at,value);fields.push_back(at);};
        ptr(0x24,0x600);ptr(0x28,0x610);ptr(0x600,0x640);ptr(0x610,0x680);
        for(unsigned at:{0x640,0x680}){gf::store_be32(block.data()+at+4,0x10050180);ptr(at+16,0x400);for(unsigned i=0;i<3;++i)gf::store_be_f32(block.data()+at+0x20+i*4,1);gf::store_be_f32(block.data()+at+0x2c,999);}
        std::vector<gf::u8> bank(32+block.size());std::copy(block.begin(),block.end(),bank.begin()+32);for(auto f:fields){const auto pos=bank.size();bank.resize(pos+4);gf::store_be32(bank.data()+pos,f);}
        const auto symbol=bank.size();bank.resize(symbol+8);gf::store_be32(bank.data()+symbol,0x10);const char name[]="scene_data";bank.insert(bank.end(),name,name+sizeof(name));gf::store_be32(bank.data(),bank.size());gf::store_be32(bank.data()+4,data_size);gf::store_be32(bank.data()+8,fields.size());gf::store_be32(bank.data()+12,1);
        frame.motion.model=std::move(bank);const auto hash=gf::character_program_model_hash({frame.motion.model.data(),frame.motion.model.size()});for(auto* b:{&frame.program,&frame.setup,&frame.runtime})gf::store_be32(b->data()+8,hash);
        frame.program[92+12]=1;attachments=profile(hash);return true;
    }
};
} // namespace startup_attachments_fixture
