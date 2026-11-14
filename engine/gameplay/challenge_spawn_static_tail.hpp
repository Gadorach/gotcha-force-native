#pragma once
#include "engine/core/source_heading.hpp"
#include "engine/gameplay/challenge_battle_spawn.hpp"
#include "engine/gameplay/stage_segment_query.hpp"
namespace gf {
struct ChallengeSpawnStaticTail {s16 heading72=0;std::array<f32,3> after_static{},actor_projection_start{};StageSegmentHit static_hit;bool actor_projection_required=false,history_publication_pending=false;bool operator==(const ChallengeSpawnStaticTail&)const=default;};
//Only452A0 model + first3F044 consumer.37B98 dynamic registry consumer remains
//mandatory; this does NOT publish actor position/history or clear that request.
inline bool challenge_spawn_static_tail(const ChallengeFirstPlacementPlan& placement,const ChallengeStageInputs& stage,const StageRayParameters& parameters,ChallengeSpawnStaticTail* out,std::string* error){
    if(out)*out={};
    if(!out||!stage.ready()||!placement.heading_required||!placement.static_projection_required||!placement.actor_projection_required||placement.proposed43b!=1||placement.chosen_cursor>=4||placement.next_cursor!=placement.chosen_cursor+1||placement.proposed43c!=(placement.chosen_cursor&1u)){if(error)*error="spawn static tail: current declared placement/static inputs required";return false;}
    ChallengeSpawnStaticTail next;if(!source_heading_s16(placement.heading_inputs[0],placement.heading_inputs[1],&next.heading72)){if(error)*error="spawn static tail: heading numeric domain";return false;}
    if(!stage_segment_query(*stage.grid(1),parameters,placement.first_projection_start,placement.pre_projection,&next.static_hit,error))return false;
    next.after_static=placement.pre_projection;if(next.static_hit.hit)next.after_static[1]=next.static_hit.point[1];next.actor_projection_start=next.after_static;next.actor_projection_start[1]=f32(next.after_static[1]+stage.parameters().high34);if(!source_intersection_detail::valid(next.actor_projection_start))return false;next.actor_projection_required=next.history_publication_pending=true;*out=next;if(error)error->clear();return true;
}
} // namespace gf
