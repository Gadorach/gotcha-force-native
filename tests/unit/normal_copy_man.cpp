#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borgs/normal_copy_man.hpp"

namespace {
unsigned checks=0;
void check(bool ok,const char* why){
    ++checks;
    if(!ok){std::fprintf(stderr,"normal Copy Man: %s\n",why);std::abort();}
}
}

int main(){
    using namespace gf;
    CopyManHeadRequest head;
    check(normal_copy_man_head_request(kNormalCopyMan,3,1,0,0,true,&head)&&!head.advance_phase&&!head.request_head,
        "charged action selection hasn't produced its exact program event yet");
    check(normal_copy_man_head_request(kNormalCopyMan,3,1,1,1,true,&head)&&head.advance_phase&&!head.request_head,
        "existing head suppresses a new request but not the class advance");
    check(normal_copy_man_head_request(kNormalCopyMan,3,1,1,0,true,&head)&&head.advance_phase&&head.request_head&&
        head.owner_slot==3&&head.object_type==85&&!head.argument,"request a caster-owned head, not an actor reload");

    CopyManContactObservation contact{3,1,85,0,0,0,true};
    CopyManContactDonor recorded;
    check(!normal_copy_man_record_donor(kNormalCopyMan,{3,1,85,0,0,0,false},&recorded)&&!recorded.recorded,
        "aim context alone cannot name a copy donor");
    contact.recipient_mask59e=0x2000;
    check(normal_copy_man_record_donor(kNormalCopyMan,contact,&recorded)&&!recorded.recorded,"receiver mask blocks copy");
    contact.recipient_mask59e=0;contact.recipient_fused4a0=1;
    check(normal_copy_man_record_donor(kNormalCopyMan,contact,&recorded)&&!recorded.recorded,"fused donor guard");
    contact.recipient_fused4a0=0;
    check(normal_copy_man_record_donor(kNormalCopyMan,contact,&recorded)&&recorded.recorded&&
        recorded.caster_slot==3&&recorded.donor_slot==1,"record actual slot at contact, not its presumed identity");

    CopyManOwnerObservation owner{3,1,0,2,0,kNormalCopyMan,kNormalCopyMan,190,true};
    CopyManDonorObservation donor{1,0,0,kReviewedCopyDonor,true};
    CopyManReloadPlan outbound;
    check(normal_copy_man_reload_request(owner,recorded,donor,true,&outbound)&&
        outbound==CopyManReloadPlan{true,3,1,2,2,1,kReviewedCopyDonor,2,190,900},
        "same-slot reload uses current donor model but caster level and HP");
    const auto requested=outbound;
    donor.special48a=1;
    check(!normal_copy_man_reload_request(owner,recorded,donor,true,&outbound)&&outbound==requested,"special donor route remains unresolved");
    donor.special48a=0;donor.current.variant=1;
    check(!normal_copy_man_reload_request(owner,recorded,donor,true,&outbound)&&outbound==requested,"no unaudited variant fallback");
    donor.current=kReviewedCopyDonor;donor.slot=2;
    check(!normal_copy_man_reload_request(owner,recorded,donor,true,&outbound)&&outbound==requested,"cannot use another slot's live model");
    donor.slot=1;owner.battle_state18=2;
    check(normal_copy_man_reload_request(owner,recorded,donor,true,&outbound)&&!outbound.requested,"busy lifecycle cannot copy");
    owner.battle_state18=1;owner.current_hp1c6=0;
    check(normal_copy_man_reload_request(owner,recorded,donor,true,&outbound)&&!outbound.requested,"defeated caster cannot consume contact");

    CopyManHealthPlan hp;
    check(!normal_copy_man_reload_health(requested,{3,2,kReviewedCopyDonor,180,false},&hp)&&!hp.hp1c6,
        "a pending tuple isn't a loaded resource");
    check(!normal_copy_man_reload_health(requested,{1,2,kReviewedCopyDonor,180,true},&hp)&&!hp.hp1c6,
        "donor slot does not satisfy caster resource binding");
    check(normal_copy_man_reload_health(requested,{3,2,kReviewedCopyDonor,180,true},&hp)&&hp==CopyManHealthPlan{180,180},
        "use absolute caster HP capped at installed maximum, not donor HP");

    owner.current=kReviewedCopyDonor;owner.current_hp1c6=140;
    CopyManExpiryStep expiry;
    check(normal_copy_man_expiry_step(owner,{900,2,0,0,0,0},&expiry)&&expiry==CopyManExpiryStep{899,false,3},"actor-local eligible tick");
    check(normal_copy_man_expiry_step(owner,{2,2,1,0,0,0},&expiry)&&expiry==CopyManExpiryStep{2,false,3},"entry transition pauses timer");
    check(normal_copy_man_expiry_step(owner,{2,2,0,1,0,0},&expiry)&&expiry==CopyManExpiryStep{2,false,3},"fused branch is separate");
    check(normal_copy_man_expiry_step(owner,{2,2,0,0,1,0},&expiry)&&expiry==CopyManExpiryStep{2,false,3},"Hyper preempts timer");
    check(normal_copy_man_expiry_step(owner,{0,2,0,0,0,0},&expiry)&&expiry==CopyManExpiryStep{0,false,3},"plain zero is not expiry");
    check(normal_copy_man_expiry_step(owner,{1,2,0,0,0,0x1000},&expiry)&&expiry==CopyManExpiryStep{1,false,3},"blocked crossing retries from one");
    CopyManReloadPlan restore;
    check(!normal_copy_man_return_request(owner,expiry,&restore)&&!restore.requested,"blocked expiry cannot publish a reload");
    check(normal_copy_man_expiry_step(owner,{expiry.ticks49a,2,0,0,0,0},&expiry)&&expiry==CopyManExpiryStep{0,true,3}&&
        normal_copy_man_return_request(owner,expiry,&restore)&&restore==CopyManReloadPlan{true,3,4,2,3,4,kNormalCopyMan,0,140,0},
        "guarded return queues original owner tuple with then-current HP, not original snapshot");
    check(normal_copy_man_reload_health(restore,{3,0,kNormalCopyMan,200,true},&hp)&&hp==CopyManHealthPlan{140,140},
        "successful original binding does not refund lost HP");
    owner.slot=4;
    check(!normal_copy_man_return_request(owner,expiry,&restore)&&restore.slot==3,"expiry from another live slot cannot request return");
    std::printf("normal Copy Man exact same-slot reload and guarded return: %u checks PASS\n",checks);
}
