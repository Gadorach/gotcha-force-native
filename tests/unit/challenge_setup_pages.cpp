#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/challenge_players.hpp"
#include "engine/gameplay/challenge_box_load_driver.hpp"
#include "tests/unit/force_inventory_fixture.hpp"
namespace {
unsigned checks=0;void check(bool value,const char* text){++checks;if(!value){std::fprintf(stderr,"Challenge setup pages: %s\n",text);std::abort();}}
struct Save final:gf::ISaveBackend {
    gf::SaveReadStatus status=gf::SaveReadStatus::Missing;std::vector<gf::u8> data;unsigned reads=0,writes=0,erases=0;
    bool load(gf::u32 slot,std::vector<gf::u8>& out)override{return read_slot(slot,out)==gf::SaveReadStatus::Loaded;}
    gf::SaveReadStatus read_slot(gf::u32 slot,std::vector<gf::u8>& out)override{++reads;if(slot)return gf::SaveReadStatus::Error;if(status==gf::SaveReadStatus::Loaded)out=data;return status;}
    bool store(gf::u32 slot,const gf::u8* p,std::size_t n)override{if(slot)return false;data.assign(p,p+n);status=gf::SaveReadStatus::Loaded;++writes;return true;}
    bool erase(gf::u32)override{++erases;return false;}
};
bool same_plain(const gf::ForceInventory& a,const gf::ForceInventory& b){
    auto x=std::make_unique<gf::ForceInventoryImages>(),y=std::make_unique<gf::ForceInventoryImages>();
    return a.ready()&&b.ready()&&a.catalog()->fingerprint()==b.catalog()->fingerprint()&&
        gf::force_inventory_encode(*a.state(),*a.catalog(),x.get())&&gf::force_inventory_encode(*b.state(),*b.catalog(),y.get())&&x->box==y->box&&x->warehouse==y->warehouse&&x->profile==y->profile;
}
}
int main(){
    using namespace gf;ChallengeSetupPagesProfile profile{2,3,2,2,3};std::array<ChallengeSetupPad,4> pads{};std::array<ChallengeSetupButtons,4> history{};ChallengePlayersEvent pe;ChallengeBoxEvent be;std::string error;
    auto players_active=[&](ChallengePlayersPage& page,u8 port,ChallengeSetupButtons carried){check(page.open(profile,port,carried),"fresh players page");check(page.step(pads,&pe)&&page.remaining()==2,"players enter timer");check(page.step(pads,&pe)&&page.step(pads,&pe)&&page.phase()==ChallengePlayersPhase::Active,"players opening before input");};
    //Carry held A from difficulty. A release during the unsampled opening is not
    //invented as a new edge; source samples only Active page callbacks.
    ChallengeSetupButtons carried;challenge_setup_input_step({0x100,true},&carried);ChallengePlayersPage page;players_active(page,1,carried);
    pads[0]={0x302,true};pads[1]={0x100,true};check(page.step(pads,&pe)&&!pe.sound_count&&page.phase()==ChallengePlayersPhase::Active,"carried A and unrelated controller cannot auto-confirm");
    pads[1]={0,true};check(page.step(pads,&pe),"release in Active");pads[1]={0x102,true};check(page.step(pads,&pe)&&pe.changed&&pe.sounds[0]==119&&pe.sounds[1]==122&&page.selection()==1,"navigate then confirm TEAM UP");
    for(unsigned i=0;i<3;++i)check(page.step(pads,&pe)&&pe.box_requested==(i==2),"player close gate");
    check(pe.team_up&&page.phase()==ChallengePlayersPhase::BoxRequested&&!page.step(pads,&pe)&&!pe.box_requested,"choice requests box once, not second-human admission");
    pads={};players_active(page,2,{});pads[2]={0x303,true};check(page.step(pads,&pe)&&pe.difficulty_requested&&!pe.box_requested&&pe.sounds[0]==123&&page.selection()==0&&!page.visible_mask()&&!page.interactive_mask(),"players B wins confirm, Left wins Right, clears own mask");
    auto box_active=[&](ChallengeLoadBox& box,u8 context,u8 port,u8 participants,const std::array<ChallengeSetupButtons,4>& inputs){
        check(box.open(profile,context,port,participants,inputs),"fresh box context");check(box.step(false,pads,nullptr,&be)&&be.request_resources,"box resource requirement once");
        check(box.step(false,pads,nullptr,&be)&&box.remaining()==1&&!be.request_resources,"box opening waits");check(box.step(false,pads,nullptr,&be)&&!box.remaining()&&box.phase()==ChallengeBoxPhase::Opening,"timer zero alone is not resource readiness");
        check(box.step(true,pads,nullptr,&be)&&box.phase()==ChallengeBoxPhase::Active&&box.interactive_mask()==1,"resource gate opens only after both conditions");
    };
    auto close_box=[&](ChallengeLoadBox& box){for(unsigned i=0;i<3;++i)check(box.step(true,pads,nullptr,&be)&&be.selection_requested==(i==2),"exact close gate");check(box.phase()==ChallengeBoxPhase::SelectionRequested,"selection request boundary");};
    ChallengeLoadBox box;pads={};box_active(box,1,0,1,history);pads[0]={0x80u<<5,true};check(box.step(true,pads,nullptr,&be)&&be.skipped&&be.sounds[0]==122&&box.phase()==ChallengeBoxPhase::Closing,"Start skips without issuing a load");close_box(box);
    auto catalog=force_inventory_fixture::catalog();ForceInventory defaults;check(challenge_box_fallback(box,catalog,nullptr,defaults)&&defaults.state()->forces[0].count==1&&!challenge_box_fallback(box,catalog,nullptr,defaults),"source starter fallback published once");
    pads={};box_active(box,2,3,8,history);pads[3]={0x1100,true};check(box.step(true,pads,nullptr,&be)&&box.phase()==ChallengeBoxPhase::Active&&!be.sound_count,"second context excludes first controller");
    pads[0]={0x100,true};pads[2]={0x200,true};check(box.step(true,pads,nullptr,&be)&&be.exit_confirmation_requested&&be.dialog_port==2&&box.port()==3&&box.participants()==8,"any eligible B wins A, modal belongs to actual B presser");
    check(box.resume_after_exit_cancel()&&box.phase()==ChallengeBoxPhase::Active&&!box.resume_after_exit_cancel(),"cancel resumes same box/history, not new resource entry");
    pads={};check(box.step(true,pads,nullptr,&be),"release second-context history");pads[0]={0x1000,true};pads[2]={0x1000,true};check(box.step(true,pads,nullptr,&be)&&be.skipped&&box.port()==0&&box.participants()==9,"first eligible Start presser joins, not every controller");close_box(box);
    ForceInventory second;check(!challenge_box_fallback(box,catalog,&defaults,defaults)&&challenge_box_fallback(box,catalog,&defaults,second)&&same_plain(second,defaults),"second fallback clones first, cannot overwrite first");
    Save saved_a,saved_b;ForceInventoryStore writer(saved_a);ForceInventory unused;check(writer.load(catalog,&unused)==ForceInventoryLoad::Missing&&writer.write(defaults),"owned native record fixture");
    const auto original_bytes=saved_a.data;const auto original_writes=saved_a.writes;ForceInventoryStore read_a(saved_a),read_b(saved_b);ChallengeBoxLoadDriver driver;
    auto start_load=[&](ChallengeLoadBox& target,u8 context,u8 port,u8 participants){pads={};box_active(target,context,port,participants,history);const unsigned actor=context==1?port:0;pads[actor]={0x1100,true};check(target.step(true,pads,nullptr,&be)&&target.phase()==ChallengeBoxPhase::PreLoad&&!be.skipped,"A wins Start without issuing immediate I/O");
        check(target.step(true,pads,nullptr,&be)&&!be.request_load&&target.step(true,pads,nullptr,&be)&&be.request_load&&be.load.valid(),"read issued only after load delay");return be.load;};
    const auto request=start_load(box,1,1,2);const auto before_reads=saved_a.reads;
    check(driver.begin(request,catalog,read_a,&read_b,&error)&&saved_a.reads==before_reads+1&&driver.notice()==ChallengeNativeLoadNotice::Loaded&&!driver.receipt().valid(),"native codec read, not yet acknowledged");
    ChallengeBoxLoadDriver duplicate;check(!duplicate.begin(request,catalog,read_a,&read_b,&error)&&saved_a.reads==before_reads+1,"one claimed read per source request");
    for(unsigned i=0;i<4;++i)check(driver.observe_notice(pads)&&!driver.receipt().valid(),"launch-held A/Start cannot dismiss native notice");
    pads={};check(driver.observe_notice(pads),"notice release arms fresh confirmation");pads[1]={0x100,true};check(driver.observe_notice(pads)&&driver.receipt().valid(),"fresh native notice dismissal earns receipt");
    auto receipt=driver.receipt();ChallengeLoadBox foreign;const auto foreign_request=start_load(foreign,1,2,4);check(!foreign.step(true,pads,&receipt,&be)&&foreign_request.valid()&&foreign.phase()==ChallengeBoxPhase::Loading,"another live box cannot consume a foreign native receipt");
    ChallengeBoxLoadReceipt empty;check(!box.step(true,pads,&empty,&be)&&box.phase()==ChallengeBoxPhase::Loading,"public empty receipt cannot fabricate load completion");
    check(box.step(true,pads,&receipt,&be)&&box.loaded()&&!request.valid()&&box.phase()==ChallengeBoxPhase::Closing,"genuine current read receipt transitions once");
    ForceInventory loaded;check(!driver.publish_loaded(box,loaded),"no publication before closing/resource boundary");close_box(box);
    check(driver.publish_loaded(box,loaded)&&loaded.ready()&&!driver.publish_loaded(box,loaded)&&same_plain(loaded,defaults),"validated inventory published once at selection handoff");
    check(saved_a.data==original_bytes&&saved_a.writes==original_writes&&!saved_a.erases,"load never writes/erases source record");
    //Missing/unavailable/corrupt native saves are explicit failure notices; only
    //after dismissal do they reach source failed-load fallback, without saving.
    for(unsigned failure=0;failure<3;++failure){Save bad;bad.status=failure==0?SaveReadStatus::Missing:failure==1?SaveReadStatus::Error:SaveReadStatus::Loaded;bad.data={1,2,3};ForceInventoryStore store(bad);ChallengeLoadBox failed;const auto q=start_load(failed,1,0,1);ChallengeBoxLoadDriver read;
        check(read.begin(q,catalog,store,nullptr,&error)&&!read.receipt().valid(),"failed native read cannot be successful acknowledgement");
        check(read.notice()==(failure==0?ChallengeNativeLoadNotice::Missing:failure==1?ChallengeNativeLoadNotice::Unavailable:ChallengeNativeLoadNotice::Invalid),"native failure classification preserved");
        pads={};check(read.observe_notice(pads),"failed notice release");pads[0]={0x200,true};check(read.observe_notice(pads),"fresh B dismisses native failure");auto r=read.receipt();check(r.valid()&&failed.step(true,pads,&r,&be)&&!failed.loaded(),"genuine failed receipt reaches source failed branch");close_box(failed);ForceInventory fallback;
        check(!read.publish_loaded(failed,fallback)&&challenge_box_fallback(failed,catalog,nullptr,fallback)&&fallback.ready()&&!bad.writes&&!bad.erases,"failed load produces explicit source defaults, never overwrite/save");}
    //Foreign/expired request identities cannot publish or satisfy another box.
    ChallengeLoadBox stale;const auto old=start_load(stale,1,0,1);check(stale.open(profile,1,0,1,history)&&!old.valid(),"new entry expires old request");ChallengeBoxLoadDriver late;const auto reads=saved_a.reads;check(!late.begin(old,catalog,read_a,nullptr,&error)&&saved_a.reads==reads,"expired request cannot issue delayed I/O");
    ChallengeLoadBox slot_b;const auto b_request=start_load(slot_b,2,3,8);ChallengeBoxLoadDriver wrong;
    check(!wrong.begin(b_request,catalog,read_a,nullptr,&error)&&!wrong.begin(b_request,catalog,read_a,&read_a,&error),"slot B must not alias slot A/OPTION namespace");
    check(saved_b.reads==0&&!saved_b.writes,"no second read before valid namespace");
    saved_b.status=SaveReadStatus::Loaded;saved_b.data=original_bytes;ChallengeBoxLoadDriver second_reader;const auto first_reads=saved_a.reads;
    check(second_reader.begin(b_request,catalog,read_a,&read_b,&error)&&saved_b.reads==1&&saved_a.reads==first_reads,"second context reads its own gameplay namespace, not OPTION or first player");
    pads={};check(second_reader.observe_notice(pads),"slot B release");pads[0]={0x100,true};check(second_reader.observe_notice(pads),"slot B notice confirmation");auto second_receipt=second_reader.receipt();check(slot_b.step(true,pads,&second_receipt,&be),"slot B receipt");close_box(slot_b);ForceInventory second_loaded;
    check(!second_reader.publish_loaded(slot_b,defaults,&defaults)&&second_reader.publish_loaded(slot_b,second_loaded,&defaults)&&same_plain(second_loaded,defaults),"slot B loaded publication cannot alias first owner");
    //Native edited provenance survives loading and player2 fallback; default
    //Challenge roster admission continues to reject it.
    ForceInventory edited;CheatSettings cheats;cheats.borg_editor=true;check(edited.create(catalog)&&edited.edit_borg(cheats,false,0,edited.revision(),BorgEditField::HP,77)==ForceChange::Applied,"owned edited save fixture");
    Save modified_save;ForceInventoryStore modified_writer(modified_save);ForceInventory scratch;check(modified_writer.load(catalog,&scratch)==ForceInventoryLoad::Missing&&modified_writer.write(edited),"native version2 record with provenance");
    ForceInventoryStore modified_store(modified_save);ChallengeLoadBox modified_box;const auto modified_request=start_load(modified_box,1,0,1);ChallengeBoxLoadDriver modified_reader;check(modified_reader.begin(modified_request,catalog,modified_store,nullptr,&error),"native edited read");
    pads={};check(modified_reader.observe_notice(pads),"modified notice release");pads[0]={0x100,true};check(modified_reader.observe_notice(pads),"modified notice dismiss");auto modified_receipt=modified_reader.receipt();check(modified_box.step(true,pads,&modified_receipt,&be),"modified receipt");close_box(modified_box);ForceInventory modified_loaded;ForceBattleRoster roster;
    check(modified_reader.publish_loaded(modified_box,modified_loaded)&&modified_loaded.state()->box[0].value==edited.state()->box[0].value&&!modified_loaded.battle_roster(0,1000,ForceCostMode::Full,&roster),"loading never clears modified flag or grants Challenge roster admission");
    pads={};ChallengeLoadBox modified_clone;box_active(modified_clone,2,2,4,history);pads[0]={0x1000,true};check(modified_clone.step(true,pads,nullptr,&be)&&be.skipped,"player2 skip after edited first inventory");close_box(modified_clone);ForceInventory clone;
    check(challenge_box_fallback(modified_clone,catalog,&modified_loaded,clone)&&clone.state()->box[0].value.edits.modified&&!clone.battle_roster(0,1000,ForceCostMode::Full,&roster),"second fallback keeps sticky provenance and roster refusal");
    check(modified_save.writes==1&&!modified_save.erases&&saved_b.writes==0,"only explicit owned fixture writes, no load-path save");
    check(!box.open(profile,0,0,1,history)&&!box.open(profile,2,4,1,history)&&!box.open(profile,1,1,1,history),"invalid context/port/participant ownership refused");
    std::printf("{\"challenge_setup_pages_owned_checks\":%u,\"all_passed\":true,\"native_file_NOT_original_memory_card_proof\":true}\n",checks);
}
