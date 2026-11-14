#pragma once
// Explicit three-bank EFB3C/EFCB8 entry. SAME gain/gate/chain/queue core as the
// old bank0-only entry; this is not permission to randomize or ACK playback.
#include "engine/assets/sound_command_banks.hpp"
#include "engine/gameplay/startup_sound_commands.hpp"
namespace gf {
inline bool sound_command_plan(const SoundCommandBanks& banks,const StartupSoundCommandQueue& input,const StartupSoundCommandObservation& observation,
        std::span<const StartupSoundScalarCommand> calls,StartupSoundCommandPlan* out,std::string* error){
    if(out && &input==&out->candidate){if(error)*error="sound commands: aliased queue/output";return false;}
    auto resolve=[&](s16 cue,const StartupSoundToneBank** selected,u8* bank_id,u32* tone,std::string* why){
        const auto id=u8(u16(cue)/128);const auto resource=banks.find(id);if(!resource){if(why)*why="sound commands: explicit cue bank missing (no bank0 alias)";return false;}
        *selected=resource.get();*bank_id=id;*tone=u16(cue)%128;return true;
    };
    return startup_sound_commands_detail::plan_bound(banks.profile(),resolve,input,observation,calls,out,error);
}
} // namespace gf
