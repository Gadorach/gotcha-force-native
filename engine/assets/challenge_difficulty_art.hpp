#pragma once
#include <vector>
#include "engine/graphics/camera.hpp"
#include "engine/assets/challenge_difficulty.hpp"
namespace gf {
struct ChallengeFrameArt {u16 mask=0,record=0;std::array<f32,3> position{};};
struct ChallengeFooterArt {u16 mask=0;f32 x=0;std::array<s16,4> records{};};
struct ChallengeEnergyPart {u16 record=0,draw_mask=0;s16 rotation_x=0,rotation_z=0;std::array<f32,3> position{};};
struct ChallengeEnergyLabel {u16 record=0;f32 scale=0;std::array<f32,3> position{};};
struct ChallengeFocusPart {u16 record=0;s16 rotation_z=0;f32 scale=1;std::array<f32,3> position{};};
struct ChallengeDifficultyArt {
    u32 bank_hash=0;Camera camera;f32 aspect=0;
    std::array<ChallengeFrameArt,4> frames;
    std::array<ChallengeFooterArt,10> footers;
    std::array<std::array<ChallengeEnergyPart,3>,3> energy;
    std::array<ChallengeEnergyLabel,2> labels;
    std::array<std::array<f32,3>,3> label_origins;
    // Source operands, in source address order8043B188..8043B1F4. The double
    // integer conversion bias occupies two words and is not used as a scalar.
    std::array<f32,28> operands{};
    std::array<s32,3> limits{};
    std::array<ChallengeFocusPart,5> focus;
    std::array<u32,3> focus_masks{};f32 focus_base_x=0,focus_rest_x=0;
    bool valid()const{
        return bank_hash&&camera.basis(nullptr)&&aspect>0&&std::abs(aspect-4.f/3)<1e-4f&&
            limits[0]>0&&limits[0]<limits[1]&&limits[1]<limits[2]&&limits[2]<=999999&&
            operands[0]==0&&operands[2]==1&&operands[5]>0&&operands[5]<=1&&
            operands[8]>0&&operands[8]<=1&&operands[20]>0;
    }
    bool load(ByteView bytes,std::string* error){
        *this={};auto fail=[&](){*this={};if(error)*error="Challenge art: profile extent/domain";return false;};
        if(!bytes.data||bytes.size!=788||bytes.be32(0)!=0x47464356||bytes.be32(4)!=2)return fail();
        bank_hash=bytes.be32(8);std::size_t at=12;bool ok=true;
        auto half=[&](){const auto v=bytes.be16(at);at+=2;return v;};
        auto word=[&](){const auto v=bytes.be32(at);at+=4;return v;};
        auto real=[&](){const auto v=bits_to_f32(word());if(!std::isfinite(v)||std::abs(v)>1e6f)ok=false;return v;};
        auto position=[&](auto& p){for(auto& v:p)v=real();};
        for(auto& v:camera.eye)v=real();
        for(auto& v:camera.target)v=real();
        for(auto& v:camera.up)v=real();
        camera.fov_y_degrees=real();camera.near_plane=real();camera.far_plane=real();aspect=real();
        for(auto& row:frames){row.mask=half();row.record=half();position(row.position);if(!row.mask||row.record>=4096)ok=false;}
        for(auto& row:footers){row.mask=half();if(half())ok=false;row.x=real();for(auto& id:row.records){id=std::bit_cast<s16>(half());if(id<-1||id>=4096)ok=false;}}
        for(auto& group:energy)for(auto& row:group){row.record=half();row.draw_mask=half();row.rotation_x=std::bit_cast<s16>(half());row.rotation_z=std::bit_cast<s16>(half());position(row.position);if(row.record>=4096||(row.draw_mask&~0x7fu))ok=false;}
        for(auto& row:labels){const auto id=word();if(id>=4096)ok=false;row.record=u16(id);row.scale=real();position(row.position);if(row.scale<=0)ok=false;}
        for(auto& p:label_origins)position(p);
        for(auto& value:operands)value=real();
        for(auto& limit:limits){const auto value=word();if(value>999999)ok=false;limit=s32(value);}
        for(auto& row:focus){row.record=half();row.rotation_z=std::bit_cast<s16>(half());row.scale=real();position(row.position);if(row.record>=4096||row.scale<=0)ok=false;}
        for(auto& mask:focus_masks){mask=word();if(mask&~31u)ok=false;}
        focus_base_x=real();focus_rest_x=real();
        if(at!=bytes.size||!ok||!valid())return fail();
        if(error)error->clear();
        return true;
    }
};
} // namespace gf
