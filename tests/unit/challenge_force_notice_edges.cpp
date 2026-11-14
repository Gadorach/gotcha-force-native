#include <cstdio>
#include <cstdlib>
#include <vector>
#include "engine/gameplay/challenge_force_notices.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"force notice edges: %s\n",why);std::abort();}}
std::vector<gf::u8> owned(){
    using namespace gf;std::vector<u8> b(88);store_be32(b.data(),0x47464e54);store_be32(b.data()+4,1);store_be32(b.data()+8,1);store_be16(b.data()+12,4);store_be16(b.data()+14,31);
    for(unsigned i=0;i<3;++i){const auto at=28+i*16;store_be16(b.data()+at,i?0xffff:5);store_be32(b.data()+at+4,f32_to_bits(f32(i)));}
    store_be32(b.data()+76,f32_to_bits(.25f));store_be16(b.data()+80,2);store_be16(b.data()+82,32);store_be16(b.data()+84,10);return b;
}}
int main(){
    using namespace gf;auto b=owned();ChallengeForceNoticesArt a;std::string error;check(a.load({b.data(),b.size()},&error)&&a.valid(),"owned typed profile");
    for(unsigned n=0;n<88;++n)check(!a.load({b.data(),n},&error)&&!a.valid(),"short input clears output");
    for(unsigned at:{0u,4u,8u}){auto bad=b;store_be32(bad.data()+at,0);check(!a.load({bad.data(),bad.size()},&error),"header identity guard");}
    for(unsigned at:{16u,20u,24u,32u,36u,40u,48u,52u,56u,64u,68u,72u,76u}){auto bad=b;store_be32(bad.data()+at,0x7fc00000);check(!a.load({bad.data(),bad.size()},&error),"NaN operand refuses");}
    for(unsigned at:{30u,46u,62u,86u}){auto bad=b;store_be16(bad.data()+at,1);check(!a.load({bad.data(),bad.size()},&error),"reserved word refuses");}
    check(a.load({b.data(),b.size()},&error),"fresh profile");ChallengeForceNotices notice;check(notice.open(a)&&notice.step({0,0,0,true,2,{-1,-1}})&&notice.draws().size()==1,"unassigned ports do not forge controller icons");
    check(notice.step({0,0,0,true,1,{0,3}})&&notice.level()==1&&notice.team_tick()==0,"source context2-to1 clamps stored level, no invented fade restart");
    check(notice.step({0,0,0,false,2,{0,3}})&&notice.draws().empty()&&notice.level()==0,"TEAM UP predicate independent of box index");
    for(const auto in:{ChallengeForceNoticesInput{16,0,0,false,0,{0,1}},ChallengeForceNoticesInput{0,0,0,true,3,{0,1}},ChallengeForceNoticesInput{0,0,0,true,1,{-2,1}},ChallengeForceNoticesInput{0,0,0,true,1,{0,4}}})check(notice.open(a)&&!notice.step(in)&&!notice.ready()&&notice.draws().empty(),"invalid input invalidates old packets");
    a.port_record_base=4093;check(!a.valid()&&!notice.open(a),"port record arithmetic remains bounded");
    std::printf("force notice edges: %u owned checks PASS\n",checks);
}
