#pragma once
#include "engine/assets/challenge_force_cards.hpp"
namespace gf {
struct ChallengeForceWidgetPart {u16 record=0;std::array<f32,3> position{};};
struct ChallengeForceArt {
    u32 bank_hash=0;
    std::array<f32,3> card_hidden{},card_active{},card_exit{},focus_hidden{},focus_active{},cost_hidden{},cost_active{};
    std::array<ChallengeForceWidgetPart,4> focus{};std::array<ChallengeForceWidgetPart,2> cost{};
    std::array<f32,3> name_origin{};
    f32 approach=0,snap=0,alpha_step=0,focus_alpha=0,name_extent=0,name_advance=0,digit_advance=0,angle_unit=0;
    s16 cost_angle=0;u16 shell_record=0;std::string random_name;
    bool valid()const{
        if(!bank_hash||shell_record>=4096||random_name.empty()||random_name.size()>40||random_name.size()%2||approach<=0||approach>1||snap<=0||alpha_step<=0||alpha_step>1||focus_alpha<=0||focus_alpha>1||name_extent<=0||name_advance<=0||digit_advance<=0||angle_unit<=0||angle_unit>.01f)return false;
        for(const auto& v:{card_hidden,card_active,card_exit,focus_hidden,focus_active,cost_hidden,cost_active,name_origin})for(f32 f:v)if(!std::isfinite(f)||std::abs(f)>1e6f)return false;
        for(f32 f:{approach,snap,alpha_step,focus_alpha,name_extent,name_advance,digit_advance,angle_unit})if(!std::isfinite(f)||std::abs(f)>1e6f)return false;
        for(const auto& p:focus){if(p.record>=4096)return false;for(f32 f:p.position)if(!std::isfinite(f)||std::abs(f)>1e6f)return false;}
        for(const auto& p:cost){if(p.record>=4096)return false;for(f32 f:p.position)if(!std::isfinite(f)||std::abs(f)>1e6f)return false;}
        return true;
    }
    bool load(ByteView b,std::string* error){
        *this={};auto fail=[&](){*this={};if(error)*error="Challenge force art: GFVA extent/domain";return false;};
        if(!b.data||b.size<244||b.be32(0)!=0x47465641||b.be32(4)!=1)return fail();
        bank_hash=b.be32(8);std::size_t at=12;auto real=[&](){const auto f=b.bef32(at);at+=4;return f;};auto vector=[&](auto& v){for(auto& f:v)f=real();};
        for(auto* v:{&card_hidden,&card_active,&card_exit,&focus_hidden,&focus_active,&cost_hidden,&cost_active})vector(*v);
        auto part=[&](auto& p){p.record=b.be16(at);const bool reserved=b.be16(at+2)!=0;at+=4;vector(p.position);return !reserved;};
        for(auto& p:focus)if(!part(p))return fail();
        for(auto& p:cost)if(!part(p))return fail();
        vector(name_origin);
        for(auto* f:{&approach,&snap,&alpha_step,&focus_alpha,&name_extent,&name_advance,&digit_advance,&angle_unit})*f=real();
        cost_angle=std::bit_cast<s16>(b.be16(at));shell_record=b.be16(at+2);at+=4;
        const auto size=b.be16(at);if(b.be16(at+2)||b.size!=at+4+size)return fail();at+=4;
        random_name.assign(reinterpret_cast<const char*>(b.data+at),size);
        if(!valid())return fail();
        if(error)error->clear();
        return true;
    }
};
} // namespace gf
