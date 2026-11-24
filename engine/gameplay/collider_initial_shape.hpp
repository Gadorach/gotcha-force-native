#pragma once
#include "engine/assets/collider_initial_shape.hpp"
#include "engine/gameplay/collider_plane_lifecycle.hpp"
namespace gf {
struct ColliderSphereInitial {f32 radius88=0,previous_radius8c=0;};
struct ColliderCapsuleInitial {
    std::array<std::array<f32,3>,2> endpoints{},previous_endpoints{},directions{},normals{};f32 radius88=0;
};
struct ColliderBoxInitial {
    std::array<std::array<f32,3>,8> corners{};
    std::array<std::array<f32,3>,6> normals{},previous_normals{},anchors{};
    u8 normals_written=0;
};
struct ColliderInitialFields {
    u8 shape58=0,phase10=0,parameter11=0,field13=0,interval4e=0;s16 start48=0,end4a=-1,generation4c=0;
    std::array<f32,3> current30{},previous3c{},center64{},previous_center70{};f32 broad_radius7c=0;
    ColliderSphereInitial sphere;ColliderCapsuleInitial capsule;ColliderBoxInitial box;ColliderPlaneFields plane;
    bool conditional_complete=false;
};
struct ColliderInitialPlan {
    ColliderInitialFields fields;CollisionRegistrationPlan membership;bool degenerate=false;
};
//8A65C first phase0/start0/end−1, without parameter effects or retirement. Native
//ownership and full source enumeration belong to the scene, not this adapter.
inline bool collider_initial_plan(const ColliderInitialShapeProfile& profile,const CharacterCollisionDescriptor& d,
    const ColliderPlaneLifecycleOwnerFields& owner,const CollisionRegistrationPeerFields& peer,const Mat4& frame,f32 c0,
    ColliderInitialPlan* out,std::string* error){
    if(out){*out={};}auto fail=[&](const char* why){if(error)*error=std::string("collider initialization: ")+why;return false;};
    using namespace collider_plane_detail;using namespace source_intersection_detail;
    if(!out||!profile.ready()||!d.declared||!owner.lifecycle_declared||!owner.registration.declared||!owner.registration.present||owner.registration.active00!=1||owner.retire1d8||owner.freeze281>1||d.start6||d.end8!=-1||(d.flags2&0x81)||!matrix(frame)||!valid(d.local_point)||!number(c0)||c0<0||c0>1e4f)return fail("fresh active source owner/timers/frame/no-effect domain required");
    ColliderInitialPlan p;auto& s=p.fields;s.shape58=d.shape;s.phase10=owner.freeze281?2:1;s.parameter11=u8(d.parameter4);s.current30=point(frame,d.local_point);s.previous3c=s.center64=s.current30;
    if(!valid(s.current30))return fail("transformed source center");
    auto scalar=[&](unsigned at,f32* value){*value=d.scalar(at);return number(*value)&&*value>=0;};
    if(d.shape==0){
        f32 radius=0;if(!scalar(0x18,&radius))return fail("sphere radius");
        s.previous_center70=s.center64;s.sphere.radius88=s.sphere.previous_radius8c=f32(radius*c0);
        s.broad_radius7c=f32((d.flags2&0x10?profile.geometry().value(2):radius)*c0);s.conditional_complete=true;
    }else if(d.shape==1){
        std::array<f32,3> extent;for(unsigned k=0;k<3;++k)if(!scalar(0x18+4*k,&extent[k]))return fail("box extent");
        for(unsigned i=0;i<8;++i){std::array<f32,3> local;for(unsigned k=0;k<3;++k)local[k]=std::fma(extent[k],profile.corners()[i][k],d.local_point[k]);s.box.corners[i]=point(frame,local);if(!valid(s.box.corners[i]))return fail("box corner");}
        for(unsigned i=0;i<6;++i){const auto& face=profile.faces()[i];auto n=cross(sub(s.box.corners[face[1]],s.box.corners[face[0]]),sub(s.box.corners[face[2]],s.box.corners[face[0]]));s.box.normals[i]=n;++s.box.normals_written;
            const f32 squared=dot(n,n);if(!std::isfinite(squared))return fail("box face normal");
            if(squared<=0)break;
            s.box.normals[i]=scale(n,f32(1.f/length(n)));
            if(i==5)s.conditional_complete=true;
        }
        if(s.conditional_complete){s.broad_radius7c=length(sub(s.box.corners[0],s.center64));s.previous_center70=s.center64;s.box.previous_normals=s.box.normals;for(unsigned i=0;i<6;++i)s.box.anchors[i]=s.box.corners[profile.faces()[i][0]];}
    }else if(d.shape==2){
        const std::array<f32,3> extent{d.scalar(0x18),d.scalar(0x1c),d.scalar(0x20)};f32 radius=0;
        if(!valid(extent)||!scalar(0x24,&radius))return fail("capsule extent/radius");
        auto& c=s.capsule;s.previous_center70=s.center64;c.endpoints[0]=point(frame,sub(d.local_point,extent));std::array<f32,3> high;for(unsigned k=0;k<3;++k)high[k]=f32(d.local_point[k]+extent[k]);c.endpoints[1]=point(frame,high);c.previous_endpoints=c.endpoints;
        if(!valid(c.endpoints[0])||!valid(c.endpoints[1]))return fail("capsule endpoints");
        c.directions[0]=sub(c.endpoints[1],c.endpoints[0]);c.directions[1]=sub(c.endpoints[0],c.endpoints[1]);
        const f32 a=dot(c.directions[0],c.directions[0]),b=dot(c.directions[1],c.directions[1]);if(!std::isfinite(a)||!std::isfinite(b))return fail("capsule direction");
        if(a>0&&b>0){for(unsigned i=0;i<2;++i)c.normals[i]=scale(c.directions[i],f32(1.f/length(c.directions[i])));c.radius88=f32(radius*c0);s.broad_radius7c=f32(f32(length(extent)*c0)+c.radius88);s.conditional_complete=true;}
    }else if(d.shape==4){
        ColliderPlaneFields seed;seed.declared=true;
        if(!collider_plane_build(profile.geometry(),d,frame,c0,seed,false,&s.plane,error))return false;
        s.previous_center70=s.plane.previous_center;s.broad_radius7c=s.plane.broad_radius;s.conditional_complete=dot(s.plane.normal,s.plane.normal)>0;
    }else return fail("unjoined source shape initializer");
    if(!number(s.broad_radius7c)||!number(s.sphere.radius88)||!number(s.capsule.radius88))return fail("derived radius domain");
    if(!collider_frame_degenerate(profile.geometry(),frame,&p.degenerate))return fail("source column-length filter");
    if(!p.degenerate){CollisionRegistrationInput input;input.presence_declared=input.descriptor_declared=true;input.owner=owner.registration;input.peer=peer;input.descriptor_slot1=d.binding1;input.descriptor_flags2=d.flags2;if(!collision_registration_plan(input,&p.membership,error))return false;}
    *out=std::move(p);if(error)error->clear();return true;
}
}
