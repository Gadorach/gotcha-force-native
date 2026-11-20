#pragma once
#include "engine/core/types.hpp"

namespace gf {
struct ModelFormRebindPrefix {
    bool requested=false;
    u16 target_model=0;
    u8 selected_variant=0,next_revision3f0=0;
    bool operator==(const ModelFormRebindPrefix&)const=default;
};

// Shared scalar prefix once an exact class leaf has selected its destination.
// The selected retail variant doesn't change. The source revision byte wraps
// and is not an actor identity, lifetime, or native acceptance token.
constexpr ModelFormRebindPrefix model_form_rebind_prefix(u16 target_model,u8 selected_variant,u8 revision3f0){
    if(!target_model)return {};
    return {true,target_model,selected_variant,u8(revision3f0+1u)};
}
}
