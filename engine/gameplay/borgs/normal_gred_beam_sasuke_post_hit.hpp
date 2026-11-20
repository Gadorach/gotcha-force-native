#pragma once
#include <bit>
#include "engine/gameplay/borgs/normal_gred_beam_sasuke_damage_tables.hpp"

namespace gf {
struct GRedBeamSasukeReceiverCounters {
    s16 restore6be=0,remaining6c2=0,threshold6c4=0,impact6c6=0,progress6c8=0;
    u8 tier6ca=0,flags6fd=0,inhibit7de=0;
    u32 behaviour5e0=0;
    f32 timer684=0,timer688=0,timer68c=0;
};
struct GRedBeamSasukeReactionPlan {
    GRedBeamSasukeReceiverCounters fields;
    s32 row2_impact=0,receiver_threshold=0;
    bool visited=false,reduced6c6=false,reset6c2=false;
};
// Ordinary, unbuffed Sasuke scalar tail at 2F304..2F444. These
// receiver counters are not HP, a selected reaction, or a KO outcome.
inline bool normal_gred_beam_sasuke_reaction_tail(NormalGRedNeutralBSource source,
    const GRedBeamSasukeContactPrefix& contact,u8 receiver_slot,s16 hp_after,f32 beam_c4,f32 receiver_b4,
    const GRedBeamSasukeReceiverCounters& in,GRedBeamSasukeReactionPlan* out){
    if(!out||!source.reviewed()||receiver_slot>=6||receiver_slot==source.slot||
       contact.beam_receiver290!=receiver_slot||contact.credit_receiver290!=receiver_slot||
       contact.receiver_impact704||contact.receiver_reaction702!=4||contact.receiver_binding71a!=2||
       in.flags6fd!=contact.receiver_flags6fd||hp_after<0)return false;
    GRedBeamSasukeReactionPlan next;next.fields=in;
    if(!hp_after){*out=next;return true;}
    auto& f=next.fields;
    if(!std::isfinite(beam_c4)||beam_c4<=0||beam_c4>16||!std::isfinite(receiver_b4)||receiver_b4<=0||receiver_b4>16||
       f.restore6be<=0||f.remaining6c2<0||f.threshold6c4<0||f.progress6c8<0||f.progress6c8>=100||f.tier6ca>63)return false;
    next.visited=true;
    const auto half_delta=[](f32 scale){return f32(.5f*f32(scale-1.f));};
    next.row2_impact=50+s32(f32(50.f*half_delta(beam_c4)));
    next.receiver_threshold=s32(f.threshold6c4)+s32(f32(f32(f.threshold6c4)*half_delta(receiver_b4)));
    if(!f.inhibit7de&&(!(f.behaviour5e0&0x01000000u)||next.receiver_threshold<=next.row2_impact)){
        f.impact6c6=std::bit_cast<s16>(u16(s32(f.impact6c6)-next.row2_impact));
        next.reduced6c6=true;
    }
    f.timer684=f.timer688=f.timer68c=60;
    if(!(f.flags6fd&0x80u)){
        f.remaining6c2=s16(f.remaining6c2-50); // exact row +4
        if(f.remaining6c2<=0){
            f.remaining6c2=f.restore6be;f.progress6c8=0;f.tier6ca=0;f.flags6fd|=0xa6u;
            next.reset6c2=true;
        }
    }
    f.progress6c8=s16(f.progress6c8+50); // independent row +5
    if(f.progress6c8>=100){f.progress6c8=0;if(f.tier6ca<63)++f.tier6ca;}
    *out=next;return true;
}

struct GRedBeamSasukePostHitInput {
    bool reviewed_post_contact_declared=false;
    u32 credit_total418=0,receiver_total41c=0;
    GRedBeamSasukeReceiverCounters receiver;
};
struct GRedBeamSasukePostHitPlan {
    GRedBeamSasukeHealthPlan health;
    GRedBeamSasukeReactionPlan reaction;
    u32 credit_total418=0,receiver_total41c=0;
    u8 credit_slot=255,receiver_slot=255;
    bool stat_write=false,request_ko_continuation=false;
};
// Detached ordinary after-hit proposal. Positive raw amount goes to
// the two stats before the HP clamp; zero crossing selects a different
// continuation. This publishes neither the counters nor the KO/effects.
inline bool normal_gred_beam_sasuke_post_hit(NormalGRedNeutralBSource source,
    const GRedBeamSasukeContactPrefix& contact,const GRedBeamSasukeHealthInput& health,
    const GRedBeamSasukeTableSample& sample,const GRedSasukeDamageTable& table,
    const GRedBeamSasukePostHitInput& in,GRedBeamSasukePostHitPlan* out){
    GRedBeamSasukeTableIndices checked;
    if(!out||!in.reviewed_post_contact_declared||sample.credit_slot!=source.slot||sample.receiver_slot!=health.receiver_slot||
       !normal_gred_beam_sasuke_table_indices(sample,&checked)||in.receiver.tier6ca!=sample.receiver_tier6ca||
       in.receiver.behaviour5e0!=health.receiver_behaviour5e0||in.receiver.flags6fd!=contact.receiver_flags6fd||
       (health.receiver_flags1da&2u))return false;
    GRedBeamSasukePostHitPlan next;
    if(!normal_gred_beam_sasuke_health_from_tables(source,contact,health,sample,table,&next.health))return false;
    next.credit_slot=source.slot;next.receiver_slot=health.receiver_slot;
    next.credit_total418=in.credit_total418;next.receiver_total41c=in.receiver_total41c;
    if(next.health.amount>0&&!health.scene_hp_hold1f){
        next.stat_write=true;next.credit_total418+=u32(next.health.amount);next.receiver_total41c+=u32(next.health.amount);
    }
    next.request_ko_continuation=next.health.reaches_zero;
    if(!normal_gred_beam_sasuke_reaction_tail(source,contact,health.receiver_slot,next.health.current1c6,
                                              health.beam_relative_c4,health.receiver_b4,in.receiver,&next.reaction))return false;
    *out=next;return true;
}
}
