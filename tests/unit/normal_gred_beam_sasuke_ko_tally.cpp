#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borgs/normal_gred_beam_sasuke_ko_tally.hpp"

namespace {
void check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"normal G-Red/Sasuke KO tally: %s\n",why);std::abort();}}
}
int main(){
    using namespace gf;
    constexpr NormalGRedNeutralBSource red{0x0615,0,2};
    GRedSasukeKoObserverPlan observers;
    GRedSasukeKoTallyInput in;
    in.caller_and_rng_declared=true;in.credit_slot=2;in.receiver_slot=5;
    in.context32=in.credit_selector3e6=in.credit_kind83=in.receiver_kind83=in.receiver_link4a0=in.receiver_variant3ed=0;
    in.receiver_model3ea=0x000a;in.rng={0x05,0x01};
    StoryRewardCounters counters;
    counters.backup_count=1;counters.backup[0]=~0x000a0000u;
    GRedSasukeKoTallyPlan p;
    check(!normal_gred_beam_sasuke_ko_tally(red,observers,counters,in,&p),"no default observer continuation");
    observers.credit_slot=2;observers.receiver_slot=5;
    check(normal_gred_beam_sasuke_ko_tally(red,observers,counters,in,&p)&&p.appended&&p.active_index==0&&p.active_count==1&&
        p.stored_word==~0x000a0009u&&p.draw==8&&p.rng==SourceByteRngState{8,3}&&p.receiver_credit5a4_slot==2,
        "append normal model key, use the new low byte, and retain credit identity");
    check(counters.active_count==0&&counters.backup[0]==~0x000a0000u&&in.rng==SourceByteRngState{5,1},
        "proposal does not mutate the input table, backup or RNG");
    counters.active_count=3;counters.active[0]=~0x100a0010u;counters.active[1]=~0x000afff6u;counters.active[2]=~0x000a0022u;
    check(normal_gred_beam_sasuke_ko_tally(red,observers,counters,in,&p)&&!p.appended&&p.active_index==1&&p.active_count==3&&
        p.stored_word==~0x000affffu,"distinct variant key, first matching entry and exact upper bound");
    counters.active[1]=~0x000afff7u;
    check(normal_gred_beam_sasuke_ko_tally(red,observers,counters,in,&p)&&p.stored_word==counters.active[1]&&
        p.rng==SourceByteRngState{8,3},"overflow preserves whole word but still draws");
    counters.active_count=u32(counters.active.size());counters.active.fill(~0x100a0000u);
    check(!normal_gred_beam_sasuke_ko_tally(red,observers,counters,in,&p)&&p.active_index==1&&
        p.stored_word==~0x000afff7u,"full table cannot append; failed proposal stays unpublished");
    counters.active.back()=~0x000a0000u;
    check(normal_gred_beam_sasuke_ko_tally(red,observers,counters,in,&p)&&!p.appended&&
        p.active_index==counters.active.size()-1&&p.stored_word==~0x000a0009u,"full table still updates a matching slot");
    in.receiver_variant3ed=1;
    check(!normal_gred_beam_sasuke_ko_tally(red,observers,counters,in,&p),"not a variant reward rule");
    in.receiver_variant3ed=0;in.context32=1;
    check(!normal_gred_beam_sasuke_ko_tally(red,observers,counters,in,&p),"alternate context is not the Story tally call");
    in.context32=0;in.receiver_link4a0=1;
    check(!normal_gred_beam_sasuke_ko_tally(red,observers,counters,in,&p),"linked receiver has a different continuation");
    std::puts("normal G-Red/Sasuke KO tally: detached source increment and refusal PASS");
}
