#pragma once
#include "engine/assets/stih.hpp"
namespace gf {
struct ChallengeStageInputRow {u32 main_id=0,main_hash=0,stage=0,model_bytes=0;StageGroundParameters ground;};
//42324/40EB4/87718: explicit source roles and ground operands. No source
//pointers, actor placement, renderer/camera policy or all-stage admission.
class ChallengeStageInputsProfile {
public:
    bool load(ByteView b,std::string* error){
        rows_.clear();auto fail=[&](){if(error)*error="Challenge stage inputs: profile extent/domain";return false;};
        if(!b.data||b.size<32||b.size>32+31*48||b.be32(0)!=0x4746434d||b.be32(4)!=1||!b.be16(8)||b.be16(8)>31||b.be16(10)!=48||b.be32(12)!=b.size-32||b.size!=32+std::size_t(b.be16(8))*48)return fail();
        for(unsigned i=16;i<32;++i)if(b.data[i])return fail();
        std::vector<ChallengeStageInputRow> rows;
        for(unsigned i=0;i<b.be16(8);++i){const unsigned at=32+48*i;ChallengeStageInputRow row;row.main_id=b.be32(at);row.main_hash=b.be32(at+4);row.stage=b.be32(at+8);row.model_bytes=b.be32(at+12);row.ground={b.bef32(at+16),b.bef32(at+20),b.bef32(at+24),b.bef32(at+28),b.bef32(at+32),b.bef32(at+36),b.bef32(at+40),true};
            if(row.main_id>65535||row.stage>127||row.model_bytes<32||row.model_bytes>(64u<<20)||b.be32(at+44)||!stage_ground_parameters_valid(row.ground))return fail();
            for(const auto& p:rows)if(p.main_id==row.main_id)return fail();
            rows.push_back(row);
        }
        rows_=std::move(rows);if(error)error->clear();return true;
    }
    bool ready()const{return !rows_.empty();}
    const ChallengeStageInputRow* find(u32 main_id)const{for(const auto& row:rows_)if(row.main_id==main_id)return &row;return nullptr;}
    std::span<const ChallengeStageInputRow> rows()const{return rows_;}
private:
    std::vector<ChallengeStageInputRow> rows_;
};
} // namespace gf
