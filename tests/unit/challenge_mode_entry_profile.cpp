#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include "tests/unit/challenge_mode_entry_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok){++checks;if(!ok)std::abort();}}
int main(){
    using namespace gf;std::string error;const auto bytes=challenge_mode_entry_fixture::bytes();ChallengePreparationEntryProfile profile;
    check(profile.load({bytes.data(),bytes.size()},&error)&&profile.normal_dispatch()&&profile.challenge_mode_lineage());
    for(unsigned at=0;at<bytes.size();++at){auto bad=bytes;bad[at]^=1;check(!profile.load({bad.data(),bad.size()},&error)&&!profile.ready()&&!profile.normal_dispatch()&&!profile.challenge_mode_lineage());}
    for(unsigned n=0;n<bytes.size();++n)check(!profile.load({bytes.data(),n},&error));
    for(unsigned version=1;version<=2;++version){
        std::array<u8,64> legacy{};std::copy_n(bytes.begin(),64,legacy.begin());store_be32(legacy.data()+4,version);store_be32(legacy.data()+8,64);
        if(version==1)std::fill(legacy.begin()+48,legacy.end(),0);
        check(profile.load({legacy.data(),legacy.size()},&error)&&profile.ready()&&profile.normal_dispatch()==(version==2)&&!profile.challenge_mode_lineage());
    }
    std::printf("fresh Challenge mode-lineage V3/legacy refusal profile: %u checks PASS\n",checks);
}
