#pragma once
#include "engine/gameplay/stage_ground.hpp"
namespace gf {
struct StageGroundSurfaceResult {
    f32 height=0;std::array<f32,3> normal{0,1,0};u32 visited=0,triangle=0;
    bool inside=false,selected=false;
    bool operator==(const StageGroundSurfaceResult&)const=default;
};
// A narrowly certified static seam, not a tolerance for creases or gaps.
inline bool stage_ground_coplanar_crossing(const StihGrid& grid,u32 from,u32 to,
                                          const std::array<f32,3>& start,const std::array<f32,3>& end){
    const auto triangles=grid.triangles();
    if(from==to||from>=triangles.size()||to>=triangles.size())return false;
    const auto& a=triangles[from];const auto& b=triangles[to];
    if(a.plane!=b.plane||a.plane[1]<=0)return false;
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
