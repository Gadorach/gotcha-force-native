#include <cstdio>
#include <cstdlib>
#include "engine/assets/battle_actor_initial_target.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"initial target profile: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;std::string error;std::vector<u8> raw(4688);auto word=[&](unsigned at,u32 value){store_be32(raw.data()+at,value);};
    StartupActorSetupProfile setup;setup.loaded=true;setup.model_id=10;setup.model_hash=123;
    word(0,0x47464e54);word(4,1);word(8,raw.size());word(12,setup.model_hash);word(16,challenge_payload_hash(startup_actor_setup_detail::config(setup)));word(20,0x802d3ee0);store_be16(raw.data()+24,10);store_be16(raw.data()+26,6);word(48,0x4dbebc20);word(52,6);
    store_be16(raw.data()+64+14,0xfffe);store_be16(raw.data()+80+2*((17*16+15)*8+7),0xfedc);
    BattleActorInitialTargetProfile p;auto view=[&](){return ByteView{raw.data(),raw.size()};};
    check(p.load(view(),&error)&&p.matches(setup,0x802d3ee0),"identity-bound class slice");
    u16 value=0;check(p.delay(255)==0xfffe&&p.suffix(17,15,255,&value)&&value==0xfedc,"last bounded selector/level row and draw masking");
    check(!p.suffix(18,15,0,&value)&&!p.suffix(17,16,0,&value)&&!p.suffix(0,0,0,nullptr),"table bounds and output required");
    check(battle_target_query_profile_valid(p.query()),"existing target query operands");
    ++setup.class_config[0];check(!p.matches(setup,0x802d3ee0),"equal model alone cannot substitute changed class config");--setup.class_config[0];
    check(!p.matches(setup,0x80365ac8),"other source config key refused");
    for(unsigned at:{0u,4u,8u,20u,24u,26u,28u,33u,34u,47u,48u,52u,56u,60u}){
        raw[at]^=1;check(!p.load(view(),&error)&&!p.matches(setup,0x802d3ee0)&&!p.suffix(0,0,0,&value),"bad format/policy clears previously loaded profile");raw[at]^=1;
    }
    check(!p.load({raw.data(),raw.size()-1},&error)&&!p.load({nullptr,raw.size()},&error),"truncated/null input refuses");
    std::printf("battle initial target profile: %u checks PASS\n",checks);
}
