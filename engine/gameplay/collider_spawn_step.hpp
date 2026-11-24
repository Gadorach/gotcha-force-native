#pragma once
#include "engine/assets/collider_spawn_sweep.hpp"
#include "engine/gameplay/collider_initial_shape.hpp"
namespace gf {
//First ordinary update/hold only. Effect flags1/80, finite clocks, shape changes
//into swept3 and arbitrary box updates remain explicitly closed.
inline bool collider_spawn_step(const ColliderSpawnSweepProfile& recipe,const ColliderInitialShapeProfile& profile,
    const CharacterCollisionDescriptor& d,const ColliderInitialPlan& before,
    const ColliderPlaneLifecycleOwnerFields& owner,const CollisionRegistrationPeerFields& peer,
    const Mat4& frame,f32 c0,ColliderInitialPlan* out,std::string* error){
    if(out && &before==out){if(error)*error="spawn collider step: aliased state/output";return false;}
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("spawn collider step: ")+why;return false;};
    using namespace collider_plane_detail;using namespace source_intersection_detail;
    if(!recipe.ready()||!profile.ready()||!out||!d.declared||!owner.lifecycle_declared||!owner.registration.declared||!owner.registration.present||owner.registration.active00!=1||owner.retire1d8||owner.freeze281>1||d.start6||d.end8!=-1||(d.flags2&0x81)||!matrix(frame)||!valid(d.local_point)||!number(c0)||c0<0||c0>1e4f)return fail("ordinary current owner/descriptor/frame required");
    const auto phase=before.fields.phase10;if((phase!=1&&phase!=2)||before.fields.end4a!=-1||before.fields.shape58!=d.shape||!before.fields.conditional_complete)return fail("coherent initialized first shape required");
    ColliderInitialPlan next=before;auto& s=next.fields;s.field13=0;next.membership={};next.degenerate=false;
    if(phase==2){if(!owner.freeze281)s.phase10=1;}
    else{
        s.previous3c=before.fields.current30;s.current30=point(frame,d.local_point);
        if(!valid(s.current30))return fail("current collider center domain");
        s.previous_center70=before.fields.center64;s.center64=s.current30;
        const f32 travel=length(sub(s.center64,s.previous_center70));if(!number(travel))return fail("shape center motion domain");
        if(d.shape==0){
            const f32 radius=d.scalar(0x18);if(!number(radius)||radius<0)return fail("sphere radius");
            s.sphere.previous_radius8c=before.fields.sphere.radius88;s.sphere.radius88=f32(radius*c0);
            s.broad_radius7c=std::fma(d.flags2&0x10?profile.geometry().value(2):radius,c0,travel);
        }else if(d.shape==2){
            const std::array<f32,3> extent{d.scalar(0x18),d.scalar(0x1c),d.scalar(0x20)};const f32 radius=d.scalar(0x24);
            if(!valid(extent)||!number(radius)||radius<0)return fail("capsule extent/radius");
            auto& capsule=s.capsule;capsule.previous_endpoints=before.fields.capsule.endpoints;
            capsule.endpoints[0]=point(frame,sub(d.local_point,extent));std::array<f32,3> high;
            for(unsigned k=0;k<3;++k)high[k]=f32(d.local_point[k]+extent[k]);
            capsule.endpoints[1]=point(frame,high);
            if(!valid(capsule.endpoints[0])||!valid(capsule.endpoints[1]))return fail("capsule endpoints");
            capsule.directions[0]=sub(capsule.endpoints[1],capsule.endpoints[0]);capsule.directions[1]=sub(capsule.endpoints[0],capsule.endpoints[1]);
            const f32 a=dot(capsule.directions[0],capsule.directions[0]),b=dot(capsule.directions[1],capsule.directions[1]);
            if(!std::isfinite(a)||!std::isfinite(b))return fail("capsule direction domain");
            if(a>0&&b>0){
                for(unsigned i=0;i<2;++i)capsule.normals[i]=scale(capsule.directions[i],f32(1.f/length(capsule.directions[i])));
                capsule.radius88=f32(radius*c0);
                //3B304 atBD54/BD58 really squares the sum written to7C.
                const f32 sum=f32(travel+f32(f32(length(extent)*c0)+capsule.radius88));
                s.broad_radius7c=f32(sum*sum);
            }else{s.broad_radius7c=0;s.conditional_complete=false;}
        }else if(d.shape==4){
            if(!collider_plane_build(profile.geometry(),d,frame,c0,before.fields.plane,true,&s.plane,error))return false;
            s.center64=s.plane.center;s.previous_center70=s.plane.previous_center;s.broad_radius7c=s.plane.broad_radius;
        }else return fail("unjoined non-frozen shape update");
        if(owner.freeze281)s.phase10=2;
    }
    if(!number(s.broad_radius7c))return fail("derived broad radius domain");
    if(!collider_frame_degenerate(profile.geometry(),frame,&next.degenerate))return fail("source column filter");
    if(!next.degenerate){CollisionRegistrationInput registration;registration.presence_declared=registration.descriptor_declared=true;registration.owner=owner.registration;registration.peer=peer;registration.descriptor_slot1=d.binding1;registration.descriptor_flags2=d.flags2;if(!collision_registration_plan(registration,&next.membership,error))return false;}
    *out=std::move(next);if(error)error->clear();return true;
}
}
