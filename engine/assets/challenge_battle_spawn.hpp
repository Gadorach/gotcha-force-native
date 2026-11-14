#pragma once
#include <array>
#include <cmath>
#include <vector>
#include <string>
#include "engine/core/types.hpp"
namespace gf {
struct ChallengeSpawnLocationRow {u32 stage=0;std::array<std::array<std::array<f32,2>,4>,2> xz{};};
//56D28 non-Story, initial ordinary placement branch only. Special-air/respawn,
//heading and the two projection consumers are separate, still-required systems.
class ChallengeBattleSpawnProfile {
public:
    bool load(ByteView b,std::string* error){
        *this={};auto fail=[&](){if(error)*error="Challenge spawn profile: extent/domain";return false;};
        if(!b.data||b.size<64||b.size>64+11*68||b.be32(0)!=0x47464357||b.be32(4)!=1||!b.be16(8)||b.be16(8)>11||b.be16(10)!=68||b.be32(12)!=b.size-64||b.size!=64+std::size_t(b.be16(8))*68)return fail();
        for(unsigned i=32;i<64;++i)if(b.data[i])return fail();
        std::array<f32,4> values{};for(unsigned i=0;i<4;++i){values[i]=b.bef32(16+4*i);if(!std::isfinite(values[i])||values[i]<0||values[i]>1e7)return fail();}
        std::vector<ChallengeSpawnLocationRow> rows;
        for(unsigned i=0;i<b.be16(8);++i){const unsigned at=64+i*68;ChallengeSpawnLocationRow row;row.stage=b.be32(at);if(row.stage>=17)return fail();for(const auto& old:rows)if(old.stage==row.stage)return fail();for(unsigned team=0;team<2;++team)for(unsigned c=0;c<4;++c)for(unsigned axis=0;axis<2;++axis){auto& x=row.xz[team][c][axis];x=b.bef32(at+4+(team*8+c*2+axis)*4);if(!std::isfinite(x)||std::abs(x)>1e7)return fail();}rows.push_back(row);}
        rows_=std::move(rows);values_=values;if(error)error->clear();return true;
    }
    const ChallengeSpawnLocationRow* find(u32 stage)const{for(const auto& row:rows_)if(row.stage==stage)return &row;return nullptr;}
    bool ready()const{return !rows_.empty();}f32 initial_y()const{return values_[0];}
    f32 lift(u8 kind)const{return kind==3?values_[1]:(kind==4||kind==5)?values_[2]:kind==10?values_[3]:0.f;}
private:
    std::vector<ChallengeSpawnLocationRow> rows_;std::array<f32,4> values_{};
};
} // namespace gf
