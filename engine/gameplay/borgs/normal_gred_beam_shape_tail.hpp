#pragma once
#include "engine/gameplay/borgs/normal_gred_beam_custody.hpp"
#include "engine/gameplay/collider_plane_lifecycle.hpp"

namespace gf {
struct GRedBeamActivationFields {
    u16 scene_token=0,beam_token26c=0;
    u32 credit_count404=0;
    u8 beam_once1ca=0,beam_byte2ad=0;
    bool operator==(const GRedBeamActivationFields&)const=default;
};
struct GRedBeamShapeTailInputs {
    GRedBeamActivationFields activation;
    ColliderPlaneLifecycleOwnerFields carrier;
    CollisionRegistrationPeerFields credit;
    bool after_shape0_init=false,null_copied_target_cc=false;
};
struct GRedBeamShapeTailPlan {
    GRedBeamActivationFields activation;
    CollisionRegistrationPlan membership;
    u8 phase10=0,hit_index11=0,interval4e=0;
    bool activation_write=false,degenerate=false,registration_sampled=false;
};

// Exact arg-43 phase-0 tail AFTER the shape-0 numeric kernel. The supplied
// fields/frame are declared samples, not an allocation, HP result or
// registry write. This does not initialize/update the sphere or retire it.
inline bool normal_gred_beam_shape_tail(NormalGRedNeutralBSource source,const GRedBeamShapeOperands& shape,
                                         const GRedBeamShapeTailInputs& in,const ColliderGeometryProfile& geometry,
                                         const Mat4& beam114,GRedBeamShapeTailPlan* out,std::string* error){
    auto fail=[&](const char* why){if(error)*error=std::string("G-Red beam shape tail: ")+why;return false;};
    if(!out||!source.reviewed()||shape.call!=GRedBeamPoolCall{4,48,u8(49+source.slot),26,0,false}||
       shape.attack_group||shape.descriptor!=15||shape.hit_row||shape.credited_slot!=source.slot||
       !shape.beam_transform114||shape.start||shape.end!=-2||shape.local_radius!=20.f||
       !in.after_shape0_init||!in.null_copied_target_cc||!in.carrier.lifecycle_declared||in.carrier.retire1d8||
       !in.carrier.registration.declared||!in.carrier.registration.present||in.carrier.registration.active00!=1||
       in.carrier.registration.kind83!=16||!in.credit.declared||!in.credit.present||in.credit.kind83)
        return fail("reviewed fresh carrier, credited slot and exact shape operands required");
    GRedBeamShapeTailPlan next;
    next.phase10=in.carrier.freeze281?2:1;next.activation=in.activation;
    // Exact descriptor flags2 bit 0 and hit row 0: +0x10 bit 0x20 clear,
    // +0x16 = 0.
    // 8A7BC invokes 8A5D0(...,1) once, BEFORE the 8AA40 column test.
    if(!next.activation.beam_once1ca){
        auto& a=next.activation;
        a.beam_once1ca=1;a.beam_byte2ad=0;a.scene_token=u16(a.scene_token+1u);
        if(!a.scene_token)a.scene_token=1;
        a.beam_token26c=a.scene_token;++a.credit_count404;
        next.activation_write=true;
    }
    if(!collider_frame_degenerate(geometry,beam114,&next.degenerate))return fail("beam matrix column filter refused");
    if(!next.degenerate){
        const CollisionRegistrationInput request{true,true,in.carrier.registration,in.credit,0,1};
        if(!collision_registration_plan(request,&next.membership,error))return false;
        next.registration_sampled=true;
    }
    *out=next;if(error)error->clear();return true;
}
}
