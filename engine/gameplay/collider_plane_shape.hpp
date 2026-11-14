#pragma once
#include "engine/assets/character_collision_bank.hpp"
#include "engine/graphics/source_intersection.hpp"
#include "engine/graphics/transform.hpp"
namespace gf {
struct ColliderPlaneFields {
    std::array<f32,3> center{},previous_center{},normal{},previous_normal{};std::array<std::array<f32,3>,6> points{};
    f32 thickness=0,radius=0,broad_radius=0;bool declared=false,initialized=false;
};
struct ColliderPlaneSegmentResult {std::array<f32,3> point{};bool hit=false;u32 source_flag=0;bool operator==(const ColliderPlaneSegmentResult&)const=default;};
namespace collider_plane_detail {
inline bool number(f32 x){return std::isfinite(x)&&std::abs(x)<=1e7f;}
inline bool matrix(const Mat4& m){for(f32 x:m.m)if(!number(x))return false;return m.m[12]==0&&m.m[13]==0&&m.m[14]==0&&m.m[15]==1;}
inline bool fields(const ColliderPlaneFields& s){if(!s.declared||!number(s.thickness)||!number(s.radius)||!number(s.broad_radius)||s.thickness<0||s.radius<0)return false;for(const auto& v:{s.center,s.previous_center,s.normal,s.previous_normal})if(!source_intersection_detail::valid(v))return false;for(const auto& v:s.points)if(!source_intersection_detail::valid(v))return false;return true;}
inline std::array<f32,3> point(const Mat4& m,const std::array<f32,3>& v){std::array<f32,3> result{};mat4_transform_point(m,v.data(),result.data());return result;}
inline f32 length(const std::array<f32,3>& v){return f32(std::sqrt(f64(source_intersection_detail::dot(v,v))));}
}
//8AA40 tests COLUMN lengths after shape construction, not determinant or an
//arbitrary epsilon. Reflection/nonuniform scale are not automatically rejected.
inline bool collider_frame_degenerate(const ColliderGeometryProfile& p,const Mat4& frame,bool* out){
    if(out)*out=false;
    if(!out||!collider_geometry_profile_valid(p)||!collider_plane_detail::matrix(frame))return false;
    for(unsigned c=0;c<3;++c){const f32 n=collider_plane_detail::length({frame.at(0,c),frame.at(1,c),frame.at(2,c)});if(!std::isfinite(n))return false;if(n<=p.value(7)){*out=true;return true;}}return true;
}
//3C3D8 initialization /3C614 update numeric fields. Prior fields are EXPLICIT;
//a zero normal retains source conditional fields, never a fabricated fallback.
//Host-libm/float matrix model; NOT original PS/frsqrte/MSL/FPSCR/hardware parity.
inline bool collider_plane_build(const ColliderGeometryProfile& p,const CharacterCollisionDescriptor& descriptor,const Mat4& frame,f32 actor_c0,const ColliderPlaneFields& prior,bool update,ColliderPlaneFields* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("collider plane fields: ")+why;return false;};using namespace collider_plane_detail;using namespace source_intersection_detail;
    if(!out||!collider_geometry_profile_valid(p)||!descriptor.declared||descriptor.shape!=4||!matrix(frame)||!fields(prior)||(update&&!prior.initialized)||!number(actor_c0)||actor_c0<0||actor_c0>1e4f)return fail("declared shape4/frame/scale/prior fields required");
    const f32 thickness=descriptor.scalar(0x18),radius=descriptor.scalar(0x1c);const std::array<f32,3> direction{descriptor.scalar(0x20),descriptor.scalar(0x24),descriptor.scalar(0x28)};
    if(!valid(descriptor.local_point)||!valid(direction)||!number(thickness)||!number(radius)||thickness<0||radius<0)return fail("shape-specific numeric domain");
    auto next=prior;next.center=point(frame,descriptor.local_point);next.previous_center=update?prior.center:next.center;if(update)next.previous_normal=prior.normal;next.thickness=f32(thickness*actor_c0);next.radius=f32(radius*actor_c0);next.initialized=true;
    if(update)next.broad_radius=f32(length(sub(next.center,next.previous_center))+f32(next.radius+next.thickness));
    auto combined=mat4_multiply(frame,mat4_translation(descriptor.local_point.data()));
    const f32 x=f32(p.value(3)*f32(p.value(4)*f32(std::atan2(f64(direction[2]),f64(direction[1]))))),z=f32(p.value(3)*f32(p.value(4)*f32(std::atan2(f64(direction[0]),f64(direction[1])))));
    combined=mat4_multiply(combined,mat4_rotation_x(x));combined=mat4_multiply(combined,mat4_rotation_z(z));const std::array<f32,3> local{0,next.radius,0};next.normal=sub(point(combined,local),next.center);const f32 squared=dot(next.normal,next.normal);
    if(!std::isfinite(squared))return fail("normal squared domain");
    if(squared>0){const f32 n=length(next.normal);if(!(n>0)||!std::isfinite(n))return fail("normal length domain");next.normal=scale(next.normal,f32(1.f/n));combined=mat4_multiply(combined,mat4_rotation_x(p.value(5)));for(auto& v:next.points){v=point(combined,local);combined=mat4_multiply(combined,mat4_rotation_z(p.value(6)));}if(!update){next.broad_radius=f32(next.radius+next.thickness);next.previous_normal=next.normal;}}
    if(!fields(next))return fail("derived fields domain");
    *out=next;if(error)error->clear();return true;
}
//389E8 plus38B34's exact-equal endpoint adjustment. This is one DECLARED shape
//query only, not a registry/owner-filter/nearest-result/actor acknowledgement.
inline bool collider_plane_segment(const ColliderGeometryProfile& p,const ColliderPlaneFields& shape,const std::array<f32,3>& start,const std::array<f32,3>& end,ColliderPlaneSegmentResult* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("collider plane segment: ")+why;return false;};using namespace source_intersection_detail;
    if(!out||!collider_geometry_profile_valid(p)||!collider_plane_detail::fields(shape)||!shape.initialized||!valid(start)||!valid(end))return fail("declared initialized shape/segment required");
    auto target=end;if(start==end)target[1]=f32(target[1]+p.value(1));SourceSegmentPlaneHit plane;if(!source_segment_plane(start,target,shape.normal,shape.center,&plane))return fail("source plane numeric domain");if(!plane.side){if(error)error->clear();return true;}
    const f32 distance=collider_plane_detail::length(sub(plane.point,shape.center));if(!std::isfinite(distance))return fail("radius distance domain");if(distance<=shape.radius){const f32 side=dot(sub(start,shape.center),shape.normal);if(!std::isfinite(side))return fail("start side domain");const auto offset=scale(shape.normal,side<0?-shape.thickness:shape.thickness);std::array<f32,3> result{};for(unsigned i=0;i<3;++i)result[i]=f32(plane.point[i]+offset[i]);if(!valid(result))return fail("output numeric domain");out->hit=true;out->point=result;}
    if(error)error->clear();
    return true;
}
} // namespace gf
