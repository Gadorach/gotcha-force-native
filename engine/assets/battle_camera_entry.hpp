#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
//First0BDA4/0C5F8/0C660/0C314 entry. No later camera modes or projection grant.
class BattleCameraEntryProfile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;auto fail=[&](){if(error)*error="battle camera entry: exact first source recipe required";return false;};
        if(!b.contains(0,128)||b.size!=128||b.be32(0)!=0x47465643||b.be32(4)!=1||b.be32(8)!=128||b.be32(12))return fail();
        const std::array<u32,12> keys{0x8000bda4,0x8000c5f8,0x8000c660,0x8000c314,0x8000d46c,0x80010980,0x8008ae10,0x8008ae60,0x80045204,0x80045238,0x8020b61c,0x8000bef4};
        for(unsigned i=0;i<keys.size();++i)if(b.be32(16+4*i)!=keys[i])return fail();
        const std::array<u32,8> values{0x3f800000,0x3f000000,0x358637be,0x40800000,0x3fa66666,0x3f99999a,0x3f666666,0x38c90fdb};
        for(unsigned i=0;i<values.size();++i)if(b.be32(64+4*i)!=values[i])return fail();
        const std::array<u32,8> tail{1536,61,32,12,2,0,0,0};
        for(unsigned i=0;i<tail.size();++i)if(b.be32(96+4*i)!=tail[i])return fail();
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
private:bool ready_=false;
};
}
