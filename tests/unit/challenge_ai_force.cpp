#include <cstdio>
#include <cstdlib>
#include "tests/unit/challenge_ai_force_fixture.hpp"
namespace {unsigned checks=0;void check(bool value,const char* why){++checks;if(!value){std::fprintf(stderr,"Challenge AI: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;using namespace challenge_ai_force_fixture;const auto c=catalog();auto p=profile(c);const SourceByteRngState input{195,1};ChallengeAiPhasePlan plan;std::string error;
    check(challenge_ai_force_plan(p,c,0,0,1,input,&plan,&error)&&error.empty(),"explicit post-stage seed and current catalog");
    check(input==SourceByteRngState{195,1}&&plan.human_mask==1&&plan.active_mask==15&&plan.slots[0].human&&plan.slots[0].team==0&&plan.slots[0].count==0,"human not reassembled or mutated");
    check(plan.slots[1].team==0&&plan.slots[2].team==1&&plan.slots[3].team==1&&plan.slots[4].team==-1&&plan.slots[5].team==-1,"source increasing free-slot roles");
    const auto& first=plan.attempts[0];check(first.slot==1&&first.category_byte==200&&first.category_choice==0&&first.category==42&&first.pool_drawn&&first.pool_byte==217&&first.pool_ordinal==1&&first.model==9&&first.authored_cost==251&&first.cost==167&&first.attempted_total==167&&first.accepted,"category then pool then discounted cost");
    check(plan.slots[1].count==2&&plan.slots[1].accepted_cost==334&&plan.slots[1].attempted_cost==334&&!plan.slots[1].over_budget&&plan.attempts[2].slot==2,"exact budget fits and cap stops further attempt");
    check(plan.slots[1].members[0]==plan.slots[1].members[1]&&plan.slots[1].members[0]==ChallengeAiMemberFields{9,0,0},"source duplicates retained; only model/variant/level fields emitted");
    auto over=profile(c,30,0,{9});check(challenge_ai_force_plan(over,c,0,0,8,input,&plan,&error)&&plan.draws==6&&plan.attempt_count==3&&plan.active_mask==15,"first over-budget pick still consumes both draws");
    for(unsigned i=0;i<3;++i)check(!plan.attempts[i].accepted&&plan.attempts[i].slot==i&&plan.slots[i].count==0&&plan.slots[i].accepted_cost==0&&plan.slots[i].attempted_cost==167&&plan.slots[i].over_budget,"no over-budget reroll/fallback/member write");
    auto empty=profile(c,1,66,{});check(challenge_ai_force_plan(empty,c,0,0,1,input,&plan,&error)&&plan.draws==3&&plan.attempt_count==3&&plan.next==SourceByteRngState{14,53},"empty pool fallback draws category only");
    for(unsigned i=0;i<3;++i)check(!plan.attempts[i].pool_drawn&&plan.attempts[i].model==8&&plan.slots[i+1].members[0]==ChallengeAiMemberFields{8,0,0},"explicit source empty-pool model8");
    auto none=profile(c,0,0);check(challenge_ai_force_plan(none,c,0,0,1,input,&plan,&error)&&!plan.draws&&!plan.attempt_count&&plan.next==input,"zero cap does not draw");
    const auto z=catalog(true);auto zeros=profile(z,30,0,{8});check(challenge_ai_force_plan(zeros,z,0,0,1,input,&plan,&error)&&plan.attempt_count==90&&plan.draws==180,"zero costs still stop at authored member caps");
    for(unsigned slot=1;slot<4;++slot)check(plan.slots[slot].count==30&&!plan.slots[slot].accepted_cost&&!plan.slots[slot].over_budget,"full source cap bounded");
    unsigned masks=0;for(unsigned mask=1;mask<16;++mask){unsigned humans=0;for(unsigned i=0;i<4;++i)humans+=(mask>>i)&1u;if(humans>2)continue;++masks;check(challenge_ai_force_plan(none,c,0,0,u8(mask),input,&plan,&error),"admitted human mask");unsigned active=0,a=0,b=0,ai=0;for(unsigned slot=0;slot<6;++slot){const auto& s=plan.slots[slot];if(s.team>=0){++active;a+=s.team==0;b+=s.team==1;ai+=!s.human;}check(s.human==bool(mask&(1u<<slot))&&(!s.human||s.team==0),"human slot identity");}check(active==4&&a==2&&b==2&&ai==4-humans,"two allies and two opponents without overwriting humans");}
    check(masks==10,"all one/two-human masks");
    for(u8 mask:{u8(0),u8(7),u8(15),u8(16),u8(255)})check(!challenge_ai_force_plan(p,c,0,0,mask,input,&plan,&error)&&plan==ChallengeAiPhasePlan{},"invalid mask clears candidate");
    check(!challenge_ai_force_plan(p,c,3,0,1,input,&plan,&error)&&!challenge_ai_force_plan(p,c,0,1,1,input,&plan,&error)&&plan==ChallengeAiPhasePlan{},"difficulty/wave guard");
    const auto other=catalog(false,true);check(!challenge_ai_force_plan(p,other,0,0,1,input,&plan,&error)&&plan==ChallengeAiPhasePlan{},"foreign catalog refuses");
    check(!challenge_ai_force_plan(p,c,0,0,1,input,nullptr,&error),"null output");
    ChallengeAiForceProfile bad;auto w=waves();w[0][0].teams[0].categories[3]=43;check(!bad.assign(c,w,{{42,{8}}},&error)&&!bad.ready(),"missing category refuses rather than filtering");
    check(!bad.assign(c,waves(),{{42,{8}},{42,{9}}},&error)&&!bad.ready(),"duplicate category identity");
    check(!bad.assign(c,waves(),{{42,{0xffff}}},&error)&&!bad.ready(),"unknown model refuses");
    w=waves();w[2].clear();check(!bad.assign(c,w,{{42,{8}}},&error),"empty difficulty slice");
    w=waves();w[2].resize(16);check(!bad.assign(c,w,{{42,{8}}},&error),"wave extent");
    check(!bad.assign(c,waves(31),{{42,{8}}},&error)&&!bad.assign(c,waves(1,0x80000000),{{42,{8}}},&error),"cap and signed budget domains");
    for(unsigned cost=0;cost<32768;++cost)check(force_cost_for_mode(s32(cost),ForceCostMode::TwoThirds)==s32(2*(cost/3)+(2*(cost%3))/3)&&force_cost_for_mode(s32(cost),ForceCostMode::Full)==s32(cost),"shared cost policy numeric domain");
    std::vector<u8> bytes(112);auto put16=[&](unsigned at,u16 x){bytes[at]=u8(x>>8);bytes[at+1]=u8(x);};auto put32=[&](unsigned at,u32 x){put16(at,u16(x>>16));put16(at+2,u16(x));};
    put32(0,0x47464349);put32(4,1);put32(8,c.fingerprint());put16(12,3);put16(14,1);for(unsigned d=0;d<3;++d)put16(16+2*d,1);put32(24,80);
    for(unsigned at=32;at<104;at+=12){put32(at,167);bytes[at+4]=1;for(unsigned k=0;k<4;++k)bytes[at+5+k]=42;}bytes[104]=42;put16(106,2);put16(108,8);put16(110,9);
    check(bad.load({bytes.data(),bytes.size()},c,&error)&&bad.for_catalog(c)&&bad.waves(0)==1,"owned wire profile");
    for(unsigned at:{0u,4u,8u,12u,14u,16u,18u,20u,22u,24u,28u,41u,105u,110u}){auto copy=bytes;copy[at]^=1;check(!bad.load({copy.data(),copy.size()},c,&error)&&!bad.ready(),"malformed wire output cleared");}
    check(!bad.load({nullptr,112},c,&error)&&!bad.load({bytes.data(),111},c,&error),"null/short wire");
    check(p.for_catalog(c)&&p.wave(0,0)&&!p.wave(0,1)&&!p.wave(3,0)&&!p.pool(43),"read-only profile access bounds");
    std::printf("Challenge AI: %u owned checks PASS (32768 numeric cost inputs, not games)\n",checks);
}
