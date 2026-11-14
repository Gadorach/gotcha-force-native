#pragma once
// Explicit no-viewer manager -> scalar -> existing command-core CANDIDATE join.
// No manager/actor commit or ACK. Actual event/backend/PCM consumers remain due.
#include "engine/gameplay/startup_sound_no_viewers.hpp"
#include "engine/gameplay/sound_commands.hpp"
namespace gf {
struct StartupSoundNoViewerManagerPlan {
    StartupSoundState manager;StartupSoundCommandQueue staging;
    std::array<StartupSoundPendingCall,48> calls{};
    std::array<StartupSoundCommandOutcome,48> outcomes{};
    unsigned count=0,tick_samples=0;bool requires_actual_event_backend_pcm_consumer=true;
};
inline bool startup_sound_no_viewer_manager_plan(const StartupSoundProfile& profile,const StartupSoundSpatialProfile& spatial,const SoundCommandBanks& banks,
        const StartupSoundState& input,const StartupSoundCommandQueue& staging,const StartupSoundOperation& operation,
        const StartupSoundNoViewerFrame& frame,std::span<const StartupSoundOwnerObservation> owners,const StartupSoundCommandObservation& controls,
        StartupSoundNoViewerManagerPlan* out,std::string* error){
    if(out&&(&input==&out->manager||&staging==&out->staging)){if(error)*error="no-viewer manager: aliased input/output";return false;}
    if(out)*out={};
    if(!out){if(error)*error="no-viewer manager: output required";return false;}
    StartupSoundPlan manager;if(!startup_sound_plan(profile,input,operation,&manager,error))return false;
    StartupSoundSpatialBatch calls;if(!startup_sound_no_viewers_resolve_batch(spatial,frame,std::span(manager.pending.data(),manager.pending_count),owners,&calls,error))return false;
    StartupSoundCommandPlan commands;if(!sound_command_plan(banks,staging,controls,std::span(calls.commands.data(),calls.count),&commands,error))return false;
    StartupSoundNoViewerManagerPlan plan;plan.manager=std::move(manager.candidate);plan.staging=std::move(commands.candidate);plan.count=manager.pending_count;plan.tick_samples=manager.tick_samples;
    for(unsigned i=0;i<plan.count;++i){plan.calls[i]=manager.pending[i];plan.outcomes[i]=commands.outcomes[i];}
    *out=std::move(plan);if(error)error->clear();return true;
}
} // namespace gf
