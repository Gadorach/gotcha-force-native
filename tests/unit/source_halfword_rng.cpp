#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/source_halfword_rng.hpp"
namespace {unsigned checks=0;void check(bool ok){++checks;if(!ok)std::abort();}}
int main(){
    using namespace gf;std::string error;std::array<u8,64> raw{};const std::array<u32,16> words{0x47465248,1,64,0,0x80005630,0x804360c4,0x800055fc,0x804360c6,0x804360c7,3,65536,255,16,1,0,0};
    for(unsigned i=0;i<words.size();++i)store_be32(raw.data()+4*i,words[i]);
    SourceHalfwordRngProfile profile,bad;check(profile.load({raw.data(),raw.size()},&error));
    for(unsigned at=0;at<raw.size();++at){auto changed=raw;changed[at]^=1;check(!bad.load({changed.data(),changed.size()},&error)&&!bad.ready());}
    //Independent integer reproduction of the actual lhz/mulli/sth/lbz/stb/lhz.
    for(unsigned original=0;original<65536;++original){u16 state=u16(original);const unsigned product=(original*3)&65535u;const unsigned upper=product>>8;const u16 expected=u16((upper<<8)|((original+upper)&255u));check(source_halfword_rng_step(state)==expected&&state==expected);}
    SourceHalfwordRngOwner owner,other;SourceByteRngOwner byte;check(owner.initialize_explicit(profile,0xbeef)&&other.initialize_explicit(profile,0xbeef)&&byte.initialize_explicit({0xef,0xbe}));
    const auto before=owner.snapshot(),foreign=other.snapshot();const auto byte_before=byte.snapshot();check(owner.current(before)&&!owner.same_owner(foreign));
    u16 expected=0xbeef,value=0;for(unsigned i=0;i<8;++i){source_halfword_rng_step(expected);check(owner.next_halfword(&value)&&value==expected&&owner.draws()==i+1&&byte.current(byte_before));}
    check(owner.same_owner(before)&&!owner.current(before)&&!owner.next_halfword(nullptr));
    SourceHalfwordRngOwner moved=std::move(owner);check(moved.same_owner(before)&&!owner.ready());moved.retire();check(!before.live()&&!moved.ready()&&!moved.initialize_explicit(profile,0xbeef));
    std::printf("independent C4 halfword RNG recurrence/ownership: %u checks PASS\n",checks);
}
