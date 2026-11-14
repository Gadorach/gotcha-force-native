#pragma once
#include "engine/core/types.hpp"
#include <array>
#include <algorithm>
#include <string>
namespace gf {
struct BattlePreparationWidgetTemplate {std::array<s8,8> model_records{};u8 descriptors=0;u32 draw10c_key=0;};
//1999C8/1999B60 constructor inputs ONLY; no199C0C/199C74 callback readiness.
class BattlePreparationWidgetProfile {
public:
    bool load(ByteView b,std::string* error){
        *this={};auto fail=[&](){if(error)*error="battle preparation widgets: header/template domain";return false;};
        if(!b.data||b.size!=144||b.be32(0)!=0x47464257||b.be32(4)!=1||b.be32(8)!=144||b.be16(12)!=7||b.be16(14)!=16||b.data[16]!=57||b.data[17]!=1||b.be16(18)||b.be32(20)!=0x80199c0c||b.be16(24)!=372||b.be16(26)||b.be32(28))return fail();
        for(unsigned i=0;i<7;++i){const unsigned at=32+16*i;auto& row=rows_[i];row.descriptors=b.data[at+8];row.draw10c_key=b.be32(at+12);if(!row.descriptors||row.descriptors>8||b.data[at+9]!=2||b.be16(at+10)||!row.draw10c_key||(row.draw10c_key&3))return fail();for(unsigned j=0;j<8;++j){row.model_records[j]=std::bit_cast<s8>(b.data[at+j]);if(row.model_records[j]< -1)return fail();}}
        ready_=true;if(error)error->clear();return true;
    }
    const BattlePreparationWidgetTemplate* find(unsigned type)const{return ready_&&type<7?&rows_[type]:nullptr;}bool ready()const{return ready_;}
private:std::array<BattlePreparationWidgetTemplate,7> rows_{};bool ready_=false;
};
} // namespace gf
