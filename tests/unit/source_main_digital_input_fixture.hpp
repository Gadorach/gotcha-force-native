#pragma once
#include <cstdlib>
#include "engine/gameplay/source_main_digital_input.hpp"
namespace main_digital_input_fixture {
inline std::array<gf::u8,64> bytes(){
    using namespace gf;std::array<u8,64> result{};
    const std::array<u32,16> words{0x47464d49,1,64,0,0x8008b650,0x8008b7c8,0x803c727c,0x803c71fc,4,32,8,12,16,0x1f70,0,0};
    for(unsigned i=0;i<words.size();++i)store_be32(result.data()+4*i,words[i]);
    return result;
}
inline gf::SourceMainDigitalInputProfile profile(){
    const auto raw=bytes();gf::SourceMainDigitalInputProfile result;std::string error;
    if(!result.load({raw.data(),raw.size()},&error))std::abort();
    return result;
}
inline gf::InputRouter aliased_router(){
    gf::InputRouter router;for(unsigned port=0;port<4;++port){auto binding=router.port(port);binding.device_pad=0;router.set_port(port,binding);}return router;
}
}
