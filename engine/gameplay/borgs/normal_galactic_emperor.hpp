#pragma once
#include "engine/gameplay/character_air_resource.hpp"
#include "engine/gameplay/stage_team_xyz_placement.hpp"

namespace gf {
struct NormalGalacticEmperorSource {
    u16 model=0;
    u8 variant=0;
    constexpr bool reviewed()const{return model==0x0e04&&!variant;}
    static constexpr u8 movement_type=8,resource_policy=1;
    static constexpr AirMeterRules meter{AirMeterRecoveryGate::PreviousGroundedCache,10000,0,2000,{0,0,0,50}};
};

enum class GalacticEmperorPinSite:u8 {ClassInit,PreClass,PostClass};
struct GalacticEmperorPinPlan {
    u32 flags5e0=0;
    bool write_root_heading=false;
    std::array<f32,3> root{};
    s16 heading72=0;
    bool operator==(const GalacticEmperorPinPlan&)const=default;
};

// Scalar prefix shared by three exact normal Emperor hook sites. The
// nonzero-role case withholds the coordinate write; it does not qualify
// an alternate start.
// A class pin isn't a final spawn or a replacement for a physical response.
inline bool normal_galactic_emperor_class_pin(NormalGalacticEmperorSource source,GalacticEmperorPinSite site,u8 role493,
                                              u32 flags5e0,GalacticEmperorPinPlan* out){
    if(!out||!source.reviewed())return false;
    switch(site){
    case GalacticEmperorPinSite::ClassInit:case GalacticEmperorPinSite::PreClass:case GalacticEmperorPinSite::PostClass:break;
    default:return false;
    }
    GalacticEmperorPinPlan next;
    next.flags5e0=flags5e0|0x40u;
    if(!role493){next.write_root_heading=true;next.root={0.f,2000.f,0.f};next.heading72=8192;}
    *out=next;return true;
}

inline bool normal_galactic_emperor_table_placement(NormalGalacticEmperorSource source,const StageTeamXyzRows& rows,
                                                   const StageTeamXyzPlacementFields& in,StageTeamXyzPlacementPlan* out){
    return source.reviewed()&&stage_team_xyz_placement_plan(rows,in,out);
}

// Numeric type-8 additive tail of the separate zero-authored-Y per-slot
// path. This receives the queried height, not a projected or pinned root.
inline bool normal_galactic_emperor_floor_fallback_y(NormalGalacticEmperorSource source,bool zero_authored_y_path,
                                                   f32 queried_floor_y,f32* out){
    if(!out||!source.reviewed()||!zero_authored_y_path||!std::isfinite(queried_floor_y))return false;
    const f32 next=queried_floor_y+1000.f;
    if(!std::isfinite(next)||std::abs(next)>1e7f)return false;
    *out=next;return true;
}

enum class GalacticEmperorMovementLane:u8 {Upper,Lower};
// Installed lane coverage only. A representable recovery selector is not
// evidence that the normal actor ever reaches it or completes its handler.
inline bool normal_galactic_emperor_selector_state(NormalGalacticEmperorSource source,GalacticEmperorMovementLane lane,
                                                   u8 selector,u8* out){
    if(!out||!source.reviewed()||(lane!=GalacticEmperorMovementLane::Upper&&lane!=GalacticEmperorMovementLane::Lower))return false;
    if(!selector){*out=52;return true;}
    if(selector>=9&&selector<=11){*out=lane==GalacticEmperorMovementLane::Upper?0:u8(selector+1u);return true;}
    return false;
}

inline bool normal_galactic_emperor_refill(NormalGalacticEmperorSource source,const CharacterAirResourceState& in,u8 cached87,
                                          CharacterAirResourceState* out){
    const auto& meter=NormalGalacticEmperorSource::meter;
    return source.reviewed()&&cached87<=1&&in.value736>=0&&in.value736<=meter.limit&&in.delay73a>=0&&
        character_air_resource_recover(in,meter,0,cached87,out);
}
}
