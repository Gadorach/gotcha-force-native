#include <cstdio>
#include <cstdlib>
#include <limits>
#include "engine/gameplay/borgs/normal_gred_beam_sasuke_hit_cues.hpp"

namespace {
void check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"normal G-Red/Sasuke hit cues: %s\n",why);std::abort();}}
}
int main(){
    using namespace gf;
    constexpr NormalGRedNeutralBSource red{0x0615,0,3};
    GRedBeamSasukeContactPrefix contact;
    contact.beam_receiver290=contact.credit_receiver290=1;
    contact.receiver_reaction702=4;contact.receiver_binding71a=2;
    contact.beam_vector2a0=contact.receiver_vector2a0={-22,1,3};
    GRedBeamSasukePostHitPlan hit;
    hit.credit_slot=3;hit.receiver_slot=1;hit.reaction.visited=true;hit.health={19,161,180,true,false};
    GRedSasukeHitCueInput in;
    in.ordinary_tail_declared=true;in.receiver_kind83=in.receiver_link4a0=0;
    in.receiver_render_index96=5;
    GRedSasukeHitCuePlan p;
    for(unsigned i=0;i<3;++i){
        const f32 t=1.5f+f32(i);
        in.receiver_scalar7fc=t;in.sound_counters8_to_11.fill(255);
        check(normal_gred_beam_sasuke_hit_cues(red,contact,hit,in,&p)&&p.request_class2&&p.request_sound&&
            p.point==contact.beam_vector2a0&&p.credit_slot==3&&p.receiver_slot==1&&
            p.visual_row==1&&p.visual_latch2af==4&&p.sound_latch2ae==4&&p.sound_family==8+i&&
            p.sound_id==65+2*int(i)&&p.sound_counters8_to_11[i]==0&&p.sound_counters8_to_11[i+1]==255&&
            p.sound_render_index96==5,"exact threshold uses lower family and advances that family's byte");
        in.receiver_scalar7fc=std::nextafter(t,4.f);in.sound_counters8_to_11.fill(4);
        check(normal_gred_beam_sasuke_hit_cues(red,contact,hit,in,&p)&&p.sound_family==9+i&&p.sound_id==66+2*int(i)&&
            p.sound_counters8_to_11[i]==4&&p.sound_counters8_to_11[i+1]==5,"next representable value uses higher family");
    }
    in.visual_latch2af=255;
    check(normal_gred_beam_sasuke_hit_cues(red,contact,hit,in,&p)&&!p.request_class2&&
        p.visual_latch2af==255&&p.request_sound,"visual latch does not suppress sound");
    in.visual_latch2af=0;in.sound_latch2ae=128;in.receiver_render_index96=255;
    in.receiver_scalar7fc=std::numeric_limits<f32>::quiet_NaN();
    check(normal_gred_beam_sasuke_hit_cues(red,contact,hit,in,&p)&&p.request_class2&&
        !p.request_sound&&p.sound_latch2ae==128&&p.sound_counters8_to_11==in.sound_counters8_to_11,
        "sound latch suppresses lookup but not visual");
    in.sound_latch2ae=0;
    check(!normal_gred_beam_sasuke_hit_cues(red,contact,hit,in,&p)&&p.sound_latch2ae==128,
        "invalid accessed sound data refuses the whole proposal");
    in.receiver_render_index96=5;in.receiver_scalar7fc=1;
    hit.reaction.visited=false;hit.request_ko_continuation=true;
    check(!normal_gred_beam_sasuke_hit_cues(red,contact,hit,in,&p),"KO selector alone is not a zero crossing");
    hit.health={19,0,7,true,true};
    check(normal_gred_beam_sasuke_hit_cues(red,contact,hit,in,&p)&&p.request_sound&&p.request_class2,
        "new zero crossing also reaches common cues");
    in.receiver_flags1da=2;
    check(!normal_gred_beam_sasuke_hit_cues(red,contact,hit,in,&p),"prior special flag is outside ordinary cue route");
    in.receiver_flags1da=0;contact.receiver_vector2a0[0]=1;
    check(!normal_gred_beam_sasuke_hit_cues(red,contact,hit,in,&p),"cannot retarget cue to a different contact point");
    std::puts("normal G-Red/Sasuke hit cues: independent latches, exact thresholds and refusal PASS");
}
