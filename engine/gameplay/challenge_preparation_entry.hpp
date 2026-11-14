#pragma once
#include "engine/assets/challenge_preparation_entry.hpp"
#include "engine/gameplay/scene_physical_fields.hpp"
#include <memory>
#include <optional>
namespace gf {
struct ChallengePreparationEntryState {
    u8 common50=1,common64=0,root00=0,substate01=0;
    u32 countera8=0,counterac=0,counterb0=0;u64 native_revision=1;
    bool operator==(const ChallengePreparationEntryState&)const=default;
};
namespace challenge_preparation_entry_detail {
//Only the fresh presentation opt-in can publish a weak reference to this owner.
//Neither copied inspection fields nor an equal profile can create that link.
struct Storage {bool live=true,handoff_ready=false,prefix_claimed=false,branch_claimed=false;std::optional<u8> scene45;std::weak_ptr<ScenePhysicalControlFields> physical_controls;ChallengePreparationEntryState state;};
}
} // namespace gf
