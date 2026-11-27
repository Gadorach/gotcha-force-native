#pragma once
#include <algorithm>
#include <array>
#include <optional>
#include <span>
#include "engine/assets/stage_body_partition.hpp"
#include "engine/gameplay/battle_swept_circle.hpp"
namespace gf {
struct NativeBorgStaticSurface {
    f32 height=0;std::array<f32,3> normal{0,1,0};u32 triangle=0;
    bool operator==(const NativeBorgStaticSurface&)const=default;
};
struct NativeBorgStageProjectileHit {
    bool hit=false;
    f32 fraction=0; // along the original centre segment, not its clipped subinterval
    u8 layer=0;u32 triangle=0;
};
//Shared native MVP contact policy, NOT recovered original solid/body response.
class IBattleMvpTerrain {
public:
    virtual ~IBattleMvpTerrain()=default;
    virtual bool height(f32 x,f32 z,f32 current_y,f32* out)const=0;
    //Height-only providers deliberately cannot authorize selected-plane pushes.
    virtual bool static_surface(f32,f32,f32,NativeBorgStaticSurface*)const{return false;}
    virtual bool supports_projectile_stage()const{return false;}
    virtual bool projectile_stage(const std::array<f32,3>&,const std::array<f32,3>&,NativeBorgStageProjectileHit*)const{return false;}
    // Portion of the requested planar step clear of explicitly authored
    // body partitions. Ground-only and legacy terrains are unobstructed.
    virtual std::span<const StageBodyPartition> body_partitions()const{return {};}
    virtual bool body_partition_sweep(BattlePoint start,BattlePoint delta,f32 radius,f32* portion)const;
    virtual bool coplanar_crossing(u32,u32,const std::array<f32,3>&,const std::array<f32,3>&)const{return false;}
    virtual bool gentle_slope_crossing(u32,u32,const std::array<f32,3>&,const std::array<f32,3>&)const{return false;}
    virtual bool inclined_ledge_crossing(u32,u32,const std::array<f32,3>&,const std::array<f32,3>&,f32)const{return false;}
    virtual bool same_face_push_exit(u32,const std::array<f32,3>&,const std::array<f32,3>&,const std::array<f32,3>&)const{return false;}
};
inline bool native_borg_partition_portion(const IBattleMvpTerrain* terrain,BattlePoint start,BattlePoint delta,f32 radius,f32* out){
    if(!out)return false;
    *out=1;
    return !terrain||(terrain->body_partition_sweep(start,delta,radius,out)&&
                      std::isfinite(*out)&&*out>=0&&*out<=1);
}
// Sweep the native square wall proxy (each exposed face is padded by the
// actor radius). Touching is allowed; crossing the interior is not. Using
// an open interval permits sliding along a face without a sticky zero hit.
inline f32 native_borg_partition_step(BattlePoint start,BattlePoint delta,BattlePoint low,BattlePoint high,f32 radius){
    f64 entry=0,exit=1;
    const auto axis=[&](f64 p,f64 d,f64 lo,f64 hi){
        lo-=radius;hi+=radius;
        if(d==0)return p>lo&&p<hi;
        auto a=(lo-p)/d,b=(hi-p)/d;
        if(a>b)std::swap(a,b);
        entry=std::max(entry,a);exit=std::min(exit,b);
        return entry<exit;
    };
    if(!axis(start.x,delta.x,low.x,high.x)||!axis(start.z,delta.z,low.z,high.z))return 1;
    return f32(std::max(0.,entry-1e-4));
}
inline bool IBattleMvpTerrain::body_partition_sweep(BattlePoint start,BattlePoint delta,f32 radius,f32* portion)const{
    if(!portion)return false;
    for(f32 v:{start.x,start.z,delta.x,delta.z,radius})if(!std::isfinite(v)||std::abs(v)>1e5f)return false;
    if(radius<=0)return false;
    const auto boxes=body_partitions();
    if(boxes.size()>kStageBodyPartitionCount)return false;
    *portion=1;
    for(const auto& box:boxes)
        *portion=std::min(*portion,native_borg_partition_step(start,delta,
            {box.min_x,box.min_z},{box.max_x,box.max_z},radius));
    return true;
}
// A flagged partition occludes a direct melee candidate between the two
// native body centres. Use the bare footprint, not the movement padding
// or the attack capsule: a thick capsule can overlap the far body before
// its own centreline has reached a wall.
inline bool native_borg_melee_path_clear(const IBattleMvpTerrain* terrain,BattlePoint from,BattlePoint to){
    if(!terrain)return true;
    const auto boxes=terrain->body_partitions();
    if(boxes.size()>kStageBodyPartitionCount)return false;
    for(const auto& box:boxes)if(box.blocks_melee&&
        native_borg_partition_step(from,{to.x-from.x,to.z-from.z},
            {box.min_x,box.min_z},{box.max_x,box.max_z},0)<1)return false;
    return true;
}
inline bool native_borg_melee_height_overlap(f32 attack_center,f32 half_height,f32 body_bottom,f32 body_height){
    return attack_center+half_height>=body_bottom&&attack_center-half_height<=body_bottom+body_height;
}
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
inline constexpr unsigned kNativeBorgContactPeerLimit=5;
// The explicit list replaces, rather than augments, the legacy single peer.
inline bool native_borg_resolve_contact(const NativeBorgContactRequest& request,
                                       std::span<const std::array<f32,3>> bodies,
                                       const IBattleMvpTerrain* terrain,NativeBorgContactResult* out){
    if(!out||bodies.size()>kNativeBorgContactPeerLimit)return false;
    for(auto v:request.position)if(!std::isfinite(v)||std::abs(v)>1e5f)return false;
    for(const auto& body:bodies)for(auto v:body)if(!std::isfinite(v)||std::abs(v)>1e5f)return false;
    if(request.vertical_end_y&&(!std::isfinite(*request.vertical_end_y)||std::abs(*request.vertical_end_y)>1e5f))return false;
    for(auto v:{request.delta.x,request.delta.z,request.arena_half,request.radius,request.height,request.step})if(!std::isfinite(v)||std::abs(v)>1e5f)return false;
    if(request.arena_half<=0||request.radius<=0||request.radius*2>=request.arena_half||request.height<=0||request.step<0)return false;
    auto delta=request.delta;const auto& position=request.position;const f32 bound=request.arena_half-request.radius;
    const f32 requested_x=position[0]+delta.x,requested_z=position[2]+delta.z;
    const f32 clamped_x=std::clamp(requested_x,-bound,bound),clamped_z=std::clamp(requested_z,-bound,bound);
    delta.x=clamped_x-position[0];delta.z=clamped_z-position[2];
    struct Candidate {BattlePoint delta{};f32 y=0;bool blocked=false,wall=false,peer=false;};
    auto probe=[&](BattlePoint movement,Candidate* candidate){
        *candidate={};
        f32 wall=1,peer=1;
        if(!native_borg_partition_portion(terrain,{position[0],position[2]},movement,request.radius,&wall))return false;
        const f32 end_y=request.vertical_end_y.value_or(position[1]);
        for(const auto& body:bodies){
            if(std::min(position[1],end_y)>=body[1]+request.height||
               std::max(position[1],end_y)<=body[1]-request.height)continue;
            const f32 rx=position[0]-body[0],rz=position[2]-body[2];
            f32 t=0;
            if(rx*movement.x+rz*movement.z<0&&
               battle_swept_hit({position[0],position[2]},movement,{body[0],body[2]},2*request.radius,&t))
                peer=std::min(peer,std::max(0.f,t-1e-4f));
        }
        candidate->wall=wall<1&&wall<=peer;candidate->peer=peer<1&&peer<wall;
        const f32 portion=std::min(wall,peer);
        candidate->delta={movement.x*portion,movement.z*portion};
        if(terrain&&!terrain->height(position[0]+candidate->delta.x,position[2]+candidate->delta.z,position[1],&candidate->y))return false;
        if(!std::isfinite(candidate->y)||std::abs(candidate->y)>20000)return false;
        candidate->blocked=candidate->y>position[1]+(request.grounded?request.step:0);return true;
    };
    Candidate selected;if(!probe(delta,&selected))return false;
    const bool blocked=selected.blocked||selected.wall;
    bool slid=false;
    if(blocked&&delta.x!=0&&delta.z!=0){
        f32 best=selected.blocked?0:selected.delta.x*selected.delta.x+selected.delta.z*selected.delta.z;
        for(const auto alternative:{BattlePoint{delta.x,0},BattlePoint{0,delta.z}}){
            Candidate candidate;if(!probe(alternative,&candidate))return false;
            const f32 square=candidate.delta.x*candidate.delta.x+candidate.delta.z*candidate.delta.z;
            if(!candidate.blocked&&!candidate.wall&&square>best){selected=candidate;best=square;slid=true;}
        }
    }
    NativeBorgContactResult result;result.position=position;result.grounded=request.grounded;
    result.terrain_blocked=blocked;result.body_hit=selected.peer;result.bounds_hit=clamped_x!=requested_x||clamped_z!=requested_z;result.slid=slid;
    if(!selected.blocked){
        result.position[0]+=selected.delta.x;result.position[2]+=selected.delta.z;
        if(result.grounded){if(result.position[1]-selected.y>request.step)result.grounded=false;else result.position[1]=selected.y;}
    }
    result.actual_delta={result.position[0]-position[0],result.position[2]-position[2]};*out=result;return true;
}
inline bool native_borg_resolve_contact(const NativeBorgContactRequest& request,const IBattleMvpTerrain* terrain,NativeBorgContactResult* out){
    const std::span<const std::array<f32,3>> bodies=request.other_active?
        std::span<const std::array<f32,3>>(&request.other_position,1):std::span<const std::array<f32,3>>{};
    return native_borg_resolve_contact(request,bodies,terrain,out);
}
}
