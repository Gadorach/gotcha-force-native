#pragma once

#include <limits>
#include "engine/gameplay/native_borg_contact.hpp"

namespace gf {

inline bool battle_partition_route_clear(const IBattleMvpTerrain& terrain,BattlePoint from,BattlePoint to,f32 radius){
    f32 portion=0;
    return native_borg_partition_portion(&terrain,from,{to.x-from.x,to.z-from.z},radius,&portion)&&portion==1;
}

struct BattlePartitionRoute {
    static constexpr unsigned capacity=4*kStageBodyPartitionCount+2;
    std::array<BattlePoint,capacity> points{}; // excluding the start, including the goal
    unsigned count=0;
    bool operator==(const BattlePartitionRoute&)const=default;
};

inline f64 battle_partition_route_length(BattlePoint from,const BattlePartitionRoute& route,unsigned first=0){
    if(first>=route.count||route.count>route.points.size())return std::numeric_limits<f64>::infinity();
    f64 length=0;
    for(unsigned i=first;i<route.count;++i){
        const auto to=route.points[i];
        length+=std::hypot(f64(to.x)-from.x,f64(to.z)-from.z);
        from=to;
    }
    return length;
}

// Bounded visibility graph of the authored rectangles, not a terrain or
// body navmesh. Each segment uses the same sweep as native body motion.
inline BattlePartitionRoute battle_partition_route(const IBattleMvpTerrain& terrain,BattlePoint start,BattlePoint goal,
                                                    f32 arena_half,f32 radius){
    const auto boxes=terrain.body_partitions();
    if(boxes.size()>kStageBodyPartitionCount||!std::isfinite(arena_half)||!std::isfinite(radius)||
       radius<=0||arena_half<=radius)return {};
    const f32 bound=arena_half-radius;
    const auto in_arena=[&](BattlePoint p){return std::abs(p.x)<=bound&&std::abs(p.z)<=bound;};
    const auto clear=[&](BattlePoint a,BattlePoint b){return battle_partition_route_clear(terrain,a,b,radius);};
    if(!in_arena(start)||!in_arena(goal)||!clear(start,start)||!clear(goal,goal))return {};
    constexpr unsigned capacity=BattlePartitionRoute::capacity;
    std::array<BattlePoint,capacity> nodes{start,goal};
    unsigned count=2;
    const f32 pad=radius+16; // leave room for braking and turning at each corner
    for(const auto& box:boxes)for(unsigned corner=0;corner<4;++corner){
        const BattlePoint point{corner&1?box.max_x+pad:box.min_x-pad,corner&2?box.max_z+pad:box.min_z-pad};
        if(in_arena(point)&&clear(point,point))nodes[count++]=point;
    }
    std::array<f64,capacity> distance;
    distance.fill(std::numeric_limits<f64>::infinity());distance[0]=0;
    std::array<unsigned,capacity> parent{};
    std::array<bool,capacity> visited{};
    for(unsigned n=0;n<count;++n){
        unsigned best=capacity;
        f64 d=std::numeric_limits<f64>::infinity();
        for(unsigned i=0;i<count;++i)if(!visited[i]&&distance[i]<d){best=i;d=distance[i];}
        if(best==capacity)return {};
        if(best==1){
            BattlePartitionRoute route;
            for(unsigned i=1;i;i=parent[i])route.points[route.count++]=nodes[i];
            std::reverse(route.points.begin(),route.points.begin()+route.count);
            return route;
        }
        visited[best]=true;
        for(unsigned i=1;i<count;++i)if(!visited[i]){
            const auto delta=BattlePoint{nodes[i].x-nodes[best].x,nodes[i].z-nodes[best].z};
            const f64 total=d+std::hypot(f64(delta.x),f64(delta.z));
            if(total<distance[i]&&clear(nodes[best],nodes[i])){
                parent[i]=best;distance[i]=total;
            }
        }
    }
    return {};
}

} // namespace gf
