#pragma once
#include <array>
#include <charconv>
#include <string_view>
#include "engine/core/types.hpp"

namespace gf {
struct BorgCruiseBoost {
    u16 height=0,capacity=0,cost=0,refill=0,delay=0,minimum=0;
    bool operator==(const BorgCruiseBoost&)const=default;
};
inline bool borg_cruise_boost_valid(const BorgCruiseBoost& c){
    return c.height>=24&&c.height<=600&&c.capacity&&c.capacity<=10000&&
        c.cost&&c.cost<=c.capacity&&c.refill&&c.refill<=c.capacity&&
        c.delay<=120&&c.minimum&&c.minimum<=120;
}
inline bool borg_read_cruise_boost(std::string_view text,BorgCruiseBoost* out){
    if(!out)return false;
    BorgCruiseBoost next;
    const std::array fields{&next.height,&next.capacity,&next.cost,&next.refill,&next.delay,&next.minimum};
    for(unsigned i=0;i<fields.size();++i){
        const auto separator=text.find('|');
        if((separator!=std::string_view::npos)!=(i+1<fields.size()))return false;
        const auto value=text.substr(0,separator);
        if(value.empty()||value.size()>5)return false;
        const auto end=value.data()+value.size();
        const auto parsed=std::from_chars(value.data(),end,*fields[i]);
        if(parsed.ec!=std::errc{}||parsed.ptr!=end)return false;
        if(i+1<fields.size())text.remove_prefix(separator+1);
    }
    if(!borg_cruise_boost_valid(next))return false;
    *out=next;return true;
}
}
