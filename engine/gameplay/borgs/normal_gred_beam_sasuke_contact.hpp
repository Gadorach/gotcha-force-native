#pragma once
#include "engine/gameplay/borgs/normal_gred_beam_custody.hpp"
#include "engine/graphics/source_intersection.hpp"

namespace gf {
struct GRedBeamSasukeContactInput {
    // Declaration of an already-admitted ordered pair, NOT a query result.
    bool admitted_pair_declared=false;
    u16 receiver_model=0,beam_token26c=0;
    u8 receiver_variant=0,receiver_slot=255,receiver_credit_slot=255;
    u8 receiver_descriptor=255,receiver_hit_row=255;
    u8 beam_latch1d9=0,credit_latch1d9=0,beam_flags2ac=0;
    std::array<f32,3> contact_vector{}; // declared 803B0720 snapshot
    std::array<u16,48> receiver_tokens20c{};
    u8 receiver_index26e=0,receiver_flags6fd=0,receiver_rank71c=0;
};
struct GRedBeamSasukeContactPrefix {
    u8 beam_receiver290=255,credit_receiver290=255;
    u8 beam_latch1d9=0,credit_latch1d9=0;
    std::array<f32,3> beam_vector2a0{},receiver_vector2a0{};
    std::array<u16,48> receiver_tokens20c{};
    u8 receiver_index26e=0,receiver_flags6fd=0,receiver_rank71c=0;
    u8 receiver_impact704=0,receiver_reaction702=0,receiver_binding71a=0;
};

// Exact normal 0615/0 arg-43 row 0 -> normal 000A/0 body 9/row 0.
inline bool normal_gred_beam_sasuke_pair_scope(NormalGRedNeutralBSource source,const GRedBeamShapeOperands& attack,
                                               const GRedBeamSasukeContactInput& in){
    return normal_gred_beam_shape_scope(source,attack)&&
        in.receiver_model==0x000a&&!in.receiver_variant&&in.receiver_slot<6&&in.receiver_slot!=source.slot&&
        in.receiver_credit_slot==in.receiver_slot&&in.receiver_descriptor==9&&!in.receiver_hit_row&&
        in.beam_token26c&&in.receiver_index26e<in.receiver_tokens20c.size();
}

// 2E56C..2E6C0 prefix only, after accepting this ordered collider pair.
// No aim target input, HP write, duplicate-hit admission or live owner
// publication. The caller must qualify the actual pair and bank identities.
inline bool normal_gred_beam_sasuke_contact_prefix(NormalGRedNeutralBSource source,const GRedBeamShapeOperands& attack,
                                                   const GRedBeamSasukeContactInput& in,GRedBeamSasukeContactPrefix* out){
    if(!out||!in.admitted_pair_declared||!normal_gred_beam_sasuke_pair_scope(source,attack,in)||
       !source_intersection_detail::valid(in.contact_vector))return false;

    GRedBeamSasukeContactPrefix next;
    next.beam_receiver290=next.credit_receiver290=in.receiver_slot;
    next.beam_latch1d9=u8(in.beam_latch1d9|0x10u); // receiver kind 0
    next.credit_latch1d9=in.credit_latch1d9;
    if(in.beam_flags2ac&1u)next.credit_latch1d9|=next.beam_latch1d9;
    next.beam_vector2a0=next.receiver_vector2a0=in.contact_vector;
    next.receiver_tokens20c=in.receiver_tokens20c;
    next.receiver_tokens20c[in.receiver_index26e]=in.beam_token26c;
    next.receiver_index26e=u8((in.receiver_index26e+1u)%next.receiver_tokens20c.size());
    // Exact attacking row bytes and receiving descriptor 9's binding.
    next.receiver_impact704=attack.hit_row;
    next.receiver_flags6fd=u8(in.receiver_flags6fd|1u);
    next.receiver_reaction702=4;
    next.receiver_rank71c=in.receiver_rank71c<1?1:in.receiver_rank71c;
    next.receiver_binding71a=2;
    *out=next;return true;
}
}
