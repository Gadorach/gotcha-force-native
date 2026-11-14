#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <limits>
#include "tests/unit/stage_ground_fixture.hpp"
namespace {
unsigned checks=0;void check(bool v,const char* why){++checks;if(!v){std::fprintf(stderr,"stage ground: %s\n",why);std::abort();}}
std::vector<gf::u8> read(const std::string& p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
}
int main(){
    using namespace gf;using namespace stage_ground_fixture;std::string error;
    auto raw=bytes({triangle(0),triangle(100),triangle(200)}),empty=bytes({}),top=bytes({triangle(150,true)});
    StihGrid floor,wall,ceiling;check(floor.load({raw.data(),raw.size()},&error)&&wall.load({empty.data(),empty.size()},&error)&&ceiling.load({top.data(),top.size()},&error),"owned shared-list grids");
    check(floor.triangles().size()==3&&floor.cell(2,2).size()==3&&floor.triangles()[2].plane[3]==-200,"56-byte records retain fourth coefficient and tag");
    const auto p=parameters();StageGroundActor a;a.fields_declared=true;f32 y=0;const std::array<const StihGrid*,3> grids={&floor,&wall,&ceiling};
    check(stage_ground_height(grids,p,a,{0,0,0},180,&y,&error)&&y==100,"highest floor strictly below height and ceiling");
    check(stage_ground_height(grids,p,a,{0,0,0},100,&y,&error)&&y==0,"height equality excludes current surface");
    check(stage_ground_height(grids,p,a,{0,0,0},-1,&y,&error)&&y==0,"no candidate returns minimum surface, not fabricated low plane");
    check(stage_ground_height(grids,p,a,{0,0,0},5000,&y,&error)&&y==200,"raisedheight returns maximum even above ceiling");
    a.previous30=200;check(stage_ground_height(grids,p,a,{0,0,0},50,&y,&error)&&y==200,"positive previous-minus-current span increases height and selects correct ceiling");a.previous30=0;
    const std::array<const StihGrid*,3> none={nullptr,nullptr,nullptr};
    check(stage_ground_height(none,p,a,{-7500,37,0},1,&y,&error)&&y==37,"strict boundary returns suppliedY without any grid read");
    a.current24=std::numeric_limits<float>::quiet_NaN();check(stage_ground_height(none,p,a,{7500,37,0},1,&y,&error)&&y==37,"outside branch does not read irrelevant actor scalars");a.current24=0;
    check(!stage_ground_height(none,p,a,{0,37,0},1,&y,&error)&&y==0,"inside missing grid refuses, not flat ground fallback");
    a.attached1db=1;check(!stage_ground_height(grids,p,a,{-7500,37,0},1,&y,&error),"unported moving-platform branch refuses before bounds short circuit");a.attached1db=0;
    a.fields_declared=false;check(!stage_ground_height(grids,p,a,{0,0,0},1,&y,&error),"actor fields must be explicit");a.fields_declared=true;
    auto singular=triangle(0);singular.plane[1]=0;auto badfloor=bytes({singular});StihGrid guarded;check(guarded.load({badfloor.data(),badfloor.size()},&error),"zero-normal fixture retained as input not silently dropped");
    check(!stage_ground_height({&guarded,&wall,&wall},p,a,{0,0,0},1,&y,&error)&&y==0,"consumed zero normal refuses entire query");
    check(!stage_ground_height(grids,p,a,{2000,0,0},1,&y,&error),"in-source-bounds but outside declared grid refuses index");
    for(unsigned size=0;size<raw.size();++size){auto truncated=raw;truncated.resize(size);check(!guarded.load({truncated.data(),truncated.size()},&error)&&!guarded.loaded(),"all truncated extents refuse and clear old grid");}
    for(unsigned at:{0u,4u,8u,12u,16u,20u,32u,36u,40u}){auto bad=raw;store_be32(bad.data()+at,0xffffffff);check(!guarded.load({bad.data(),bad.size()},&error),"bad header/directory domains refuse");}
    for(unsigned at:{24u,28u}){auto valid=raw;store_be32(valid.data()+at,0xffffffff);check(guarded.load({valid.data(),valid.size()},&error),"signed minimum -1 is valid, not a null/sentinel header field");}
    const auto list=40+16*4;for(u32 ref:{1u,55u,168u,0xfffffffeu}){auto bad=raw;store_be32(bad.data()+list,ref);check(!guarded.load({bad.data(),bad.size()},&error),"triangle offset is a bounded byte offset, not index/pointer");}
    {auto bad=raw;store_be32(bad.data()+list+3*4,0);check(!guarded.load({bad.data(),bad.size()},&error),"missing cell sentinel refuses before triangle bytes");}
    {auto bad=raw;store_be32(bad.data()+list+16+4,0x7fc00000);check(!guarded.load({bad.data(),bad.size()},&error),"nonfinite triangle data refuses");}
    unsigned originals=0;std::array<StihGrid,3> source;
    for(unsigned i=0;i<3;++i){auto b=read("local/recomp-evidence/native-startup-landings-20261110/inputs-v2/stff-slot"+std::to_string(i+1)+".stih");if(b.empty())continue;check(source[i].load({b.data(),b.size()},&error),"recovered original grid loads completely");++originals;}
    check(!originals||originals==3,"all-or-none original coverage");unsigned sampled=0;
    if(originals){for(int x=-7000;x<=7000;x+=500)for(int z=-7000;z<=7000;z+=500){check(stage_ground_height({&source[0],&source[1],&source[2]},p,a,{float(x),1000,float(z)},80,&y,&error)&&std::isfinite(y),"actual grid query under declared static actor fields");++sampled;}}
    std::printf("{\"stage_ground_checks\":%u,\"original_grids\":%u,\"declared_original_queries\":%u,\"all_passed\":true}\n",checks,originals,sampled);
}
