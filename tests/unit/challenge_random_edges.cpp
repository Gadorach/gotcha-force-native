#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/challenge_random_force.hpp"
#include "tests/unit/force_inventory_fixture.hpp"
namespace {
unsigned checks=0;void check(bool v,const char* why){++checks;if(!v){std::fprintf(stderr,"Random edges: %s\n",why);std::abort();}}
gf::ChallengeForceRequest random_request(gf::ForceInventory& owner,gf::ChallengeForceSelection& page,gf::s32 energy){using namespace gf;ChallengeForceEvent event;std::array<ChallengeSetupPad,4> pads{};check(page.open({1,1},owner,0,0,1,false,energy,{}),"page");for(unsigned i=0;i<3;++i){check(page.step(owner,pads,&event),"open gate");}pads[0]={1,true};check(page.step(owner,pads,&event),"left Random");pads[0]={0x100,true};check(page.step(owner,pads,&event)&&page.step(owner,pads,&event),"confirm close");ChallengeForceRequest request;check(page.take_random_request(owner,&request),"scope");return request;}
}
int main(){
    using namespace gf;const auto base=force_inventory_fixture::catalog();std::vector<ForceBorgMetadata> entries(base.entries().begin(),base.entries().end());entries[2].cost.fill(0);ForceCatalog catalog;std::string error;check(catalog.assign(entries,{0x1001,0x1002,0x1003},{{0x1001,0x1002,0x1003}},base.starter_name(),base.unnamed_name(),base.owner_name(),base.letter_byte(),&error),"owned zero-cost metadata");
    ChallengeRandomForceProfile profile;profile.catalog_hash=catalog.fingerprint();profile.preferred_model=profile.default_model=0x1001;profile.special_model=0x1002;profile.name[0]='R';
    auto state=std::make_unique<ForceInventoryState>();for(unsigned i=0;i<40;++i)state->box[i].value=force_inventory_fixture::borg(i?2:0);state->box_count=40;ForceInventory owner;check(owner.load_state(catalog,*state),"owned40 box entries");ChallengeForceSelection page;auto token=random_request(owner,page,100);SourceByteRngState seed{0,0},next{71,29};ChallengeRandomAssembly exact;ChallengeRandomRoster roster;
    check(exact.begin(token,owner,profile,seed,&error)&&exact.trace().size()==30&&exact.take(owner,&roster,&next)&&roster.members().size()==30&&roster.source_spent()==100&&roster.nominal_cost()==100&&!roster.fallback(),"at exact energy, zero-cost entries still fill source cap30; not stored-prefix early stop");
    for(unsigned i=0;i<30;++i){check(roster.members()[i].source_slot==i&&exact.trace()[i].candidates==40-i,"all30 picks without replacement/in increasing zero-byte order");}
    auto bytes=std::vector<u8>(80);store_be32(bytes.data(),0x47465246);store_be32(bytes.data()+4,1);store_be32(bytes.data()+8,profile.catalog_hash);store_be16(bytes.data()+12,profile.preferred_model);store_be16(bytes.data()+14,profile.special_model);store_be16(bytes.data()+16,profile.default_model);store_be16(bytes.data()+18,30);store_be16(bytes.data()+20,200);store_be16(bytes.data()+22,2000);bytes[24]='R';ChallengeRandomForceProfile loaded;
    check(challenge_random_force_load({bytes.data(),bytes.size()},catalog,&loaded,&error)&&loaded.for_catalog(catalog),"owned profile load");
    for(unsigned at:{0u,4u,8u,12u,14u,16u,18u,20u,22u,65u,79u}){auto bad=bytes;bad[at]^=0x80;check(!challenge_random_force_load({bad.data(),bad.size()},catalog,&loaded,&error)&&!loaded.for_catalog(catalog),"malformed profile clears output");}
    check(!challenge_random_force_load({},catalog,&loaded,&error)&&!challenge_random_force_load({bytes.data(),79},catalog,&loaded,&error),"null/truncated profile");
    ForceInventory neighbor;check(neighbor.create(catalog),"neighbor");token=random_request(owner,page,100);ChallengeRandomAssembly foreign;check(foreign.begin(token,owner,profile,seed,&error),"current assembly");next={71,29};check(!foreign.take(neighbor,&roster,&next)&&!roster.current(owner)&&next==SourceByteRngState{71,29},"other inventory cannot consume or rewind RNG output");
    token=random_request(owner,page,100);ChallengeRandomAssembly reloaded;check(reloaded.begin(token,owner,profile,seed,&error)&&owner.load_state(catalog,*state)&&!reloaded.take(owner,&roster,&next),"reload identity invalidates even when revision resets to same number");
    ChallengeForceRequest expired;{ChallengeForceSelection temporary;expired=random_request(owner,temporary,100);}ChallengeRandomAssembly late;check(!expired.current(owner)&&!late.begin(expired,owner,profile,seed,&error),"expired page request cannot generate later");
    std::printf("{\"challenge_random_extra_edge_checks\":%u,\"all_passed\":true,\"zero_cost_fixture_not_original_catalog_data\":true}\n",checks);
}
