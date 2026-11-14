#pragma once
// Source8005568C/8006DCC0/80055EE4 scalar subset. Dependent callbacks that may
// mutate actor state REFUSE, not a fake continuation after an unconsumed event.
#include <span>
#include "engine/assets/startup_actor_runtime.hpp"
#include "engine/gameplay/startup_actor_setup.hpp"
#include "engine/gameplay/startup_actor_class.hpp"
namespace gf {
struct StartupActorReload {s16 current=0,previous=0,target=0,mode=0,interval=0;f32 remaining=0;};
struct StartupActorPeerObservation {u32 handle=0;u8 phase18=0;};
struct StartupActorBookkeeping {
    u32 model_hash=0;bool initialized=false;StartupActorClassState klass;
    f32 motion_delta=0,timer_delta=0,rate768=0;
    f32 timer694=0;std::array<f32,5> timers690{}; //690/698/69C/6A0/6A4
    std::array<f32,4> timers684{}; //684/688/7D8/68C
    f32 timer720=0,field8c8=0,size7fc=0,scale_b4=0;
    u8 phase18=1,phase19=0,field6fc=0,field7de=0,field4a1=0,field71c=0;
    std::array<u8,4> byte_counters{}; //656/593/594/596
    u8 field708=0,field709=0,field707=0,field661=0,cached87=0,field747=0,field73f=0;
    u8 field6ca=0,field7d4=0,field5da=0,field5db=0,field8cc=0,field1eb=0,field276=0,field7c9=0,field597=0;
    s16 counter6b8=0,counter6ba=0,value1c6=0,value736=0,limit736=0,increment736=0,counter73a=0;
    s16 field6c2=0,field6be=0,field6c8=0;
    u32 flags8c4=0;std::array<u8,6> counters_a8{};
    std::array<u32,8> peers{};u8 peer_count=0;
    std::array<StartupActorReload,3> reloads{};
};
namespace startup_actor_bookkeeping_detail {
inline s16 narrow(s32 v){return std::bit_cast<s16>(u16(v));}
inline bool valid(const StartupActorBookkeeping& s){
    if(!s.initialized||!s.klass.callbacks_initialized||(s.klass.model_id!=0x615&&s.klass.model_id!=10)||s.peer_count>8)return false;
    for(f32 v:{s.motion_delta,s.timer_delta,s.rate768})if(!std::isfinite(v)||v<0||v>16)return false;
    for(f32 v:{s.timer694,s.timer720,s.field8c8,s.size7fc,s.scale_b4})if(!std::isfinite(v)||std::abs(v)>1e7f)return false;
    for(f32 v:s.timers690)if(!std::isfinite(v)||std::abs(v)>1e7f)return false;
    for(f32 v:s.timers684)if(!std::isfinite(v)||std::abs(v)>1e7f)return false;
    for(const auto& r:s.reloads)if(!std::isfinite(r.remaining)||std::abs(r.remaining)>1e7f)return false;
    for(unsigned i=0;i<8;++i)if((i<s.peer_count)!=(s.peers[i]!=0))return false;
    return true;
}
} // namespace startup_actor_bookkeeping_detail
inline bool startup_actor_bookkeeping_initialize(const StartupActorSetupProfile& setup,const StartupActorRuntimeProfile& runtime,
                                                 const StartupActorClassState& klass,StartupActorBookkeeping* out,std::string* error){
    auto fail=[&](const char* why){if(error)*error=std::string("startup actor bookkeeping init: ")+why;return false;};
    if(!out||!startup_actor_setup_detail::valid(setup)||!startup_actor_runtime_valid(runtime)||setup.model_id!=runtime.model_id||setup.model_hash!=runtime.model_hash||klass.model_id!=setup.model_id||!klass.callbacks_initialized)return fail("model/config/class binding");
    StartupActorInitialSetup initial;if(!startup_actor_initial_setup(setup,{},&initial,error))return false;
    StartupActorBookkeeping s;s.model_hash=setup.model_hash;s.klass=klass;s.klass.field272=0;
    s.motion_delta=s.timer_delta=s.rate768=setup.one; //4BB14:804370F0 ->1DC8/1DCC;56180 ->768
    s.value1c6=runtime.row[0];s.value736=initial.health736;s.limit736=initial.health738;s.increment736=std::bit_cast<s16>(startup_actor_setup_detail::data(setup).be16(8));
    s.size7fc=initial.field7fc;s.scale_b4=initial.scale_b4;s.field661=setup.class_config[1];s.field8cc=setup.actor_data[386];
    for(unsigned i=0;i<2;++i){auto& r=s.reloads[i*2];const unsigned at=1+i*4;r.current=runtime.row[at];r.previous=runtime.row[at+1];r.target=runtime.row[at+1];r.mode=runtime.row[at+2];r.interval=runtime.row[at+3];if(!r.current&&(r.mode==0||r.mode==1))r.remaining=f32(r.interval);}
    s.initialized=true;if(!startup_actor_bookkeeping_detail::valid(s))return fail("initial scalar domain");*out=s;return true;
}
inline bool startup_actor_pre_update(const StartupActorRuntimeProfile& p,const StartupActorBookkeeping& input,
                                    std::span<const StartupActorPeerObservation> peers,StartupActorBookkeeping* out,std::string* error){
    using namespace startup_actor_bookkeeping_detail;
    auto fail=[&](const char* why){if(error)*error=std::string("startup actor preupdate: ")+why;return false;};
    if(!out||!startup_actor_runtime_valid(p)||p.model_hash!=input.model_hash||p.model_id!=input.klass.model_id||!valid(input)||peers.size()>8)return fail("state/profile/peer domain");
    auto s=input;auto& flags=s.klass.flags5e0;
    if(!(flags&3u)&&s.timer694>0){s.timer694=f32(s.timer694-s.motion_delta);if(s.timer694<=0){s.timer694=0;flags&=~3u;s.field73f=0;}}
    for(auto& v:s.timers690)if(v>0){v=f32(v-s.rate768);if(v<0)v=0;}
    if(!s.field6fc){if(s.field7de)s.field7de=u8(s.field7de-1u);}else s.field7de=1;
    if(s.counter6b8){if(s.field4a1)s.counter6b8=0;else if(!s.field71c)s.counter6b8=narrow(s32(s.counter6b8)-1);}
    for(auto& v:s.byte_counters)if(v)v=u8(v-1u);
    if(s.counter6ba){if(s.phase18==1&&s.phase19==2){s.counter6ba=narrow(s32(s.counter6ba)-1);if(s.counter6ba%30==0&&s.value1c6>1)s.value1c6=narrow(s32(s.value1c6)-1);}if(s.counter6ba<1)s.flags8c4&=~4u;}
    for(unsigned i=0;i<6;++i)if(s.field708&(1u<<i)){if(s.field709&(1u<<i))s.counters_a8[i]=2;if(s.counters_a8[i])--s.counters_a8[i];}
    if(u8(s.field709&s.field708&u8(~s.field707)))return fail("800F0104 consumer required");
    const bool special=s.field661==2||s.field661==3||s.field661==6;
    if((special&&!(flags&0x80u))||(!special&&!s.cached87)){if(!s.counter73a){s.value736=narrow(s32(s.value736)+s.increment736);if(s.value736>s.limit736)s.value736=s.limit736;}else s.counter73a=narrow(s32(s.counter73a)-1);if(!special)s.field747=0;}
    if(!(flags&0x01000000u))for(unsigned i=0;i<4;++i){auto& v=s.timers684[i];if(v>0){v=f32(v-s.timer_delta);if(v<=0){v=0;if(i==0)s.field6c2=s.field6be;else if(i==1)return fail("8003D3E8 consumer required");else if(i==2)return fail("8003D444 consumer required");else{s.field6c8=0;s.field6ca=0;}}}}
    if(s.timer720>0){s.timer720=f32(s.timer720-s.timer_delta);if(s.timer720<=0){s.timer720=0;s.klass.field272=0;}}
    for(unsigned i=0;i<8;++i){const u32 handle=s.peers[i];if(!handle)continue;bool known=false;u8 phase=0;for(const auto& observation:peers)if(observation.handle==handle){if(known&&phase!=observation.phase18)return fail("inconsistent peer phase");phase=observation.phase18;known=true;}
        if(!known)return fail("missing peer phase observation");
        if(std::bit_cast<s8>(phase)>1){for(unsigned j=i;j<7;++j)s.peers[j]=s.peers[j+1];s.peers[7]=0;--s.peer_count;}}
    const f32 rate=s.field6fc?f32(s.rate768*p.reload_multiplier):s.rate768;
    for(unsigned i=0;i<3;++i){auto& r=s.reloads[i];if((s.field597&(1u<<i))||r.remaining<=0)continue;
        if(r.mode==0||r.mode==1){r.remaining=f32(r.remaining-rate);if(r.remaining<=0){r.current=r.previous=r.target;if(r.mode==0)r.remaining=0;}
            else if(r.mode==1){if(r.interval<=0)return fail("reload interval domain");const s32 count=s32(f32(r.remaining/f32(r.interval)))+1,value=s32(r.target)-count;if(r.current!=value)r.current=r.previous=narrow(value);}}}
    if(!(flags&0x80000000u)&&s.field7d4){if(flags&0x02000000u)return fail("8005C290 recovery consumer required");s.value736=s.limit736;s.field747=0;}
    if(s.field5da==1){if(s.field8c8>=0)s.field8c8=f32(s.field8c8+s.timer_delta);}else s.field8c8=0;
    if(!valid(s))return fail("derived scalar bounds");
    *out=s;return true;
}
inline bool startup_actor_post_update(const StartupActorRuntimeProfile& p,const StartupActorBookkeeping& input,StartupActorBookkeeping* out){
    using namespace startup_actor_bookkeeping_detail;
    if(!out||!startup_actor_runtime_valid(p)||p.model_hash!=input.model_hash||p.model_id!=input.klass.model_id||!valid(input))return false;
    auto s=input;s.cached87=u8((s.klass.flags5e0>>6)&1u);const s8 mode=std::bit_cast<s8>(s.field8cc);
    if(mode==1)s.field1eb=0;else if(mode==0)s.field1eb=f32(s.size7fc*s.scale_b4)<p.size_threshold;else s.field1eb=1;
    s.field276=(s.klass.flags5e0&0x20000000u)!=0;if(s.field7c9)s.klass.field272=0xffff;*out=s;return true;
}
} // namespace gf
