#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borgs/normal_gred_beam_sasuke_post_hit.hpp"

namespace {
void check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"normal G-Red/Sasuke post-hit: %s\n",why);std::abort();}}
}
int main(){
    using namespace gf;
    constexpr NormalGRedNeutralBSource red{0x0615,0,0};
    GRedBeamSasukeContactPrefix contact;
    contact.beam_receiver290=contact.credit_receiver290=1;
    contact.receiver_reaction702=4;contact.receiver_binding71a=2;contact.receiver_flags6fd=1;
    GRedBeamSasukeReceiverCounters state;
    state.restore6be=90;state.remaining6c2=100;state.threshold6c4=51;state.impact6c6=123;
    state.progress6c8=50;state.tier6ca=62;state.flags6fd=1;state.behaviour5e0=0x01000000u;
    GRedBeamSasukeReactionPlan p;
    check(normal_gred_beam_sasuke_reaction_tail(red,contact,1,180,1,1,state,&p)&&p.visited&&
        p.row2_impact==50&&p.receiver_threshold==51&&!p.reduced6c6&&p.fields.impact6c6==123&&
        p.fields.remaining6c2==50&&p.fields.progress6c8==0&&p.fields.tier6ca==63,
        "independent base-50 counter and progress paths at a higher threshold");
    state.threshold6c4=50;state.tier6ca=63;
    check(normal_gred_beam_sasuke_reaction_tail(red,contact,1,180,1,1,state,&p)&&p.reduced6c6&&
        p.fields.impact6c6==73&&p.fields.tier6ca==63&&p.fields.timer684==60&&p.fields.timer688==60&&p.fields.timer68c==60,
        "equal threshold, top tier, and three reset clocks");
    state.inhibit7de=1;state.behaviour5e0=0;
    check(normal_gred_beam_sasuke_reaction_tail(red,contact,1,180,1,1,state,&p)&&!p.reduced6c6,
        "inhibit is checked before the unconditional decrement path");
    state.inhibit7de=0;state.threshold6c4=250;
    check(normal_gred_beam_sasuke_reaction_tail(red,contact,1,180,1,1,state,&p)&&p.reduced6c6,
        "clear behaviour bit bypasses the threshold with no inhibit");
    state.behaviour5e0=0;state.threshold6c4=49;state.remaining6c2=50;state.progress6c8=99;state.tier6ca=3;
    check(normal_gred_beam_sasuke_reaction_tail(red,contact,1,180,.93f,.91f,state,&p)&&p.row2_impact==49&&
        p.receiver_threshold==47&&p.fields.impact6c6==74&&p.reset6c2&&p.fields.remaining6c2==90&&
        p.fields.progress6c8==50&&!p.fields.tier6ca&&p.fields.flags6fd==0xa7,
        "round scale additions separately; depleted counter resets before the progress add");
    state.flags6fd=contact.receiver_flags6fd=0x81;
    check(normal_gred_beam_sasuke_reaction_tail(red,contact,1,180,1,1,state,&p)&&
        !p.reset6c2&&p.fields.remaining6c2==50&&p.fields.progress6c8==0&&p.fields.tier6ca==4,
        "flag 80 skips subtraction but not the independent tier increment");
    check(normal_gred_beam_sasuke_reaction_tail(red,contact,1,0,1,1,state,&p)&&!p.visited&&
        p.fields.progress6c8==99&&p.fields.timer688==0,"zero HP bypasses surviving scalar tail");
    check(!normal_gred_beam_sasuke_reaction_tail(red,contact,2,180,1,1,state,&p),
        "cannot transfer the reaction to another receiver");
    std::puts("normal G-Red/Sasuke post-hit: ordinary surviving counters and zero bypass PASS");
}
