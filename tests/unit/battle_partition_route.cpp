#include <cstdio>
#include <cstdlib>
#include <vector>
#include "engine/gameplay/battle_partition_route.hpp"

namespace {
using namespace gf;

void check(bool result,const char* message){
    if(!result){std::fprintf(stderr,"partition route: %s\n",message);std::exit(1);}
}

struct FlatStage : IBattleMvpTerrain {
    std::vector<StageBodyPartition> boxes;
    std::span<const StageBodyPartition> body_partitions()const override{return boxes;}
    bool height(f32,f32,f32,f32* out)const override{if(!out)return false;*out=0;return true;}
};

void clear_edges(const FlatStage& stage,BattlePoint start,BattlePoint goal,const BattlePartitionRoute& route){
    check(route.count&&route.points[route.count-1]==goal,"route ends at requested goal");
    for(unsigned i=0;i<route.count;++i){
        const auto step=route.points[i];
        check(battle_partition_route_clear(stage,start,step,42),"each waypoint edge uses the native partition sweep");
        start=step;
    }
}
}

int main(){
    FlatStage stage;
    const BattlePoint start{0,-180},goal{0,180};
    const auto direct=battle_partition_route(stage,start,goal,900,42);
    check(direct.count==1&&direct.points[0]==goal,"open stage needs no detour");
    stage.boxes={{"wall",-350,-6,350,6}};
    check(!battle_partition_route_clear(stage,start,goal,42),"centerline through partition is blocked");
    const auto around=battle_partition_route(stage,start,goal,900,42);
    clear_edges(stage,start,goal,around);
    check(around.count==3&&std::abs(around.points[0].x)>392&&around.points[0].x==around.points[1].x,
          "route walks around one end, not through a corner");
    check(around==battle_partition_route(stage,start,goal,900,42),"equal-length choices are repeatable");
    stage.boxes={{"sealed",-900,-6,900,6}};
    check(!battle_partition_route(stage,start,goal,900,42).count,"no detour beyond the arena boundary");
    check(!battle_partition_route(stage,{0,0},goal,900,42).count,"blocked start does not get a route");

    stage.boxes={{"lower",-600,-160,250,-144},{"upper",-250,144,600,160}};
    const BattlePoint lower{0,-400},upper{0,400};
    const auto zigzag=battle_partition_route(stage,lower,upper,600,42);
    clear_edges(stage,lower,upper,zigzag);
    bool right=false,left=false;
    for(unsigned i=0;i<zigzag.count;++i){
        right|=zigzag.points[i].x>292;
        left|=zigzag.points[i].x<-292;
    }
    check(right&&left,"multiple partitions force legal detours on both sides");
    stage.boxes.resize(kStageBodyPartitionCount+1);
    check(!battle_partition_route(stage,lower,upper,600,42).count,"visibility graph refuses excess geometry");
}
