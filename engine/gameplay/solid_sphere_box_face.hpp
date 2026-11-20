#pragma once
#include <string>
#include "engine/graphics/source_intersection.hpp"

namespace gf {
struct SolidBoxFacePlane {
    std::array<f32,3> normal{},anchor{};
};
struct SolidSphereBoxFaceInput {
    std::array<f32,3> center{},previous_center{};
    f32 radius=0;
    std::array<SolidBoxFacePlane,6> faces{},previous_faces{};
    bool declared=false;
};
struct SolidSphereBoxFaceResult {
    bool contact=false;
    unsigned face=0;
    std::array<f32,3> normal{};
    f32 clearance=0,previous_clearance=0,signed_separation=0;
    bool operator==(const SolidSphereBoxFaceResult&)const=default;
};

namespace solid_sphere_box_detail {
// The opposite plane pairs are (0,3), (1,4), (2,5). Source face order
// matters: the four other planes, not the opposite one, bound each face.
inline constexpr unsigned opposite(unsigned i){return (i+3)%6;}
inline bool box(const std::array<SolidBoxFacePlane,6>& faces){
    using namespace source_intersection_detail;
    for(const auto& f:faces)if(!valid(f.normal)||!valid(f.anchor)||std::abs(dot(f.normal,f.normal)-1.f)>.001f)return false;
    for(unsigned i=0;i<6;++i){
        const auto& f=faces[i];const auto& other=faces[opposite(i)];
        if(dot(f.normal,other.normal)>-.999f||dot(sub(other.anchor,f.anchor),f.normal)>=0)return false;
        for(unsigned j=0;j<6;++j)if(j!=i&&j!=opposite(i)&&std::abs(dot(f.normal,faces[j].normal))>.001f)return false;
    }
    return true;
}
}

// 32FF8's single-face proposal after 30348 has supplied both plane histories.
// This is a bounded float model, not a sweep or an actor response. The selected
// normal and signed separation are inputs to the separate 358F4 owner.
// Multi-face cases and the inside-box fallback are not fabricated as misses.
inline bool solid_sphere_box_face(const SolidSphereBoxFaceInput& in,SolidSphereBoxFaceResult* out,std::string* error){
    using namespace source_intersection_detail;
    auto fail=[&](const char* why){if(error)*error=std::string("sphere-box face: ")+why;return false;};
    if(!out||!in.declared||!valid(in.center)||!valid(in.previous_center)||
       !std::isfinite(in.radius)||in.radius<=0||in.radius>1e4f||
       !solid_sphere_box_detail::box(in.faces)||!solid_sphere_box_detail::box(in.previous_faces))
        return fail("declared sphere and both oriented box histories required");
    std::array<f32,6> now{},before{};unsigned inside=0;
    for(unsigned i=0;i<6;++i){
        const auto& face=in.faces[i];const auto& previous=in.previous_faces[i];
        now[i]=dot(sub(in.center,face.anchor),face.normal);
        before[i]=dot(sub(in.previous_center,previous.anchor),previous.normal);
        inside+=now[i]<=0.f;
    }
    unsigned faces=0,index=0;
    for(unsigned i=0;i<6;++i){
        if(now[i]>in.radius||before[i]<0.f)continue;
        bool within=true;
        for(unsigned j=0;j<6;++j)if(j!=i&&j!=solid_sphere_box_detail::opposite(i))within=within&&now[j]<=in.radius;
        if(within){++faces;index=i;}
    }
    SolidSphereBoxFaceResult result;
    if(faces==1)result={true,index,in.faces[index].normal,now[index],before[index],f32(now[index]-in.radius)};
    else if(faces||inside==6)return fail("multi-face or interior continuation is not implemented");
    *out=result;if(error)error->clear();return true;
}
} // namespace gf
