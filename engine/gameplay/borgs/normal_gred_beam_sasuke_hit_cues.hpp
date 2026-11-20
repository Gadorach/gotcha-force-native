#pragma once
#include "engine/gameplay/borgs/normal_gred_beam_sasuke_post_hit.hpp"

namespace gf {
struct GRedSasukeHitCueInput {
    bool ordinary_tail_declared=false;
    u8 receiver_kind83=255,receiver_link4a0=255,receiver_flags1da=0;
    u8 visual_latch2af=0,sound_latch2ae=0,receiver_render_index96=255;
    f32 receiver_scalar7fc=0;
    std::array<u8,4> sound_counters8_to_11{};
};
struct GRedSasukeHitCuePlan {
    std::array<f32,3> point{};
    u8 credit_slot=255,receiver_slot=255;
    u8 visual_latch2af=0,sound_latch2ae=0;
    std::array<u8,4> sound_counters8_to_11{};
    bool request_class2=false,request_sound=false;
    u8 visual_row=255,sound_family=255,sound_render_index96=255;
    s16 sound_id=-1;
};
// Exact argument-43 shooting row 0 vs normal Sasuke body 9 row 0,
// 2F514..2F8BC, through the 19550 visual request and F088C sound
// selection. Neither the class-2 allocation nor playback is performed.
inline bool normal_gred_beam_sasuke_hit_cues(NormalGRedNeutralBSource source,
    const GRedBeamSasukeContactPrefix& contact,const GRedBeamSasukePostHitPlan& hit,
    const GRedSasukeHitCueInput& in,GRedSasukeHitCuePlan* out){
    const bool survival=hit.reaction.visited&&hit.health.current1c6>0&&!hit.health.reaches_zero&&!hit.request_ko_continuation;
    const bool zero_crossing=!hit.reaction.visited&&hit.health.current1c6==0&&hit.health.write_health&&
                             hit.health.reaches_zero&&hit.request_ko_continuation;
    if(!out||!source.reviewed()||!in.ordinary_tail_declared||in.receiver_kind83||in.receiver_link4a0||
       (in.receiver_flags1da&2u)||hit.credit_slot!=source.slot||hit.receiver_slot>=6||hit.receiver_slot==source.slot||
       hit.receiver_slot!=contact.beam_receiver290||hit.receiver_slot!=contact.credit_receiver290||
       contact.receiver_impact704||contact.receiver_reaction702!=4||contact.receiver_binding71a!=2||
       (!survival&&!zero_crossing)||
       !source_intersection_detail::valid(contact.beam_vector2a0)||contact.beam_vector2a0!=contact.receiver_vector2a0)return false;
    GRedSasukeHitCuePlan next;
    next.point=contact.beam_vector2a0;next.credit_slot=source.slot;next.receiver_slot=hit.receiver_slot;
    next.visual_latch2af=in.visual_latch2af;next.sound_latch2ae=in.sound_latch2ae;
    next.sound_counters8_to_11=in.sound_counters8_to_11;
    if(!in.visual_latch2af){
        // Passed with the BEAM carrier, not the credited G-Red actor.
        next.visual_latch2af=4;next.request_class2=true;next.visual_row=1;
    }
    if(!in.sound_latch2ae){
        if(!std::isfinite(in.receiver_scalar7fc)||in.receiver_render_index96>=6)return false;
        const f32 k=in.receiver_scalar7fc;
        const unsigned row=unsigned(k>1.5f)+unsigned(k>2.5f)+unsigned(k>3.5f);
        auto& counter=next.sound_counters8_to_11[row];
        constexpr std::array<s16,4> base{64,66,68,70};
        next.sound_id=s16(base[row]+s16(counter&1u));counter=u8(counter+1u);
        next.sound_latch2ae=4;next.request_sound=true;next.sound_family=u8(8+row);
        next.sound_render_index96=in.receiver_render_index96;
    }
    *out=next;return true;
}
}
