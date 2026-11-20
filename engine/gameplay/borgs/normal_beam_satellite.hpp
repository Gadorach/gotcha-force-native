#pragma once
#include "engine/gameplay/character_air_resource.hpp"
#include "engine/gameplay/character_movement_input.hpp"

namespace gf {
struct NormalBeamSatelliteSource {
    u16 model=0;
    u8 variant=0;
    constexpr bool reviewed()const{return model==0x0e01&&!variant;}
    static constexpr u8 movement_type=9,resource_policy=1;
    static constexpr u8 idle=0,turn=1,travel=2,brake=3;
    static constexpr AirMeterRules meter{AirMeterRecoveryGate::PreviousGroundedCache,10000,0,2000,{0,0,0,50}};
};

struct BeamSatellitePlacementRows {
    u8 stage1c=0;
    bool declared=false;
    std::array<std::array<std::array<f32,3>,2>,2> xyz{}; // team, side, axis
};
struct BeamSatellitePlacementFields {
    u8 stage1c=0,team88=0,cursor_fe=0,placed43b=0,side43c=0;
    bool table_path_declared=false;
};
struct BeamSatellitePlacementPlan {
    std::array<f32,3> pre_projection{};
    u8 row_index=0,placed43b=0,side43c=0,next_cursor_fe=0;
    bool write_placement_fields=false,write_team_cursor=false;
    bool operator==(const BeamSatellitePlacementPlan&)const=default;
};

// 579DC table branch only, with caller-owned rows for the current raw stage.
// Emits a candidate, not committed actor/cursor writes. The static and actor
// projection queries remain mandatory before final position/history publication.
inline bool normal_beam_satellite_table_placement(NormalBeamSatelliteSource source,const BeamSatellitePlacementRows& rows,
                                                   const BeamSatellitePlacementFields& in,BeamSatellitePlacementPlan* out){
    if(!out||!source.reviewed()||!rows.declared||!in.table_path_declared||in.stage1c>=8||
       rows.stage1c!=in.stage1c||in.team88>=2||in.placed43b>1||(in.placed43b&&in.side43c>1))return false;
    BeamSatellitePlacementPlan next;
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

// Last steering leaves for the reviewed idle/turn/travel paths. They neither
// integrate a position nor claim that an earlier action/contact leaf declined.
inline bool normal_beam_satellite_steering_request(NormalBeamSatelliteSource source,u8 selector,u32 flags5e0,
                                                   bool earlier_declined,const CharacterMovementInputState& in,u8* out){
    using S=NormalBeamSatelliteSource;
    if(!out||!source.reviewed()||!earlier_declined||!character_movement_input_state_valid(in)||(in.commands5d4&~3u))return false;
    const bool steering=(in.previous_held&0xf0000u)!=0;
    switch(selector){
    case S::idle:*out=steering&&!(flags5e0&2u)?S::turn:S::idle;return true;
    case S::turn:case S::travel:*out=steering?selector:S::brake;return true;
    default:return false;
    }
}

// Uses the actual turn-helper completion result, not elapsed animation time.
inline bool normal_beam_satellite_turn_handoff(NormalBeamSatelliteSource source,u8 selector,bool turn_complete,u8* out){
    using S=NormalBeamSatelliteSource;
    if(!out||!source.reviewed()||selector!=S::turn)return false;
    *out=turn_complete?S::travel:S::turn;return true;
}

struct BeamSatelliteBrakeHandoff {
    u8 selector=0;
    u32 flags5e0=0;
    bool ordinary_reset_requested=false;
};
// Continued lower-3 tail uses speed after damping. Reports the ordinary reset
// requirement, but does not perform the remaining reset/pose/actor writes.
inline bool normal_beam_satellite_brake_handoff(NormalBeamSatelliteSource source,u8 selector,f32 speed44,u32 flags5e0,
                                               BeamSatelliteBrakeHandoff* out){
    using S=NormalBeamSatelliteSource;
    if(!out||!source.reviewed()||selector!=S::brake||!std::isfinite(speed44))return false;
    const bool stop=speed44<1.f;
    *out={stop?S::idle:S::brake,stop?((flags5e0&0x00ff0003u)|0x40u):flags5e0,stop};
    return true;
}

// Init/pre/post flag fragment. Not constructed from root height or contact.
inline bool normal_beam_satellite_airborne_flags(NormalBeamSatelliteSource source,u32 flags5e0,u32* out){
    if(!out||!source.reviewed())return false;
    *out=flags5e0|0x40u;return true;
}

// Resource branch consumes the logical-airborne cache published by 55DCC,
// not the class hook being proposed this tick or a floor query.
inline bool normal_beam_satellite_refill(NormalBeamSatelliteSource source,const CharacterAirResourceState& in,u8 cached87,
                                        CharacterAirResourceState* out){
    const auto& meter=NormalBeamSatelliteSource::meter;
    return source.reviewed()&&cached87<=1&&in.value736>=0&&in.value736<=meter.limit&&in.delay73a>=0&&
        character_air_resource_recover(in,meter,0,cached87,out);
}
}
