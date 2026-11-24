#pragma once
#include <string_view>
#include <utility>
#include <vector>
#include "engine/core/types.hpp"

namespace gf {
inline constexpr unsigned kBorgMovementSampleRate=48000;
inline constexpr std::size_t kBorgMovementSoundBytes=kBorgMovementSampleRate*4*3;
inline constexpr std::size_t kBorgImpactSoundBytes=kBorgMovementSampleRate*4*4;

// Package PCM: stereo 48 kHz signed 16-bit little endian, up to three seconds.
inline bool borg_movement_sound_decode(std::string_view bytes,std::vector<s16>* out,
                                      std::size_t limit=kBorgMovementSoundBytes){
    if(!out||bytes.empty()||limit>kBorgImpactSoundBytes||bytes.size()>limit||bytes.size()%4)return false;
    std::vector<s16> result(bytes.size()/2);
    for(std::size_t i=0;i<result.size();++i)
        result[i]=std::bit_cast<s16>(u16(u8(bytes[2*i])|(u16(u8(bytes[2*i+1]))<<8)));
    *out=std::move(result);return true;
}
}
