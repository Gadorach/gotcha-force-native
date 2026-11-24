#pragma once
#include "engine/assets/collider_visibility.hpp"
#include "engine/gameplay/collider_initial_shape.hpp"
namespace gf {
struct ColliderVisibilityHit {std::array<f32,3> point{};bool hit=false,start_inside=false,closest_nudged=false;u8 box_clip_mask=0;};
namespace collider_visibility_detail {
using namespace source_intersection_detail;
using collider_plane_detail::length;
inline Vec add(const Vec& a,const Vec& b){Vec out;for(unsigned i=0;i<3;++i)out[i]=f32(a[i]+b[i]);return out;}
struct Segment {Vec start,end,delta,reverse,normal,negative;};
inline bool segment(Vec start,Vec end,Segment* out){
    if(!out||!valid(start)||!valid(end))return false;
    if(start==end)end[1]=f32(end[1]+1);
    Segment next;next.start=start;next.end=end;next.delta=sub(end,start);next.reverse=sub(start,end);
    const auto n=length(next.delta);if(!(n>0)||!std::isfinite(n))return false;
    next.normal=scale(next.delta,f32(1/n));next.negative=scale(next.reverse,f32(1/n));*out=next;return true;
}
//38C00's nonparallel branch, including last-qualifying projection selection and
//its two ordered endpoint/reprojection clamps. Parallel table paths stay closed.
inline bool closest_nonparallel(const Segment& a,const Segment& b,Vec* pa,Vec* pb,bool* nudged,std::string* error){
    auto normal=cross(a.delta,b.delta);const auto square=dot(normal,normal);
    if(!std::isfinite(square)||square<=0){if(error)*error="visibility capsule: parallel source38C00 branch unsupported";return false;}
    normal=scale(normal,f32(1/length(normal)));f32 separation=dot(sub(a.start,b.start),normal);
    if(std::abs(separation)<bits_to_f32(0x3a83126f))separation=0;
    const auto offset=scale(normal,separation),delta=sub(add(b.start,offset),a.start);
    const f32 d1=f32(f32(b.delta[0]*a.delta[1])-f32(b.delta[1]*a.delta[0]));
    const f32 d2=f32(f32(b.delta[0]*a.delta[2])-f32(b.delta[2]*a.delta[0]));
    const f32 d3=f32(f32(b.delta[2]*a.delta[1])-f32(b.delta[1]*a.delta[2]));
    unsigned choice=0;const f32 threshold=bits_to_f32(0x3c23d70a);
    if(std::abs(d1)>threshold)choice=1;
    if(std::abs(d2)>threshold)choice=2;
    if(std::abs(d3)>threshold)choice=3;
    bool clamped=false;
    if(!choice){*pa=a.start;*pb=b.start;clamped=true;}
    else{
        const f32 numerator=choice==1?f32(f32(a.delta[0]*delta[1])-f32(a.delta[1]*delta[0])):choice==2?f32(f32(a.delta[0]*delta[2])-f32(a.delta[2]*delta[0])):f32(f32(a.delta[2]*delta[1])-f32(a.delta[1]*delta[2]));
        const f32 t=f32(numerator/(choice==1?d1:choice==2?d2:d3));if(!std::isfinite(t))return false;
        *pb=add(b.start,scale(b.delta,t));*pa=add(*pb,offset);
        if(dot(sub(*pa,a.start),a.delta)<0){*pa=a.start;clamped=true;}
        if(dot(sub(*pa,a.end),a.reverse)<0){*pa=a.end;clamped=true;}
        if(dot(sub(*pb,b.start),b.delta)<0){*pb=b.start;clamped=true;}
        if(dot(sub(*pb,b.end),b.reverse)<0){*pb=b.end;clamped=true;}
    }
    if(clamped){
        auto candidate=add(*pa,scale(a.normal,dot(sub(*pb,*pa),a.normal)));
        if(dot(sub(candidate,a.start),a.delta)>0&&dot(sub(candidate,a.end),a.reverse)>0)*pa=candidate;
        candidate=add(*pb,scale(b.normal,dot(sub(*pa,*pb),b.normal)));
        if(dot(sub(candidate,b.start),b.delta)>0&&dot(sub(candidate,b.end),b.reverse)>0)*pb=candidate;
    }
    *nudged=length(sub(*pa,*pb))<bits_to_f32(0x3a83126f);
    if(*nudged)(*pa)[0]=f32((*pa)[0]+2); //3951C's source80436F9C, not an epsilon
    return valid(*pa)&&valid(*pb);
}
inline bool box(const ColliderBoxInitial& box,Vec start,Vec end,ColliderVisibilityHit* out){
    u8 clip=0;
    for(unsigned i=0;i<6;++i){
        const auto& n=box.normals[i];const auto& anchor=box.anchors[i];
        const f32 a=dot(sub(anchor,start),n),b=dot(sub(anchor,end),n);
        const unsigned mask=(a>=0?1u:0u)|(b>=0?2u:0u);
        if(!mask)return true;
        if(mask==1){auto delta=sub(end,start);const f32 denominator=dot(delta,n);const auto correction=denominator==0?scale(n,b):scale(delta,f32(b/denominator));end=add(end,correction);clip|=2;}
        else if(mask==2){auto delta=sub(start,end);const f32 denominator=dot(delta,n);const auto correction=denominator==0?scale(n,a):scale(delta,f32(a/denominator));start=add(start,correction);clip|=1;}
        if(!valid(start)||!valid(end))return false;
    }
    out->hit=true;out->box_clip_mask=clip;out->start_inside=clip==2;out->point=clip==2?end:start;return true;
}
inline bool sphere(const ColliderInitialFields& shape,Vec start,Vec end,ColliderVisibilityHit* out){
    const f32 radius=shape.sphere.radius88;if(!std::isfinite(radius)||radius<0)return false;
    out->start_inside=length(sub(shape.center64,start))<=radius;
    Segment s;if(!segment(start,end,&s))return false;
    const auto offset=sub(s.start,shape.center64);const f32 a=dot(s.normal,s.normal),b=f32(2*dot(s.normal,offset)),c=f32(dot(offset,offset)-f32(radius*radius));
    const f32 discriminant=f32(f32(b*b)-f32(f32(4*a)*c));
    if(!std::isfinite(discriminant)||!(a>0))return false;
    if(discriminant<0)return true;
    const f32 root=f32(std::sqrt(f64(discriminant))),denominator=f32(2*a);
    const auto plus_offset=scale(s.normal,f32(f32(-b+root)/denominator)),minus_offset=scale(s.normal,f32(f32(-b-root)/denominator));
    const auto plus=add(s.start,plus_offset),minus=add(s.start,minus_offset);
    const unsigned mask=(dot(plus_offset,s.normal)<0?1u:0u)|(dot(sub(plus,s.end),s.negative)<0?2u:0u)|(dot(minus_offset,s.normal)<0?4u:0u)|(dot(sub(minus,s.end),s.negative)<0?8u:0u);
    Vec candidate;
    if(mask==1||mask==2){candidate=minus;out->hit=true;}
    else if(mask==4||mask==8){candidate=plus;out->hit=true;}
    else{
        candidate=length(sub(plus,s.start))>length(sub(minus,s.start))?minus:plus;
        const auto to_start=sub(shape.center64,start),to_end=sub(shape.center64,end);auto direction=sub(end,start);
        const auto n=length(direction);if(!(n>0)||!std::isfinite(n))return false;
        direction=scale(direction,f32(1/n));
        const f32 first=dot(to_start,direction),last=dot(to_end,direction);const unsigned side=(first<0?1u:0u)|(last>0?2u:0u);
        Vec closest;
        if(side==0)closest=add(start,scale(direction,first));else if(side==1)closest=start;else if(side==2)closest=end;else return true;
        out->hit=length(sub(shape.center64,closest))<radius;
    }
    if(out->hit){if(!valid(candidate))return false;out->point=candidate;}return true;
}
}
//Declared shape query only. Native visibility selection and owner masks are
//separate; an unsupported query refuses instead of becoming an unobstructed ray.
inline bool collider_visibility_segment(const ColliderVisibilityProfile& profile,const ColliderInitialShapeProfile& geometry,
    const ColliderInitialFields& shape,const std::array<f32,3>& start,const std::array<f32,3>& end,ColliderVisibilityHit* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("visibility segment: ")+why;return false;};
    using namespace collider_visibility_detail;
    if(!out||!profile.ready()||!geometry.ready()||!valid(start)||!valid(end)||!shape.conditional_complete)return fail("source profile/current represented shape/segment required");
    ColliderVisibilityHit hit;
    if(shape.shape58==0){if(!valid(shape.center64)||!sphere(shape,start,end,&hit))return fail("sphere source arithmetic/degenerate original segment");}
    else if(shape.shape58==1){
        for(unsigned i=0;i<6;++i)if(!valid(shape.box.normals[i])||!valid(shape.box.anchors[i])||!(dot(shape.box.normals[i],shape.box.normals[i])>0))return fail("complete source box planes required");
        if(!box(shape.box,start,end,&hit))return fail("box source arithmetic");
    }
    else if(shape.shape58==2){
        const auto& capsule=shape.capsule;const f32 radius=capsule.radius88;
        if(!std::isfinite(radius)||radius<0||radius>1e7f)return fail("capsule radius numeric domain");
        for(const auto* group:{&capsule.endpoints,&capsule.directions,&capsule.normals})for(const auto& v:*group)if(!valid(v))return fail("capsule source vectors");
        const auto a=dot(sub(start,capsule.endpoints[0]),capsule.normals[0]),b=dot(sub(start,capsule.endpoints[1]),capsule.normals[1]);
        const unsigned mask=(a<0?1u:0u)|(b<0?2u:0u);
        const auto closest=mask==1?capsule.endpoints[0]:mask==0?add(capsule.endpoints[1],scale(capsule.normals[1],b)):capsule.endpoints[1];
        hit.start_inside=length(sub(start,closest))<=radius;
        Segment ray,axis;if(!segment(start,end,&ray)||!segment(capsule.endpoints[0],capsule.endpoints[1],&axis))return fail("capsule source segment");
        //Axis is the collider's original directed segment, including its own
        //published normalized directions rather than a second normalization.
        axis.delta=capsule.directions[0];axis.reverse=capsule.directions[1];axis.normal=capsule.normals[0];axis.negative=capsule.normals[1];
        Vec ray_point,axis_point;
        if(!closest_nonparallel(ray,axis,&ray_point,&axis_point,&hit.closest_nudged,error))return false;
        hit.hit=length(sub(ray_point,axis_point))<radius;if(hit.hit)hit.point=ray_point;
    }else if(shape.shape58==4){
        ColliderPlaneSegmentResult plane;if(!collider_plane_segment(geometry.geometry(),shape.plane,start,end,&plane,error))return false;
        hit.hit=plane.hit;hit.point=plane.point;
    }else return fail("unjoined source shape query");
    *out=hit;if(error)error->clear();return true;
}
}
