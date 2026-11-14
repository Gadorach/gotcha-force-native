#pragma once
#include "engine/assets/challenge_battle_spawn.hpp"
#include "engine/gameplay/challenge_initial_party.hpp"
#include "engine/gameplay/challenge_stage_inputs.hpp"
namespace gf {
struct ChallengeDeclaredTeamInput {u8 humans=0,active=0;std::array<s8,6> teams{{-1,-1,-1,-1,-1,-1}};};
struct ChallengeBattleTeamFields {
    u8 id=0,members=0,humans=0,mask=0,cursor_fe=0,previous_ff=0;std::array<s8,6> slots{{-1,-1,-1,-1,-1,-1}};std::array<u8,5> fields_0c_to_10{};s16 field18=0,field1a=1;
    bool operator==(const ChallengeBattleTeamFields&)const=default;
};
struct ChallengeBattleTeamPlan {std::array<ChallengeBattleTeamFields,4> teams;SourceByteRngState after;u8 field51=0,draws=0;bool operator==(const ChallengeBattleTeamPlan&)const=default;};
//29CA4/29E98/2A1C0 numeric candidate. Four draws include empty teams2/3.
//Does not mutate party/RNG or acknowledge native battle-system initialization.
inline bool challenge_battle_team_plan(const ChallengeDeclaredTeamInput& in,SourceByteRngState state,ChallengeBattleTeamPlan* out){
    if(out)*out={};
    unsigned humans=0,active=0;std::array<unsigned,2> counts{};
    for(unsigned slot=0;slot<6;++slot){const bool h=(in.humans&(1u<<slot))!=0,a=(in.active&(1u<<slot))!=0;if(h){++humans;if(!a||in.teams[slot]!=0)return false;}if(a){++active;if(in.teams[slot]<0||in.teams[slot]>1)return false;++counts[unsigned(in.teams[slot])];}else if(in.teams[slot]!=-1)return false;}
    if(!out||!in.humans||(in.humans&~15u)||(in.active&~63u)||humans>2||active!=4||counts[0]!=2||counts[1]!=2)return false;
    ChallengeBattleTeamPlan plan;plan.after=state;
    for(unsigned team=0;team<4;++team){auto& t=plan.teams[team];t.id=u8(team);t.cursor_fe=t.previous_ff=source_byte_rng_step(plan.after)&3u;++plan.draws;for(unsigned slot=0;slot<6;++slot)if((in.active&(1u<<slot))&&in.teams[slot]==s8(team)){t.slots[t.members++]=s8(slot);t.mask|=u8(1u<<slot);t.humans+=bool(in.humans&(1u<<slot));}}
    plan.field51=humans==1?0:2;*out=plan;return true;
}
inline bool challenge_battle_team_plan(const ChallengeInitialPartyCandidate& party,const std::array<const ForceInventory*,2>& owners,SourceByteRngState state,ChallengeBattleTeamPlan* out){
    if(out)*out={};
    if(!party.current(owners))return false;
    ChallengeDeclaredTeamInput in;in.humans=party.humans();in.active=party.active();for(unsigned i=0;i<6;++i)in.teams[i]=party.team(i);return challenge_battle_team_plan(in,state,out);
}
struct ChallengeSpawnActorFields {bool declared=false;u8 mode490=0,first43b=0,movement661=0;StageGroundActor ground;};
struct ChallengeFirstPlacementPlan {
    std::array<f32,3> pre_projection{};std::array<f32,2> heading_inputs{};std::array<f32,3> first_projection_start{};
    u8 chosen_cursor=0,next_cursor=0,proposed43b=0,proposed43c=0;bool heading_required=false,static_projection_required=false,actor_projection_required=false;
    bool operator==(const ChallengeFirstPlacementPlan&)const=default;
};
//56D28's ordinary INITIAL non-Story position candidate. No actor writes, heading
//result or final position/history publication:3F044 then37B98 remain unfulfilled.
inline bool challenge_first_placement_plan(const ChallengeBattleSpawnProfile& profile,const ChallengeStageInputs& stage,const ChallengeSpawnActorFields& actor,u8 team,u8 cursor,ChallengeFirstPlacementPlan* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("Challenge first placement: ")+why;return false;};
    const auto* row=profile.find(stage.stage());if(!out||!profile.ready()||!stage.ready()||!row||!actor.declared||actor.mode490||actor.first43b||team>=2||actor.movement661>11||actor.movement661==8||actor.movement661==9||actor.movement661==11)return fail("declared initial ordinary actor/stage/team required; no special-air/respawn substitute");
    ChallengeFirstPlacementPlan plan;plan.chosen_cursor=cursor&3u;plan.next_cursor=u8(plan.chosen_cursor+1);plan.proposed43b=1;plan.proposed43c=plan.chosen_cursor&1u;plan.pre_projection={row->xz[team][plan.chosen_cursor][0],profile.initial_y(),row->xz[team][plan.chosen_cursor][1]};
    auto ground=actor.ground;ground.current24=ground.previous30=profile.initial_y();f32 height=0;if(!stage.ground(ground,plan.pre_projection,stage.parameters().high34,&height,error))return false;
    plan.pre_projection[1]=f32(height+profile.lift(actor.movement661));plan.heading_inputs={-plan.pre_projection[0],-plan.pre_projection[2]};plan.first_projection_start=plan.pre_projection;plan.first_projection_start[1]=f32(plan.pre_projection[1]+stage.parameters().high34);
    for(f32 x:plan.pre_projection)if(!std::isfinite(x)||std::abs(x)>1e7)return fail("position domain");
    for(f32 x:plan.first_projection_start)if(!std::isfinite(x)||std::abs(x)>1e7)return fail("projection domain");
    plan.heading_required=plan.static_projection_required=plan.actor_projection_required=true;*out=plan;if(error)error->clear();return true;
}
} // namespace gf
