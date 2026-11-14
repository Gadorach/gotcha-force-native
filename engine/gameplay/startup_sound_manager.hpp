#pragma once
// EFDC8/EFE70/EFF30/F0104/F0214/F02D0/F094C state transitions ONLY.
// Plans retain ordered, UNFULFILLED calls at the spatial/voice boundary. They
// are not a PCM producer or authority to bypass an actor's F0104 refusal.
#include "engine/assets/startup_sound.hpp"
#include "engine/gameplay/startup_sound_timebase.hpp"
#include <utility>
namespace gf {
inline constexpr u16 startup_sound_no_descriptor=0xffff;
struct StartupSoundRow {
    u32 timestamp=0; s32 lease=0; u16 descriptor=startup_sound_no_descriptor;
    u64 owner=0; // native generation key, NOT a source/host pointer;0 is null
    bool operator==(const StartupSoundRow&)const=default;
};
struct StartupSoundState {
    std::array<StartupSoundRow,48> rows{}; u8 mode=0,pause_depth=0;
    bool operator==(const StartupSoundState&)const=default;
};
enum class StartupSoundCallKind:u8 {StartDirect,StartBiased,UpdateDirect,UpdateBiased,PauseCentered};
struct StartupSoundPendingCall {
    StartupSoundCallKind kind=StartupSoundCallKind::StartDirect; s16 cue=0; u64 owner=0;
    // Biased calls retain source f1;PauseCentered requires volume0/pan64/pitch8192
    // and a live owner's signed96 at execution. No volume/pan/voice result guessed.
    f32 viewer_bias=0;
    bool operator==(const StartupSoundPendingCall&)const=default;
};
enum class StartupSoundOperationKind:u8 {Refresh,Restart,Clear,Update,Pause,Resume,Reset,StopAll};
struct StartupSoundOperation {
    StartupSoundOperationKind kind=StartupSoundOperationKind::Refresh;
    s32 actor96=0; u32 slot=5,descriptor=0; u64 owner=0;
    bool tick_declared=false; u32 tick=0,bus_clock=0;
};
struct StartupSoundPlan {
    StartupSoundState candidate;
    std::array<StartupSoundPendingCall,48> pending{}; unsigned pending_count=0,tick_samples=0;
    // There is intentionally no commit/ack/sink here. A future owner must stage
    // these state changes WITH actual consumers; simply discarding pending is not success.
};
inline bool startup_sound_state_valid(const StartupSoundProfile& p,const StartupSoundState& s){
    if(!startup_sound_profile_valid(p)||s.mode>2)return false;
    for(const auto& row:s.rows)if(row.descriptor!=startup_sound_no_descriptor&&!startup_sound_descriptor(p,row.descriptor))return false;
    return true;
}
namespace startup_sound_manager_detail {
template<class Tick> inline bool plan(const StartupSoundProfile& p,const StartupSoundState& input,
                               const StartupSoundOperation& op,Tick tick_source,StartupSoundPlan* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("startup sound plan: ")+why;return false;};
    if(!out||!startup_sound_state_valid(p,input))return fail("profile/state domain");
    StartupSoundPlan plan;plan.candidate=input;auto& s=plan.candidate;
    auto emit=[&](StartupSoundCallKind kind,s16 cue,u64 owner){
        const bool biased=kind==StartupSoundCallKind::StartBiased||kind==StartupSoundCallKind::UpdateBiased;
        plan.pending[plan.pending_count++]={kind,cue,owner,biased?p.viewer_bias:0};
    };
    u32 sampled_tick=op.tick,sampled_bus=op.bus_clock;
    auto tick=[&](){if(!tick_source(&sampled_tick,&sampled_bus))return false;plan.tick_samples=1;return true;};
    auto start=[&](const StartupSoundDescriptor& d,u64 owner){emit(d.type==1?StartupSoundCallKind::StartBiased:StartupSoundCallKind::StartDirect,d.start,owner);};
    auto stop=[&](const StartupSoundDescriptor& d,u64 owner){if(d.stop!=255)emit(StartupSoundCallKind::StartDirect,d.stop,owner);};
    switch(op.kind){
    case StartupSoundOperationKind::Refresh:
    case StartupSoundOperationKind::Restart:
    case StartupSoundOperationKind::Clear:{
        const u32 slot=op.slot&255u;
        if(op.actor96<0||op.actor96>=6||slot>=8||!op.owner)return fail("live native owner/signed actor96/slot required");
        auto& row=s.rows[unsigned(op.actor96)*8+slot];
        const auto* old=row.descriptor==startup_sound_no_descriptor?nullptr:startup_sound_descriptor(p,row.descriptor);
        if(op.kind==StartupSoundOperationKind::Clear){
            // F02D0 writes owner even if no descriptor, but clears it only when
            // there WAS a descriptor. It never clears lease.
            row.owner=op.owner;
            if(old){stop(*old,op.owner);row.owner=0;row.timestamp=0;row.descriptor=startup_sound_no_descriptor;}
            break;
        }
        const auto* requested=startup_sound_descriptor(p,u16(op.descriptor&255u));
        if(!requested)return fail("descriptor cell not in audited sparse set");
        if(op.kind==StartupSoundOperationKind::Restart||!old||old->start!=requested->start){
            if(!tick())return fail("conditional OSGetTick observation required");
            if(op.kind==StartupSoundOperationKind::Refresh&&old)stop(*old,op.owner);
            row.descriptor=requested->id;row.timestamp=sampled_tick;start(*requested,op.owner);
        }
        // Equal start retains OLD descriptor/timestamp, even for a different ID.
        row.owner=op.owner;row.lease=2;break;
    }
    case StartupSoundOperationKind::Update:{
        // EFF30 samples once even with no live rows or with mode1.
        if(!tick())return fail("manager OSGetTick observation required");
        for(auto& row:s.rows){
            if(row.descriptor==startup_sound_no_descriptor)continue;
            const auto& d=*startup_sound_descriptor(p,row.descriptor);
            if(s.mode==0){row.lease=std::bit_cast<s32>(u32(row.lease)-1u);
                if(row.lease<1){
                    // Unlike F02D0, expiry has NO stop==255 guard and always
                    // calls direct F036C, including type1 selections.
                    emit(StartupSoundCallKind::StartDirect,d.stop,row.owner);
                    row.owner=0;row.descriptor=startup_sound_no_descriptor;continue;
                }
            }
            // Disassembly computes division BEFORE mode tests, including paused
            // and forced updates. Do not skip divisor validation in those modes.
            const u32 divisor=sampled_bus/500000u;
            if(!divisor)return fail("source bus-clock divisor required for surviving row");
            const u32 elapsed=u32(u32(sampled_tick-row.timestamp)*8u)/divisor;
            if((s.mode==0&&elapsed>=d.interval)||s.mode==2){
                // Only explicit self rows next==0 are admitted by this profile.
                row.timestamp=sampled_tick;
                emit(d.type==1?StartupSoundCallKind::UpdateBiased:StartupSoundCallKind::UpdateDirect,d.start,row.owner);
            }
        }
        if(s.mode==2)s.mode=0;
        break;
    }
    case StartupSoundOperationKind::Pause:
        s.mode=1;s.pause_depth=u8(s.pause_depth+1u);
        for(const auto& row:s.rows)if(row.descriptor!=startup_sound_no_descriptor){
            if(!row.owner)return fail("pause dereferences a live owner");
            emit(StartupSoundCallKind::PauseCentered,startup_sound_descriptor(p,row.descriptor)->start,row.owner);
        }
        break;
    case StartupSoundOperationKind::Resume:
        s.pause_depth=u8(s.pause_depth-1u);
        if(std::bit_cast<s8>(s.pause_depth)<1){s.pause_depth=0;s.mode=2;}
        break;
    case StartupSoundOperationKind::Reset:
    case StartupSoundOperationKind::StopAll:
        for(auto& row:s.rows){
            if(op.kind==StartupSoundOperationKind::StopAll&&row.descriptor!=startup_sound_no_descriptor)stop(*startup_sound_descriptor(p,row.descriptor),row.owner);
            row.owner=0;row.descriptor=startup_sound_no_descriptor;
        }
        s.mode=0; // EFDC8 retains timestamps,leases AND pause-depth byte.
        break;
    default:return fail("unknown operation");
    }
    *out=plan;if(error)error->clear();return true;
}
} // namespace startup_sound_manager_detail
inline bool startup_sound_plan(const StartupSoundProfile& p,const StartupSoundState& input,
                               const StartupSoundOperation& op,StartupSoundPlan* out,std::string* error){
    auto tick=[&](u32* value,u32* bus){if(!op.tick_declared)return false;*value=op.tick;*bus=op.bus_clock;return true;};
    return startup_sound_manager_detail::plan(p,input,op,tick,out,error);
}
struct StartupSoundClockedPlan {StartupSoundPlan manager;StartupSoundTickSample sampled;bool clock_sampled=false;};
inline bool startup_sound_plan_sampled(const StartupSoundProfile& p,const StartupSoundState& input,
        const StartupSoundOperation& operation,StartupSoundTickSource& clock,StartupSoundClockedPlan* out,std::string* error){
    if(out && &input==&out->manager.candidate){if(error)*error="startup sampled manager: aliased input/output";return false;}
    if(out)*out={};
    if(!out){if(error)*error="startup sampled manager: output required";return false;}
    StartupSoundClockedPlan candidate;std::string clock_error;
    auto tick=[&](u32* value,u32* bus){if(!clock.sample(&candidate.sampled,&clock_error))return false;candidate.clock_sampled=true;*value=candidate.sampled.tick;*bus=candidate.sampled.bus_clock;return true;};
    if(!startup_sound_manager_detail::plan(p,input,operation,tick,&candidate.manager,error)){if(error&&!clock_error.empty())*error+="; "+clock_error;return false;}
    *out=std::move(candidate);if(error)error->clear();return true;
}
} // namespace gf
