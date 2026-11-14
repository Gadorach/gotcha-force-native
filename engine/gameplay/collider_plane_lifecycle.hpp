#pragma once
#include "engine/gameplay/collider_plane_shape.hpp"
#include "engine/gameplay/collision_registration.hpp"
namespace gf {
struct ColliderPlaneLifecycleFields {
    bool declared=false;u8 phase10=0,field13=0,parameter11=0,interval4e=0;
    s16 start48=0,end4a=0,generation4c=0;
    std::array<f32,3> current30{},previous3c{};ColliderPlaneFields shape;
};
struct ColliderPlaneLifecycleOwnerFields {
    CollisionRegistrationOwnerFields registration;
    bool lifecycle_declared=false;u8 retire1d8=0,freeze281=0;
    bool clock_declared=false;f32 clock1cdc=0;
    bool generation_declared=false;u8 generation1cec=0,generation1ced=0;
};
enum class ColliderPlaneLifecycleOperation:u8 {IgnoredPhase,Waiting,Initialize,Update,Hold,RetirementRequired};
struct ColliderPlaneLifecyclePlan {
    ColliderPlaneLifecycleFields fields;ColliderPlaneLifecycleOperation operation=ColliderPlaneLifecycleOperation::IgnoredPhase;
    CollisionRegistrationPlan membership;bool generation_read=false,clock_read=false,center_transformed=false;
    bool shape_initialized=false,shape_updated=false,frame_filter_used=false,degenerate=false,membership_used=false;
    bool retirement_required=false,registry_publication_pending=false;
};
//88D98 clears a successful class4 allocation's1C0 bytes (then restores class/
//pool tags);8AB30 copies these descriptor fields. This returns DECLARED data,
//not an allocation-success, live actor/bone frame or registry capability.
inline bool collider_plane_declared_body_seed(const CharacterCollisionDescriptor& d,ColliderPlaneLifecycleFields* out,std::string* error){
    if(out)*out={};
    if(!out||!d.declared||d.shape!=4){if(error)*error="collider plane body seed: explicit shape4 descriptor required";return false;}
    ColliderPlaneLifecycleFields s;s.declared=s.shape.declared=true;s.parameter11=u8(d.parameter4);s.start48=d.start6;s.end4a=d.end8;*out=s;if(error)error->clear();return true;
}
namespace collider_plane_lifecycle_detail {
inline bool clock_s16(const ColliderPlaneLifecycleOwnerFields& owner,s16* out){
    if(!out||!owner.clock_declared||!std::isfinite(owner.clock1cdc)||f64(owner.clock1cdc)<-2147483648.||f64(owner.clock1cdc)>=2147483648.)return false;
    *out=std::bit_cast<s16>(u16(s32(owner.clock1cdc)));return true; //fctiwz thenextsh, NOT saturation
}
inline bool same(const std::array<f32,3>& a,const std::array<f32,3>& b){for(unsigned i=0;i<3;++i)if(f32_to_bits(a[i])!=f32_to_bits(b[i]))return false;return true;}
}
//8A65C source phase/timer/generation/type4/filter/membership candidate. The
//frame/C0/actor snapshots are DECLARED INPUTS, not genuine native frame authority.
//Source effects for flags1/80 REFUSE when reached; neither retirement requests
//nor list masks acknowledge pool cleanup, live registries or query readiness.
inline bool collider_plane_lifecycle_plan(const ColliderGeometryProfile& profile,const CharacterCollisionDescriptor& d,const ColliderPlaneLifecycleFields& input,const ColliderPlaneLifecycleOwnerFields& owner,const CollisionRegistrationPeerFields& peer,const Mat4& frame,f32 actor_c0,ColliderPlaneLifecyclePlan* out,std::string* error){
    if(out && &input==&out->fields){if(error)*error="collider plane lifecycle: aliased state/output";return false;}
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("collider plane lifecycle: ")+why;return false;};
    if(!out||!input.declared||!owner.lifecycle_declared||!owner.registration.declared||!owner.registration.present)return fail("declared collider/source owner required");
    ColliderPlaneLifecyclePlan next;next.fields=input;auto done=[&](){*out=std::move(next);if(error)error->clear();return true;};
    auto retire=[&](){next.operation=ColliderPlaneLifecycleOperation::RetirementRequired;next.retirement_required=true;return done();};
    if(owner.retire1d8||!owner.registration.active00)return retire();
    if(!owner.registration.kind83&&input.end4a!=-1){
        if(!owner.generation_declared)return fail("source signed generation bytes required");
        next.generation_read=true;
        const s32 high=std::bit_cast<s8>(owner.generation1cec),low=std::bit_cast<s8>(owner.generation1ced);
        const s32 packed=high*4096+low; //PPC shift/add32; signed byte range fits s32, DO NOT narrow before cmpw
        if(s32(input.generation4c)!=packed)return retire();
    }
    next.fields.field13=0;const auto phase=input.phase10;
    if(phase>=3)return done();
    if(!d.declared||d.shape!=4||!collider_geometry_profile_valid(profile))return fail("shape4 source descriptor/profile required");
    auto& s=next.fields;
    if(phase==0||phase==1){
        if(!collider_plane_detail::matrix(frame)||!source_intersection_detail::valid(d.local_point))return fail("declared numeric frame/local point required");
        if(phase==1)s.previous3c=input.current30;
        s.current30=collider_plane_detail::point(frame,d.local_point);if(!source_intersection_detail::valid(s.current30))return fail("transformed center numeric domain");if(phase==0)s.previous3c=s.current30;next.center_transformed=true;
        if(phase==0&&s.start48>0){
            if(owner.registration.kind83){s.start48=s16(s32(s.start48)-1);next.operation=ColliderPlaneLifecycleOperation::Waiting;return done();}
            s16 clock=0;if(!collider_plane_lifecycle_detail::clock_s16(owner,&clock))return fail("finite fctiwz-s16 clock required");next.clock_read=true;
            if(clock<s.start48){next.operation=ColliderPlaneLifecycleOperation::Waiting;return done();}
        }
        if(phase==1&&s.end4a>=0){
            if(owner.registration.kind83){s.end4a=s16(s32(s.end4a)-1);if(s.end4a<=0)return retire();}
            else{s16 clock=0;if(!collider_plane_lifecycle_detail::clock_s16(owner,&clock))return fail("finite fctiwz-s16 clock required");next.clock_read=true;if(clock>s.end4a)return retire();}
        }
        if(d.flags2&0x81u)return fail("8A5D0/owner-parameter side-effect consumer required");
        if(!collider_plane_detail::fields(input.shape)||(phase==1&&(!input.shape.initialized||!collider_plane_lifecycle_detail::same(input.current30,input.shape.center))))return fail("explicit coherent prior shape state required");
        if(phase==0){s.phase10=1;s.interval4e=0;next.operation=ColliderPlaneLifecycleOperation::Initialize;next.shape_initialized=true;}
        else{next.operation=ColliderPlaneLifecycleOperation::Update;next.shape_updated=true;}
        if(!collider_plane_build(profile,d,frame,actor_c0,input.shape,phase==1,&s.shape,error))return false;
        if(owner.freeze281)s.phase10=2;
    }else{
        if(!collider_plane_detail::fields(input.shape)||!input.shape.initialized||!collider_plane_lifecycle_detail::same(input.current30,input.shape.center))return fail("frozen phase requires coherent initialized shape");
        next.operation=ColliderPlaneLifecycleOperation::Hold;if(!owner.freeze281)s.phase10=1;
    }
    if(!collider_frame_degenerate(profile,frame,&next.degenerate))return fail("frame column filter domain");
    next.frame_filter_used=true;
    if(!next.degenerate){CollisionRegistrationInput registration;registration.presence_declared=registration.descriptor_declared=true;registration.owner=owner.registration;registration.peer=peer;registration.descriptor_slot1=d.binding1;registration.descriptor_flags2=d.flags2;if(!collision_registration_plan(registration,&next.membership,error))return false;next.membership_used=next.registry_publication_pending=true;}
    return done();
}
} // namespace gf
