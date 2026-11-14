#pragma once
#include "engine/assets/force_list_art.hpp"
namespace gf {
struct ForceTooltipArt {
    u32 bank_hash=0;std::array<ForceListWidget,2> widgets;std::array<std::array<f32,3>,4> corners;
    std::array<s16,6> variants;std::array<std::array<f32,3>,2> numbers;std::array<f32,3> text_origin{};std::array<u32,2> colours{};
    f32 extent=0,advance=0,digit_advance=0,digit_scale=0,split_x=0,split_z=0;u16 digit_record=0,digits=0,lifetime=0;
    bool load(ByteView b,std::string* error){
        *this={};auto fail=[&](){if(error)*error="tooltip: GFTT extent/domain";return false;};
        if(!b.data||b.size!=180||b.be32(0)!=0x47465454u||b.be32(4)!=1)return fail();
        bank_hash=b.be32(8);std::size_t at=12;
        auto real=[&](){const auto v=b.bef32(at);at+=4;return v;};auto point=[&](auto& p){for(auto& v:p)v=real();};
        for(auto& w:widgets){const auto id=std::bit_cast<s32>(b.be32(at));at+=4;if(id<-1||id>=4096)return fail();w.record=s16(id);w.pass=0x44;point(w.position);}
        for(auto& p:corners)point(p);
        for(auto& id:variants){id=std::bit_cast<s16>(b.be16(at));at+=2;if(id<-1||id>=4096)return fail();}
        for(auto& p:numbers)point(p);
        point(text_origin);
        extent=real();advance=real();digit_advance=real();digit_scale=real();split_x=real();split_z=real();digit_record=b.be16(at);digits=b.be16(at+2);lifetime=b.be16(at+4);if(b.be16(at+6))return fail();at+=8;for(auto& c:colours){c=b.be32(at);at+=4;}
        auto valid=[](const auto& p){for(auto v:p)if(!std::isfinite(v)||std::abs(v)>1e6f)return false;return true;};for(const auto& w:widgets)if(!valid(w.position))return fail();for(const auto& p:corners)if(!valid(p))return fail();for(const auto& p:numbers)if(!valid(p))return fail();
        if(!bank_hash||at!=b.size||!valid(text_origin)||!valid(std::array{extent,advance,digit_advance,digit_scale,split_x,split_z})||extent<=0||advance<=0||digit_advance<=0||digit_scale<=0||digit_record>=4096||digits!=6||lifetime!=10)return fail();
        if(error)error->clear();
        return true;
    }
};
} // namespace gf
