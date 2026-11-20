#pragma once
#include "engine/gameplay/stage_ground.hpp"

namespace gf {
struct StagePostSolidFloorInput {
    StageGroundActor actor;
    std::array<f32,3> position{},solid_displacement{};
    f32 query_height=0;
    bool occupied=false;
    u8 moved1dd=0,hold1df=0;
};
struct StagePostSolidFloorResult {
    std::array<f32,3> position{};
    f32 queried_height=0,lift=0;
    bool queried=false;
    u8 hold1df=0;
    bool operator==(const StagePostSolidFloorResult&)const=default;
};

// Detached static floor correction after an already completed solid response.
// The caller must own moved1DD and the accumulated1CC displacement. An endpoint
// floor hit cannot stand in for them. No body response, attachment, callback,
// velocity change or live actor acceptance is supplied here.
inline bool stage_ground_post_solid_floor(const std::array<const StihGrid*,3>& grids,const StageGroundParameters& p,
                                          const StagePostSolidFloorInput& in,StagePostSolidFloorResult* out,std::string* error){
    using namespace stage_ground_detail;
    auto fail=[&](const char* why){if(error)*error=std::string("post-solid floor: ")+why;return false;};
    if(!out||!in.actor.fields_declared||!vector(in.position)||!vector(in.solid_displacement)||
       in.actor.current24!=in.position[1])return fail("matching actor fields and displacement required");
    StagePostSolidFloorResult next;next.position=in.position;next.hold1df=in.hold1df;
    if(in.occupied&&in.moved1dd){
        if(in.actor.attached1db)return fail("attached correction requires the platform owner");
        if(!parameters(p)||!inside(p,in.position))return fail("verified static stage bounds required");
        if(!stage_ground_height(grids,p,in.actor,in.position,in.query_height,&next.queried_height,error))return false;
        next.queried=true;
        const f32 floor_y=f32(next.queried_height-in.actor.offset6d0);
        if(!number(floor_y))return fail("derived floor height out of domain");
        if(floor_y>in.position[1]){
            next.lift=f32(floor_y-in.position[1]);next.position[1]=floor_y;
            if(!number(next.lift))return fail("derived lift out of domain");
            const auto& d=in.solid_displacement;
            const f64 squared=f64(d[0])*d[0]+f64(d[1])*d[1]+f64(d[2])*d[2];
            // Bounded host normalization, not paired-single/FPSCR parity.
            if(f32(squared)>0.f&&f32(f64(d[1])/std::sqrt(squared))<=-.5f)next.hold1df=2;
        }
    }
    *out=next;if(error)error->clear();return true;
}
} // namespace gf
