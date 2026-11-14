#pragma once
#include "engine/gameplay/startup_actor_class.hpp"
namespace startup_actor_class_fixture {
inline std::array<gf::u8,24> profile(){
    std::array<gf::u8,24> bytes{};gf::store_be32(bytes.data(),0x47464143);gf::store_be16(bytes.data()+4,1);gf::store_be16(bytes.data()+6,2);gf::store_be32(bytes.data()+8,24);gf::store_be16(bytes.data()+16,0x615);gf::store_be16(bytes.data()+18,10);gf::store_be32(bytes.data()+20,0x3f666666);return bytes;
}
inline std::array<gf::s64,15> fields(const gf::StartupActorClassState& s){return {s.model_id,s.callbacks_initialized,s.flags5e0,s.field4a8,s.field6ee,s.field582,s.field58e,s.field7c8,s.field6e9,s.field6ec,s.fieldaf,s.field6f6,s.field272,s.field1dfc,s.angle18e2};}
inline gf::StartupActorClassState state(gf::u16 id){
    gf::StartupActorClassState s;s.model_id=id;s.flags5e0=0;s.field4a8=17;s.field6ee=23;s.field582=31;s.field58e=255;s.field7c8=43;s.field6e9=1;s.field6ec=255;s.fieldaf=59;s.field6f6=0xffff;s.field272=0xffff;s.field1dfc=-12345;s.angle18e2=-32768;return s;
}
} // namespace startup_actor_class_fixture
