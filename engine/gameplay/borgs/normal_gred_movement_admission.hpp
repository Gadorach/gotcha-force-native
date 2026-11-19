#pragma once
#include <array>
#include "engine/core/types.hpp"

namespace gf {
// Each row tries the first permitted intent for one recovered lower selector.
// A present empty row leaves continuation to the movement handler; an absent
// row is unsupported. ContactTail requests a check, not a contact result.
struct NormalGRedMovementAdmissionRules {
    enum class Intent:u8 {
        None,Jump,GroundDash,Walk,Stop,ReleaseLift,AirDash,ReadyRelaunch,
        Relaunch,ReleaseDash,ContactTail
    };
    struct Rule {u8 selector;std::array<Intent,3> order;};
    static constexpr std::array rules{
        Rule{0,{Intent::Jump,Intent::GroundDash,Intent::Walk}},
        Rule{1,{Intent::Jump,Intent::GroundDash,Intent::Stop}},
        Rule{2,{Intent::Jump,Intent::GroundDash,Intent::Stop}},
        Rule{3,{Intent::Jump,Intent::GroundDash,Intent::Walk}},
        Rule{4,{}},
        Rule{5,{Intent::ReleaseLift,Intent::AirDash}},
        Rule{6,{Intent::ReadyRelaunch,Intent::AirDash,Intent::ContactTail}},
        Rule{7,{Intent::Jump,Intent::GroundDash}},
        Rule{8,{Intent::ReleaseLift,Intent::AirDash,Intent::ContactTail}},
        Rule{12,{Intent::Jump}},
        Rule{13,{Intent::Jump}},
        Rule{14,{Intent::Jump}},
        Rule{15,{}},
        Rule{16,{Intent::Relaunch,Intent::ReleaseDash,Intent::ContactTail}},
        Rule{17,{Intent::ReadyRelaunch,Intent::AirDash,Intent::ContactTail}},
    };
    static constexpr const Rule* for_selector(u8 selector){
        for(const auto& rule:rules)if(rule.selector==selector)return &rule;
        return nullptr;
    }
};
}
