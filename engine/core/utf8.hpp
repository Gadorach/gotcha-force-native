#pragma once
#include <string_view>
#include "engine/core/types.hpp"

namespace gf {
struct Utf8Scalar {
    u32 value=0;
    u8 bytes=0; // Zero means no valid scalar at the start.
};
inline constexpr bool unicode_scalar_valid(u32 code){
    return code<=0x10ffffu&&!(code>=0xd800u&&code<=0xdfffu);
}
inline constexpr bool unicode_display_scalar(u32 code){
    return unicode_scalar_valid(code)&&code>=0x20u&&!(code>=0x7fu&&code<=0x9fu);
}
inline Utf8Scalar utf8_front(std::string_view input){
    if(input.empty())return {};
    const auto lead=u8(input[0]);
    u32 code=0,minimum=0;u8 length=0;
    if(lead<0x80)return {lead,1};
    if(lead>=0xc2&&lead<=0xdf){code=lead&0x1fu;minimum=0x80;length=2;}
    else if(lead>=0xe0&&lead<=0xef){code=lead&0xfu;minimum=0x800;length=3;}
    else if(lead>=0xf0&&lead<=0xf4){code=lead&7u;minimum=0x10000;length=4;}
    if(!length||length>input.size())return {};
    for(u8 i=1;i<length;++i){
        const auto next=u8(input[i]);
        if((next&0xc0u)!=0x80u)return {};
        code=(code<<6)|(next&0x3fu);
    }
    return code>=minimum&&unicode_scalar_valid(code)?Utf8Scalar{code,length}:Utf8Scalar{};
}
}
