#pragma once
#include "engine/gameplay/startup_actor_material.hpp"
#include "tests/unit/startup_actor_setup_fixture.hpp"
#include "tests/unit/startup_actor_class_fixture.hpp"
namespace startup_actor_bookkeeping_fixture {
inline std::vector<gf::u8> runtime(){
    std::vector<gf::u8> b(52);gf::store_be32(b.data(),0x47464252);gf::store_be16(b.data()+4,1);gf::store_be16(b.data()+6,0x615);gf::store_be32(b.data()+8,1234);gf::store_be32(b.data()+12,52);gf::store_be16(b.data()+16,18);
    const std::array<gf::u16,9> values={200,5,5,0,180,0,0,1,180};for(unsigned i=0;i<9;++i)gf::store_be16(b.data()+24+i*2,values[i]);gf::store_be_f32(b.data()+44,2);gf::store_be_f32(b.data()+48,10);return b;
}
inline std::vector<gf::s64> fields(const gf::StartupActorBookkeeping& s){
    std::vector<gf::s64> v;const auto c=startup_actor_class_fixture::fields(s.klass);v.insert(v.end(),c.begin(),c.end());v.push_back(s.model_hash);v.push_back(s.initialized);
    for(float x:{s.motion_delta,s.timer_delta,s.rate768,s.timer694})v.push_back(gf::f32_to_bits(x));
    for(auto x:s.timers690)v.push_back(gf::f32_to_bits(x));
    for(auto x:s.timers684)v.push_back(gf::f32_to_bits(x));
    for(float x:{s.timer720,s.field8c8,s.size7fc,s.scale_b4})v.push_back(gf::f32_to_bits(x));
    for(auto x:{s.phase18,s.phase19,s.field6fc,s.field7de,s.field4a1,s.field71c})v.push_back(x);
    for(auto x:s.byte_counters)v.push_back(x);
    for(auto x:{s.field708,s.field709,s.field707,s.field661,s.cached87,s.field747,s.field73f,s.field6ca,s.field7d4,s.field5da,s.field8cc,s.field1eb,s.field276,s.field7c9,s.field597})v.push_back(x);
    for(auto x:{s.counter6b8,s.counter6ba,s.value1c6,s.value736,s.limit736,s.increment736,s.counter73a,s.field6c2,s.field6be,s.field6c8})v.push_back(x);
    v.push_back(s.flags8c4);for(auto x:s.counters_a8)v.push_back(x);for(auto x:s.peers)v.push_back(x);v.push_back(s.peer_count);
    for(const auto& r:s.reloads){for(auto x:{r.current,r.previous,r.target,r.mode,r.interval})v.push_back(x);v.push_back(gf::f32_to_bits(r.remaining));}return v;
}
inline bool load(gf::StartupActorSetupProfile& setup,gf::StartupActorRuntimeProfile& p,gf::StartupActorClassProfile& classes,gf::StartupActorBookkeeping& state,std::string* error){
    auto a=startup_actor_setup_fixture::profile(),b=runtime();auto c=startup_actor_class_fixture::profile();gf::store_be16(a.data()+48+8,2000);
    if(!gf::startup_actor_setup_load({a.data(),a.size()},1234,&setup,error)||!gf::startup_actor_runtime_load({b.data(),b.size()},1234,&p,error)||!gf::startup_actor_class_load({c.data(),c.size()},&classes,error))return false;
    gf::StartupActorClassState input,out;input.model_id=0x615;std::vector<gf::StartupActorClassEffect> effects;return gf::startup_actor_class_initialize(classes,input,&out,&effects)&&gf::startup_actor_bookkeeping_initialize(setup,p,out,&state,error);
}
} // namespace startup_actor_bookkeeping_fixture
