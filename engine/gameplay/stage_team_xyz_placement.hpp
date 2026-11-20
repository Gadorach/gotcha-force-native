#pragma once
#include <array>
#include <cmath>
#include "engine/core/types.hpp"

namespace gf {
struct StageTeamXyzRows {
    u8 stage1c=0;
    bool declared=false;
    std::array<std::array<std::array<f32,3>,2>,2> xyz{}; // team, side, axis
};
struct StageTeamXyzPlacementFields {
    u8 stage1c=0,team88=0,cursor_fe=0,placed43b=0,side43c=0;
    bool table_path_declared=false;
};
struct StageTeamXyzPlacementPlan {
    std::array<f32,3> pre_projection{};
    u8 row_index=0,placed43b=0,side43c=0,next_cursor_fe=0;
    bool write_placement_fields=false,write_team_cursor=false;
    bool operator==(const StageTeamXyzPlacementPlan&)const=default;
};

// 579DC arithmetic only. Callers must own the exact model/mode branch and
// current raw-stage rows. No final transform, projection, cursor/history
// publication, fallback, or geometry admission comes from this proposal.
inline bool stage_team_xyz_placement_plan(const StageTeamXyzRows& rows,const StageTeamXyzPlacementFields& in,
                                          StageTeamXyzPlacementPlan* out){
    if(!out||!rows.declared||!in.table_path_declared||in.stage1c>=8||
       rows.stage1c!=in.stage1c||in.team88>=2||in.placed43b>1||(in.placed43b&&in.side43c>1))return false;
    StageTeamXyzPlacementPlan next;
    const bool fresh=!in.placed43b;
    next.placed43b=1;
    next.side43c=fresh?u8(in.cursor_fe&1u):in.side43c;
    next.next_cursor_fe=fresh?u8((in.cursor_fe&3u)+1u):in.cursor_fe;
    next.write_placement_fields=next.write_team_cursor=fresh;
    next.row_index=u8(in.stage1c*4u+in.team88*2u+next.side43c);
    next.pre_projection=rows.xyz[in.team88][next.side43c];
    for(f32 coord:next.pre_projection)if(!std::isfinite(coord)||std::abs(coord)>1e7f)return false;
    *out=next;return true;
}
}
