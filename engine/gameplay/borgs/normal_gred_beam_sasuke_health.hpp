#pragma once
#include "engine/gameplay/borgs/normal_gred_beam_sasuke_contact.hpp"

namespace gf {
// Resolved source factors for this ordinary, direct-credit pair. Their
// lookup context remains caller-owned; an absent lookup is NOT unity.
struct GRedBeamSasukeHealthFactors {
    bool declared=false;
    f32 credit_difficulty=0,credit_health=0,credit_force=0,credit_rank=0;
    f32 receiver_difficulty=0,receiver_health=0,receiver_force=0,receiver_rank=0,receiver_tier=0;
};
struct GRedBeamSasukeHealthInput {
    bool reviewed_ordinary=false,scene_hp_hold1f=false;
    u8 receiver_slot=255,receiver_flags1da=0;
    u32 receiver_mask5a0=0,receiver_behaviour5e0=0;
    f32 beam_relative_c4=0,receiver_b4=0;
    s16 current1c6=0,max1c4=0,previous1c8=0;
    GRedBeamSasukeHealthFactors factors;
};
struct GRedBeamSasukeHealthPlan {
    s16 amount=0,current1c6=0,previous1c8=0;
    bool write_health=false,reaches_zero=false;
};

// Ordinary, direct normal 0615/0 -> normal 000A/0, after the separate
// admitted contact prefix. This proposes only the source amount and
// 3D344 HP fields. No stats, effects, KO dispatch or actor publication.
inline bool normal_gred_beam_sasuke_health_plan(NormalGRedNeutralBSource source,
                                                const GRedBeamSasukeContactPrefix& contact,
                                                const GRedBeamSasukeHealthInput& in,GRedBeamSasukeHealthPlan* out){
    if(!out||!source.reviewed()||!in.reviewed_ordinary||in.receiver_slot>=6||in.receiver_slot==source.slot||
       contact.beam_receiver290!=in.receiver_slot||contact.credit_receiver290!=in.receiver_slot||
       contact.receiver_impact704||contact.receiver_reaction702!=4||contact.receiver_binding71a!=2||
       in.current1c6<0||in.max1c4<=0||in.current1c6>in.max1c4||in.previous1c8<0)return false;
    GRedBeamSasukeHealthPlan next;next.current1c6=in.current1c6;next.previous1c8=in.previous1c8;
    if(!(in.receiver_flags1da&2u)&&!(in.receiver_mask5a0&4u)){
        const auto& f=in.factors;
        const auto bounded=[](f32 x){return std::isfinite(x)&&x>=0&&x<=16;};
        if(!f.declared||!bounded(in.beam_relative_c4)||!bounded(in.receiver_b4)||
           in.beam_relative_c4<=0||in.receiver_b4<=0)return false;
        for(f32 k:{f.credit_difficulty,f.credit_health,f.credit_force,f.credit_rank,f.receiver_difficulty,
                   f.receiver_health,f.receiver_force,f.receiver_rank,f.receiver_tier})if(!bounded(k))return false;
        // 3CD5C first adds the half-relative projectile adjustment to row +0.
        f32 value=f32(20.f+f32(20.f*f32(.5f*f32(in.beam_relative_c4-1.f))));
        for(f32 k:{f.credit_difficulty,f.credit_health,f.credit_force,f.credit_rank,f.receiver_difficulty})
            value=f32(value*k); // exact class-pair factor between the two sides is 1
        const f32 receiver_inverse=f32(1.f/f32(1.f+f32(.5f*f32(in.receiver_b4-1.f))));
        value=f32(value*receiver_inverse);
        for(f32 k:{f.receiver_health,f.receiver_force,f.receiver_rank,f.receiver_tier})value=f32(value*k);
        if(in.receiver_behaviour5e0&0x04000000u)value=f32(value*.5f);
        if(!std::isfinite(value)||value>=32768.f)return false;
        next.amount=s16(value<1.f?1.f:value); // source floor at 1 before truncation
    }
    // The source still computes the amount when the independent HP writer is held.
    if(in.current1c6&&!in.scene_hp_hold1f){
        next.write_health=true;next.previous1c8=in.current1c6;
        const s32 left=s32(in.current1c6)-s32(next.amount);
        next.current1c6=left<0?0:s16(left);
        next.reaches_zero=next.current1c6==0;
    }
    *out=next;return true;
}
}
