#pragma once
#include <string_view>
#include "engine/core/types.hpp"

namespace gf {
inline bool display_name_valid(std::string_view name,std::size_t byte_limit){
    if(name.empty()||name.size()>byte_limit)return false;
    for(std::size_t i=0;i<name.size();){
        const auto c=u8(name[i++]);u32 code=0,minimum=0;unsigned continuation=0;
        if(c<0x80)code=c;
        else if(c>=0xc2&&c<=0xdf){code=c&0x1fu;minimum=0x80;continuation=1;}
        else if(c>=0xe0&&c<=0xef){code=c&0xfu;minimum=0x800;continuation=2;}
        else if(c>=0xf0&&c<=0xf4){code=c&7u;minimum=0x10000;continuation=3;}
        else return false;
        if(continuation>name.size()-i)return false;
        for(unsigned n=0;n<continuation;++n){const auto next=u8(name[i++]);if((next&0xc0u)!=0x80u)return false;code=(code<<6)|(next&0x3fu);}
        if(code<minimum||code>0x10ffffu||(code>=0xd800&&code<=0xdfff)||code<32||(code>=0x7f&&code<=0x9f))return false;
    }return true;
}
}
