#pragma once
#include <array>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
class BattleInitialAiProfile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;auto fail=[&](){if(error)*error="initial AI: exact source inputs/class constructors required";return false;};
        if(!b.contains(0,80)||b.size!=80||b.be32(0)!=0x4746414a||b.be32(4)!=1||b.be32(8)!=80||b.be32(12)||b.be32(56)!=0x461c4000||b.be16(78))return fail();
        const std::array<u32,10> keys{0x80027a44,0x802cfaa0,0x8019c60c,0x801985b0,0x800c627c,0x801fcc50,0x800c6394,0x800c6724,0x801fcda8,0x801fd09c};
        for(unsigned i=0;i<keys.size();++i)if(b.be32(16+4*i)!=keys[i])return fail();
        const std::array<u8,18> expected{31,16,12,30,12,6,8,29,9,14,12,12,12,29,0,0,0,0};
        for(unsigned i=0;i<expected.size();++i)if(b.data[60+i]!=expected[i])return fail();
        inputs_=expected;ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
    bool input(u8 selector,u8* out)const{if(!ready_||!out||selector>=inputs_.size())return false;*out=inputs_[selector];return true;}
    f32 point_height()const{return bits_to_f32(0x461c4000);}
private:std::array<u8,18> inputs_{};bool ready_=false;
};
}
