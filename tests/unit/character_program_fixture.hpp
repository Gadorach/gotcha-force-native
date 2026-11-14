#pragma once
// Owned byte programs and a deliberately synthetic clock driver. This tests VM
// ordering; it is NOT the original AObj/actor lifecycle or root-motion policy.
#include "engine/gameplay/character_program.hpp"
namespace character_program_fixture {
inline std::vector<gf::u8> bytes(){
    std::vector<gf::u8> commands={1,5,0,0,0,0,0,0,2,255,2,0,0,0,0,0,3,1,2,0,0,0,2,0x30,11,1,64,0,0,2,0,2,
        2,254,3,0,0,0,0,0,0,0,255,255,1,5,1,0,0,0,0,0,0,2,255,255,7,0,254,255,11,0,64,0,0,1,255,255};
    std::vector<gf::u8> b(92,0);gf::store_be32(b.data(),0x47464d50);gf::store_be16(b.data()+4,1);b[6]=3;b[7]=1;gf::store_be32(b.data()+8,0x12345678);
    gf::store_be16(b.data()+12,2);b[14]=2;b[15]=3;gf::store_be32(b.data()+20,32);gf::store_be32(b.data()+24,44);gf::store_be32(b.data()+28,92);
    b[32]=5;gf::store_be32(b.data()+36,92);gf::store_be32(b.data()+40,commands.size());
    for(unsigned i=0;i<2;++i){const unsigned at=44+i*24;b[at]=5;b[at+1]=gf::u8(i);b[at+4]=255;
        b[at+4+5]=gf::u8(i);b[at+4+6]=1;b[at+4+8]=255;b[at+4+9]=b[at+4+10]=255;
        b[at+4+13]=gf::u8(i);b[at+4+14]=1;b[at+4+16]=gf::u8(i?26:255);
    }
    b.insert(b.end(),commands.begin(),commands.end());gf::store_be32(b.data()+16,b.size());return b;
}
class Clock final:public gf::ICharacterProgramMotion {
public:
    std::array<gf::s16,128> ends{};int refuse_select_lane=-1,refuse_advance_lane=-1;bool rewind=false;
    Clock(){ends.fill(4);ends[1]=3;}
    bool select(unsigned lane,const gf::CharacterProgramDescriptor& d,bool,const gf::CharacterProgramMotion&,gf::f32 start,gf::f32,gf::CharacterProgramMotion* out)const override{
        if(int(lane)==refuse_select_lane)return false;
        *out={true,0,start,ends[d.choice],gf::s8(start>=ends[d.choice]),0,{}};return true;
    }
    bool advance(unsigned lane,const gf::CharacterProgramMotion& old,gf::f32 delta,gf::CharacterProgramMotion* out)const override{
        if(int(lane)==refuse_advance_lane)return false;
        *out=old;out->elapsed+=delta;out->frame+=delta;out->rewound=rewind?1:0;
        if(out->frame>=out->end_frame)out->finished=1;
        return true;
    }
};
} // namespace character_program_fixture
