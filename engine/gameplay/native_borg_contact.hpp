#pragma once
#include <algorithm>
#include <array>
#include <optional>
#include "engine/gameplay/battle_swept_circle.hpp"
namespace gf {
struct NativeBorgStaticSurface {
    f32 height=0;std::array<f32,3> normal{0,1,0};u32 triangle=0;
    bool operator==(const NativeBorgStaticSurface&)const=default;
};
//Shared native MVP contact policy, NOT recovered original solid/body response.
class IBattleMvpTerrain {
public:
    virtual ~IBattleMvpTerrain()=default;
    virtual bool height(f32 x,f32 z,f32 current_y,f32* out)const=0;
    //Height-only providers deliberately cannot authorize selected-plane pushes.
    virtual bool static_surface(f32,f32,f32,NativeBorgStaticSurface*)const{return false;}
    virtual bool coplanar_crossing(u32,u32,const std::array<f32,3>&,const std::array<f32,3>&)const{return false;}
    virtual bool gentle_slope_crossing(u32,u32,const std::array<f32,3>&,const std::array<f32,3>&)const{return false;}
};
struct NativeBorgContactRequest {
    std::array<f32,3> position{},other_position{};BattlePoint delta{};
    f32 arena_half=1800,radius=42,height=100,step=45;
    bool grounded=true,other_active=false;
    std::optional<f32> vertical_end_y{}; //source airborne proxy only; default preserves ordinary MVP response
};
struct NativeBorgContactResult {
    std::array<f32,3> position{};BattlePoint actual_delta{};
    bool grounded=true,terrain_blocked=false,body_hit=false,bounds_hit=false,slid=false;
};
inline bool native_borg_resolve_contact(const NativeBorgContactRequest& request,const IBattleMvpTerrain* terrain,NativeBorgContactResult* out){
    if(!out)return false;
    for(auto v:request.position)if(!std::isfinite(v)||std::abs(v)>1e5f)return false;
    if(request.other_active)for(auto v:request.other_position)if(!std::isfinite(v)||std::abs(v)>1e5f)return false;
    if(request.vertical_end_y&&(!std::isfinite(*request.vertical_end_y)||std::abs(*request.vertical_end_y)>1e5f))return false;
    for(auto v:{request.delta.x,request.delta.z,request.arena_half,request.radius,request.height,request.step})if(!std::isfinite(v)||std::abs(v)>1e5f)return false;
    if(request.arena_half<=0||request.radius<=0||request.radius*2>=request.arena_half||request.height<=0||request.step<0)return false;
    auto delta=request.delta;const auto& position=request.position;const f32 bound=request.arena_half-request.radius;
    const f32 requested_x=position[0]+delta.x,requested_z=position[2]+delta.z;
    const f32 clamped_x=std::clamp(requested_x,-bound,bound),clamped_z=std::clamp(requested_z,-bound,bound);
    delta.x=clamped_x-position[0];delta.z=clamped_z-position[2];
    struct Candidate {BattlePoint delta{};f32 y=0,portion=1;bool blocked=false;};
    auto probe=[&](BattlePoint movement,Candidate* candidate){
        *candidate={};
        const f32 end_y=request.vertical_end_y.value_or(position[1]);
        const bool vertical_overlap=std::min(position[1],end_y)<request.other_position[1]+request.height&&
            std::max(position[1],end_y)>request.other_position[1]-request.height;
        if(request.other_active&&vertical_overlap){
            const f32 rx=position[0]-request.other_position[0],rz=position[2]-request.other_position[2];
            if(rx*movement.x+rz*movement.z<0){f32 t=0;if(battle_swept_hit({position[0],position[2]},movement,{request.other_position[0],request.other_position[2]},2*request.radius,&t))candidate->portion=std::max(0.f,t-1e-4f);}
        }
        candidate->delta={movement.x*candidate->portion,movement.z*candidate->portion};
        if(terrain&&!terrain->height(position[0]+candidate->delta.x,position[2]+candidate->delta.z,position[1],&candidate->y))return false;
        if(!std::isfinite(candidate->y)||std::abs(candidate->y)>20000)return false;
        candidate->blocked=candidate->y>position[1]+(request.grounded?request.step:0);return true;
    };
    Candidate selected;if(!probe(delta,&selected))return false;
    const bool blocked=selected.blocked;
    if(blocked&&delta.x!=0&&delta.z!=0){
        f32 best=0;
        for(const auto alternative:{BattlePoint{delta.x,0},BattlePoint{0,delta.z}}){
            Candidate candidate;if(!probe(alternative,&candidate))return false;
            const f32 square=candidate.delta.x*candidate.delta.x+candidate.delta.z*candidate.delta.z;
            if(!candidate.blocked&&square>best){selected=candidate;best=square;}
        }
    }
    NativeBorgContactResult result;result.position=position;result.grounded=request.grounded;
    result.terrain_blocked=blocked;result.body_hit=selected.portion<1;result.bounds_hit=clamped_x!=requested_x||clamped_z!=requested_z;result.slid=blocked&&!selected.blocked;
    if(!selected.blocked){
        result.position[0]+=selected.delta.x;result.position[2]+=selected.delta.z;
        if(result.grounded){if(result.position[1]-selected.y>request.step)result.grounded=false;else result.position[1]=selected.y;}
    }
    result.actual_delta={result.position[0]-position[0],result.position[2]-position[2]};*out=result;return true;
}
}
