#pragma once
#include "engine/gameplay/character_program_directional_control.hpp"
#include "engine/gameplay/actor_sound_events.hpp"
namespace gf {
// Source5B880/5B98C for the TWO represented primary motion lanes. Reads actual
// same-VM native snapshots, not caller-supplied clock observations. Extra physical
// lanes, actorB4/owner production and downstream sound delivery remain boundaries.
// Pure candidate DATA output: invoking this does not consume/acknowledge a frame
// or sound. A serialized actor owner must publish it with its accepted update.
inline bool character_program_sound_events(const CharacterProgramVm& vm,const CharacterProgramDirectionalControl& control,
                                           const ActorSoundEventBank& bank,f32 scale_b4,u64 owner,
                                           std::vector<StartupSoundPendingCall>* out,std::string* error){
    if(out)out->clear();
    auto fail=[&](const char* why){if(error)*error=std::string("program sound events: ")+why;return false;};
    CharacterProgramClassFields fields;
    if(!out||!owner||!vm.ready()||!vm.actor_directional_control_mode()||!vm.observe_class_fields(&fields)||
       !fields.scope.lock()||control.scope().lock()!=fields.scope.lock()||!bank.ready()||bank.model_hash()!=vm.model_hash())return fail("VM/model/event-bank/control scope");
    std::vector<StartupSoundPendingCall> calls;
    for(unsigned lane=0;lane<2;++lane){const auto& state=vm.state().lanes[lane];const u8 script=state.metadata[4];
        if(script==255)continue; //source sentinel; no invented null motion reads
        if(script>=128)return fail("signed script outside audited native domain");
        const auto* snapshot=control.snapshot(state.motion);
        if(!snapshot||snapshot->lane()!=lane)return fail("same-library accepted primary snapshot");
        ActorSoundClock clock;clock.elapsed=state.motion.elapsed;clock.previous_elapsed=snapshot->previous_elapsed();
        clock.frame=state.motion.frame;clock.previous_frame=snapshot->previous_frame();clock.end_frame=state.motion.end_frame;clock.scale_b4=scale_b4;
        std::vector<StartupSoundPendingCall> selected;
        if(!actor_sound_events(bank,script,clock,owner,&selected,error))return false;
        calls.insert(calls.end(),selected.begin(),selected.end());
    }
    *out=std::move(calls);if(error)error->clear();return true;
}
} // namespace gf
