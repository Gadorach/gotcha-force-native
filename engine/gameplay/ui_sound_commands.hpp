#pragma once
// Original null-owner wrappers: F0468 -> EFB3C (gated); F03C0 -> EFC04
// (no global disabled read) for OPTION row/value119. Both settings-weight once.
#include "engine/gameplay/ui_sound_operations.hpp"
#include "engine/gameplay/sound_commands.hpp"
namespace gf {
struct UiSoundCommandPlan {StartupSoundCommandQueue candidate;std::array<StartupSoundCommandOutcome,2> outcomes{};unsigned count=0;};
inline bool ui_sound_command_plan(const SoundCommandBanks& banks,const StartupSoundCommandQueue& input,const StartupSoundCommandObservation& observation,
        const UiSoundBatch& operations,UiSoundCommandPlan* out,std::string* error){
    if(out && &input==&out->candidate){if(error)*error="UI sound commands: aliased queue/output";return false;}
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("UI sound commands: ")+why;return false;};
    if(!out||operations.overflow||operations.count>2||!startup_sound_commands_detail::queue_valid(input))return fail("operation/queue/output budget");
    UiSoundCommandPlan plan;plan.candidate=input;
    for(unsigned i=0;i<operations.count;++i){const auto& op=operations.operations[i];s16 cue=0;bool bypass=false;
        switch(op.kind){
        case UiSoundOperationKind::MainNavigate:if(op.item>=7)return fail("main item domain");cue=119;break;
        case UiSoundOperationKind::MainConfirm:if(op.item==0||op.item>=7)return fail("non-OPTION confirm item required");cue=124;break;
        case UiSoundOperationKind::MainOptionEnter:if(op.item!=0)return fail("OPTION seal required");cue=122;break;
        case UiSoundOperationKind::OptionNavigate:if(op.item>9)return fail("OPTION row domain");cue=119;bypass=true;break;
        case UiSoundOperationKind::OptionValue:if(op.item>=8)return fail("OPTION value row required");cue=119;bypass=true;break;
        case UiSoundOperationKind::OptionActivate:if(op.item!=8&&op.item!=9)return fail("represented OPTION action required");cue=122;break;
        case UiSoundOperationKind::OptionBack:if(op.item>9)return fail("OPTION back row domain");cue=123;break;
        default:return fail("unrepresented operation kind");
        }
        auto gates=observation;
        if(op.kind==UiSoundOperationKind::MainNavigate||op.kind==UiSoundOperationKind::MainConfirm||op.kind==UiSoundOperationKind::MainOptionEnter){if(op.sfx_index!=-1)return fail("main operation settings belong to external owner snapshot");}
        else{if(op.sfx_index<0||op.sfx_index>=5)return fail("captured OPTION source SFX setting required");gates.settings_declared=true;gates.sfx_index=op.sfx_index;}
        if(bypass){gates.gate_declared=true;gates.disabled=0;}
        StartupSoundScalarCommand call;call.cue=cue;call.volume=127;call.pan=64;call.pitch=8192; // null owner/group0 regardless of confirming controller
        StartupSoundCommandPlan result;if(!sound_command_plan(banks,plan.candidate,gates,std::span(&call,1),&result,error))return false;
        plan.candidate=std::move(result.candidate);plan.outcomes[plan.count++]=result.outcomes[0];
    }
    *out=std::move(plan);if(error)error->clear();return true;
}
} // namespace gf
