#pragma once
#include "tests/unit/character_model_fixture.hpp"
namespace attachment_alternate_fixture {
inline std::vector<gf::u8> profile(){
    std::vector<gf::u8> b(52);gf::store_be32(b.data(),0x47464141);gf::store_be16(b.data()+4,1);gf::store_be16(b.data()+6,9);gf::store_be32(b.data()+8,52);
    const gf::u32 words[]={0x20080000,0x50140000,4,10,0x60002001,0x400a,0x20,7};for(unsigned i=0;i<8;++i)gf::store_be32(b.data()+16+i*4,words[i]);gf::store_be32(b.data()+48,0x3ecccccd);return b;
}
inline std::vector<gf::u8> archive(bool explicit_pe=false){
    auto bank=character_model_fixture::archive();character_model_fixture::word(bank,32+0x30,0x80);character_model_fixture::word(bank,32+0x84,0x10050180);
    character_model_fixture::word(bank,32+0x134,0x400a);character_model_fixture::word(bank,32+0x138,0);character_model_fixture::word(bank,32+0x284,0xff0000ff);character_model_fixture::scalar(bank,32+0x28c,.25f);
    if(!explicit_pe)return bank;
    const auto size=gf::load_be32(bank.data()+4),count=gf::load_be32(bank.data()+8);std::vector<gf::u8> block(bank.begin()+32,bank.begin()+32+size);std::vector<gf::u32> fields;
    for(unsigned i=0;i<count;++i)fields.push_back(gf::load_be32(bank.data()+32+size+i*4));
    gf::store_be32(block.data()+0x144,0x600);fields.push_back(0x144);const gf::u8 pe[]={0x33,128,0,0,0,4,5,15,3,4,0,7};std::copy_n(pe,12,block.data()+0x600);
    const char symbol[]="scene_data";std::vector<gf::u8> out(32+size+fields.size()*4+8+sizeof(symbol));std::copy(block.begin(),block.end(),out.begin()+32);
    for(unsigned i=0;i<fields.size();++i)gf::store_be32(out.data()+32+size+i*4,fields[i]);
    const auto at=32+size+fields.size()*4;gf::store_be32(out.data()+at,0x10);std::copy_n(symbol,sizeof(symbol),out.data()+at+8);gf::store_be32(out.data(),out.size());gf::store_be32(out.data()+4,size);gf::store_be32(out.data()+8,fields.size());gf::store_be32(out.data()+12,1);return out;
}
} // namespace attachment_alternate_fixture
