#pragma once
#include "engine/assets/challenge_ai_force.hpp"
#include "engine/gameplay/force_cost.hpp"
#include "engine/gameplay/source_byte_rng.hpp"
namespace gf {
//Only fields explicitly written by801962C4. Acquired/experience/edits and native
//inventory provenance are deliberately NOT synthesized into ForceBorg objects.
struct ChallengeAiMemberFields {u16 model=0;u8 variant=0,level=0;bool operator==(const ChallengeAiMemberFields&)const=default;};
struct ChallengeAiSlotPlan {
    s8 team=-1;bool human=false,over_budget=false;u8 count=0;u32 accepted_cost=0,attempted_cost=0;
    std::array<ChallengeAiMemberFields,30> members{};
    bool operator==(const ChallengeAiSlotPlan&)const=default;
};
struct ChallengeAiAttempt {
    u8 slot=0,category_byte=0,category_choice=0,category=0,pool_byte=0;bool pool_drawn=false,accepted=false;
    u16 pool_ordinal=0,model=0;u32 authored_cost=0,cost=0,attempted_total=0;
    bool operator==(const ChallengeAiAttempt&)const=default;
};
struct ChallengeAiPhasePlan {
    u8 human_mask=0,active_mask=0;u32 draws=0;SourceByteRngState next;
    std::array<ChallengeAiSlotPlan,6> slots;std::array<ChallengeAiAttempt,180> attempts{};unsigned attempt_count=0;
    bool operator==(const ChallengeAiPhasePlan&)const=default;
};
//Pure later-Challenge phase candidate: current human mask + explicit POST-stage
//RNG input. Does not assemble/mutate humans, commit RNG, apply actor fields, or
//acknowledge a complete party. Roles and AI attempts traverse source slots0..5.
inline bool challenge_ai_force_plan(const ChallengeAiForceProfile& profile,const ForceCatalog& catalog,unsigned difficulty,unsigned wave,u8 humans,SourceByteRngState input,ChallengeAiPhasePlan* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("Challenge AI plan: ")+why;return false;};
    const auto* selected=profile.wave(difficulty,wave);unsigned allies=0;
    for(unsigned slot=0;slot<4;++slot)allies+=(humans>>slot)&1u;
    if(!out||!profile.for_catalog(catalog)||!selected||!humans||(humans&~15u)||allies>2)return fail("profile/catalog/wave/one-or-two-human mask required");
    ChallengeAiPhasePlan plan;plan.human_mask=plan.active_mask=humans;plan.next=input;unsigned enemies=0;
    for(unsigned slot=0;slot<6;++slot){auto& result=plan.slots[slot];result.human=(humans&(1u<<slot))!=0;
        if(result.human){result.team=0;continue;}
        if(allies<2){result.team=0;++allies;}else if(enemies<2){result.team=1;++enemies;}else continue;
        plan.active_mask|=u8(1u<<slot);const auto& team=selected->teams[unsigned(result.team)];
        while(result.count<team.member_cap){
            if(plan.attempt_count>=plan.attempts.size())return fail("native attempt budget");
            ChallengeAiAttempt attempt;attempt.slot=u8(slot);attempt.category_byte=source_byte_rng_step(plan.next);++plan.draws;
            attempt.category_choice=attempt.category_byte&3u;attempt.category=team.categories[attempt.category_choice];const auto* pool=profile.pool(attempt.category);
            if(!pool)return fail("missing category pool");
            if(pool->models.empty())attempt.model=8;
            else{attempt.pool_drawn=true;attempt.pool_byte=source_byte_rng_step(plan.next);++plan.draws;attempt.pool_ordinal=u16(attempt.pool_byte%pool->models.size());attempt.model=pool->models[attempt.pool_ordinal];}
            const auto* row=catalog.find(attempt.model);if(!row)return fail("missing authored model");
            attempt.authored_cost=u32(row->cost[0]);attempt.cost=u32(force_cost_for_mode(row->cost[0],ForceCostMode::TwoThirds));
            result.attempted_cost+=attempt.cost;attempt.attempted_total=result.attempted_cost;attempt.accepted=result.attempted_cost<=team.budget;plan.attempts[plan.attempt_count++]=attempt;
            if(!attempt.accepted){result.over_budget=true;break;}
            result.accepted_cost=result.attempted_cost;result.members[result.count++]={attempt.model,0,0};
        }
    }
    *out=plan;if(error)error->clear();return true;
}
} // namespace gf
