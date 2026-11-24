#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/challenge_character_resources.hpp"
#include "tests/unit/battle_character_resource_fixture.hpp"
#include "tests/unit/challenge_ai_force_fixture.hpp"
namespace {unsigned checks=0;void check(bool value,const char* why){++checks;if(!value){std::fprintf(stderr,"battle character resources: %s\n",why);std::abort();}}
struct NoFiles final:gf::IFileSystem {unsigned calls=0;bool read(const std::string&,std::vector<gf::u8>&)override{++calls;return false;}bool file_size(const std::string&,gf::u64*)override{++calls;return false;}bool write(const std::string&,const gf::u8*,std::size_t)override{++calls;return false;}bool exists(const std::string&)override{++calls;return false;}bool erase(const std::string&)override{++calls;return false;}};}
int main(){
    using namespace gf;using namespace battle_character_resource_fixture;const auto catalog=challenge_ai_force_fixture::catalog(),foreign=challenge_ai_force_fixture::catalog(false,true);auto bytes=bundle();auto spec=row(bytes);auto pb=profile(spec);BattleCharacterResourceProfile p;std::string error;
    check(p.load({pb.data(),pb.size()},catalog,&error)&&p.for_catalog(catalog)&&!p.for_catalog(foreign)&&p.find(8)&&!p.find(9),"catalog-bound explicit subset, no default model substitution");
    for(unsigned at:{0u,4u,9u,11u,15u,16u,31u,33u,35u,48u,50u,51u,53u,54u}){auto bad=pb;bad[at]^=1;check(!p.load({bad.data(),bad.size()},catalog,&error)&&!p.for_catalog(catalog)&&!p.find(8),"malformed profile clears prior bindings");}
    check(!p.load({nullptr,pb.size()},catalog,&error)&&!p.load({pb.data(),pb.size()-1},catalog,&error),"null/short profile");check(p.load({pb.data(),pb.size()},catalog,&error),"profile reload");
    BattleCharacterResource resource;check(!resource.ready()&&!resource.actor_data().data&&!resource.model_hsd().data,"unloaded resources expose no old views");
    for(unsigned variant=0;variant<6;++variant){check(resource.load(spec,u8(variant),{bytes.data(),bytes.size()},&error)&&resource.ready()&&resource.variant()==variant,"all six declared byte selections, not six admitted actor classes");check(resource.actor_data().size==432&&resource.actor_data().be32(0)==0x12345678&&resource.battle_data().size==8&&resource.motion_bank().size==64&&resource.model_wrapper().size==52&&resource.model_prefix().size==16&&resource.model_hsd().size==32&&resource.model_hsd().be32(4)==variant+1,"exact source role extents and wrapper-declared HSD; padding retained");}
    check(resource.block(0).size==432&&resource.block(0).data==resource.actor_data().data&&!resource.block(1).data&&resource.block(2).size==8&&resource.block(3).size==64&&resource.block(9).data==resource.model_wrapper().data&&!resource.block(10).data&&!resource.block(32).data,"checked complete block views preserve zero slots and selected wrapper identity");
    auto owned_copy=resource;bytes[96]=0;check(resource.actor_data().be32(0)==0x12345678&&owned_copy.actor_data().be32(0)==0x12345678,"input lifetime/copies share immutable owned bytes");bytes=bundle();
    check(!resource.load(spec,6,{bytes.data(),bytes.size()},&error)&&!resource.ready()&&!resource.actor_data().data,"invalid variant invalidates old selection");
    auto bad=bytes;bad.back()^=1;check(!resource.load(spec,0,{bad.data(),bad.size()},&error)&&!resource.ready(),"wrong hash refuses, no stale model");
    for(unsigned offset:{0u,4u,8u,12u,16u,20u}){bad=bytes;bad[offset]^=1;auto declared=row(bad);check(!resource.load(declared,0,{bad.data(),bad.size()},&error),"bundle structure independently of hash");}
    const unsigned wrapper=96+432+8+64;
    for(u32 offset:{0u,3u,12u,50u,0xfffffffcu}){bad=bytes;put(bad,wrapper,offset);const auto declared=row(bad);check(!resource.load(declared,0,{bad.data(),bad.size()},&error)&&!resource.model_wrapper().data,"model first-word offset cannot be scanned or guessed");}
    for(u32 size:{0u,31u,64u,0xffffffffu}){bad=bytes;put(bad,wrapper+16,size);const auto declared=row(bad);check(!resource.load(declared,0,{bad.data(),bad.size()},&error),"declared HSD extent cannot consume padding/outside bytes");}
    check(!resource.load(spec,0,{nullptr,bytes.size()},&error)&&!resource.load(spec,0,{bytes.data(),bytes.size()-1},&error),"null/short resources");
    NoFiles files;ChallengeInitialPartyCandidate fake;std::array<const ForceInventory*,2> owners{};std::array<u8,6> selected;selected.fill(255);ChallengeCharacterResources request;ChallengeCharacterDescriptorReceipt empty;
    check(!empty.valid(owners)&&!empty.data(owners)&&!request.receipt(0).valid(owners),"no forged descriptor receipt from public fields");check(!request.begin(fake,owners,selected,p,files,"owned",&error)&&request.status()==ChallengeCharacterResourceStatus::Failed&&files.calls==0,"fake party refuses before I/O");check(!request.begin(fake,owners,selected,p,files,"owned",&error)&&files.calls==0,"failed request cannot retry");request.retire();check(request.status()==ChallengeCharacterResourceStatus::Retired&&!request.receipt(0).valid(owners),"retirement has no constructor acknowledgement");
    ChallengeCharacterResources queued;check(!queued.begin_queued(fake,owners,selected,p,{},"owned",&error)&&!queued.advance_queued(files,owners,&error)&&!queued.queue_state(owners)&&!queued.successful_drain(owners)&&files.calls==0,"default/fabricated entry cannot enqueue, perform work or report a drain");
    std::printf("battle character resources: %u owned checks PASS\n",checks);
}
