#pragma once
// Source18F88/19178 and19338/19450: signed time, sentinel represented by owned
// array extent, at MOST one key-index advance per callback. No clamp/rewind loop.
#include <array>
#include <span>
#include <cmath>
#include "engine/core/types.hpp"
namespace gf {
template<unsigned N> struct LinearKey {s32 time=0;std::array<f32,N> value{};};
template<unsigned N> inline bool linear_key_sample(std::span<const LinearKey<N>> keys,f32 time,u16* index,std::array<f32,N>* value){
    if(!index||!value||!std::isfinite(time)||std::abs(time)>8192||keys.size()>128)return false;
    if(keys.empty())return true; // NULL source curve leaves existing output untouched
    if(*index>=keys.size())return false;
    unsigned at=*index;
    if(keys[at].time<-8192||keys[at].time>8192||(at+1<keys.size()&&(keys[at+1].time<-8192||keys[at+1].time>8192)))return false;
    if(at+1<keys.size()&&f32(keys[at+1].time)<=time)++at;
    auto result=keys[at].value;
    if(at+1<keys.size()){
        const auto& current=keys[at];const auto& next=keys[at+1];if(next.time<-8192||next.time>8192||next.time<=current.time)return false;
        const f32 dt=f32(time-f32(current.time)),denominator=f32(next.time-current.time);
        for(unsigned k=0;k<N;++k)result[k]=f32(result[k]+f32(f32(dt*f32(next.value[k]-current.value[k]))/denominator));
    }
    for(auto v:result)if(!std::isfinite(v)||std::abs(v)>1e7f)return false;
    *index=u16(at);*value=result;return true;
}
} // namespace gf
