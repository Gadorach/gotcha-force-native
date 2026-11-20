#pragma once
#include "engine/gameplay/borgs/normal_gred_beam_sasuke_ko_observers.hpp"
#include "engine/gameplay/source_byte_rng.hpp"
#include "engine/gameplay/story_reward_counters.hpp"

namespace gf {
struct GRedSasukeKoTallyInput {
    bool caller_and_rng_declared=false;
    u8 credit_slot=255,receiver_slot=255,context32=255,credit_selector3e6=255;
    u8 credit_kind83=255,receiver_kind83=255,receiver_link4a0=255,receiver_variant3ed=255;
    u16 receiver_model3ea=0;
    SourceByteRngState rng;
};
struct GRedSasukeKoTallyPlan {
    u32 active_index=0,stored_word=0,active_count=0;
    SourceByteRngState rng;
    u8 draw=0,receiver_credit5a4_slot=255;
    bool appended=false;
};

// Direct post-KO caller 2F204..2F304 and its 18214C tally update.
// Returns one active-table patch and one prospective byte-RNG step, not
// a drop roll, an inventory grant, a save write, or a native actor reference.
inline bool normal_gred_beam_sasuke_ko_tally(NormalGRedNeutralBSource source,
    const GRedSasukeKoObserverPlan& observers,const StoryRewardCounters& counters,
    const GRedSasukeKoTallyInput& in,GRedSasukeKoTallyPlan* out){
    if(!out||!source.reviewed()||!in.caller_and_rng_declared||in.credit_slot!=source.slot||in.receiver_slot>=6||
       in.credit_slot==in.receiver_slot||observers.credit_slot!=in.credit_slot||observers.receiver_slot!=in.receiver_slot||
       in.context32||in.credit_selector3e6||in.credit_kind83||in.receiver_kind83||in.receiver_link4a0||
       in.receiver_model3ea!=0x000a||in.receiver_variant3ed||!story_reward_counters_valid(counters))return false;
    const u32 key=u32(in.receiver_model3ea)<<16;
    u32 index=0,word=key;
    for(;index<counters.active_count;++index){
        const u32 decoded=~counters.active[index];
        if((decoded&0xffff0000u)==key){word=decoded;break;}
    }
    GRedSasukeKoTallyPlan next;
    next.appended=index==counters.active_count;
    // The source has no capacity check here. Refuse a detached append
    // beyond the active table rather than proposing a write into its backup.
    if(next.appended&&index==counters.active.size())return false;
    next.active_index=index;next.active_count=counters.active_count+u32(next.appended);
    next.rng=in.rng;next.draw=source_byte_rng_step(next.rng);
    const u32 increment=(next.draw&15u)+1u;
    if((word&0xffffu)<0x10000u-increment)word+=increment;
    next.stored_word=~word;next.receiver_credit5a4_slot=in.credit_slot;
    *out=next;return true;
}
}
