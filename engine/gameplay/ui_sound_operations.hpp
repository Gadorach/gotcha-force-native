#pragma once
// Operation outputs, not raw pad clicks. Native diagnostic/modal actions are
// deliberately absent. Capacity2 covers navigation/value then accepted action.
#include <array>
#include "engine/core/types.hpp"
namespace gf {
enum class UiSoundOperationKind:u8 {MainNavigate,MainConfirm,MainOptionEnter,OptionNavigate,OptionValue,OptionActivate,OptionBack};
struct UiSoundOperation {UiSoundOperationKind kind=UiSoundOperationKind::MainNavigate;u8 item=0;s8 sfx_index=-1;bool operator==(const UiSoundOperation&)const=default;};
struct UiSoundBatch {
    std::array<UiSoundOperation,2> operations{};unsigned count=0;bool overflow=false;
    void emit(UiSoundOperationKind kind,u8 item,s8 sfx_index=-1){if(count>=operations.size()){overflow=true;return;}operations[count++]={kind,item,sfx_index};}
    bool operator==(const UiSoundBatch&)const=default;
};
} // namespace gf
