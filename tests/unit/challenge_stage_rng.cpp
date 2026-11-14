#include <cstdio>
#include <cstdlib>
#include <vector>
#include "engine/gameplay/challenge_stage_rng.hpp"
namespace {unsigned checks=0;void check(bool value,const char* why){++checks;if(!value){std::fprintf(stderr,"stage RNG: %s\n",why);std::abort();}}
gf::ChallengeStageRngProfile owned(){gf::ChallengeStageRngProfile p;for(unsigned i=0;i<p.rows.size();++i)p.rows[i]={gf::u8(i),4};return p;}
}
int main(){
    using namespace gf;auto p=owned();ChallengeStageRngPlan out;const SourceByteRngState input{195,1};
    check(p.valid()&&challenge_stage_rng_plan(p,input,-1,&out),"explicit owned profile/state");
    check(out.stage==2&&out.field_1d==1&&out.field_1e==2&&out.draws==3&&out.next==SourceByteRngState{14,53},"three ordered draws and new-low returns");
    check(input==SourceByteRngState{195,1},"input not mutated");
    check(challenge_stage_rng_plan(p,input,2,&out)&&out.stage==8&&out.field_1d==2&&out.field_1e==1&&out.draws==4&&out.next==SourceByteRngState{173,159},"previous stage rerolls and retains consumption");
    check(!challenge_stage_rng_plan(p,{0,0},0,&out)&&out==ChallengeStageRngPlan{},"stuck source stream refuses with empty output");
    check(challenge_stage_rng_plan(p,{0,0},-1,&out)&&out.stage==0&&out.draws==3&&out.next==SourceByteRngState{},"zero seed not silently reseeded");
    check(!challenge_stage_rng_plan(p,input,-1,nullptr),"null output");
    auto bad=p;bad.rows[1].stage=bad.rows[0].stage;check(!challenge_stage_rng_plan(bad,input,-1,&out)&&out==ChallengeStageRngPlan{},"duplicate stage rows refuse");
    bad=p;bad.rows[0].stage=128;check(!bad.valid(),"signed source stage domain");
    for(u16 count:{u16(0),u16(257)}){bad=p;bad.rows[0].field_1e_count=count;check(!challenge_stage_rng_plan(bad,input,-1,&out)&&out==ChallengeStageRngPlan{},"invalid resource modulo divisor");}
    p.rows[0].field_1e_count=256;check(challenge_stage_rng_plan(p,{0,0},-1,&out)&&out.field_1e==0,"byte-sized divisor upper bound");
    std::vector<u8> b(64);auto put16=[&](unsigned at,u16 x){b[at]=u8(x>>8);b[at+1]=u8(x);};auto put32=[&](unsigned at,u32 x){put16(at,u16(x>>16));put16(at+2,u16(x));};
    put32(0,0x47464347);put32(4,1);put16(8,11);put16(10,3);for(unsigned i=0;i<11;++i){b[12+4*i]=u8(i);put16(14+4*i,4);}
    ChallengeStageRngProfile loaded;std::string error;check(challenge_stage_rng_load({b.data(),b.size()},&loaded,&error)&&loaded.valid()&&error.empty(),"owned binary profile");
    for(unsigned at:{0u,4u,9u,11u,13u,56u,63u}){auto copy=b;copy[at]^=1;check(!challenge_stage_rng_load({copy.data(),copy.size()},&loaded,&error)&&!loaded.valid(),"profile guards clear output");}
    check(!challenge_stage_rng_load({b.data(),b.size()-1},&loaded,&error)&&!challenge_stage_rng_load({nullptr,64},&loaded,&error),"short/null bytes");
    check(!challenge_stage_rng_load({b.data(),b.size()},nullptr,&error),"null profile output");
    p=owned();for(int previous=-128;previous<128;++previous){check(challenge_stage_rng_plan(p,input,s8(previous),&out)&&out.stage!=previous&&out.field_1d<3&&out.field_1e<4,"all previous signed byte values");}
    std::printf("stage RNG: %u owned checks PASS\n",checks);
}
