#pragma once
#include "engine/core/types.hpp"
#include <array>
#include <string>
namespace gf {
class BattleViewsProfile {
public:
    bool load(ByteView b,std::string* error){ready_=false;auto fail=[&](){if(error)*error="battle views: exact initial source vectors/record layout required";return false;};if(!b.data||b.size!=112||b.be32(0)!=0x47465649||b.be32(4)!=1||b.be32(8)!=112||b.be32(12)!=13||b.be32(16)!=996||b.be32(20)!=32||b.be32(24)!=6||b.be32(28)||b.be32(104)||b.be32(108))return fail();const std::array<f32,18> expected{0,0,10,0,1,0,0,0,-1,-10000,8500,10000,0,1,0,0,-2000,0};for(unsigned i=0;i<18;++i){if(b.be32(32+4*i)!=f32_to_bits(expected[i]))return fail();vectors_[i]=b.bef32(32+4*i);}ready_=true;if(error)error->clear();return true;}
    bool ready()const{return ready_;}const f32* vector(unsigned i)const{return ready_&&i<6?vectors_.data()+3*i:nullptr;}
private:std::array<f32,18> vectors_{};bool ready_=false;
};
} // namespace gf
