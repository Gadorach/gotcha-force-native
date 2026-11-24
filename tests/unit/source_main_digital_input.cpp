#include <cstdio>
#include <cstdlib>
#include "tests/unit/source_main_digital_input_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"MAIN digital input: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;std::string error;const auto profile=main_digital_input_fixture::profile();const auto raw=main_digital_input_fixture::bytes();SourceMainDigitalInputProfile bad;
    for(unsigned at=0;at<raw.size();++at){auto copy=raw;copy[at]^=1;check(!bad.load({copy.data(),copy.size()},&error)&&!bad.ready(),"sealed source profile refuses edits");}
    for(unsigned n=0;n<raw.size();++n)check(!bad.load({raw.data(),n},&error),"truncated recipe refuses");
    const auto router=main_digital_input_fixture::aliased_router();
    const std::array<std::array<u32,2>,9> buttons{{{1u<<0,0x100},{1u<<1,0x200},{1u<<2,0x400},{1u<<3,0x800},{1u<<4,0x1000},{1u<<9,0x40},{1u<<10,0x20},{1u<<11,0x10},{0,0}}};
    for(const auto& button:buttons)for(unsigned transition=0;transition<4;++transition){
        InputSnapshot previous,current;previous.port_count=current.port_count=1;
        previous.pads[0].buttons=(transition&1)?button[0]:0;current.pads[0].buttons=(transition&2)?button[0]:0;
        SourceMainDigitalInputOwner owner;check(owner.begin_explicit_history(profile,router,previous,40,&error)&&!owner.receipt().current(),"explicit previous frame is not a pressed-event sample");
        check(owner.publish(current,41,&error),error.c_str());const auto receipt=owner.receipt();const auto& state=*owner.state();
        const u32 p=(transition&1)?button[1]:0,h=(transition&2)?button[1]:0;
        for(const auto& port:state.ports)check(port==SourceMainDigitalPort{h,p,h&~p,p&~h},"routed source digital held/previous/pressed/released");
        u32 pressed=0;check(receipt.covers(0x0a,0x1f70)&&receipt.masked_pressed(0x0a,0x1f70,&pressed)&&pressed==(h&~p),"selected port union is pressed, not held/repeat");
        const auto saved=state;
        check(!owner.publish(current,41,&error)&&!owner.publish(current,43,&error)&&*owner.state()==saved&&receipt.current(),"duplicate/skipped samples refuse without invalidating current receipt");
        check(!receipt.covers(0x10,0x1f70)&&!receipt.covers(0,0x1f70)&&!receipt.covers(1,1)&&!receipt.covers(1,0x10000)&&!receipt.masked_pressed(0x0a,0x1f70,nullptr),"unsupported ports/directions/analogue/repeat/missing output refuse");
        check(owner.publish(current,42,&error)&&!receipt.current()&&!receipt.masked_pressed(0x0a,0x1f70,&pressed)&&!pressed,"new sample closes old edge capability");
        check(owner.receipt().masked_pressed(0x0a,0x1f70,&pressed)&&!pressed,"holding a button never manufactures another press");
        SourceMainDigitalInputOwner moved=std::move(owner);const auto live=moved.receipt();check(live.current()&&!owner.receipt().current(),"move preserves one source stream");
        const auto copied=*moved.state();moved.retire();check(!live.current()&&copied.sample==42,"copied state cannot retain input authority");
    }
    InputRouter four;InputSnapshot short_previous,full;short_previous.port_count=1;full.port_count=4;full.pads[1].buttons=1;full.pads[3].buttons=1;
    SourceMainDigitalInputOwner incomplete;check(incomplete.begin_explicit_history(profile,four,short_previous,0,&error)&&incomplete.publish(full,1,&error),"partially reported prior frame");
    check(!incomplete.receipt().covers(0x0a,0x1f70)&&incomplete.state()->known_ports==1,"unreported fallback is not known released history");
    check(incomplete.publish(full,2,&error)&&incomplete.receipt().covers(0x0a,0x1f70),"both reported snapshots establish edge history");
    u32 pressed=1;check(incomplete.receipt().masked_pressed(0x0a,0x1f70,&pressed)&&!pressed,"newly reported already-held buttons are not invented presses");
    SourceMainDigitalInputOwner foreign;check(foreign.begin_explicit_history(profile,four,full,1,&error)&&foreign.publish(full,2,&error)&&!foreign.receipt().same_sample(incomplete.receipt()),"same serial/value from another owner is not the same source sample");
    InputSnapshot analogue;analogue.port_count=1;analogue.pads[0].axis[4]=32767;analogue.pads[0].axis[0]=32767;
    SourceMainDigitalInputOwner digital;check(digital.begin_explicit_history(profile,router,short_previous,0,&error)&&digital.publish(analogue,1,&error)&&digital.receipt().masked_pressed(0x0a,0x1f70,&pressed)&&!pressed,"no inferred digital trigger or analogue direction policy");
    InputRouter touch_router=router;TouchLayout touch;touch.enabled=true;check(touch.add_button({0,0,100,100,1}),"shared touch mapping");touch_router.set_touch_layout(touch);
    InputSnapshot touch_now;touch_now.port_count=1;check(touch_now.push_touch(1,50,50),"reported touch");
    SourceMainDigitalInputOwner touch_owner;check(touch_owner.begin_explicit_history(profile,touch_router,short_previous,0,&error)&&touch_owner.publish(touch_now,1,&error)&&touch_owner.receipt().masked_pressed(0x0a,0x1f70,&pressed)&&pressed==0x100,"existing shared touch router can produce a genuine new digital press");
    touch_now.pads[0].buttons=2;check(touch_owner.publish(touch_now,2,&error)&&touch_owner.receipt().masked_pressed(0x0a,0x1f70,&pressed)&&pressed==0x200,"existing physical-priority policy remains shared");
    auto invalid=full;invalid.port_count=5;const auto saved=*incomplete.state();check(!incomplete.publish(invalid,3,&error)&&*incomplete.state()==saved,"invalid input extent has no effects");
    SourceMainDigitalInputOwner overflow;check(overflow.begin_explicit_history(profile,router,short_previous,std::numeric_limits<u64>::max(),&error)&&!overflow.publish(short_previous,0,&error),"sample serial never wraps into a fresh capability");
    SourceMainDigitalInputOwner absent;check(!absent.begin_explicit_history(bad,router,short_previous,0,&error)&&!absent.state(),"invalid producer profile cannot seed history");
    std::printf("MAIN digital input shared routing/pressed-history ownership: %u checks PASS\n",checks);
}
