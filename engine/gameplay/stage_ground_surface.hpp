#pragma once
#include <algorithm>
#include "engine/gameplay/stage_ground.hpp"
namespace gf {
struct StageGroundSurfaceResult {
    f32 height=0;std::array<f32,3> normal{0,1,0};u32 visited=0,triangle=0;
    bool inside=false,selected=false;
    bool operator==(const StageGroundSurfaceResult&)const=default;
};
// Exact shared vertices and a path through the edge interior are mandatory.
// The gentle mode is a native admission limit, not an original angle rule.
inline bool stage_ground_slope_crossing(const StihGrid& grid,u32 from,u32 to,
                                       const std::array<f32,3>& start,const std::array<f32,3>& end,bool gentle=false){
    const auto triangles=grid.triangles();
    if(from==to||from>=triangles.size()||to>=triangles.size())return false;
    const auto& a=triangles[from];const auto& b=triangles[to];
    if(a.plane[1]<=0||b.plane[1]<=0)return false;
    const bool coplanar=a.plane==b.plane;
    if(!coplanar){
        if(!gentle||a.plane[1]<.8f||b.plane[1]<.8f)return false;
        f64 dot=0,la=0,lb=0;
        for(unsigned i=0;i<3;++i){
            dot+=f64(a.plane[i])*b.plane[i];la+=f64(a.plane[i])*a.plane[i];lb+=f64(b.plane[i])*b.plane[i];
        }
        constexpr f64 cos_ten_degrees=.984807753012208;
        if(dot<=0||dot*dot<cos_ten_degrees*cos_ten_degrees*la*lb)return false;
    }
    std::array<unsigned,2> common{};
    unsigned matches=0,other_a=0,other_b=0;
    for(unsigned i=0;i<3;++i){
        bool shared=false;
        for(unsigned j=0;j<3;++j)if(a.vertices[i]==b.vertices[j]){shared=true;break;}
        if(shared){if(matches==2)return false;common[matches++]=i;}
        else other_a=i;
    }
    if(matches!=2)return false;
    unsigned unmatched=0;
    for(unsigned i=0;i<3;++i)if(b.vertices[i]!=a.vertices[common[0]]&&b.vertices[i]!=a.vertices[common[1]]){
        ++unmatched;other_b=i;
    }
    if(unmatched!=1)return false;
    const auto& p=a.vertices[common[0]];const auto& q=a.vertices[common[1]];
    if(!coplanar)for(const auto& edge_vertex:{p,q}){
        const auto height=[&](const StihTriangle& t,f32* y){
            return stage_ground_detail::height(t,{f32(t.vertices[0][0]-edge_vertex[0]),f32(t.vertices[0][2]-edge_vertex[2])},y);
        };
        f32 y_a=0,y_b=0;
        if(!height(a,&y_a)||!height(b,&y_b)||std::abs(y_a-y_b)>.01f||
           std::abs(y_a-edge_vertex[1])>.01f||std::abs(y_b-edge_vertex[1])>.01f)return false;
    }
    const auto side=[](const auto& u,const auto& v,const auto& point){
        return (f64(v[0])-u[0])*(f64(point[2])-u[2])-(f64(v[2])-u[2])*(f64(point[0])-u[0]);
    };
    // A shared edge must actually separate the two projected triangle interiors.
    const f64 left=side(p,q,a.vertices[other_a]),right=side(p,q,b.vertices[other_b]);
    const f64 start_side=side(p,q,start),end_side=side(p,q,end);
    if(!((left<0&&right>0&&start_side<=0&&end_side>=0)||
         (left>0&&right<0&&start_side>=0&&end_side<=0)))return false;
    // Opposite endpoint sides alone allow crossing the infinite line outside
    // the edge, where the union of two triangles can contain a gap.
    const f64 near=side(start,end,p),far=side(start,end,q);
    return (near<0&&far>0)||(near>0&&far<0);
}
inline bool stage_ground_coplanar_crossing(const StihGrid& grid,u32 from,u32 to,
                                          const std::array<f32,3>& start,const std::array<f32,3>& end){
    return stage_ground_slope_crossing(grid,from,to,start,end);
}
namespace stage_ground_detail {
struct ProjectedFloorSpan {
    f64 enter=0,exit=1;
    bool intersects=true,start_inside=true,end_inside=true,edge_exit=false;
};
// Clip against the three XZ half planes. A point or edge touch counts when
// checking competing faces, but never proves an interior exit.
inline bool projected_floor_span(const StihTriangle& face,const std::array<f32,3>& start,
                                 const std::array<f32,3>& end,ProjectedFloorSpan* out){
    auto side=[](const auto& a,const auto& b,const auto& p){
        return (f64(b[0])-a[0])*(f64(p[2])-a[2])-(f64(b[2])-a[2])*(f64(p[0])-a[0]);
    };
    const auto& v=face.vertices;
    const f64 winding=side(v[0],v[1],v[2]);
    if(!out||!winding||face.plane[1]<=0)return false;
    const f64 sign=winding>0?1:-1;
    ProjectedFloorSpan span;
    std::array<f64,3> from{},to{};
    unsigned outgoing=3;
    for(unsigned i=0;i<3;++i){
        from[i]=sign*side(v[i],v[(i+1)%3],start);
        to[i]=sign*side(v[i],v[(i+1)%3],end);
        span.start_inside=span.start_inside&&from[i]>0;
        span.end_inside=span.end_inside&&to[i]>0;
        if(from[i]<0&&to[i]<0){span.intersects=false;*out=span;return true;}
        if(from[i]<0)span.enter=std::max(span.enter,-from[i]/(to[i]-from[i]));
        if(to[i]<0){
            const f64 exit=from[i]/(from[i]-to[i]);
            if(exit<span.exit){span.exit=exit;outgoing=i;}
        }
    }
    span.intersects=span.enter<=span.exit;
    span.edge_exit=span.start_inside&&outgoing<3&&span.exit>0&&span.exit<1;
    if(span.edge_exit)for(unsigned i=0;i<3;++i)if(i!=outgoing){
        const auto& a=v[i];const auto& b=v[(i+1)%3];
        if(std::fma(span.exit,to[i]-from[i],from[i])<=.001*std::hypot(f64(b[0])-a[0],f64(b[2])-a[2]))
            span.edge_exit=false;
    }
    *out=span;return true;
}
inline f64 projected_floor_clearance(const StihTriangle& face,const std::array<f32,3>& point){
    const auto& v=face.vertices[0];const auto& n=face.plane;
    return f64(point[1])-v[1]+(f64(n[0])*(f64(point[0])-v[0])+f64(n[2])*(f64(point[2])-v[2]))/n[1];
}
} // namespace stage_ground_detail

// Evidence for a projected foot step that starts on an upward face, descends
// inside that same face, then exits via one edge without entering any higher
// floor. Scoped to a single known floor cell, not a capsule or wall sweep.
inline bool stage_ground_same_face_push_exit(const StihGrid& grid,const StageGroundParameters& p,u32 face,
                                             const std::array<f32,3>& start,const std::array<f32,3>& integrated,
                                             const std::array<f32,3>& projected){
    using namespace stage_ground_detail;
    if(!parameters(p)||!vector(start)||!vector(integrated)||!vector(projected)||
       !inside(p,start)||!inside(p,integrated)||!inside(p,projected)||
       start[1]<=integrated[1]||projected[1]<integrated[1])return false;
    std::span<const u32> refs,approach_refs,exit_refs;
    if(!cell(grid,p,start,&refs)||!cell(grid,p,integrated,&approach_refs)||!cell(grid,p,projected,&exit_refs)||
       refs.data()!=approach_refs.data()||refs.data()!=exit_refs.data()||refs.size()!=approach_refs.size()||refs.size()!=exit_refs.size()||
       refs.size()>128||std::find(refs.begin(),refs.end(),face)==refs.end())return false;
    const auto& selected=grid.triangles()[face];
    ProjectedFloorSpan approach,exit;
    if(!projected_floor_span(selected,start,integrated,&approach)||!approach.start_inside||!approach.end_inside||
       !projected_floor_span(selected,start,projected,&exit)||!exit.edge_exit)return false;
    constexpr f64 tolerance=.001; // Narrow documented native near-plane limit, not a source physics constant.
    const f64 from=projected_floor_clearance(selected,start);
    if(from<0||from>tolerance||projected_floor_clearance(selected,integrated)>=-tolerance||
       std::fma(exit.exit,projected_floor_clearance(selected,projected)-from,from)<0)return false;
    for(u32 index:refs)if(index!=face){
        const auto& other=grid.triangles()[index];
        for(const auto* end:{&integrated,&projected}){
            ProjectedFloorSpan span;
            if(!projected_floor_span(other,start,*end,&span))return false;
            if(!span.intersects)continue;
            const f64 at_start=projected_floor_clearance(other,start),change=projected_floor_clearance(other,*end)-at_start;
            if(std::min(std::fma(span.enter,change,at_start),std::fma(span.exit,change,at_start))<=tolerance)return false;
        }
    }
    return true;
}
//3E0A8's STATIC height+normal sibling. Reuses shared cell/projection/plane and
//ceiling kernels; its outside return is the source constant-5000, NOT pointY.
//Moving platforms and3F688's nonzero87/capsule-query branch remain refused.
inline bool stage_ground_surface(const std::array<const StihGrid*,3>& grids,const StageGroundParameters& p,const StageGroundActor& actor,const std::array<f32,3>& position,f32 height,StageGroundSurfaceResult* out,std::string* error){
    auto fail=[&](){if(error)*error="stage surface: current static source grids/actor/plane required";return false;};
    using namespace stage_ground_detail;
    if(!out||!actor.fields_declared||actor.attached1db||!parameters(p)||!vector(position)||!number(height))return fail();
    StageGroundSurfaceResult result;result.height=-5000;
    if(!inside(p,position)){*out=result;if(error)error->clear();return true;}
    result.inside=true;for(f32 v:{actor.current24,actor.previous30,actor.offset6d0,actor.previous6d4})if(!number(v))return fail();
    const bool raised=height>=p.high34;auto point=position;point[1]=f32(position[1]+actor.offset6d0);
    const f32 previous=f32(actor.previous30+actor.previous6d4),current=f32(actor.current24+actor.offset6d0),span=f32(previous-current);
    if(!vector(point)||!number(previous)||!number(current)||!number(span))return fail();
    f32 ceiling=0;if(!stage_ground_detail::ceiling(grids,p,point,previous,&ceiling))return fail();
    if(span>p.zero)height=f32(height+span);
    height=f32(height+position[1]);if(!number(height)||!grids[0])return fail();
    std::span<const u32> refs;if(!cell(*grids[0],p,position,&refs))return fail();
    f32 best=p.low68,low=p.high34,high=p.low68;std::array<f32,3> best_n{0,1,0},low_n{0,1,0},high_n{0,1,0};u32 best_i=0,low_i=0,high_i=0;bool best_set=false,low_set=false,high_set=false;
    for(u32 index:refs){
        ++result.visited;const auto& t=grids[0]->triangles()[index];std::array<f32,2> delta;if(!projected(t,point,false,&delta))continue;
        f32 y=0;if(!stage_ground_detail::height(t,delta,&y))return fail();
        const std::array<f32,3> normal{t.plane[0],t.plane[1],t.plane[2]};if(!vector(normal))return fail();
        if(height>y&&ceiling>y&&best<=y){best=y;best_n=normal;best_i=index;best_set=true;}
        if(low>=y){low=y;low_n=normal;low_i=index;low_set=true;}
        if(high<=y){high=y;high_n=normal;high_i=index;high_set=true;}
    }
    if(raised){result.height=high;result.normal=high_n;result.triangle=high_i;result.selected=high_set;}
    else if(best!=p.low68){result.height=best;result.normal=best_n;result.triangle=best_i;result.selected=best_set;}
    else{result.normal=low_n;result.triangle=low_i;result.selected=low_set;result.height=low==p.high34?p.low68:low;}
    *out=result;if(error){error->clear();}return true;
}
}
