#include <cstdio>
#include <cstdlib>
#include <type_traits>
#include "engine/gameplay/challenge_final_setup.hpp"
namespace {unsigned checks=0;void check(bool b,const char* why){++checks;if(!b){std::fprintf(stderr,"Challenge final: %s\n",why);std::abort();}}
struct Files final:gf::IFileSystem {unsigned calls=0;bool read(const std::string&,std::vector<gf::u8>&)override{++calls;return false;}bool file_size(const std::string&,gf::u64*)override{++calls;return false;}bool write(const std::string&,const gf::u8*,std::size_t)override{++calls;return false;}bool exists(const std::string&)override{++calls;return false;}bool erase(const std::string&)override{++calls;return false;}};
}
int main(){
    using namespace gf;static_assert(!std::is_copy_constructible_v<ChallengeFinalSetup>);std::vector<u8> bundle(36);auto p16=[](auto& b,unsigned at,u16 x){b[at]=u8(x>>8);b[at+1]=u8(x);};auto p32=[&](auto& b,unsigned at,u32 x){p16(b,at,u16(x>>16));p16(b,at+2,u16(x));};
    p32(bundle,0,0x47464e42);p32(bundle,4,1);p32(bundle,8,2);p32(bundle,12,20);p32(bundle,24,1);p32(bundle,28,4);bundle[32]=1;bundle[33]=2;bundle[34]=3;bundle[35]=4;
    check(challenge_native_bundle_valid({bundle.data(),bundle.size()}),"owned bundle retains explicit empty slot plus data");
    for(unsigned at:{0u,4u,11u,15u,16u,20u,24u,28u}){auto copy=bundle;copy[at]^=1;check(!challenge_native_bundle_valid({copy.data(),copy.size()}),"bundle header/index/extent guard");}
    auto corrupt=bundle;corrupt[35]^=1;check(challenge_native_bundle_valid({corrupt.data(),corrupt.size()})&&challenge_payload_hash({corrupt.data(),corrupt.size()})!=challenge_payload_hash({bundle.data(),bundle.size()}),"extent validity is distinct from payload identity");
    check(!challenge_native_bundle_valid({nullptr,36})&&!challenge_native_bundle_valid({bundle.data(),35}),"null/short bundle");
    const u8 a='a';check(challenge_payload_hash({&a,1})==0xe40c292c&&challenge_payload_hash({nullptr,0})==2166136261u,"hash consistency, not authentication");
    std::vector<u8> bytes(64);p32(bytes,0,0x47464342);p32(bytes,4,1);p16(bytes,8,1);p16(bytes,10,32);p32(bytes,12,32);p16(bytes,16,1);p16(bytes,18,3);bytes[32]=7;bytes[33]=2;bytes[34]=3;p32(bytes,36,5);p32(bytes,40,36);p32(bytes,44,challenge_payload_hash({bundle.data(),bundle.size()}));p32(bytes,48,0xffffffffu);
    ChallengeFinalizationProfile profile;std::string error;check(profile.load({bytes.data(),bytes.size()},&error)&&profile.ready()&&profile.wait()==1&&profile.cover()==3&&profile.find(7,2,3)&&!profile.find(7,2,4),"owned final profile rows and counters");
    for(unsigned at:{0u,4u,9u,11u,15u,17u,20u,31u,35u,36u,55u,56u,60u}){auto copy=bytes;copy[at]^=1;check(!profile.load({copy.data(),copy.size()},&error)&&!profile.ready(),"profile malformed/reserved/domain guards clear prior data");}
    for(unsigned at:{32u,33u,34u}){auto copy=bytes;copy[at]=at==32?128:at==33?3:17;check(!profile.load({copy.data(),copy.size()},&error),"stage parameter domain");}
    auto zero=bytes;p16(zero,18,0);check(!profile.load({zero.data(),zero.size()},&error),"zero cover refused");check(!profile.load({nullptr,64},&error)&&!profile.load({bytes.data(),63},&error),"null/short final profile");
    Files files;ChallengeStageDataDriver driver;ChallengeStageDataRequest request;check(!request.valid()&&!driver.begin(request,files,"owned",&error)&&!files.calls&&!driver.receipt().valid()&&!driver.receipt().failed(),"no fake request/read/receipt from a boolean");
    ChallengeNativeStageData data;check(!data.slots(0)&&!data.slots(2)&&!data.slot(0,0).data&&!data.bytes(),"default data and slot bounds");
    SourceByteRngOwner rng;check(rng.initialize_explicit({0,0}),"explicit stream");const auto before=rng.snapshot();u8 byte=9;check(rng.next_byte(&byte)&&rng.same_owner(before)&&!rng.current(before),"identity continuation permits genuine intervening draws, not stale commit");
    ChallengeSetupPresentation p;ChallengeFinalSetup final;ChallengeFinalEvent event;check(!final.step(p,rng,nullptr,&event,&error)&&final.phase()==ChallengeFinalPhase::Closed&&!final.data(),"closed final controller does no callbacks");
    ChallengeInitialPartyCandidate candidate;std::array<const ForceInventory*,2> owners{};check(!final.take(owners,&candidate)&&!candidate.ready(),"no early party publication");
    std::printf("Challenge final: %u owned checks PASS\n",checks);
}
