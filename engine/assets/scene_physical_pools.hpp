#pragma once
#include "engine/assets/scene_object_pool.hpp"
namespace gf {
struct ScenePhysicalPoolRow {u32 relative_offset=0;u16 count=0,stride=0;u8 physical=0,source_class=0;bool operator==(const ScenePhysicalPoolRow&)const=default;};
//88398(physical recipe0, capacityE0000), relative to its aligned source start.
//Not a host/guest arena-address mapping or a generic alternate-profile recipe.
class ScenePhysicalPoolsProfile {
public:
    bool load(ByteView b,std::string* error){
        *this={};auto fail=[&](){if(error)*error="physical pools: exact recipe0/aligned-layout domain required";return false;};
        if(!b.data||b.size!=224||b.be32(0)!=0x4746504f||b.be32(4)!=1||b.be32(8)!=224||b.be32(12)||b.be32(16)!=0xfe800||b.be32(20)!=0xe0000||b.be32(24)!=0xca500||b.be32(28))return fail();
        const std::array<ScenePhysicalPoolRow,8> expected{{{0,6,7680,0,0},{0xb400,1,256,1,1},{0xb500,512,452,2,2},{0x43d00,512,740,3,3},{0xa0500,384,448,4,4},{0xca500,0,0,5,5},{0xca500,0,0,6,6},{0xca500,0,0,7,7}}};
        for(unsigned i=0;i<8;++i){const unsigned at=32+16*i;ScenePhysicalPoolRow r{b.be32(at),b.be16(at+4),b.be16(at+6),b.data[at+8],b.data[at+9]};if(r!=expected[i]||b.be16(at+10)||b.be32(at+12))return fail();rows_[i]=r;}
        std::array<u8,96> old{};store_be32(old.data(),0x47464f50);store_be32(old.data()+4,1);store_be32(old.data()+8,96);store_be16(old.data()+16,512);store_be16(old.data()+18,452);old[20]=old[21]=2;std::copy_n(b.data+160,64,old.data()+32);
        if(!class2_.load({old.data(),old.size()},error)){return fail();}ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}const ScenePhysicalPoolRow* row(unsigned i)const{return ready_&&i<8?&rows_[i]:nullptr;}
    const SceneObjectPoolProfile& class2_profile()const{return class2_;}
private:std::array<ScenePhysicalPoolRow,8> rows_{};SceneObjectPoolProfile class2_;bool ready_=false;
};
} // namespace gf
