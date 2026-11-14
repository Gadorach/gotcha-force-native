#pragma once
#include "engine/gameplay/startup_feedback.hpp"
namespace startup_feedback_fixture {
inline std::vector<gf::u8> bytes(gf::u32 mode=2){
    using namespace gf;std::vector<u8> b(56);store_be32(b.data(),0x47464642);store_be16(b.data()+4,1);store_be16(b.data()+6,8);store_be32(b.data()+8,56);store_be32(b.data()+16,mode);store_be32(b.data()+20,0x944400);store_be_f32(b.data()+24,.5f);store_be32(b.data()+28,6);store_be16(b.data()+36,16);store_be16(b.data()+38,16);store_be16(b.data()+40,1);store_be16(b.data()+42,1);store_be16(b.data()+44,14);store_be16(b.data()+46,14);store_be_f32(b.data()+48,255);return b;
}
inline gf::StartupEffect enabled(bool on){gf::StartupEffect e;e.kind=gf::StartupEffectKind::Command;e.opcode=15;e.operands[0]=on;return e;}
inline gf::StartupEffect alpha(float value){gf::StartupEffect e;e.kind=gf::StartupEffectKind::ConfigSample;e.value=value;return e;}
}
