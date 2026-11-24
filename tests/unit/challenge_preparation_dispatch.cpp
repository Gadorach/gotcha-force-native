#include <cstdio>
#include <cstdlib>
#include "tests/unit/challenge_preparation_entry_fixture.hpp"
namespace {unsigned checks=0;void check(bool b,const char* why){++checks;if(!b){std::fprintf(stderr,"preparation dispatch: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;auto raw=preparation_entry_fixture::bytes();std::string error;ChallengePreparationEntryProfile p;
    check(p.load({raw.data(),raw.size()},&error)&&!p.normal_dispatch(),"legacy profile preserves unknown enclosing mode");
    store_be32(raw.data()+4,2);store_be32(raw.data()+48,0x80017918);store_be32(raw.data()+52,0x80196948);store_be32(raw.data()+56,0x01030100);store_be32(raw.data()+60,0x802d2e48);
    check(p.load({raw.data(),raw.size()},&error)&&p.normal_dispatch(),"explicit normal dispatcher and return recipe");
    for(unsigned at=0;at<raw.size();++at){auto bad=raw;bad[at]^=1;check(!p.load({bad.data(),bad.size()},&error)&&!p.ready()&&!p.normal_dispatch(),"mutated source recipe never retains dispatch permission");}
    store_be32(raw.data()+4,1);check(!p.load({raw.data(),raw.size()},&error),"legacy version cannot smuggle enclosing dispatch");
    std::printf("challenge preparation dispatch: %u checks PASS\n",checks);
}
