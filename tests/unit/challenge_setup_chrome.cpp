#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/challenge_setup_chrome.hpp"
namespace {
unsigned checks=0;void check(bool value,const char* why){++checks;if(!value){std::fprintf(stderr,"Challenge chrome: %s\n",why);std::abort();}}
gf::ChallengeDifficultyArt art(){
    using namespace gf;ChallengeDifficultyArt a;a.bank_hash=1;a.camera.eye[2]=10;a.camera.target[2]=0;a.aspect=4.f/3;a.limits={1,2,3};
    a.operands={0,3,1,2,-1,.1f,120,9,.15f,-.15f,-96,-297,-120,.4f,7,.5f,176,0,63,40,106,-40,-350,160,364,91,0,11};
    for(unsigned i=0;i<4;++i)a.frames[i]={u16(1u<<i),u16(10+i),{0,0,0}};
    for(unsigned i=0;i<10;++i)a.footers[i]={u16(1u<<(i%4)),f32(i),{s16(100+i),-1,-1,-1}};
    return a;
}
}
int main(){
    using namespace gf;const auto a=art();check(a.valid(),"owned art fixture");ChallengeSetupChrome legacy,full;
    check(legacy.open(a)&&full.open(a,true),"legacy and explicit full-mask domains");
    check(legacy.updates()==0&&legacy.background_time()==1&&full.background_time()==1,"one constructor callback, not first caller update");
    const auto initial=full.draws();check(initial.size()==12&&initial[0].record==98&&initial[1].record==13,"constructor background/frame/footer order");
    for(unsigned mask:{8u,2u,1u,4u,0u,8u}){
        for(unsigned i=0;i<32;++i){const auto prior=full.background_time();check(full.step(u8(mask),u8(mask)),"page masks advance same chrome");check(full.background_time()==(prior==119?0:prior+1),"continuous shared background, no page reset");}
        const f32 expected=mask==8?0:mask==2?1:mask==1?2:3;
        check(std::abs(full.background_palette()-expected)<1e-6f,"source visible priority/retention palette target");
    }
    check(full.updates()==192,"same owner survives all page switches");
    const auto snapshot=full.draws();const auto clock=full.background_time();for(unsigned i=0;i<4;++i)check(full.draws().size()==snapshot.size()&&full.background_time()==clock,"reads/draw plans do not tick");
    check(!legacy.step(2,2)&&!legacy.ready()&&legacy.draws().empty(),"old entry still refuses player masks and clears stale draws");
    check(!full.step(16,0)&&!full.ready()&&full.draws().empty(),"unknown masks latch refusal");check(!full.step(8,8),"failed owner cannot replay an old page");
    check(full.open(a,true),"explicit restart only");
    for(unsigned i=0;i<8192;++i)check(full.step(0,0),"bounded numeric callback property");
    check(!full.step(0,0)&&!full.ready()&&full.draws().empty(),"8192 work budget refuses, never forces next page");
    auto invalid=a;invalid.bank_hash=0;check(!full.open(invalid,true)&&!full.ready()&&full.draws().empty(),"failed replacement clears owner");
    std::printf("{\"challenge_chrome_owned_checks\":%u,\"numeric_budget_callbacks\":8192,\"all_passed\":true}\n",checks);
}
