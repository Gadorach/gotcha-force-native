#include <cstdio>
#include <cstdlib>
#include "tests/unit/battle_character_resource_fixture.hpp"
#include "engine/assets/character_motion.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"character resource absence: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;using namespace battle_character_resource_fixture;auto bytes=bundle();const unsigned begin=96+432+8;bytes.erase(bytes.begin()+begin,bytes.begin()+begin+64);put(bytes,12,u32(bytes.size()-16));put(bytes,44,0); //slot3 length at20+8*3
    const auto spec=row(bytes);std::string error;BattleCharacterResource resource;
    for(unsigned variant=0;variant<6;++variant)check(resource.load(spec,u8(variant),{bytes.data(),bytes.size()},&error)&&resource.ready()&&!resource.motion_present()&&resource.motion_bank().size==0&&resource.model_hsd().be32(4)==variant+1,"absent source motion preserves independently selected model bytes");
    CharacterMotionBank bank;check(!character_motion_bank_load(resource.motion_bank(),&bank,&error)&&!bank.loaded,"OLD motion loader still refuses absence; no fallback/program/actor grant");
    auto nonempty=bundle();auto r=row(nonempty);check(resource.load(r,0,{nonempty.data(),nonempty.size()},&error)&&resource.motion_present(),"nonempty source slot remains distinct");
    auto short_slot=bytes;short_slot.insert(short_slot.begin()+begin,4,0);put(short_slot,12,u32(short_slot.size()-16));put(short_slot,44,4);r=row(short_slot);check(!resource.load(r,0,{short_slot.data(),short_slot.size()},&error)&&!resource.ready()&&!resource.motion_present(),"nonempty short motion header still refuses, clears prior resource");
    std::printf("character resource absence: %u new owned checks PASS\n",checks);
}
