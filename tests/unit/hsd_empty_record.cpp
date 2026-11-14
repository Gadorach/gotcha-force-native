#include <cstdio>
#include <cstdlib>
#include "engine/assets/hsd_record_animation.hpp"
#include "tests/unit/hsd_record_animation_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"empty record: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;hsd_record_animation_fixture::Fixture f;f.ptr(0x24,0xf00);f.ptr(0xf00,0xe00);for(unsigned k=0;k<3;++k)f.real(0xe20+4*k,1);
    auto bytes=f.archive();HsdRecordAnimationBank bank;std::string error;HsdRecordAnimationBinding binding;
    check(bank.load({bytes.data(),bytes.size()},&error),"bank with explicit empty second record");check(bank.is_empty_static_record(1)&&!bank.is_empty_static_record(0)&&!bank.is_empty_static_record(2),"exact empty metadata only");
    check(!bank.select(1,0,0,0,&binding,&error)&&!binding.scene(),"legacy empty selection still refuses");check(bank.select(0,0,0,0,&binding,&error),"ordinary drawable retained");
    for(auto flags:{1u,0x4000u,0x1000u}){auto changed=f;changed.word(0xe04,flags);bytes=changed.archive();HsdRecordAnimationBank other;const bool loaded=other.load({bytes.data(),bytes.size()},&error);check(!loaded||!other.is_empty_static_record(1),"special joint never classified empty");}
    auto animated=f;animated.ptr(0xf04,0x50);bytes=animated.archive();check(bank.load({bytes.data(),bytes.size()},&error)&&!bank.is_empty_static_record(1),"animation excludes placeholder classification");
    check(!bank.load({},&error)&&!bank.is_empty_static_record(1),"failed reload retires metadata");std::printf("empty static record: %u owned checks PASS\n",checks);
}
