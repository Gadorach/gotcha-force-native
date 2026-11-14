#pragma once
// EF390 phases + EF970 stop predicate, ONE explicit audio-control callback.
// No graphics tick/delta/FPS inference, backend calls or event completion ACK.
#include "engine/assets/sound_envelope.hpp"
namespace gf {
struct SoundEnvelopeState {
    u8 phase=0;u32 level=0;bool done=false;
    bool operator==(const SoundEnvelopeState&)const=default;
};
struct SoundEnvelopeObservation {
    u8 event_stop=0; s32 requested=0;
    // Read only on EF970's event_stop==0 branch (including FIRST callback).
    bool backend_state_declared=false;u16 backend_state=0;
    bool stolen=false;
};
struct SoundEnvelopePlan {
    SoundEnvelopeState state;
    u32 sustain_level=0,attack=0,decay=0,sustain=0,release=0;
    bool publish_parameters=false,start_backend=false,stop_and_free_backend=false;
    unsigned stop_checks=0,backend_reads=0;bool stop_transition=false;
    bool operator==(const SoundEnvelopePlan&)const=default;
};
inline bool sound_voice_envelope_step(const SoundEnvelopeProfile& profile,u32 word,const SoundEnvelopeState& state,const SoundEnvelopeObservation& observation,SoundEnvelopePlan* out,std::string* error){
    // Input state may be inside the previous output. Never erase it on alias.
    if(out && &state==&out->state){if(error)*error="voice envelope: aliased input/output";return false;}
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("voice envelope: ")+why;return false;};
    constexpr u32 full=0x07ffffffu;
    if(!out||!profile.ready()||state.phase>6||(state.phase>0&&state.phase<6&&state.level>full)||(state.done&&state.phase!=6))return fail("profile/state domain");
    SoundEnvelopePlan plan;plan.state=state;
    // These indices are literal BE sample-word fields; they share ONE table.
    plan.sustain_level=((word>>13)&63u)*0x200000u;
    plan.attack=profile.rate(word>>25);plan.decay=profile.rate(((word>>19)&63u)*2);
    plan.sustain=profile.rate(((word>>6)&63u)*2);plan.release=profile.rate((word&63u)*2);
    if(observation.stolen){plan.state.phase=6;plan.state.done=true;}
    bool first=false;
    auto stop=[&](){
        ++plan.stop_checks;
        if(!observation.event_stop){
            if(!observation.backend_state_declared)return -1;
            ++plan.backend_reads;
            if(observation.backend_state||first)return 0;
            plan.state.level=0;
        }
        plan.state.phase=observation.requested==0x10?5:4;plan.stop_transition=true;return 1;
    };
    auto release=[&](){
        if(stop()<0)return false;
        // EF970 can change phase4 to5, but THIS callback still uses normal rate.
        if(plan.state.level<=plan.release){plan.state.level=0;plan.state.phase=6;plan.state.done=true;plan.stop_and_free_backend=true;}
        else plan.state.level-=plan.release;
        return true;
    };
    switch(plan.state.phase){
    case 0:plan.state.level=0;plan.state.phase=1;first=true;[[fallthrough]];
    case 1:{const int result=stop();if(result<0)return fail("missing backend-state observation");if(!result){const u32 sum=plan.state.level+plan.attack;if(sum>=full){plan.state.level=full;plan.state.phase=2;}else plan.state.level=sum;}break;}
    case 2:{const int result=stop();if(result<0)return fail("missing backend-state observation");if(!result){const s64 remaining=s64(plan.state.level)-plan.decay;if(remaining<=plan.sustain_level){plan.state.level=plan.sustain_level;plan.state.phase=3;}else plan.state.level=u32(remaining);}break;}
    case 3:{const int result=stop();if(result<0)return fail("missing backend-state observation");if(!result){if(word&0x1000u){const u32 sum=plan.state.level+plan.sustain;plan.state.level=sum>full?full:sum;}
            else if(plan.state.level<=plan.sustain){plan.state.level=0;plan.state.phase=4;if(!release())return fail("missing backend-state observation");}
            else plan.state.level-=plan.sustain;}break;}
    case 4:if(!release())return fail("missing backend-state observation");break;
    case 5:if(plan.state.level<=0x01555555u){plan.state.level=0;plan.state.phase=6;plan.state.done=true;plan.stop_and_free_backend=true;}else plan.state.level-=0x01555555u;break;
    case 6:break;
    }
    // Changing 1/2/3 to release is not itself the terminal/free action. Even
    // zero level is published for one callback; release executes next time.
    plan.publish_parameters=!observation.stolen&&state.phase!=6&&!plan.stop_and_free_backend;
    plan.start_backend=first&&plan.publish_parameters;
    *out=plan;if(error)error->clear();return true;
}
} // namespace gf
