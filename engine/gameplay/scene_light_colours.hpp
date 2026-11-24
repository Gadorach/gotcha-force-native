#pragma once
#include <string>
#include "engine/graphics/lighting.hpp"
namespace gf {
struct SceneLightColourChannel {
    std::array<u8,72> controls{};std::array<u32,8> current{};
    bool operator==(const SceneLightColourChannel&)const=default;
};
struct SceneLightColours {
    std::array<u32,8> baseline{};u32 count=0;std::array<SceneLightColourChannel,4> channels{};
    bool operator==(const SceneLightColours&)const=default;
};
//40910 clears 0x1C4 bytes;40940 copies each reached LObj RGBA into the baseline
//and all four channel current-colour arrays. Animation is not requested here.
inline bool initialize_scene_light_colours(const SceneLighting& rig,SceneLightColours* out,std::string* error){
    if(out)*out={};
    if(!out||!rig.declared||!menu_light_rig_supported(rig)||rig.lights.size()>8){if(error)*error="scene light colours: supported authored rig/eight-colour source capacity required";return false;}
    SceneLightColours next;
    for(const auto& light:rig.lights){
        u32 rgba=0;for(auto value:light.colour)rgba=(rgba<<8)|value;
        next.baseline[next.count]=rgba;for(auto& channel:next.channels)channel.current[next.count]=rgba;
        ++next.count;
    }
    *out=next;if(error)error->clear();return true;
}
} // namespace gf
