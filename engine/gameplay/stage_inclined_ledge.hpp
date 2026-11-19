#pragma once
#include <algorithm>
#include "engine/assets/stih.hpp"

namespace gf {
namespace stage_ledge_detail {
using Point=std::array<f32,3>;
inline f64 side(const Point& a,const Point& b,const Point& p){
    return (f64(b[0])-a[0])*(f64(p[2])-a[2])-(f64(b[2])-a[2])*(f64(p[0])-a[0]);
}
struct Span {
    f64 first=0,last=1;
    unsigned enter=3,leave=3;
    bool ambiguous=false;
};
enum class Clip {Miss,Hit,Uncertain};
// Closed projected interval plus the two authored edges that constrain it.
// Collinear and corner paths cannot certify an upper strip.
inline Clip clip(const StihTriangle& triangle,const Point& a,const Point& b,Span* out){
    const auto& v=triangle.vertices;
    for(unsigned axis:{0u,2u}){
        const auto [lo,hi]=std::minmax({v[0][axis],v[1][axis],v[2][axis]});
        if(std::max(a[axis],b[axis])<lo||std::min(a[axis],b[axis])>hi)return Clip::Miss;
    }
    const f64 area=side(v[0],v[1],v[2]);
    if(!area||!std::isfinite(area))return Clip::Uncertain;
    Span span;
    for(unsigned i=0;i<3;++i){
        const auto& p=v[i];const auto& q=v[(i+1)%3];
        const f64 sign=area>0?1:-1;
        const f64 x=sign*side(p,q,a),y=sign*side(p,q,b);
        if(x<0&&y<0)return Clip::Miss;
        if(x==0||y==0)span.ambiguous=true;
        if(x<0){
            const f64 at=x/(x-y);
            if(at==span.first)span.ambiguous=true;
            if(at>span.first){span.first=at;span.enter=i;}
        }else if(y<0){
            const f64 at=x/(x-y);
            if(at==span.last)span.ambiguous=true;
            if(at<span.last){span.last=at;span.leave=i;}
        }
    }
    if(span.first>span.last)return Clip::Miss;
    *out=span;return Clip::Hit;
}
inline f64 height(const StihTriangle& t,f64 x,f64 z){
    const auto& base=t.vertices[0];
    return base[1]+(t.plane[0]*(base[0]-x)+t.plane[2]*(base[2]-z))/t.plane[1];
}
inline bool plane_fits(const StihTriangle& t){
    for(const auto& v:t.vertices)if(!(std::abs(height(t,v[0],v[2])-v[1])<=.01))return false;
    return true;
}
inline bool same_edge(const StihTriangle& a,unsigned leave,const StihTriangle& b,unsigned enter){
    if(leave>=3||enter>=3)return false;
    const auto &p=a.vertices[leave],&q=a.vertices[(leave+1)%3],&r=b.vertices[enter],&s=b.vertices[(enter+1)%3];
    return (p==r&&q==s)||(p==s&&q==r);
}
}

// Native geometry witness, not an original movement rule. Admits only a
// downhill exit from one inclined face over an edge-connected descending
// cliff strip. One flat face must underlie the whole move. No gaps or
// overlaps in the upper strip, step-sized drops, or vertex exits qualify.
// This supplements (never replaces) source and native selection/sweep checks.
inline bool stage_inclined_ledge_crossing(std::span<const StihTriangle> triangles,u32 from,u32 to,
                                          const std::array<f32,3>& start,const std::array<f32,3>& end,f32 step){
    using namespace stage_ledge_detail;
    constexpr std::size_t triangle_budget=8192;
    constexpr f64 join_epsilon=1e-8;
    if(from==to||from>=triangles.size()||to>=triangles.size()||triangles.size()>triangle_budget||
       !std::isfinite(step)||step<0||end[1]!=start[1])return false;
    for(const auto& p:{start,end})for(const auto v:p)if(!std::isfinite(v)||std::abs(v)>1e5f)return false;
    const auto& upper=triangles[from];const auto& floor=triangles[to];
    const f64 drop=f64(start[1])-floor.vertices[0][1];
    if(upper.plane[1]<.8f||upper.plane[1]>=1.f||floor.plane[0]!=0||floor.plane[1]!=1||floor.plane[2]!=0||
       !(drop>step)||!plane_fits(upper)||!plane_fits(floor))return false;
    const f64 bottom=floor.vertices[0][1];
    if(!(std::abs(height(upper,start[0],start[2])-start[1])<=.01))return false;
    Span origin,base;
    if(clip(upper,start,end,&origin)!=Clip::Hit||origin.ambiguous||origin.first!=0||origin.last<=0||origin.last>=1||
       clip(floor,start,end,&base)!=Clip::Hit||base.ambiguous||base.first!=0||base.last!=1)return false;
    const auto height_at=[&](const StihTriangle& t,f64 fraction){
        return height(t,std::lerp(f64(start[0]),f64(end[0]),fraction),std::lerp(f64(start[2]),f64(end[2]),fraction));
    };
    auto previous_height=height_at(upper,origin.last);
    if(!(previous_height<=start[1]+.01&&previous_height>bottom+.01))return false;
    struct Face {u32 index;Span span;f64 first_y,last_y;};
    std::array<Face,8> strip{};
    std::size_t count=0;
    for(u32 i=0;i<triangles.size();++i){
        if(i==from||i==to)continue;
        const auto& t=triangles[i];Span span;
        const auto hit=clip(t,start,end,&span);
        if(hit==Clip::Miss)continue;
        if(hit!=Clip::Hit||t.plane[1]<=0)return false;
        const f64 first=height_at(t,span.first),last=height_at(t,span.last);
        if(!std::isfinite(first)||!std::isfinite(last))return false;
        if(first<bottom-.01&&last<bottom-.01)continue; // buried, never supporting
        const f64 norm=std::hypot(f64(t.plane[0]),f64(t.plane[1]),f64(t.plane[2]));
        if(span.ambiguous||span.first>=span.last||span.first<origin.last-join_epsilon||span.last>=1||
           !(t.plane[1]*2<=norm)||!plane_fits(t)||first<=last||last<=bottom+.01||count==strip.size())return false;
        strip[count++]={i,span,first,last};
    }
    if(!count)return false;
    for(std::size_t i=1;i<count;++i){
        const auto face=strip[i];auto at=i;
        while(at&&strip[at-1].span.first>face.span.first){strip[at]=strip[at-1];--at;}
        strip[at]=face;
    }
    u32 previous=from;
    Span span=origin;
    for(std::size_t i=0;i<count;++i){
        const auto& face=strip[i];
        if(std::abs(face.span.first-span.last)>join_epsilon||std::abs(face.first_y-previous_height)>.01||
           !same_edge(triangles[previous],span.leave,triangles[face.index],face.span.enter))return false;
        previous=face.index;span=face.span;previous_height=face.last_y;
    }
    return previous_height-bottom>step;
}
}
