#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <functional>
#include "engine/gameplay/challenge_box_load_driver.hpp"
#include "tests/unit/force_inventory_fixture.hpp"
namespace {
unsigned checks=0;void check(bool value,const char* why){++checks;if(!value){std::fprintf(stderr,"native box load edges: %s\n",why);std::abort();}}
struct Save final:gf::ISaveBackend {
    std::vector<gf::u8> bytes;unsigned reads=0,writes=0;bool exists=false,throw_read=false;std::function<void()> callback;
    bool load(gf::u32 slot,std::vector<gf::u8>& out)override{return read_slot(slot,out)==gf::SaveReadStatus::Loaded;}
    gf::SaveReadStatus read_slot(gf::u32 slot,std::vector<gf::u8>& out)override{++reads;if(callback)callback();if(throw_read)throw std::runtime_error("owned read uncertainty");if(slot)return gf::SaveReadStatus::Error;if(!exists)return gf::SaveReadStatus::Missing;out=bytes;return gf::SaveReadStatus::Loaded;}
    bool store(gf::u32 slot,const gf::u8* data,std::size_t n)override{if(slot)return false;bytes.assign(data,data+n);exists=true;++writes;return true;}
    bool erase(gf::u32)override{return false;}
};
}
int main(){
    using namespace gf;auto catalog=force_inventory_fixture::catalog();ChallengeSetupPagesProfile profile{1,1,1,1,1};std::array<ChallengeSetupPad,4> pads{};std::array<ChallengeSetupButtons,4> inputs{};ChallengeBoxEvent event;std::string error;
    Save save;ForceInventoryStore writer(save);ForceInventory source,scratch;check(source.create(catalog)&&source.place(0,1,1)==ForceChange::Applied&&writer.load(catalog,&scratch)==ForceInventoryLoad::Missing&&writer.write(source),"owned two-member native record");
    const auto original=save.bytes;const auto writes=save.writes;ForceInventoryStore store(save);
    auto request=[&](ChallengeLoadBox& box){pads={};check(box.open(profile,1,0,1,inputs)&&box.step(true,pads,nullptr,&event)&&box.step(true,pads,nullptr,&event),"new source box reaches Active");pads[0]={0x100,true};check(box.step(true,pads,nullptr,&event)&&box.step(true,pads,nullptr,&event)&&event.request_load,"delayed read request");return event.load;};
    ChallengeLoadBox cancelled;const auto q=request(cancelled);ChallengeBoxLoadDriver reader;check(reader.begin(q,catalog,store,nullptr,&error)&&reader.notice()==ChallengeNativeLoadNotice::Loaded,"genuine record read before native decision");
    pads={};check(reader.observe_notice(pads),"notice release");pads[0]={0x300,true};check(reader.observe_notice(pads)&&reader.notice()==ChallengeNativeLoadNotice::Cancelled,"native B priority cancels loaded data, never accidental accept");auto cancelled_receipt=reader.receipt();check(cancelled_receipt.valid()&&cancelled.step(true,pads,&cancelled_receipt,&event)&&!cancelled.loaded(),"cancelled actual read follows failed/cancelled source outcome");
    check(cancelled.step(true,pads,nullptr,&event)&&event.selection_requested,"cancel close boundary");ForceInventory fallback;check(!reader.publish_loaded(cancelled,fallback)&&challenge_box_fallback(cancelled,catalog,nullptr,fallback)&&fallback.state()->forces[0].count==1,"cancel cannot publish cached two-member record");
    check(save.bytes==original&&save.writes==writes,"cancel does not write/erase saved bytes");
    ChallengeLoadBox expired;const auto old=request(expired);ChallengeBoxLoadReceipt old_receipt;
    {ChallengeBoxLoadDriver short_lived;check(short_lived.begin(old,catalog,store,nullptr,&error),"lifetime read");pads={};check(short_lived.observe_notice(pads),"lifetime release");pads[0]={0x100,true};check(short_lived.observe_notice(pads),"lifetime acknowledge");old_receipt=short_lived.receipt();check(old_receipt.valid(),"receipt while read owner lives");}
    check(!old_receipt.valid()&&!expired.step(true,pads,&old_receipt,&event)&&expired.phase()==ChallengeBoxPhase::Loading,"destroyed read owner cannot satisfy source loading gate");
    ChallengeLoadBox replaced;const auto replaced_request=request(replaced);Save reentrant;reentrant.exists=true;reentrant.bytes=original;reentrant.callback=[&](){check(replaced.open(profile,1,0,1,inputs),"owned read callback replaces source entry");};ForceInventoryStore reentrant_store(reentrant);ChallengeBoxLoadDriver interrupted;
    check(interrupted.begin(replaced_request,catalog,reentrant_store,nullptr,&error)&&!replaced_request.valid()&&!interrupted.receipt().valid()&&!interrupted.observe_notice(pads),"read completed after entry replacement earns no live receipt or notice authority");
    ChallengeLoadBox unknown;const auto uncertain=request(unknown);Save throws;throws.exists=true;throws.bytes=original;throws.throw_read=true;ForceInventoryStore throwing_store(throws);ChallengeBoxLoadDriver uncertain_reader;bool caught=false;
    try{(void)uncertain_reader.begin(uncertain,catalog,throwing_store,nullptr,&error);}catch(const std::runtime_error&){caught=true;}
    check(caught&&throws.reads==1&&!uncertain_reader.receipt().valid()&&!uncertain_reader.observe_notice(pads),"uncertain read never manufactures completion");
    ChallengeBoxLoadDriver retry;check(!retry.begin(uncertain,catalog,throwing_store,nullptr,&error)&&throws.reads==1,"claimed uncertain read cannot auto-replay");
    auto bytes=std::vector<u8>(32);store_be32(bytes.data(),0x47465053);store_be32(bytes.data()+4,1);for(unsigned i=0;i<5;++i)store_be16(bytes.data()+8+i*2,1);ChallengeSetupPagesProfile loaded;
    check(challenge_setup_pages_load({bytes.data(),bytes.size()},&loaded,&error)&&loaded.valid(),"owned profile reader");
    for(unsigned at:{0u,4u,8u,10u,12u,14u,16u,18u,31u}){auto bad=bytes;bad[at]^=0x80;check(!challenge_setup_pages_load({bad.data(),bad.size()},&loaded,&error)&&!loaded.valid(),"corrupt header/counters/reserved refuses and clears");}
    check(!challenge_setup_pages_load({bytes.data(),31},&loaded,&error)&&!challenge_setup_pages_load({},&loaded,&error),"truncated/null profile");
    std::printf("{\"challenge_box_extra_owned_checks\":%u,\"all_passed\":true,\"native_cancel_policy_NOT_original_card_dialog\":true}\n",checks);
}
