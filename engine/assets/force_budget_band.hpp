#pragma once
#include "engine/core/types.hpp"
#include <array>
#include <vector>
#include <string>
#include <cmath>
namespace gf {
struct ForceBudgetBandPart {s16 angle=0;u16 frame=0;};
struct ForceBudgetBandArt {
    u32 bank_hash=0;u16 record=0,pulse_step=0;std::array<f32,3> origin{};f32 angle_unit=0;std::vector<ForceBudgetBandPart> parts;
    bool has_following=false;ForceBudgetBandPart following;
    bool load(ByteView b,std::string* error){
        *this={};auto fail=[&](){if(error)*error="budget band: GFGB extent/domain";return false;};
        if(!b.data||b.size<36||b.be32(0)!=0x47464742u||(b.be32(4)!=1&&b.be32(4)!=2))return fail();
        has_following=b.be32(4)==2;const auto count=b.be16(14);if(!count||count>32||b.size!=36+std::size_t(count)*4+(has_following?4:0)||b.be16(34))return fail();bank_hash=b.be32(8);record=b.be16(12);pulse_step=b.be16(32);angle_unit=b.bef32(28);
        if(!bank_hash||record>=4096||!pulse_step||!std::isfinite(angle_unit)||angle_unit<=0||angle_unit>1)return fail();
        for(unsigned i=0;i<3;++i){origin[i]=b.bef32(16+i*4);if(!std::isfinite(origin[i])||std::abs(origin[i])>1e6)return fail();}
        for(unsigned i=0;i<count;++i){const auto at=36+i*4;ForceBudgetBandPart p{std::bit_cast<s16>(b.be16(at)),b.be16(at+2)};if(p.frame>8192)return fail();parts.push_back(p);}
        if(has_following){const auto at=36+count*4;following={std::bit_cast<s16>(b.be16(at)),b.be16(at+2)};if(following.frame>8192)return fail();}
        if(error)error->clear();
        return true;
    }
};
} // namespace gf
