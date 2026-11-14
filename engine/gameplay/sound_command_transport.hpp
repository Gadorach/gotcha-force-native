#pragma once
// Atomic pure three-bank calls -> source32 staging -> source48 ring transport.
// Caller still commits event/native/PCM state only after downstream success.
#include "engine/gameplay/sound_commands.hpp"
#include "engine/gameplay/startup_sound_dispatch.hpp"
#include "engine/assets/sound_voice_bank.hpp"
namespace gf {
inline bool sound_command_voice_bindings_valid(const SoundCommandBanks& commands,const SoundVoiceLibrary& voices){
    if(!commands.ready()||!voices.ready())return false;
    for(unsigned id=0;id<3;++id)if(const auto tones=commands.find(u8(id))){const auto voice=voices.find(u8(id));if(!voice||tones->tones!=voice->bank.tones)return false;}
    return true;
}
struct SoundCommandTransportPlan {
    StartupSoundCommandQueue staging;StartupSoundRing ring;
    std::array<StartupSoundCommandOutcome,48> outcomes{};unsigned count=0,forwarded=0,skipped=0,dropped=0;
    bool requires_event_voice_tail=true;
};
inline bool sound_command_transport(const SoundCommandBanks& banks,const StartupSoundCommandQueue& input,const StartupSoundRing& ring,
        const StartupSoundCommandObservation& observation,std::span<const StartupSoundScalarCommand> calls,SoundCommandTransportPlan* out,std::string* error){
    if(out&&(&input==&out->staging||&ring==&out->ring)){if(error)*error="sound command transport: aliased input/output";return false;}
    if(out)*out={};
    if(!out){if(error)*error="sound command transport: output required";return false;}
    StartupSoundCommandPlan commands;if(!sound_command_plan(banks,input,observation,calls,&commands,error))return false;
    StartupSoundTransferPlan transport;if(!startup_sound_ring_transfer(commands.candidate,ring,&transport,error))return false;
    SoundCommandTransportPlan plan;plan.staging=std::move(transport.staging);plan.ring=std::move(transport.ring);plan.outcomes=commands.outcomes;plan.count=commands.count;plan.forwarded=transport.forwarded;plan.skipped=transport.skipped;plan.dropped=transport.dropped;
    *out=std::move(plan);if(error)error->clear();return true;
}
} // namespace gf
