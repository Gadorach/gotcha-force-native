#pragma once
#include "engine/gameplay/gred_air_dash.hpp"
#include "engine/gameplay/gred_surface_contact.hpp"
namespace gf {
//61338 and61560 do NOT share6235C's .8 floor tail:
//wind-up clears48 after ret1, active has no caller response. Null754 keeps
//50 and airborne history in both cases. A real65E64 callback remains distinct.
//Default zero698 remains a declared legacy boundary. A caller owning a live
//countdown must pass it explicitly; contact does not clear it or grant authority.
inline bool gred_air_dash_static_contact(const CharacterMovementProfile& p,const CharacterMovementStaticGround& ground,
                                         const CharacterMovementMotionState& before,const CharacterMovementMotionState& proposed,
                                          u8 selector,bool landing_callback,bool allow_push,GRedSurfaceContact* out,std::string* error,f32 timer698=0){
    auto fail=[&](){if(error)*error="air-dash contact: reviewed wind-up/active caller and callback required";return false;};
    if(!out||!p.air_dash_operands||(selector!=15&&selector!=16)||(selector==15&&landing_callback))return fail();
    GRedSurfaceContact contact;
    if(!gred_surface_contact(p,ground,before,proposed,{timer698,0,allow_push,landing_callback},&contact,error))return false;
    if(selector==15&&contact.kind==GRedSurfaceContactKind::Snapped)contact.motion.vertical48=p.zero;
    *out=contact;if(error)error->clear();return true;
}
}
